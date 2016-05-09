// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright © 2008- 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
 
#include "Internal.h"
#ifdef SUPPORT_CUDA
#include "FFT_Simulation_CUDA_impl.h"
#include "FFT_Simulation_Manager_CUDA_impl.h"
#include "Simulation_Util.h"
#include "CircularFIFO.h"

#include <malloc.h>
#include <string.h>

namespace
{
#if WAVEWORKS_ENABLE_D3D10 || WAVEWORKS_ENABLE_D3D10
	const DXGI_SAMPLE_DESC kNoSample = {1, 0};
#endif

	bool cudaQueryResultIsError(cudaError_t result)
	{
		return result != cudaErrorNotReady && result != cudaSuccess;
	};

	typedef NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::CudaDeviceInfo CudaDeviceInfo;
}

// CUDA stubs
extern "C"
{
	cudaError cuda_SetConstants (void* constants,
							float2* Gauss,
							float2* H0, 
							float2* Ht, 
							float4* Dt,
							float* Omega,
							int resolution,
							float fft_period,
							float window_in,
							float window_out,
							float2 wind_dir,
							float wind_speed,
							float wind_dependency,
							float wave_amplitude,
							float small_wave_fraction,
							float choppy_scale,
							cudaStream_t cu_stream);

	cudaError cuda_ComputeH0(int resolution, int constantsIndex, cudaStream_t cu_stream);
	cudaError cuda_ComputeRows(int resolution, double time, int constantsIndex, cudaStream_t cu_stream);
	cudaError cuda_ComputeColumns(float4* displacement, int resolution, int constantsIndex, cudaStream_t cu_stream);
	cudaError cuda_ComputeColumns_array(cudaArray* displacement, int resolution, int constantsIndex, cudaStream_t cu_stream);
}

NVWaveWorks_FFT_Simulation_CUDA_Impl::NVWaveWorks_FFT_Simulation_CUDA_Impl(NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl* pManager, const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params) :
	m_pManager(pManager),
	m_params(params)
{
	m_numCudaDevices = 0;
	m_pCudaDeviceStates = NULL;

    for(int slot = 0; slot != NumReadbackSlots; ++slot)
    {
		m_readback_slots[slot].m_host_Dxyz = NULL;
		m_readback_slots[slot].m_device_Dxyz = NULL;
		m_readback_slots[slot].m_cudaDevice = -1;
		m_readback_slots[slot].m_completion_evt = NULL;
		m_readback_slots[slot].m_staging_evt = NULL;
		m_readback_slots[slot].m_kickID = GFSDK_WaveWorks_InvalidKickID;
    }

    m_active_readback_slot = 0;
	m_active_readback_host_Dxyz = NULL;
    m_end_inflight_readback_slots = 1;
	m_working_readback_slot = NULL;

	m_pReadbackFIFO = NULL;

    m_active_timer_slot = 0;
    m_end_inflight_timer_slots = 1;
	m_timer_slots[m_active_timer_slot].m_elapsed_time = 0.f;						// Ensure first call to getTimings() gives reasonable results
	m_timer_slots[m_active_timer_slot].m_kickID = GFSDK_WaveWorks_InvalidKickID;	// Ensure first call to getTimings() gives reasonable results
	m_working_timer_slot = NULL;

    m_DisplacementMapIsCUDARegistered = false;
	m_GaussAndOmegaInitialised = false;
    //m_H0Dirty = true;
    m_ReadbackInitialised = false;
	m_DisplacementMapVersion = GFSDK_WaveWorks_InvalidKickID;

	m_readback_element_size = 0;

    memset(&m_d3d, 0, sizeof(m_d3d));
    m_d3dAPI = nv_water_d3d_api_undefined;
}

NVWaveWorks_FFT_Simulation_CUDA_Impl::~NVWaveWorks_FFT_Simulation_CUDA_Impl()
{
    releaseAll();
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::initD3D9(IDirect3DDevice9* pD3DDevice)
{
#if WAVEWORKS_ENABLE_D3D9
    HRESULT hr;

    if(nv_water_d3d_api_d3d9 != m_d3dAPI)
    {
        releaseAll();
    }
    else if(m_d3d._9.m_pd3d9Device != pD3DDevice)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_d3d9;
        m_d3d._9.m_pd3d9Device = pD3DDevice;
        m_d3d._9.m_pd3d9Device->AddRef();

		// Use 4x32F for D3D9
		m_readback_element_size = sizeof(float4);

		m_numCudaDevices = m_pManager->GetNumCudaDevices();
		m_pCudaDeviceStates = new CudaDeviceState[m_numCudaDevices];
		memset(m_pCudaDeviceStates, 0, m_numCudaDevices * sizeof(CudaDeviceState));
		for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
		{
			m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice = m_pManager->GetCudaDeviceInfo(cuda_dev_index).m_cudaDevice;
			m_pCudaDeviceStates[cuda_dev_index].m_constantsIndex = -1;
		}

        V_RETURN(allocateAllResources());
    }

    return S_OK;
#else
    return E_FAIL;
#endif
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::initD3D10(ID3D10Device* pD3DDevice)
{
#if WAVEWORKS_ENABLE_D3D10
    HRESULT hr;

    if(nv_water_d3d_api_d3d10 != m_d3dAPI)
    {
        releaseAll();
    }
    else if(m_d3d._10.m_pd3d10Device != pD3DDevice)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_d3d10;
        m_d3d._10.m_pd3d10Device = pD3DDevice;
        m_d3d._10.m_pd3d10Device->AddRef();

		// Use 4x32F for D3D10
		m_readback_element_size = sizeof(float4);

		m_numCudaDevices = m_pManager->GetNumCudaDevices();
		m_pCudaDeviceStates = new CudaDeviceState[m_numCudaDevices];
		memset(m_pCudaDeviceStates, 0, m_numCudaDevices * sizeof(CudaDeviceState));
		for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
		{
			m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice = m_pManager->GetCudaDeviceInfo(cuda_dev_index).m_cudaDevice;
		}

        V_RETURN(allocateAllResources());
    }

    return S_OK;
#else
    return E_FAIL;
#endif
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::initD3D11(ID3D11Device* pD3DDevice)
{
#if WAVEWORKS_ENABLE_D3D11
    HRESULT hr;

    if(nv_water_d3d_api_d3d11 != m_d3dAPI)
    {
        releaseAll();
    }
    else if(m_d3d._11.m_pd3d11Device != pD3DDevice)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_d3d11;
        m_d3d._11.m_pd3d11Device = pD3DDevice;
        m_d3d._11.m_pd3d11Device->AddRef();

		// Use 4x16F for D3D11
		m_readback_element_size = sizeof(ushort4);

		m_numCudaDevices = m_pManager->GetNumCudaDevices();
		m_pCudaDeviceStates = new CudaDeviceState[m_numCudaDevices];
		memset(m_pCudaDeviceStates, 0, m_numCudaDevices * sizeof(CudaDeviceState));
		for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
		{
			m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice = m_pManager->GetCudaDeviceInfo(cuda_dev_index).m_cudaDevice;
		}

        V_RETURN(allocateAllResources());
    }

    return S_OK;
#else
    return E_FAIL;
#endif
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::initGL2(void* pGLContext)
{
#if WAVEWORKS_ENABLE_GL
    HRESULT hr;

	if(nv_water_d3d_api_gl2 != m_d3dAPI)
    {
        releaseAll();
    }
	else if(m_d3d._GL2.m_pGLContext != pGLContext)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_gl2;

		// Use 4x16F for GL2
		m_readback_element_size = sizeof(ushort4);

		m_numCudaDevices = m_pManager->GetNumCudaDevices();
		m_pCudaDeviceStates = new CudaDeviceState[m_numCudaDevices];
		memset(m_pCudaDeviceStates, 0, m_numCudaDevices * sizeof(CudaDeviceState));
		for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
		{
			m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice = m_pManager->GetCudaDeviceInfo(cuda_dev_index).m_cudaDevice;
		}
        V_RETURN(allocateAllResources());
    }

    return S_OK;
#else
    return E_FAIL;
#endif
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::initNoGraphics()
{
    HRESULT hr;

    if(nv_water_d3d_api_none != m_d3dAPI)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_none;

		// Use 4x32F for no-gfx
		m_readback_element_size = sizeof(float4);

		m_numCudaDevices = m_pManager->GetNumCudaDevices();
		m_pCudaDeviceStates = new CudaDeviceState[m_numCudaDevices];
		memset(m_pCudaDeviceStates, 0, m_numCudaDevices * sizeof(CudaDeviceState));
		for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
		{
			m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice = m_pManager->GetCudaDeviceInfo(cuda_dev_index).m_cudaDevice;
		}

        V_RETURN(allocateAllResources());
    }

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::reinit(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params)
{
	HRESULT hr;

    BOOL bRelease = FALSE;
    BOOL bAllocate = FALSE;
    BOOL bRecalcH0 = FALSE;
	BOOL bReinitGaussAndOmega = FALSE;

    if(params.fft_resolution != m_params.fft_resolution ||
        params.readback_displacements != m_params.readback_displacements)
    {
        bRelease = TRUE;
        bAllocate = TRUE;

		// We're reallocating, which breaks various lockstep/synchronization assumptions...
		V_RETURN(m_pManager->beforeReallocateSimulation());
    }

    if(	params.fft_period != m_params.fft_period || 
        params.fft_resolution != m_params.fft_resolution
        )
    {
        bReinitGaussAndOmega = TRUE;
    }

    if(	params.wave_amplitude != m_params.wave_amplitude ||
        params.wind_speed != m_params.wind_speed ||
        params.wind_dir.x != m_params.wind_dir.x ||
		params.wind_dir.y != m_params.wind_dir.y ||
        params.wind_dependency != m_params.wind_dependency ||
        params.small_wave_fraction != m_params.small_wave_fraction ||
        params.window_in != m_params.window_in ||
        params.window_out != m_params.window_out ||
		bReinitGaussAndOmega
        )
    {
        bRecalcH0 = TRUE;
    }

	m_params = params;

    if(bRelease)
    {
        releaseAllResources();
    }

    if(bAllocate)
    {
        V_RETURN(allocateAllResources());
    }

    if(bReinitGaussAndOmega)
    {
		m_GaussAndOmegaInitialised = false;
    }

	if(bRecalcH0)
	{
		for(unsigned int i = 0; i < m_numCudaDevices; i ++)
		{
			m_pCudaDeviceStates[i].m_H0Dirty = true;
		}
	}

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::allocateCudaResources()
{
	int gauss_size = m_resolution * m_resolution;
	int h0_size = (m_resolution + 1) * (m_resolution + 1);
	int omega_size = m_half_resolution_plus_one * m_half_resolution_plus_one;
	int htdt_size = m_half_resolution_plus_one * m_resolution;
	int output_size = m_resolution * m_resolution;

	for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
	{
		CudaDeviceState& dev_state = m_pCudaDeviceStates[cuda_dev_index];
		CUDA_V_RETURN(cudaSetDevice(dev_state.m_cudaDevice));

		CUDA_V_RETURN(cudaMalloc((void **)&dev_state.m_device_Gauss, gauss_size * sizeof(float2)));
		CUDA_V_RETURN(cudaMalloc((void **)&dev_state.m_device_H0, h0_size * sizeof(float2)));
		CUDA_V_RETURN(cudaMalloc((void **)&dev_state.m_device_Omega, omega_size * sizeof(float2)));
		CUDA_V_RETURN(cudaMalloc((void **)&dev_state.m_device_Ht, htdt_size * sizeof(float2))); 
		CUDA_V_RETURN(cudaMalloc((void **)&dev_state.m_device_Dt, htdt_size * sizeof(float4))); 

		// Optional completion events for displacements readback
		if(m_params.readback_displacements)
		{
			for(int slot = 0; slot != NumReadbackSlots; ++slot)
			{
				CUDA_V_RETURN(cudaEventCreate(&dev_state.m_readback_completion_evts[slot]));
				CUDA_V_RETURN(cudaEventCreate(&dev_state.m_readback_staging_evts[slot]));
				CUDA_V_RETURN(cudaMalloc((void **)&dev_state.m_readback_device_Dxyzs[slot], output_size * m_readback_element_size));
			}
		}

		// Timer events
		for(int slot = 0; slot != NumTimerSlots; ++slot)
		{
			CUDA_V_RETURN(cudaEventCreate(&dev_state.m_start_timer_evts[slot]));
			CUDA_V_RETURN(cudaEventCreate(&dev_state.m_stop_timer_evts[slot]));
			CUDA_V_RETURN(cudaEventCreate(&dev_state.m_start_fft_timer_evts[slot]));
			CUDA_V_RETURN(cudaEventCreate(&dev_state.m_stop_fft_timer_evts[slot]));
		}
	}

    // Optional page-locked mem for displacements readback
    if(m_params.readback_displacements)
    {
        for(int slot = 0; slot != NumReadbackSlots; ++slot)
        {
            CUDA_V_RETURN(cudaMallocHost((void **)&m_readback_slots[slot].m_host_Dxyz, output_size * m_readback_element_size));
            memset(m_readback_slots[slot].m_host_Dxyz, 0, output_size * m_readback_element_size);
        }

        m_active_readback_slot = 0;
		m_active_readback_host_Dxyz = NULL;
        m_end_inflight_readback_slots = 1;
		m_readback_slots[m_active_readback_slot].m_kickID = GFSDK_WaveWorks_InvalidKickID;

		const int num_readback_FIFO_entries = m_params.num_readback_FIFO_entries;
		if(num_readback_FIFO_entries)
		{
			m_pReadbackFIFO = new CircularFIFO<ReadbackFIFOSlot>(num_readback_FIFO_entries);
			for(int i = 0; i != m_pReadbackFIFO->capacity(); ++i)
			{
				ReadbackFIFOSlot& slot = m_pReadbackFIFO->raw_at(i);
				CUDA_V_RETURN(cudaMallocHost((void **)&slot.host_Dxyz, output_size * m_readback_element_size));
				memset(slot.host_Dxyz, 0, output_size * m_readback_element_size);
				slot.kickID = GFSDK_WaveWorks_InvalidKickID;
			}
		}

        m_ReadbackInitialised = true;
    }

	// Init timer slots
    m_active_timer_slot = 0;
    m_end_inflight_timer_slots = 1;
	m_timer_slots[m_active_timer_slot].m_elapsed_time = 0.f;						// Ensure first call to getTimings() gives reasonable results
	m_timer_slots[m_active_timer_slot].m_kickID = GFSDK_WaveWorks_InvalidKickID;	// Ensure first call to getTimings() gives reasonable results

	for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
	{
		CudaDeviceState& dev_state = m_pCudaDeviceStates[cuda_dev_index];
		CUDA_V_RETURN(cudaSetDevice(dev_state.m_cudaDevice));

		// clear
		CUDA_V_RETURN(cudaMemset(dev_state.m_device_Gauss, 0, gauss_size * sizeof(float2)));
		CUDA_V_RETURN(cudaMemset(dev_state.m_device_H0, 0, h0_size * sizeof(float2)));
		CUDA_V_RETURN(cudaMemset(dev_state.m_device_Omega, 0, omega_size * sizeof(float2)));
		CUDA_V_RETURN(cudaMemset(dev_state.m_device_Ht, 0, htdt_size * sizeof(float2)));
		CUDA_V_RETURN(cudaMemset(dev_state.m_device_Dt, 0, htdt_size * sizeof(float4)));
	}

    m_cudaResourcesInitialised = true;

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::releaseCudaResources()
{
    HRESULT hr;

	if(m_ReadbackInitialised)
	{
		V_RETURN(waitForAllInFlightReadbacks());
	}

	V_RETURN(waitForAllInFlightTimers());

	for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
	{
		CudaDeviceState& dev_state = m_pCudaDeviceStates[cuda_dev_index];
		CUDA_V_RETURN(cudaSetDevice(dev_state.m_cudaDevice));

		CUDA_SAFE_FREE(dev_state.m_device_Gauss);
		CUDA_SAFE_FREE(dev_state.m_device_H0);
		CUDA_SAFE_FREE(dev_state.m_device_Ht);
		CUDA_SAFE_FREE(dev_state.m_device_Dt);
		CUDA_SAFE_FREE(dev_state.m_device_Omega);

		if(m_ReadbackInitialised)
		{
			for(int slot = 0; slot != NumReadbackSlots; ++slot)
			{
				CUDA_V_RETURN(cudaEventDestroy(dev_state.m_readback_completion_evts[slot]));
				CUDA_V_RETURN(cudaEventDestroy(dev_state.m_readback_staging_evts[slot]));
				CUDA_SAFE_FREE(dev_state.m_readback_device_Dxyzs[slot]);
			}
		}

		// Timer events
		for(int slot = 0; slot != NumTimerSlots; ++slot)
		{
			CUDA_V_RETURN(cudaEventDestroy(dev_state.m_start_timer_evts[slot]));
			CUDA_V_RETURN(cudaEventDestroy(dev_state.m_stop_timer_evts[slot]));
			CUDA_V_RETURN(cudaEventDestroy(dev_state.m_start_fft_timer_evts[slot]));
			CUDA_V_RETURN(cudaEventDestroy(dev_state.m_stop_fft_timer_evts[slot]));
		}
	}

    if(m_ReadbackInitialised)
    {
        for(int slot = 0; slot != NumReadbackSlots; ++slot)
        {
            CUDA_SAFE_FREE_HOST(m_readback_slots[slot].m_host_Dxyz);
        }

        m_ReadbackInitialised = false;
    }

	if(m_pReadbackFIFO)
	{
		for(int i = 0; i != m_pReadbackFIFO->capacity(); ++i)
		{
			CUDA_SAFE_FREE_HOST(m_pReadbackFIFO->raw_at(i).host_Dxyz);
		}
		SAFE_DELETE(m_pReadbackFIFO);
	}

    m_cudaResourcesInitialised = false;

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::preKick(int constantsIndex)
{
    HRESULT hr;

	// Check for timers
	if(m_cudaResourcesInitialised)
	{
		V_RETURN(queryTimers());
	}

    // Register displacement map, if necessary
    if(!m_DisplacementMapIsCUDARegistered)
    {
        V_RETURN(registerDisplacementMapWithCUDA());
    }

    // Init cuda resources, if necessary
    if(!m_cudaResourcesInitialised)
    {
        V_RETURN(allocateCudaResources());
        V_RETURN(initGaussAndOmega());
    }
    else if(!m_GaussAndOmegaInitialised)
    {
        V_RETURN(initGaussAndOmega());
    }

	// Be sure to use the correct cuda device for the current frame (important in SLI)
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	CudaDeviceState& dev_state = m_pCudaDeviceStates[activeCudaDeviceIndex];
	const CudaDeviceInfo& dev_info = m_pManager->GetCudaDeviceInfo(activeCudaDeviceIndex);

	if(dev_state.m_H0Dirty || dev_state.m_constantsIndex != constantsIndex) 
	{
		void* device_constants = (char*)dev_info.m_device_constants + dev_info.m_constants_size / MAX_NUM_CASCADES * constantsIndex;

		float wind_dir_len = sqrtf(m_params.wind_dir.x*m_params.wind_dir.x + m_params.wind_dir.y*m_params.wind_dir.y);
		float2 wind_dir = { m_params.wind_dir.x / wind_dir_len, m_params.wind_dir.y / wind_dir_len };

		CUDA_V_RETURN(cuda_SetConstants(device_constants, dev_state.m_device_Gauss, dev_state.m_device_H0,
			dev_state.m_device_Ht, dev_state.m_device_Dt, dev_state.m_device_Omega,
			m_resolution, m_params.fft_period, m_params.window_in, m_params.window_out, 
			wind_dir, m_params.wind_speed, m_params.wind_dependency, m_params.wave_amplitude, 
			m_params.small_wave_fraction, m_params.choppy_scale, dev_info.m_kernel_stream));

		dev_state.m_constantsIndex = constantsIndex;
	}

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::kickPreInterop(double dSimTime, gfsdk_U64 kickID)
{
	HRESULT hr;

	// Be sure to use the correct cuda device for the current frame (important in SLI)
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	CudaDeviceState& dev_state = m_pCudaDeviceStates[activeCudaDeviceIndex];
	const CudaDeviceInfo& dev_info = m_pManager->GetCudaDeviceInfo(activeCudaDeviceIndex);
	// already done in simulation manager (doing it again would flush pushbuffer)
	// CUDA_V_RETURN(cudaSetDevice(dev_state.m_cudaDevice));

	// Start CUDA workload timer
	m_working_timer_slot = NULL;
	if(m_params.enable_CUDA_timers)
	{
		V_RETURN(consumeAvailableTimerSlot(dev_state, kickID, &m_working_timer_slot));
		assert(m_working_timer_slot != NULL);
		CUDA_V_RETURN(cudaEventRecord(m_working_timer_slot->m_start_timer_evt,dev_info.m_kernel_stream));
	}

	// ------------------------------ Update H(0) if necessary ------------------------------------
	if(dev_state.m_H0Dirty) 
	{
		updateH0(dev_state, dev_info.m_kernel_stream);
		dev_state.m_H0Dirty = false;
	}

    // ------------------------------ Calculate H(t) from H(0) ------------------------------------
	const double fModeSimTime = dSimTime * (double)m_params.time_scale;
    CUDA_V_RETURN(cuda_ComputeRows(m_resolution, fModeSimTime, dev_state.m_constantsIndex, dev_info.m_kernel_stream));

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::kickWithinInteropD3D9(gfsdk_U64 kickID)
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	assert(nv_water_d3d_api_d3d9 == m_d3dAPI);

	// Be sure to use the correct cuda device for the current frame (important in SLI)
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	CudaDeviceState& dev_state = m_pCudaDeviceStates[activeCudaDeviceIndex];
	const CudaDeviceInfo& dev_info = m_pManager->GetCudaDeviceInfo(activeCudaDeviceIndex);

	int output_size = m_resolution * m_resolution;

    float4* tex_data = NULL;
	IDirect3DResource9* mapped_resource = m_d3d._9.m_pd3d9PerCudaDeviceResources[activeCudaDeviceIndex].m_pd3d9DisplacementMap;
    CUDA_V_RETURN(cudaD3D9ResourceGetMappedPointer((void**)&tex_data, mapped_resource, 0, 0));

    // Fill displacement texture
    CUDA_V_RETURN(cuda_ComputeColumns(tex_data, m_resolution, dev_state.m_constantsIndex, dev_info.m_kernel_stream));

    // Optionally, get data staged for readback
	m_working_readback_slot = NULL;
	if(m_ReadbackInitialised) {
		V_RETURN(consumeAvailableReadbackSlot(dev_state, kickID, &m_working_readback_slot));
		CUDA_V_RETURN(cudaMemcpyAsync(m_working_readback_slot->m_device_Dxyz, tex_data, output_size * sizeof(float4), cudaMemcpyDeviceToDevice, dev_info.m_kernel_stream));

		// The copy out of staging is done on a separate stream with the goal of allowing the copy to occur
		// in parallel with other GPU workloads, so we need to do some inter-stream sync here
		CUDA_V_RETURN(cudaEventRecord(m_working_readback_slot->m_staging_evt,dev_info.m_kernel_stream));
	}

	// CUDA workload is done, stop the clock and unmap as soon as we can so as not to block the graphics pipe
	if(m_working_timer_slot)
	{
		CUDA_V_RETURN(cudaEventRecord(m_working_timer_slot->m_stop_timer_evt,dev_info.m_kernel_stream));
	}
#endif

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::kickWithinInteropD3D10(gfsdk_U64 kickID)
{
#if WAVEWORKS_ENABLE_D3D10
	HRESULT hr;

	assert(nv_water_d3d_api_d3d10 == m_d3dAPI);

	// Be sure to use the correct cuda device for the current frame (important in SLI)
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	CudaDeviceState& dev_state = m_pCudaDeviceStates[activeCudaDeviceIndex];
	const CudaDeviceInfo& dev_info = m_pManager->GetCudaDeviceInfo(activeCudaDeviceIndex);

	int output_size = m_resolution * m_resolution;

    float4* tex_data = NULL;
	ID3D10Resource* mapped_resource = m_d3d._10.m_pd3d10PerCudaDeviceResources[activeCudaDeviceIndex].m_pd3d10DisplacementMapResource;
    CUDA_V_RETURN(cudaD3D10ResourceGetMappedPointer((void**)&tex_data, mapped_resource, 0));

    // Fill displacement texture
	CUDA_V_RETURN(cuda_ComputeColumns(tex_data, m_resolution, dev_state.m_constantsIndex, dev_info.m_kernel_stream));

    // Optionally, get data staged for readback
	m_working_readback_slot = NULL;
	if(m_ReadbackInitialised) {
		V_RETURN(consumeAvailableReadbackSlot(dev_state, kickID, &m_working_readback_slot));
		CUDA_V_RETURN(cudaMemcpyAsync(m_working_readback_slot->m_device_Dxyz, tex_data, output_size * sizeof(float4), cudaMemcpyDeviceToDevice, dev_info.m_kernel_stream));

		// The copy out of staging is done on a separate stream with the goal of allowing the copy to occur
		// in parallel with other GPU workloads, so we need to do some inter-stream sync here
		CUDA_V_RETURN(cudaEventRecord(m_working_readback_slot->m_staging_evt,dev_info.m_kernel_stream));
	}

	// CUDA workload is done, stop the clock and unmap as soon as we can so as not to block the graphics pipe
	if(m_working_timer_slot)
	{
		CUDA_V_RETURN(cudaEventRecord(m_working_timer_slot->m_stop_timer_evt,dev_info.m_kernel_stream));
	}
#endif

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::kickWithinInteropD3D11(gfsdk_U64 kickID)
{
#if WAVEWORKS_ENABLE_D3D11
	HRESULT hr;

	assert(nv_water_d3d_api_d3d11 == m_d3dAPI);

	// Be sure to use the correct cuda device for the current frame (important in SLI)
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	CudaDeviceState& dev_state = m_pCudaDeviceStates[activeCudaDeviceIndex];
	const CudaDeviceInfo& dev_info = m_pManager->GetCudaDeviceInfo(activeCudaDeviceIndex);

    cudaArray* tex_array;
	cudaGraphicsResource* mapped_resource = m_d3d._11.m_pd3d11PerCudaDeviceResources[activeCudaDeviceIndex].m_pd3d11RegisteredDisplacementMapResource;
    CUDA_V_RETURN(cudaGraphicsSubResourceGetMappedArray(&tex_array, mapped_resource, 0, 0));

    // Fill displacement texture
	CUDA_V_RETURN(cuda_ComputeColumns_array(tex_array, m_resolution, dev_state.m_constantsIndex, dev_info.m_kernel_stream));

    // Optionally, get data staged for readback
	m_working_readback_slot = NULL;
	if(m_ReadbackInitialised) {
		V_RETURN(consumeAvailableReadbackSlot(dev_state, kickID, &m_working_readback_slot));
		CUDA_V_RETURN(cudaMemcpy2DFromArrayAsync(	m_working_readback_slot->m_device_Dxyz,
													m_resolution * sizeof(ushort4),
													tex_array, 0, 0,
													m_resolution * sizeof(ushort4),
													m_resolution,
													cudaMemcpyDeviceToDevice,
													dev_info.m_kernel_stream
													));

		// The copy out of staging is done on a separate stream with the goal of allowing the copy to occur
		// in parallel with other GPU workloads, so we need to do some inter-stream sync here
		CUDA_V_RETURN(cudaEventRecord(m_working_readback_slot->m_staging_evt,dev_info.m_kernel_stream));
	}

	// CUDA workload is done, stop the clock and unmap as soon as we can so as not to block the graphics pipe
	if(m_working_timer_slot)
	{
		CUDA_V_RETURN(cudaEventRecord(m_working_timer_slot->m_stop_timer_evt,dev_info.m_kernel_stream));
	}
#endif

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::kickWithinInteropGL2(gfsdk_U64 kickID)
{
#if WAVEWORKS_ENABLE_GL
	HRESULT hr;

	assert(nv_water_d3d_api_gl2 == m_d3dAPI);

	// Be sure to use the correct cuda device for the current frame (important in SLI)
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	CudaDeviceState& dev_state = m_pCudaDeviceStates[activeCudaDeviceIndex];
	const CudaDeviceInfo& dev_info = m_pManager->GetCudaDeviceInfo(activeCudaDeviceIndex);

    cudaArray* tex_array;
	cudaGraphicsResource* mapped_resource = m_d3d._GL2.m_pGL2PerCudaDeviceResources[activeCudaDeviceIndex].m_pGL2RegisteredDisplacementMapResource;
    CUDA_V_RETURN(cudaGraphicsSubResourceGetMappedArray(&tex_array, mapped_resource, 0, 0));

    // Fill displacement texture
	CUDA_V_RETURN(cuda_ComputeColumns_array(tex_array, m_resolution, dev_state.m_constantsIndex, dev_info.m_kernel_stream));

	// Copy to GL texture
	// CUDA_V_RETURN(cudaMemcpyToArray(tex_array, 0, 0, cuda_dest_resource, size_tex_data, cudaMemcpyDeviceToDevice));
		
    // Optionally, get data staged for readback
	m_working_readback_slot = NULL;
	if(m_ReadbackInitialised) {
		V_RETURN(consumeAvailableReadbackSlot(dev_state, kickID, &m_working_readback_slot));
		CUDA_V_RETURN(cudaMemcpy2DFromArrayAsync(	m_working_readback_slot->m_device_Dxyz,
													m_resolution * sizeof(ushort4),
													tex_array, 0, 0,
													m_resolution * sizeof(ushort4),
													m_resolution,
													cudaMemcpyDeviceToDevice
													));
	}

	// CUDA workload is done, stop the clock and unmap as soon as we can so as not to block the graphics pipe
	if(m_working_timer_slot)
	{
		CUDA_V_RETURN(cudaEventRecord(m_working_timer_slot->m_stop_timer_evt,dev_info.m_kernel_stream));
	}
#endif

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::kickWithinInteropNoGfx(gfsdk_U64 kickID)
{
	HRESULT hr;

	assert(nv_water_d3d_api_none == m_d3dAPI);

	// Be sure to use the correct cuda device for the current frame (important in SLI)
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	CudaDeviceState& dev_state = m_pCudaDeviceStates[activeCudaDeviceIndex];
	const CudaDeviceInfo& dev_info = m_pManager->GetCudaDeviceInfo(activeCudaDeviceIndex);

	int output_size = m_resolution * m_resolution;

	float4* device_displacementMap = m_d3d._noGFX.m_pNoGraphicsPerCudaDeviceResources[activeCudaDeviceIndex].m_Device_displacementMap;

    // Fill displacement texture
	CUDA_V_RETURN(cuda_ComputeColumns(device_displacementMap, m_resolution, dev_state.m_constantsIndex, dev_info.m_kernel_stream));

    // Optionally, get data staged for readback
	m_working_readback_slot = NULL;
	if(m_ReadbackInitialised) {
		V_RETURN(consumeAvailableReadbackSlot(dev_state, kickID, &m_working_readback_slot));
		CUDA_V_RETURN(cudaMemcpyAsync(m_working_readback_slot->m_device_Dxyz, device_displacementMap, output_size * sizeof(float4), cudaMemcpyDeviceToDevice, dev_info.m_kernel_stream));

		// The copy out of staging is done on a separate stream with the goal of allowing the copy to occur
		// in parallel with other GPU workloads, so we need to do some inter-stream sync here
		CUDA_V_RETURN(cudaEventRecord(m_working_readback_slot->m_staging_evt,dev_info.m_kernel_stream));
	}

	// CUDA workload is done, stop the clock
	if(m_working_timer_slot)
	{
		CUDA_V_RETURN(cudaEventRecord(m_working_timer_slot->m_stop_timer_evt,dev_info.m_kernel_stream));
	}

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::kickWithinInterop(gfsdk_U64 kickID)
{
	HRESULT hr;

    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        {
			V_RETURN(kickWithinInteropD3D9(kickID));
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
			V_RETURN(kickWithinInteropD3D10(kickID));
        }
		break;
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			V_RETURN(kickWithinInteropD3D11(kickID));
        }
        break;
#endif
#if WAVEWORKS_ENABLE_GL
    case nv_water_d3d_api_gl2:
        {
			V_RETURN(kickWithinInteropGL2(kickID));
        }
        break;
#endif
	case nv_water_d3d_api_none:
		{
			V_RETURN(kickWithinInteropNoGfx(kickID));
		}
		break;
	default:
		return E_FAIL;
	}

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::kickPostInterop(gfsdk_U64 kickID)
{
	// Be sure to use the correct cuda device for the current frame (important in SLI)
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	const CudaDeviceInfo& dev_info = m_pManager->GetCudaDeviceInfo(activeCudaDeviceIndex);

	const int output_size = m_resolution * m_resolution;

	if(m_working_readback_slot) {
		// Do readback out of staging area
		CUDA_V_RETURN(cudaStreamWaitEvent(dev_info.m_readback_stream,m_working_readback_slot->m_staging_evt,0));
		CUDA_V_RETURN(cudaMemcpyAsync(m_working_readback_slot->m_host_Dxyz, m_working_readback_slot->m_device_Dxyz, output_size * m_readback_element_size, cudaMemcpyDeviceToHost, dev_info.m_readback_stream));
		CUDA_V_RETURN(cudaEventRecord(m_working_readback_slot->m_completion_evt,dev_info.m_readback_stream));
	}

    // Update displacement map version
    m_DisplacementMapVersion = kickID;

	// We're done with slots
	m_working_readback_slot = NULL;
	m_working_timer_slot = NULL;

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::allocateAllResources()
{
    HRESULT hr;

	m_resolution = m_params.fft_resolution;
	m_half_resolution_plus_one = m_resolution / 2 + 1;

    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        {
			m_d3d._9.m_pd3d9PerCudaDeviceResources = new D3D9Objects::PerCudaDeviceResources[m_numCudaDevices];

			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D9Objects::PerCudaDeviceResources& pcdr = m_d3d._9.m_pd3d9PerCudaDeviceResources[cuda_dev_index];
				V_RETURN(m_d3d._9.m_pd3d9Device->CreateTexture(m_resolution, m_resolution, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &pcdr.m_pd3d9DisplacementMap, NULL));
				pcdr.m_d3d9DisplacementmapIsRegistered = false;
			}
        }
        break;
#endif

#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
			m_d3d._10.m_pd3d10PerCudaDeviceResources = new D3D10Objects::PerCudaDeviceResources[m_numCudaDevices];

            // Create displacement map
            D3D10_TEXTURE2D_DESC displacementMapTD;
            displacementMapTD.Width = m_resolution;
            displacementMapTD.Height = m_resolution;
            displacementMapTD.MipLevels = 1;
            displacementMapTD.ArraySize = 1;
            displacementMapTD.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            displacementMapTD.SampleDesc = kNoSample;
            displacementMapTD.Usage = D3D10_USAGE_DEFAULT;
            displacementMapTD.BindFlags = D3D10_BIND_SHADER_RESOURCE;
            displacementMapTD.CPUAccessFlags = 0;
            displacementMapTD.MiscFlags = 0;

			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D10Objects::PerCudaDeviceResources& pcdr = m_d3d._10.m_pd3d10PerCudaDeviceResources[cuda_dev_index];
				V_RETURN(m_d3d._10.m_pd3d10Device->CreateTexture2D(&displacementMapTD, NULL, &pcdr.m_pd3d10DisplacementMapResource));
				V_RETURN(m_d3d._10.m_pd3d10Device->CreateShaderResourceView(pcdr.m_pd3d10DisplacementMapResource, NULL, &pcdr.m_pd3d10DisplacementMap));
				pcdr.m_d3d10DisplacementmapIsRegistered = false;
			}
        }
		break;
#endif

#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			m_d3d._11.m_pd3d11PerCudaDeviceResources = new D3D11Objects::PerCudaDeviceResources[m_numCudaDevices];

            // Create displacement maps
            D3D11_TEXTURE2D_DESC displacementMapTD;
            displacementMapTD.Width = m_resolution;
            displacementMapTD.Height = m_resolution;
            displacementMapTD.MipLevels = 1;
            displacementMapTD.ArraySize = 1;
            displacementMapTD.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            displacementMapTD.SampleDesc = kNoSample;
            displacementMapTD.Usage = D3D11_USAGE_DEFAULT;
            displacementMapTD.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            displacementMapTD.CPUAccessFlags = 0;
            displacementMapTD.MiscFlags = 0;

			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D11Objects::PerCudaDeviceResources& pcdr = m_d3d._11.m_pd3d11PerCudaDeviceResources[cuda_dev_index];
				V_RETURN(m_d3d._11.m_pd3d11Device->CreateTexture2D(&displacementMapTD, NULL, &pcdr.m_pd3d11DisplacementMapResource));
				V_RETURN(m_d3d._11.m_pd3d11Device->CreateShaderResourceView(pcdr.m_pd3d11DisplacementMapResource, NULL, &pcdr.m_pd3d11DisplacementMap));
				pcdr.m_pd3d11RegisteredDisplacementMapResource = NULL;
			}
        }
		break;
#endif
#if WAVEWORKS_ENABLE_GL
    case nv_water_d3d_api_gl2:
		{
			m_d3d._GL2.m_pGL2PerCudaDeviceResources = new GL2Objects::PerCudaDeviceResources[m_numCudaDevices];

            // Create displacement maps
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				GL2Objects::PerCudaDeviceResources& pcdr = m_d3d._GL2.m_pGL2PerCudaDeviceResources[cuda_dev_index];
				if(pcdr.m_GL2DisplacementMapTexture !=0 ) NVSDK_GLFunctions.glDeleteTextures(1, &pcdr.m_GL2DisplacementMapTexture); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glGenTextures(1,&pcdr.m_GL2DisplacementMapTexture); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, pcdr.m_GL2DisplacementMapTexture); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_resolution, m_resolution, 0, GL_RGBA, GL_FLOAT, NULL); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERRORS;				
				pcdr.m_pGL2RegisteredDisplacementMapResource = NULL;
			}
        }
		break;
#endif
    case nv_water_d3d_api_none:
        {
			m_d3d._noGFX.m_pNoGraphicsPerCudaDeviceResources = new NoGraphicsObjects::PerCudaDeviceResources[m_numCudaDevices];
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				NoGraphicsObjects::PerCudaDeviceResources& pcdr = m_d3d._noGFX.m_pNoGraphicsPerCudaDeviceResources[cuda_dev_index];
				pcdr.m_Device_displacementMap = NULL;
			}
        }
		break;

    default:
        return E_FAIL;
    }

	// Remaining allocations are deferred, in order to ensure that they occur on the host's simulation thread
    m_cudaResourcesInitialised = false;
    m_DisplacementMapIsCUDARegistered = false;
	m_GaussAndOmegaInitialised = false;
    for(unsigned int i = 0; i < m_numCudaDevices; i++)
	{
		m_pCudaDeviceStates[i].m_H0Dirty = true;
	}
    m_ReadbackInitialised = false;

	// Displacement map contents are initially undefined
	m_DisplacementMapVersion = GFSDK_WaveWorks_InvalidKickID;

    return S_OK;
}

void NVWaveWorks_FFT_Simulation_CUDA_Impl::releaseAll()
{
	releaseAllResources();

#if WAVEWORKS_ENABLE_GRAPHICS
    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        {
			SAFE_RELEASE(m_d3d._9.m_pd3d9Device);
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
			SAFE_RELEASE(m_d3d._10.m_pd3d10Device);
        }
		break;
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			SAFE_RELEASE(m_d3d._11.m_pd3d11Device);
        }
        break;
#endif
#if WAVEWORKS_ENABLE_GL
    case nv_water_d3d_api_gl2:
        {
			// nothing to do?
        }
        break;
#endif
	}
#endif

	m_d3dAPI = nv_water_d3d_api_undefined;

	SAFE_DELETE_ARRAY(m_pCudaDeviceStates);
	m_numCudaDevices = 0;
}

void NVWaveWorks_FFT_Simulation_CUDA_Impl::releaseAllResources()
{
    if(m_DisplacementMapIsCUDARegistered)
    {
        unregisterDisplacementMapWithCUDA();
    }

    if(m_cudaResourcesInitialised)
    {
        releaseCudaResources();
    }

    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        {
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D9Objects::PerCudaDeviceResources& pcdr = m_d3d._9.m_pd3d9PerCudaDeviceResources[cuda_dev_index];
				SAFE_RELEASE(pcdr.m_pd3d9DisplacementMap);
			}

			SAFE_DELETE_ARRAY(m_d3d._9.m_pd3d9PerCudaDeviceResources);
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D10Objects::PerCudaDeviceResources& pcdr = m_d3d._10.m_pd3d10PerCudaDeviceResources[cuda_dev_index];
				SAFE_RELEASE(pcdr.m_pd3d10DisplacementMapResource);
				SAFE_RELEASE(pcdr.m_pd3d10DisplacementMap);
			}

			SAFE_DELETE_ARRAY(m_d3d._10.m_pd3d10PerCudaDeviceResources);
        }
		break;
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D11Objects::PerCudaDeviceResources& pcdr = m_d3d._11.m_pd3d11PerCudaDeviceResources[cuda_dev_index];
				SAFE_RELEASE(pcdr.m_pd3d11DisplacementMapResource);
				SAFE_RELEASE(pcdr.m_pd3d11DisplacementMap);
			}

			SAFE_DELETE_ARRAY(m_d3d._11.m_pd3d11PerCudaDeviceResources);
        }
        break;
#endif
#if WAVEWORKS_ENABLE_GL
    case nv_water_d3d_api_gl2:
        {
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				GL2Objects::PerCudaDeviceResources& pcdr = m_d3d._GL2.m_pGL2PerCudaDeviceResources[cuda_dev_index];
				if(pcdr.m_GL2DisplacementMapTexture !=0) NVSDK_GLFunctions.glDeleteTextures(1, &pcdr.m_GL2DisplacementMapTexture); CHECK_GL_ERRORS;
			}
			SAFE_DELETE_ARRAY(m_d3d._GL2.m_pGL2PerCudaDeviceResources);
        }
        break;
#endif
	case nv_water_d3d_api_none:
        {
			SAFE_DELETE_ARRAY(m_d3d._noGFX.m_pNoGraphicsPerCudaDeviceResources);
        }
        break;
    }
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::registerDisplacementMapWithCUDA()
{
    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        {
			bool all_registered = true;
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D9Objects::PerCudaDeviceResources& pcdr = m_d3d._9.m_pd3d9PerCudaDeviceResources[cuda_dev_index];
				if(pcdr.m_pd3d9DisplacementMap)
				{
					if(!pcdr.m_d3d9DisplacementmapIsRegistered)
					{
						CUDA_V_RETURN(cudaSetDevice(m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice));
						CUDA_V_RETURN(cudaD3D9RegisterResource(pcdr.m_pd3d9DisplacementMap, cudaD3D9RegisterFlagsNone));
						CUDA_V_RETURN(cudaD3D9ResourceSetMapFlags(pcdr.m_pd3d9DisplacementMap,cudaD3D9MapFlagsWriteDiscard));
						pcdr.m_d3d9DisplacementmapIsRegistered = true;
					}
				}
				else
				{
					all_registered = false;
				}
			}
			m_DisplacementMapIsCUDARegistered = all_registered;
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
			bool all_registered = true;
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D10Objects::PerCudaDeviceResources& pcdr = m_d3d._10.m_pd3d10PerCudaDeviceResources[cuda_dev_index];
				if(pcdr.m_pd3d10DisplacementMapResource)
				{
					if(!pcdr.m_d3d10DisplacementmapIsRegistered)
					{
						CUDA_V_RETURN(cudaSetDevice(m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice));
						CUDA_V_RETURN(cudaD3D10RegisterResource(pcdr.m_pd3d10DisplacementMapResource, cudaD3D10RegisterFlagsNone));
						CUDA_V_RETURN(cudaD3D10ResourceSetMapFlags(pcdr.m_pd3d10DisplacementMapResource,cudaD3D10MapFlagsWriteDiscard));
						pcdr.m_d3d10DisplacementmapIsRegistered = true;
					}
				}
				else
				{
					all_registered = false;
				}
			}
			m_DisplacementMapIsCUDARegistered = all_registered;
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			bool all_registered = true;
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D11Objects::PerCudaDeviceResources& pcdr = m_d3d._11.m_pd3d11PerCudaDeviceResources[cuda_dev_index];
				if(pcdr.m_pd3d11DisplacementMapResource)
				{
					if(NULL == pcdr.m_pd3d11RegisteredDisplacementMapResource)
					{
						CUDA_V_RETURN(cudaSetDevice(m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice));
						CUDA_V_RETURN(cudaGraphicsD3D11RegisterResource(&pcdr.m_pd3d11RegisteredDisplacementMapResource, pcdr.m_pd3d11DisplacementMapResource, cudaGraphicsRegisterFlagsSurfaceLoadStore));
						CUDA_V_RETURN(cudaGraphicsResourceSetMapFlags(pcdr.m_pd3d11RegisteredDisplacementMapResource, cudaGraphicsMapFlagsWriteDiscard));
					}
				}
				else
				{
					all_registered = false;
				}
			}
			m_DisplacementMapIsCUDARegistered = all_registered;
        }
        break;
#endif
#if WAVEWORKS_ENABLE_GL
    case nv_water_d3d_api_gl2:
        {
			bool all_registered = true;
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				GL2Objects::PerCudaDeviceResources& pcdr = m_d3d._GL2.m_pGL2PerCudaDeviceResources[cuda_dev_index];
				if(pcdr.m_GL2DisplacementMapTexture)
				{
					if(NULL == pcdr.m_pGL2RegisteredDisplacementMapResource)
					{
						CUDA_V_RETURN(cudaSetDevice(m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice));
						CUDA_V_RETURN(cudaGraphicsGLRegisterImage(&pcdr.m_pGL2RegisteredDisplacementMapResource, pcdr.m_GL2DisplacementMapTexture, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsSurfaceLoadStore));
					}
				}
				else
				{
					all_registered = false;
				}
			}
			m_DisplacementMapIsCUDARegistered = all_registered;
        }
        break;
#endif
    case nv_water_d3d_api_none:
        {
			int output_size = m_resolution * m_resolution;

			// Well this is something of a fake - there's no graphics, so no interop as such, however we can re-use all our existing infrastucture
			// if we use a simple CUDA device alloc instead
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				NoGraphicsObjects::PerCudaDeviceResources& pcdr = m_d3d._noGFX.m_pNoGraphicsPerCudaDeviceResources[cuda_dev_index];
				CUDA_V_RETURN(cudaSetDevice(m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice));
				CUDA_V_RETURN(cudaMalloc((void **)&pcdr.m_Device_displacementMap, output_size * sizeof(float4)));
			}
			m_DisplacementMapIsCUDARegistered = true;
        }
        break;
     default:
        return E_FAIL;
    }

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::unregisterDisplacementMapWithCUDA()
{
    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        {
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D9Objects::PerCudaDeviceResources& pcdr = m_d3d._9.m_pd3d9PerCudaDeviceResources[cuda_dev_index];
				if(pcdr.m_pd3d9DisplacementMap)
				{
					CUDA_V_RETURN(cudaSetDevice(m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice));
					CUDA_V_RETURN(cudaD3D9UnregisterResource(pcdr.m_pd3d9DisplacementMap));
					pcdr.m_d3d9DisplacementmapIsRegistered = false;
				}
			}
            m_DisplacementMapIsCUDARegistered = false;
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D10Objects::PerCudaDeviceResources& pcdr = m_d3d._10.m_pd3d10PerCudaDeviceResources[cuda_dev_index];
				if(pcdr.m_pd3d10DisplacementMapResource)
				{
					CUDA_V_RETURN(cudaSetDevice(m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice));
					CUDA_V_RETURN(cudaD3D10UnregisterResource(pcdr.m_pd3d10DisplacementMapResource));
					pcdr.m_d3d10DisplacementmapIsRegistered = false;
				}
			}
            m_DisplacementMapIsCUDARegistered = false;
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				D3D11Objects::PerCudaDeviceResources& pcdr = m_d3d._11.m_pd3d11PerCudaDeviceResources[cuda_dev_index];
				if(pcdr.m_pd3d11DisplacementMapResource)
				{
					CUDA_V_RETURN(cudaSetDevice(m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice));
					CUDA_V_RETURN(cudaGraphicsUnregisterResource(pcdr.m_pd3d11RegisteredDisplacementMapResource));
					pcdr.m_pd3d11RegisteredDisplacementMapResource = NULL;
				}
			}
            m_DisplacementMapIsCUDARegistered = false;
        }
        break;
#endif
#if WAVEWORKS_ENABLE_GL
    case nv_water_d3d_api_gl2:
        {
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				GL2Objects::PerCudaDeviceResources& pcdr = m_d3d._GL2.m_pGL2PerCudaDeviceResources[cuda_dev_index];
				if(pcdr.m_GL2DisplacementMapTexture != 0)
				{
					CUDA_V_RETURN(cudaSetDevice(m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice));
					CUDA_V_RETURN(cudaGraphicsUnregisterResource(pcdr.m_pGL2RegisteredDisplacementMapResource));
					pcdr.m_pGL2RegisteredDisplacementMapResource = NULL;
				}
			}
            m_DisplacementMapIsCUDARegistered = false;
        }
        break;
#endif
	case nv_water_d3d_api_none:
        {
			for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
			{
				NoGraphicsObjects::PerCudaDeviceResources& pcdr = m_d3d._noGFX.m_pNoGraphicsPerCudaDeviceResources[cuda_dev_index];
				if(pcdr.m_Device_displacementMap)
				{
					CUDA_V_RETURN(cudaSetDevice(m_pCudaDeviceStates[cuda_dev_index].m_cudaDevice));
					CUDA_SAFE_FREE(pcdr.m_Device_displacementMap);
				}
			}
            m_DisplacementMapIsCUDARegistered = false;
        }
        break;
     default:
        return E_FAIL;
    }

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::updateH0(const CudaDeviceState& cu_dev_state, cudaStream_t cu_kernel_stream)
{
	CUDA_V_RETURN(cuda_ComputeH0(m_resolution, cu_dev_state.m_constantsIndex, cu_kernel_stream));

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::initGaussAndOmega()
{
	int omega_width = m_resolution + 4;
	int gauss_width = gauss_map_resolution + 4;

	float2* gauss = new float2[gauss_map_size];
	float* omega = new float[omega_width * (m_resolution + 1)];

	GFSDK_WaveWorks_Simulation_Util::init_gauss(m_params, gauss);
	GFSDK_WaveWorks_Simulation_Util::init_omega(m_params, omega);

	// copy actually used gauss window around center of max resolution buffer
	// note that we need to generate full resolution to maintain pseudo-randomness
	float2* gauss_src = gauss + (gauss_map_resolution - m_resolution) / 2 * (1 + gauss_width);
	for(int i=0; i<m_resolution; ++i)
		memmove(gauss + i * m_resolution, gauss_src + i * gauss_width, m_resolution * sizeof(float2));

	// strip unneeded padding
	for(int i=0; i<m_half_resolution_plus_one; ++i)
		memmove(omega + i * m_half_resolution_plus_one, omega + i * omega_width, m_half_resolution_plus_one * sizeof(float));

	int gauss_size = m_resolution * m_resolution;
	int omega_size = m_half_resolution_plus_one * m_half_resolution_plus_one;

	for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
	{
		CudaDeviceState& dev_state = m_pCudaDeviceStates[cuda_dev_index];
		CUDA_V_RETURN(cudaSetDevice(dev_state.m_cudaDevice));

		CUDA_V_RETURN(cudaMemcpy(dev_state.m_device_Gauss, gauss, gauss_size * sizeof(float2), cudaMemcpyHostToDevice));
		CUDA_V_RETURN(cudaMemcpy(dev_state.m_device_Omega, omega, omega_size * sizeof(float), cudaMemcpyHostToDevice));
	}

    SAFE_DELETE_ARRAY(gauss);
    SAFE_DELETE_ARRAY(omega);

    m_GaussAndOmegaInitialised = true;

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::waitForAllInFlightReadbacks()
{
	HRESULT hr;

	// Consume the readbacks
	int wait_slot = (m_active_readback_slot + 1) % NumReadbackSlots;
	while(wait_slot != m_end_inflight_readback_slots)
	{
		V_RETURN(collectSingleReadbackResult(true));
		wait_slot = (m_active_readback_slot + 1) % NumReadbackSlots;
	}

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::waitForAllInFlightTimers()
{
    // The slot after the active slot is always the first in-flight slot
    for(	int slot = (m_active_timer_slot + 1) % NumTimerSlots;
            slot != m_end_inflight_timer_slots;
            slot = (slot + 1) % NumTimerSlots
            )
    {
		CUDA_V_RETURN(cudaSetDevice(m_timer_slots[slot].m_cudaDevice));
        CUDA_V_RETURN(cudaEventSynchronize(m_timer_slots[slot].m_start_timer_evt));
		CUDA_V_RETURN(cudaEventSynchronize(m_timer_slots[slot].m_stop_timer_evt));
    }

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::consumeAvailableReadbackSlot(CudaDeviceState& cu_dev_state, gfsdk_U64 kickID, ReadbackSlot** ppSlot)
{
    if(m_active_readback_slot == m_end_inflight_readback_slots)
    {
        // No slots available - we must wait for the oldest in-flight readback to complete
        int wait_slot = (m_active_readback_slot + 1) % NumReadbackSlots;
		CUDA_V_RETURN(cudaSetDevice(m_readback_slots[wait_slot].m_cudaDevice));
        CUDA_V_RETURN(cudaEventSynchronize(m_readback_slots[wait_slot].m_completion_evt));
        m_active_readback_slot = wait_slot;
		m_active_readback_host_Dxyz = m_readback_slots[wait_slot].m_host_Dxyz;

		// Restore the CUDA device
		CUDA_V_RETURN(cudaSetDevice(cu_dev_state.m_cudaDevice));
    }

    // Consume a slot!
    *ppSlot = &m_readback_slots[m_end_inflight_readback_slots];
	(*ppSlot)->m_cudaDevice = cu_dev_state.m_cudaDevice;
	(*ppSlot)->m_completion_evt = cu_dev_state.m_readback_completion_evts[m_end_inflight_readback_slots];
	(*ppSlot)->m_staging_evt = cu_dev_state.m_readback_staging_evts[m_end_inflight_readback_slots];
	(*ppSlot)->m_device_Dxyz = cu_dev_state.m_readback_device_Dxyzs[m_end_inflight_readback_slots];
	(*ppSlot)->m_kickID = kickID;
    m_end_inflight_readback_slots = (m_end_inflight_readback_slots + 1) % NumReadbackSlots;

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::consumeAvailableTimerSlot(CudaDeviceState& cu_dev_state, gfsdk_U64 kickID, TimerSlot** ppSlot)
{
	HRESULT hr;

    if(m_active_timer_slot == m_end_inflight_timer_slots)
    {
        // No slots available - we must wait for the oldest in-flight timer to complete
        int wait_slot = (m_active_timer_slot + 1) % NumTimerSlots;
		CUDA_V_RETURN(cudaSetDevice(m_timer_slots[wait_slot].m_cudaDevice));
        CUDA_V_RETURN(cudaEventSynchronize(m_timer_slots[wait_slot].m_start_timer_evt));
		CUDA_V_RETURN(cudaEventSynchronize(m_timer_slots[wait_slot].m_stop_timer_evt));
        m_active_timer_slot = wait_slot;
		V_RETURN(getElapsedTimeForActiveSlot());

		// Restore the CUDA device
		CUDA_V_RETURN(cudaSetDevice(cu_dev_state.m_cudaDevice));
    }

    // Consume a slot!
    *ppSlot = &m_timer_slots[m_end_inflight_timer_slots];
	(*ppSlot)->m_cudaDevice = cu_dev_state.m_cudaDevice;
	(*ppSlot)->m_start_timer_evt = cu_dev_state.m_start_timer_evts[m_end_inflight_timer_slots];
	(*ppSlot)->m_stop_timer_evt = cu_dev_state.m_stop_timer_evts[m_end_inflight_timer_slots];
	(*ppSlot)->m_elapsed_time = 0.f;
	(*ppSlot)->m_kickID = kickID;
    m_end_inflight_timer_slots = (m_end_inflight_timer_slots + 1) % NumTimerSlots;

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::resetReadbacks()
{
	HRESULT hr;

	if(!m_ReadbackInitialised)
	{
		// Nothing to reset
		return S_OK;
	}

	V_RETURN(waitForAllInFlightReadbacks());

    m_active_readback_slot = 0;
	m_active_readback_host_Dxyz = NULL;
    m_end_inflight_readback_slots = 1;
	m_readback_slots[m_active_readback_slot].m_kickID = GFSDK_WaveWorks_InvalidKickID;

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::canCollectSingleReadbackResultWithoutBlocking()
{
	if(!m_ReadbackInitialised)
	{
		return S_FALSE;
	}

    const int wait_slot = (m_active_readback_slot + 1) % NumReadbackSlots;
	if(wait_slot == m_end_inflight_readback_slots)
	{
		// Nothing in-flight...
		return S_FALSE;
	}

	// Do the query
	CUDA_V_RETURN(cudaSetDevice(m_readback_slots[wait_slot].m_cudaDevice));
	const cudaError_t query_result = cudaEventQuery(m_readback_slots[wait_slot].m_completion_evt);
	if(cudaSuccess == query_result)
	{
		// Whaddyaknow, it's ready!
		return S_OK;
	}
	else if(cudaQueryResultIsError(query_result))
	{
		// Fail
		return E_FAIL;
	}
	else
	{
		// Not ready
		return S_FALSE;
	}
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::collectSingleReadbackResult(bool blocking)
{
	if(!m_ReadbackInitialised)
	{
		return S_FALSE;
	}

    const int wait_slot = (m_active_readback_slot + 1) % NumReadbackSlots;

    // Just consume one readback result per check (per function name!)
    if(wait_slot != m_end_inflight_readback_slots)
    {
		CUDA_V_RETURN(cudaSetDevice(m_readback_slots[wait_slot].m_cudaDevice));

		if(blocking)
		{
			CUDA_V_RETURN(cudaEventSynchronize(m_readback_slots[wait_slot].m_completion_evt));
            m_active_readback_slot = wait_slot;
			m_active_readback_host_Dxyz = m_readback_slots[wait_slot].m_host_Dxyz;
			return S_OK;
		}
		else
		{
			const cudaError_t query_result = cudaEventQuery(m_readback_slots[wait_slot].m_completion_evt);
			if(cudaSuccess == query_result)
			{
				m_active_readback_slot = wait_slot;
				m_active_readback_host_Dxyz = m_readback_slots[wait_slot].m_host_Dxyz;
				return S_OK;
			}
			else if(cudaQueryResultIsError(query_result))
			{
				return E_FAIL;
			}
		}
    }

	// Nothing in-flight, or else not ready yet
    return S_FALSE;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::getElapsedTimeForActiveSlot()
{
	float elapsed_ms;
	CUDA_V_RETURN(cudaEventElapsedTime(&elapsed_ms, m_timer_slots[m_active_timer_slot].m_start_timer_evt, m_timer_slots[m_active_timer_slot].m_stop_timer_evt));
	m_timer_slots[m_active_timer_slot].m_elapsed_time = elapsed_ms;

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::queryTimers()
{
	HRESULT hr;

    const int wait_slot = (m_active_timer_slot + 1) % NumTimerSlots;

    // Just consume one timer result per check
    if(wait_slot != m_end_inflight_timer_slots)
    {
		CUDA_V_RETURN(cudaSetDevice(m_timer_slots[wait_slot].m_cudaDevice));
        const cudaError_t query_result_start = cudaEventQuery(m_timer_slots[wait_slot].m_start_timer_evt);
		const cudaError_t query_result_stop = cudaEventQuery(m_timer_slots[wait_slot].m_stop_timer_evt);
        if(cudaSuccess == query_result_start && cudaSuccess == query_result_stop)
        {
            m_active_timer_slot = wait_slot;
			V_RETURN(getElapsedTimeForActiveSlot());
        }
        else if(cudaQueryResultIsError(query_result_start) || cudaQueryResultIsError(query_result_stop))
        {
            return E_FAIL;
        }
    }

    return S_OK;
}

void NVWaveWorks_FFT_Simulation_CUDA_Impl::addDisplacements(	const BYTE* pReadbackData,
																const gfsdk_float2* inSamplePoints,
																gfsdk_float4* outDisplacements,
																UINT numSamples,
																float multiplier
																)
{
	switch(m_d3dAPI)
	{
	case nv_water_d3d_api_d3d11:
	case nv_water_d3d_api_gl2:
		// These paths use the surface<>-based variants of the CUDA kernels, which output to 16F
		{
			const UINT row_pitch = sizeof(ushort4) * m_resolution;
			GFSDK_WaveWorks_Simulation_Util::add_displacements_float16(m_params, pReadbackData, row_pitch, inSamplePoints, outDisplacements, numSamples, multiplier);
		}
		break;
	default:
		{
			const UINT row_pitch = sizeof(float4) * m_resolution;
			GFSDK_WaveWorks_Simulation_Util::add_displacements_float32(m_params, pReadbackData, row_pitch, inSamplePoints, outDisplacements, numSamples, multiplier);
		}
		break;
	}
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::addDisplacements(	const gfsdk_float2* inSamplePoints,
																gfsdk_float4* outDisplacements,
																UINT numSamples
																)
{
	if(!getReadbackCursor(NULL))
	{
		return S_OK;
	}

    const BYTE* pRB = reinterpret_cast<BYTE*>(m_active_readback_host_Dxyz);
	addDisplacements(pRB, inSamplePoints, outDisplacements, numSamples);

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::addArchivedDisplacements(	float coord,
																		const gfsdk_float2* inSamplePoints,
																		gfsdk_float4* outDisplacements,
																		UINT numSamples
																		)
{
	if(NULL == m_pReadbackFIFO)
	{
		// No FIFO, nothing to add
		return S_OK;
	}
	else if(0 == m_pReadbackFIFO->range_count())
	{
		// No entries, nothing to add
		return S_OK;
	}

	const float coordMax = float(m_pReadbackFIFO->range_count()-1);

	// Clamp coord to archived range
	float coord_clamped = coord;
	if(coord_clamped < 0.f)
		coord_clamped = 0.f;
	else if(coord_clamped > coordMax)
		coord_clamped = coordMax;

	// Figure out what interp is required
	const float coord_round = floorf(coord_clamped);
	const float coord_frac = coord_clamped - coord_round;
	const int coord_lower = (int)coord_round;
	if(0.f != coord_frac)
	{
		const int coord_upper = coord_lower + 1;

		addDisplacements(
			(const BYTE*)m_pReadbackFIFO->range_at(coord_lower).host_Dxyz,
			inSamplePoints, outDisplacements, numSamples,
			1.f-coord_frac);

		addDisplacements(
			(const BYTE*)m_pReadbackFIFO->range_at(coord_upper).host_Dxyz,
			inSamplePoints, outDisplacements, numSamples,
			coord_frac);
	}
	else
	{
		addDisplacements(
			(const BYTE*)m_pReadbackFIFO->range_at(coord_lower).host_Dxyz,
			inSamplePoints, outDisplacements, numSamples,
			1.f);
	}

    return S_OK;
}

bool NVWaveWorks_FFT_Simulation_CUDA_Impl::getReadbackCursor(gfsdk_U64* pKickID)
{
	if(!m_params.readback_displacements || !m_ReadbackInitialised)
	{
		return false;
	}

	if(GFSDK_WaveWorks_InvalidKickID == m_readback_slots[m_active_readback_slot].m_kickID)
	{
		// No results yet
		return false;
	}

	if(pKickID)
	{
		*pKickID = m_readback_slots[m_active_readback_slot].m_kickID;
	}

	return true;
}

bool NVWaveWorks_FFT_Simulation_CUDA_Impl::hasReadbacksInFlight() const
{
	if(!m_params.readback_displacements || !m_ReadbackInitialised)
	{
		return false;
	}

	int begin_inflight_readback_slots = (m_active_readback_slot + 1) % NumReadbackSlots;
	return begin_inflight_readback_slots != m_end_inflight_readback_slots;
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::getTimings(NVWaveWorks_FFT_Simulation_Timings& timings) const
{
	timings.GPU_simulation_time = m_timer_slots[m_active_timer_slot].m_elapsed_time;
	timings.GPU_FFT_simulation_time = 0.0f;
	return S_OK;
}


LPDIRECT3DTEXTURE9 NVWaveWorks_FFT_Simulation_CUDA_Impl::GetDisplacementMapD3D9()
{
#if WAVEWORKS_ENABLE_D3D9
	assert(m_d3dAPI == nv_water_d3d_api_d3d9);
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	return m_d3d._9.m_pd3d9PerCudaDeviceResources ? m_d3d._9.m_pd3d9PerCudaDeviceResources[activeCudaDeviceIndex].m_pd3d9DisplacementMap : NULL;
#else
    return NULL;
#endif
}

ID3D10ShaderResourceView** NVWaveWorks_FFT_Simulation_CUDA_Impl::GetDisplacementMapD3D10()
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	return m_d3d._10.m_pd3d10PerCudaDeviceResources ? &m_d3d._10.m_pd3d10PerCudaDeviceResources[activeCudaDeviceIndex].m_pd3d10DisplacementMap : NULL;
#else
    return NULL;
#endif
}

ID3D11ShaderResourceView** NVWaveWorks_FFT_Simulation_CUDA_Impl::GetDisplacementMapD3D11()
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	return m_d3d._11.m_pd3d11PerCudaDeviceResources ? &m_d3d._11.m_pd3d11PerCudaDeviceResources[activeCudaDeviceIndex].m_pd3d11DisplacementMap : NULL;
#else
    return NULL;
#endif
}

GLuint NVWaveWorks_FFT_Simulation_CUDA_Impl::GetDisplacementMapGL2()
{
#if WAVEWORKS_ENABLE_GL
	assert(m_d3dAPI == nv_water_d3d_api_gl2);
	const int activeCudaDeviceIndex = m_pManager->GetActiveCudaDeviceIndex();
	return m_d3d._GL2.m_pGL2PerCudaDeviceResources ? m_d3d._GL2.m_pGL2PerCudaDeviceResources[activeCudaDeviceIndex].m_GL2DisplacementMapTexture : NULL;
#else
	return 0;
#endif
}

IDirect3DResource9* NVWaveWorks_FFT_Simulation_CUDA_Impl::getD3D9InteropResource(unsigned int deviceIndex)
{
#if WAVEWORKS_ENABLE_D3D9
	assert(m_d3dAPI == nv_water_d3d_api_d3d9);
	return m_d3d._9.m_pd3d9PerCudaDeviceResources[deviceIndex].m_pd3d9DisplacementMap;
#else
	return NULL;
#endif
}

ID3D10Resource* NVWaveWorks_FFT_Simulation_CUDA_Impl::getD3D10InteropResource(unsigned int deviceIndex)
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);
	return m_d3d._10.m_pd3d10PerCudaDeviceResources[deviceIndex].m_pd3d10DisplacementMapResource;
#else
	return NULL;
#endif
}

cudaGraphicsResource* NVWaveWorks_FFT_Simulation_CUDA_Impl::getInteropResource(unsigned int deviceIndex)
{
	switch(m_d3dAPI)
	{
#if WAVEWORKS_ENABLE_D3D11
	case nv_water_d3d_api_d3d11:
		return m_d3d._11.m_pd3d11PerCudaDeviceResources[deviceIndex].m_pd3d11RegisteredDisplacementMapResource;
#endif

#if WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_gl2:
		return m_d3d._GL2.m_pGL2PerCudaDeviceResources[deviceIndex].m_pGL2RegisteredDisplacementMapResource;
#endif

	default:
		assert(false);	// ...shouldn't ever happen
		return NULL;
	}
}

HRESULT NVWaveWorks_FFT_Simulation_CUDA_Impl::archiveDisplacements()
{
	gfsdk_U64 kickID = GFSDK_WaveWorks_InvalidKickID;
	if(getReadbackCursor(&kickID) && m_pReadbackFIFO)
	{
		// We avoid big memcpys by swapping pointers, specifically we will either evict a FIFO entry or else use a free one and
		// swap it with one of the slots used for in-flight readbacks
		//
		// First job is to check whether the FIFO already contains this result. We know that if it does contain this result,
		// it will be the last one pushed on...
		if(m_pReadbackFIFO->range_count())
		{
			if(kickID == m_pReadbackFIFO->range_at(0).kickID)
			{
				// It is an error to archive the same results twice...
				return E_FAIL;
			}
		}

		// Assuming the current results have not been archived, the next-up readback buffer should match the one we are serving up
		// for addDisplacements...
		assert(m_active_readback_host_Dxyz == m_readback_slots[m_active_readback_slot].m_host_Dxyz);

		ReadbackFIFOSlot& slot = m_pReadbackFIFO->consume_one();
		m_readback_slots[m_active_readback_slot].m_host_Dxyz = slot.host_Dxyz;
		slot.host_Dxyz = m_active_readback_host_Dxyz;
		slot.kickID = kickID;
	}

	return S_OK;
}

#endif //SUPPORT_CUDA
