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

#include "FFT_Simulation_Manager_CUDA_impl.h"
#include "FFT_Simulation_CUDA_impl.h"

#ifdef SUPPORT_CUDA
#include <malloc.h>
#include <string.h>

#if defined(TARGET_PLATFORM_NIXLIKE)
#define _alloca alloca
#endif

extern "C"
{
	cudaError cuda_GetConstantsSize(size_t* size);
	cudaError cuda_GetConstantsAddress(void** ptr);
}

NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl() :
	m_NextKickID(0),
	m_StagingCursorIsValid(false),
	m_StagingCursorKickID(0)
{
	m_numCudaDevices = 0;
	m_activeCudaDeviceIndex = 0;
	m_pCudaDeviceInfos = NULL;
	m_cudaResourcesInitialised = false;
	m_d3dAPI = nv_water_d3d_api_undefined;
}

NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::~NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl()
{
	releaseAll();
}

void NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::releaseAll()
{
    if(m_cudaResourcesInitialised)
    {
        releaseCudaResources();
    }

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
    }

	assert(0 == m_Simulations.size());	// It is an error to destroy a non-empty manager
	m_Simulations.erase_all();

	m_d3dAPI = nv_water_d3d_api_undefined;

	SAFE_DELETE_ARRAY(m_pCudaDeviceInfos);
	m_numCudaDevices = 0;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::initD3D9(IDirect3DDevice9* pD3DDevice)
{
#if WAVEWORKS_ENABLE_D3D9
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

		CUDA_V_RETURN(cudaD3D9GetDevices(&m_numCudaDevices, NULL, 0, pD3DDevice, cudaD3D9DeviceListAll));
		int* pCudaDevices = (int*)_alloca(m_numCudaDevices * sizeof(int));
		CUDA_V_RETURN(cudaD3D9GetDevices(&m_numCudaDevices, pCudaDevices, m_numCudaDevices, pD3DDevice, cudaD3D9DeviceListAll));
		m_pCudaDeviceInfos = new CudaDeviceInfo[m_numCudaDevices];
		memset(m_pCudaDeviceInfos, 0, m_numCudaDevices * sizeof(CudaDeviceInfo));
		for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
		{
			m_pCudaDeviceInfos[cuda_dev_index].m_cudaDevice = pCudaDevices[cuda_dev_index];
		}
    }

    return S_OK;
#else
    return E_FAIL;
#endif
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::initD3D10(ID3D10Device* pD3DDevice)
{
#if WAVEWORKS_ENABLE_D3D10
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

		CUDA_V_RETURN(cudaD3D10GetDevices(&m_numCudaDevices, NULL, 0, pD3DDevice, cudaD3D10DeviceListAll));
		int* pCudaDevices = (int*)_alloca(m_numCudaDevices * sizeof(int));
		CUDA_V_RETURN(cudaD3D10GetDevices(&m_numCudaDevices, pCudaDevices, m_numCudaDevices, pD3DDevice, cudaD3D10DeviceListAll));
		m_pCudaDeviceInfos = new CudaDeviceInfo[m_numCudaDevices];
		memset(m_pCudaDeviceInfos, 0, m_numCudaDevices * sizeof(CudaDeviceInfo));
		for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
		{
			m_pCudaDeviceInfos[cuda_dev_index].m_cudaDevice = pCudaDevices[cuda_dev_index];
		}
    }

    return S_OK;
#else
    return E_FAIL;
#endif
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::initD3D11(ID3D11Device* pD3DDevice)
{
#if WAVEWORKS_ENABLE_D3D11
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

		CUDA_V_RETURN(cudaD3D11GetDevices(&m_numCudaDevices, NULL, 0, pD3DDevice, cudaD3D11DeviceListAll));
		int* pCudaDevices = (int*)_alloca(m_numCudaDevices * sizeof(int));
		CUDA_V_RETURN(cudaD3D11GetDevices(&m_numCudaDevices, pCudaDevices, m_numCudaDevices, pD3DDevice, cudaD3D11DeviceListAll));
		m_pCudaDeviceInfos = new CudaDeviceInfo[m_numCudaDevices];
		memset(m_pCudaDeviceInfos, 0, m_numCudaDevices * sizeof(CudaDeviceInfo));
		for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
		{
			m_pCudaDeviceInfos[cuda_dev_index].m_cudaDevice = pCudaDevices[cuda_dev_index];
		}
    }

    return S_OK;
#else
    return E_FAIL;
#endif
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::initGL2(void* pGLContext)
{
#if WAVEWORKS_ENABLE_GL
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

		CUDA_V_RETURN(cudaGLGetDevices(&m_numCudaDevices, NULL, 0, cudaGLDeviceListAll));
		int* pCudaDevices = (int*)_alloca(m_numCudaDevices * sizeof(int));
		CUDA_API_RETURN(cudaGLGetDevices(&m_numCudaDevices, pCudaDevices, m_numCudaDevices, cudaGLDeviceListAll));
		m_pCudaDeviceInfos = new CudaDeviceInfo[m_numCudaDevices];
		memset(m_pCudaDeviceInfos, 0, m_numCudaDevices * sizeof(CudaDeviceInfo));
		for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
		{
			m_pCudaDeviceInfos[cuda_dev_index].m_cudaDevice = pCudaDevices[cuda_dev_index];
		}
    }
    return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::initNoGraphics()
{
    if(nv_water_d3d_api_none != m_d3dAPI)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_none;

		int cuda_device;
		CUDA_V_RETURN(cudaGetDevice(&cuda_device));

		m_numCudaDevices = 1;
		m_pCudaDeviceInfos = new CudaDeviceInfo[m_numCudaDevices];
		memset(m_pCudaDeviceInfos, 0, m_numCudaDevices * sizeof(CudaDeviceInfo));
		m_pCudaDeviceInfos->m_cudaDevice = cuda_device;
    }

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::allocateCudaResources()
{
	for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
	{
		CudaDeviceInfo& dev_state = m_pCudaDeviceInfos[cuda_dev_index];
		CUDA_V_RETURN(cudaSetDevice(dev_state.m_cudaDevice));

		CUDA_V_RETURN(cuda_GetConstantsSize(&dev_state.m_constants_size));
		CUDA_V_RETURN(cuda_GetConstantsAddress(&dev_state.m_constants_address));
		CUDA_V_RETURN(cudaMalloc((void **)&dev_state.m_device_constants, dev_state.m_constants_size));
		CUDA_V_RETURN(cudaMemset(dev_state.m_device_constants, 0, dev_state.m_constants_size));

		CUDA_V_RETURN(cudaStreamCreateWithFlags(&dev_state.m_kernel_stream,cudaStreamNonBlocking));
		CUDA_V_RETURN(cudaStreamCreateWithFlags(&dev_state.m_readback_stream,cudaStreamNonBlocking));
	}

    m_cudaResourcesInitialised = true;

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::releaseCudaResources()
{
	for(unsigned int cuda_dev_index = 0; cuda_dev_index != m_numCudaDevices; ++cuda_dev_index)
	{
		CudaDeviceInfo& dev_state = m_pCudaDeviceInfos[cuda_dev_index];
		CUDA_V_RETURN(cudaSetDevice(dev_state.m_cudaDevice));

		CUDA_SAFE_FREE(dev_state.m_device_constants);

		CUDA_V_RETURN(cudaStreamDestroy(dev_state.m_kernel_stream));
		CUDA_V_RETURN(cudaStreamDestroy(dev_state.m_readback_stream));
	}

    m_cudaResourcesInitialised = false;

    return S_OK;
}

NVWaveWorks_FFT_Simulation* NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::createSimulation(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params)
{
	NVWaveWorks_FFT_Simulation_CUDA_Impl* pResult = new NVWaveWorks_FFT_Simulation_CUDA_Impl(this,params);
	m_Simulations.push_back(pResult);
	return pResult;
}

void NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::releaseSimulation(NVWaveWorks_FFT_Simulation* pSimulation)
{
	//remove from list
	m_Simulations.erase(pSimulation);

	SAFE_DELETE(pSimulation);
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::beforeReinit(const GFSDK_WaveWorks_Detailed_Simulation_Params& /*params*/, bool /*reinitOnly*/)
{
	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::checkForReadbackResults()
{
	HRESULT hr;

	// The goal here is to evolve the readback state of all our simulations in lockstep, so that either all our simulations collect
	// a single readback or else none do (IOW: 'some' is *not* permitted, because it would break lockstep)

	NVWaveWorks_FFT_Simulation_CUDA_Impl** pBeginSimulationsSrc = (NVWaveWorks_FFT_Simulation_CUDA_Impl**)_alloca(m_Simulations.size() * sizeof(NVWaveWorks_FFT_Simulation_CUDA_Impl*));
	memcpy(pBeginSimulationsSrc,m_Simulations.begin(),m_Simulations.size() * sizeof(NVWaveWorks_FFT_Simulation_CUDA_Impl*));
	NVWaveWorks_FFT_Simulation_CUDA_Impl** pEndSimulationsSrc = pBeginSimulationsSrc + m_Simulations.size();

	NVWaveWorks_FFT_Simulation_CUDA_Impl** pBeginSimulationsNoResult = (NVWaveWorks_FFT_Simulation_CUDA_Impl**)_alloca(m_Simulations.size() * sizeof(NVWaveWorks_FFT_Simulation_CUDA_Impl*));;
	NVWaveWorks_FFT_Simulation_CUDA_Impl** pEndSimulationsNoResult = pBeginSimulationsNoResult;

	// Do an initial walk thru and see if any readbacks arrived (without blocking), and write any that did not get a readback result into dst
	for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = pBeginSimulationsSrc; pSim != pEndSimulationsSrc; ++pSim)
	{
		hr = (*pSim)->collectSingleReadbackResult(false);
		if(FAILED(hr))
		{
			return hr;
		}

		if(S_FALSE == hr)
		{
			(*pEndSimulationsNoResult) = (*pSim);
			++pEndSimulationsNoResult;
		}
	}

	// If no results are ready, we're in sync so don't try again
	if((pEndSimulationsNoResult-pBeginSimulationsNoResult) != m_Simulations.size())
	{
		// Otherwise, wait on the remaining results
		for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = pBeginSimulationsNoResult; pSim != pEndSimulationsNoResult; ++pSim)
		{
			V_RETURN((*pSim)->collectSingleReadbackResult(true));
		}
	}

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::kick(Graphics_Context* /*pGC*/, double dSimTime, gfsdk_U64& kickID)
{
	HRESULT hr;

	kickID = m_NextKickID;

    if(!m_cudaResourcesInitialised)
    {
        V_RETURN(allocateCudaResources());
	}

	// Check for readback results - note that we do this at the manager level in order to guarantee lockstep between
	// the simulations that form a cascade. We either want all of simulations to collect a result, or none - some is
	// not an option
	checkForReadbackResults();

	// Be sure to use the correct cuda device for the current frame (important in SLI)
	int cuda_device = -1;
	if(1 == m_numCudaDevices)
	{
		m_activeCudaDeviceIndex = 0;
		cuda_device = m_pCudaDeviceInfos[m_activeCudaDeviceIndex].m_cudaDevice;
		CUDA_V_RETURN(cudaSetDevice(cuda_device));
	}
	else
	{
		// Multiple devices, we will have to do it the 'long' way
		switch(m_d3dAPI)
		{
#if WAVEWORKS_ENABLE_D3D9
		case nv_water_d3d_api_d3d9:
			{
				unsigned int cuda_device_count = 0;
				CUDA_V_RETURN(cudaD3D9GetDevices(&cuda_device_count, &cuda_device, 1, m_d3d._9.m_pd3d9Device, cudaD3D9DeviceListCurrentFrame));
				CUDA_V_RETURN(cudaSetDevice(cuda_device));
				break;
			}
#endif
#if WAVEWORKS_ENABLE_D3D10
		case nv_water_d3d_api_d3d10:
			{
				unsigned int cuda_device_count = 0;
				CUDA_V_RETURN(cudaD3D10GetDevices(&cuda_device_count, &cuda_device, 1, m_d3d._10.m_pd3d10Device, cudaD3D10DeviceListCurrentFrame));
				CUDA_V_RETURN(cudaSetDevice(cuda_device));
				break;
			}
#endif
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			{
				unsigned int cuda_device_count = 0;
				CUDA_V_RETURN(cudaD3D11GetDevices(&cuda_device_count, &cuda_device, 1, m_d3d._11.m_pd3d11Device, cudaD3D11DeviceListCurrentFrame));
				CUDA_V_RETURN(cudaSetDevice(cuda_device));
				break;
			}
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				unsigned int cuda_device_count = 0;
				CUDA_V_RETURN(cudaGLGetDevices(&cuda_device_count, &cuda_device, 1, cudaGLDeviceListCurrentFrame));
				CUDA_V_RETURN(cudaSetDevice(cuda_device));
				break;
			}
#endif
		case nv_water_d3d_api_none:
			{
				assert(1 == m_numCudaDevices);		// Well by the time we get here we're guaranteed to hit this assert,
													// but the assert neatly documents the violated expecation i.e. the only
													// supported no-graphics CUDA path is single device
				break;
			}
        default:
            return E_FAIL;
		}

		// Match the current device to our list
		for(unsigned int cuda_device_index = 0; cuda_device_index != m_numCudaDevices; ++cuda_device_index)
		{
			if(cuda_device == m_pCudaDeviceInfos[cuda_device_index].m_cudaDevice)
			{
				m_activeCudaDeviceIndex = cuda_device_index;
				break;
			}
		}
	}
	
	const CudaDeviceInfo& active_dev_info = m_pCudaDeviceInfos[m_activeCudaDeviceIndex];

	for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		V_RETURN((*pSim)->preKick(pSim - m_Simulations.begin()));
	}
	CUDA_V_RETURN(cudaMemcpyAsync(active_dev_info.m_constants_address, active_dev_info.m_device_constants, 
		active_dev_info.m_constants_size, cudaMemcpyDeviceToDevice, active_dev_info.m_kernel_stream));

	// Do all the CUDA work as far as interop
	for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		V_RETURN((*pSim)->kickPreInterop(dSimTime,kickID));
	}

	// Map for interop
	V_RETURN(mapInteropResources(active_dev_info));

	// Do all interop CUDA work
	for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		V_RETURN((*pSim)->kickWithinInterop(kickID));
	}

	// Unmap for interop
	V_RETURN(unmapInteropResources(active_dev_info));

	// Do post-interop CUDA work
	for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		V_RETURN((*pSim)->kickPostInterop(kickID));
	}

	m_StagingCursorIsValid = true;
	m_StagingCursorKickID = kickID;
	++m_NextKickID;

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::mapInteropResources(const CudaDeviceInfo& cdi)
{
	switch(m_d3dAPI)
	{
#if WAVEWORKS_ENABLE_D3D9
	case nv_water_d3d_api_d3d9:
		{
			const int num_resources = m_Simulations.size();
			IDirect3DResource9** pInteropResources = (IDirect3DResource9**)alloca(sizeof(IDirect3DResource9*)*num_resources);
			int i = 0;
			for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim, ++i)
			{
				pInteropResources[i] = (*pSim)->getD3D9InteropResource(m_activeCudaDeviceIndex);
			}
			CUDA_V_RETURN(cudaD3D9MapResources(num_resources, pInteropResources));	// @TODO: why no cu_stream?
			break;
		}
#endif
#if WAVEWORKS_ENABLE_D3D10
	case nv_water_d3d_api_d3d10:
		{
			const int num_resources = m_Simulations.size();
			ID3D10Resource** pInteropResources = (ID3D10Resource**)alloca(sizeof(ID3D10Resource*)*num_resources);
			int i = 0;
			for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim, ++i)
			{
				pInteropResources[i] = (*pSim)->getD3D10InteropResource(m_activeCudaDeviceIndex);
			}
			CUDA_V_RETURN(cudaD3D10MapResources(num_resources, pInteropResources));	// @TODO: why no cu_stream?
			break;
		}
#endif
#if WAVEWORKS_ENABLE_D3D11 || WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_d3d11:
	case nv_water_d3d_api_gl2:
		{
			const int num_resources = m_Simulations.size();
			cudaGraphicsResource** pInteropResources = (cudaGraphicsResource**)alloca(sizeof(cudaGraphicsResource*)*num_resources);
			int i = 0;
			for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim, ++i)
			{
				pInteropResources[i] = (*pSim)->getInteropResource(m_activeCudaDeviceIndex);
			}
			CUDA_V_RETURN(cudaGraphicsMapResources(num_resources, pInteropResources, cdi.m_kernel_stream));
			break;
		}
#endif
	case nv_water_d3d_api_none:
		{
			// Nothing to do...
			break;
		}
    default:
        return E_FAIL;
	}

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::unmapInteropResources(const CudaDeviceInfo& cdi)
{
	switch(m_d3dAPI)
	{
#if WAVEWORKS_ENABLE_D3D9
	case nv_water_d3d_api_d3d9:
		{
			const int num_resources = m_Simulations.size();
			IDirect3DResource9** pInteropResources = (IDirect3DResource9**)alloca(sizeof(IDirect3DResource9*)*num_resources);
			int i = 0;
			for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim, ++i)
			{
				pInteropResources[i] = (*pSim)->getD3D9InteropResource(m_activeCudaDeviceIndex);
			}
			CUDA_V_RETURN(cudaD3D9UnmapResources(num_resources, pInteropResources));	// @TODO: why no cu_stream?
			break;
		}
#endif
#if WAVEWORKS_ENABLE_D3D10
	case nv_water_d3d_api_d3d10:
		{
			const int num_resources = m_Simulations.size();
			ID3D10Resource** pInteropResources = (ID3D10Resource**)alloca(sizeof(ID3D10Resource*)*num_resources);
			int i = 0;
			for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim, ++i)
			{
				pInteropResources[i] = (*pSim)->getD3D10InteropResource(m_activeCudaDeviceIndex);
			}
			CUDA_V_RETURN(cudaD3D10UnmapResources(num_resources, pInteropResources));	// @TODO: why no cu_stream?
			break;
		}
#endif
#if WAVEWORKS_ENABLE_D3D11 || WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_d3d11:
	case nv_water_d3d_api_gl2:
		{
			const int num_resources = m_Simulations.size();
			cudaGraphicsResource** pInteropResources = (cudaGraphicsResource**)alloca(sizeof(cudaGraphicsResource*)*num_resources);
			int i = 0;
			for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim, ++i)
			{
				pInteropResources[i] = (*pSim)->getInteropResource(m_activeCudaDeviceIndex);
			}
			CUDA_V_RETURN(cudaGraphicsUnmapResources(num_resources, pInteropResources, cdi.m_kernel_stream));
			break;
		}
#endif
	case nv_water_d3d_api_none:
		{
			// Nothing to do...
			break;
		}
    default:
        return E_FAIL;
	}

	return S_OK;
}

bool NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::getStagingCursor(gfsdk_U64* pKickID)
{
	if(pKickID && m_StagingCursorIsValid)
	{
		*pKickID = m_StagingCursorKickID;
	}

	return m_StagingCursorIsValid;
}

NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::advanceStagingCursor(bool /*block*/)
{
	// The CUDA pipeline pipeline is not async wrt the API, so there can never be any pending kicks and we can return immediately
	return AdvanceCursorResult_None;
}
NVWaveWorks_FFT_Simulation_Manager::WaitCursorResult NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::waitStagingCursor()
{
	// The CUDA pipeline is not async wrt the API, so there can never be any pending kicks and we can return immediately
	return WaitCursorResult_None;
}

bool NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::getReadbackCursor(gfsdk_U64* pKickID)
{
	if(0 == m_Simulations.size())
		return false;

	// We rely on collectSingleReadbackResult() to maintain lockstep between the cascade members, therefore we can in theory
	// query any member to get the readback cursor...

	// ...but let's check that theory in debug builds!!!
#ifdef _DEV
	if(m_Simulations.size() > 1)
	{
		gfsdk_U64 sim0KickID;
		bool sim0GRCresult = m_Simulations[0]->getReadbackCursor(&sim0KickID);
		for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin()+1; pSim != m_Simulations.end(); ++pSim)
		{
			gfsdk_U64 simNKickID;
			bool simNGRCresult = (*pSim)->getReadbackCursor(&simNKickID);
			assert(simNGRCresult == sim0GRCresult);
			if(sim0GRCresult)
			{
				assert(sim0KickID == simNKickID);
			}
		}

	}
#endif

	return m_Simulations[0]->getReadbackCursor(pKickID);
}

NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::advanceReadbackCursor(bool block)
{
	if(0 == m_Simulations.size())
		return AdvanceCursorResult_None;

	// First, check whether we even have readbacks in-flight
	const bool hasReadbacksInFlightSim0 = m_Simulations[0]->hasReadbacksInFlight();

	// Usual paranoid verficiation that we're maintaining lockstep...
#ifdef _DEV
	if(m_Simulations.size() > 1)
	{
		for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin()+1; pSim != m_Simulations.end(); ++pSim)
		{
			assert(hasReadbacksInFlightSim0 == (*pSim)->hasReadbacksInFlight());
		}
	}
#endif

	if(!hasReadbacksInFlightSim0)
	{
		return AdvanceCursorResult_None;
	}

	if(!block)
	{
		// Non-blocking case - in order to maintain lockstep, either all of the simulations should consume a readback,
		// or none. Therefore we need to do an initial pass to test whether the 'all' case applies (and bail if not)...
		for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
		{
			HRESULT hr = (*pSim)->canCollectSingleReadbackResultWithoutBlocking();
			if(FAILED(hr))
			{
				return AdvanceCursorResult_Failed;
			}
			else if(S_FALSE == hr)
			{
				// Cannot advance, would have blocked -> bail
				return AdvanceCursorResult_WouldBlock;
			}
		}
	}

	// We have readbacks in flight, and in the non-blocking case we *should* be in a position to consume them without
	// any waiting, so just visit each simulation in turn with a blocking wait for the next readback to complete...
	for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		if(FAILED((*pSim)->collectSingleReadbackResult(true)))
		{
			return AdvanceCursorResult_Failed;
		}
	}

	return AdvanceCursorResult_Succeeded;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::archiveDisplacements()
{
	HRESULT hr;

	if(!getReadbackCursor(NULL))
	{
		return E_FAIL;
	}

	for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		V_RETURN((*pSim)->archiveDisplacements());
	}

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::getTimings(GFSDK_WaveWorks_Simulation_Manager_Timings& timings)
{
	// CUDA implementation doesn't update these CPU implementation related timings
	timings.time_start_to_stop = 0;
	timings.time_total = 0;
	timings.time_wait_for_completion = 0;
	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl::beforeReallocateSimulation()
{
	HRESULT hr;

	// A simulation is about to be reallocated...

	// Implication 1: at least some displacement map contents will become undefined and
	// will need a kick to make them valid again, which in turn means that we can no longer
	// consider any kick that was previously staged as still being staged...
	m_StagingCursorIsValid = false;

	// Implication 2: some of the readback tracking will be reset, meaning we break
	// lockstep. We can avoid this by forcible resetting all readback tracking
	for(NVWaveWorks_FFT_Simulation_CUDA_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		V_RETURN((*pSim)->resetReadbacks());
	}

	return S_OK;
}

#endif // SUPPORT_CUDA
