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

#ifndef _NVWAVEWORKS_FFT_SIMULATION_MANAGER_CUDA_IMPL_H
#define _NVWAVEWORKS_FFT_SIMULATION_MANAGER_CUDA_IMPL_H

#include "FFT_Simulation_Manager.h"
#include "Sim_Array.h"

class NVWaveWorks_FFT_Simulation_CUDA_Impl;

class NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl : public NVWaveWorks_FFT_Simulation_Manager
{
public:

	NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl();
	~NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl();

    virtual HRESULT initD3D9(IDirect3DDevice9* pD3DDevice);
    virtual HRESULT initD3D10(ID3D10Device* pD3DDevice);
    virtual HRESULT initD3D11(ID3D11Device* pD3DDevice);
	virtual HRESULT initGL2(void* pGLContext);
	virtual HRESULT initNoGraphics();

	// Mandatory NVWaveWorks_FFT_Simulation_Manager interface
	NVWaveWorks_FFT_Simulation* createSimulation(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params);
	void releaseSimulation(NVWaveWorks_FFT_Simulation* pSimulation);
	HRESULT beforeReinit(const GFSDK_WaveWorks_Detailed_Simulation_Params& params, bool reinitOnly);
	HRESULT kick(Graphics_Context* pGC, double dSimTime, gfsdk_U64& kickID);
	bool getStagingCursor(gfsdk_U64* pKickID);
	AdvanceCursorResult advanceStagingCursor(bool block);
	bool getReadbackCursor(gfsdk_U64* pKickID);
	AdvanceCursorResult advanceReadbackCursor(bool block);
	WaitCursorResult waitStagingCursor();
	HRESULT archiveDisplacements();
	HRESULT getTimings(GFSDK_WaveWorks_Simulation_Manager_Timings& timings);

	// Hooks used by cascade members
	unsigned int GetActiveCudaDeviceIndex() const { return m_activeCudaDeviceIndex; }
	unsigned int GetNumCudaDevices() const { return m_numCudaDevices; }

	struct CudaDeviceInfo
	{
#ifdef SUPPORT_CUDA
		// Device IDs
		int m_cudaDevice;

		// device memory for all cascades
		void* m_device_constants;
		void* m_constants_address;
		size_t m_constants_size;

		// Streams
		cudaStream_t m_kernel_stream;
		cudaStream_t m_readback_stream;
#endif
	};

	const CudaDeviceInfo& GetCudaDeviceInfo(unsigned int ix) const { return m_pCudaDeviceInfos[ix]; }
	HRESULT beforeReallocateSimulation();

private:

	Sim_Array<NVWaveWorks_FFT_Simulation_CUDA_Impl> m_Simulations;

	gfsdk_U64 m_NextKickID;

	bool m_StagingCursorIsValid;
	gfsdk_U64 m_StagingCursorKickID;

	unsigned int m_numCudaDevices;
	unsigned int m_activeCudaDeviceIndex;
	CudaDeviceInfo* m_pCudaDeviceInfos;

	bool m_cudaResourcesInitialised;

	void releaseAll();

	HRESULT releaseCudaResources();
	HRESULT allocateCudaResources();

	HRESULT checkForReadbackResults();

	HRESULT mapInteropResources(const CudaDeviceInfo& cdi);
	HRESULT unmapInteropResources(const CudaDeviceInfo& cdi);

	// D3D API handling
	nv_water_d3d_api m_d3dAPI;

#if WAVEWORKS_ENABLE_D3D9
    struct D3D9Objects
    {
		IDirect3DDevice9* m_pd3d9Device;
    };
#endif

#if WAVEWORKS_ENABLE_D3D10
    struct D3D10Objects
    {
		ID3D10Device* m_pd3d10Device;
    };
#endif

#if WAVEWORKS_ENABLE_D3D11
    struct D3D11Objects
    {
		ID3D11Device* m_pd3d11Device;
    };
#endif
#if WAVEWORKS_ENABLE_GL
    struct GL2Objects
    {
		void* m_pGLContext;
    };
#endif
    union
    {
#if WAVEWORKS_ENABLE_D3D9
        D3D9Objects _9;
#endif

#if WAVEWORKS_ENABLE_D3D10
        D3D10Objects _10;
#endif

#if WAVEWORKS_ENABLE_D3D11
		D3D11Objects _11;
#endif

#if WAVEWORKS_ENABLE_GL
		GL2Objects  _GL2;
#endif
    } m_d3d;
};

#endif	// _NVWAVEWORKS_FFT_SIMULATION_MANAGER_CUDA_IMPL_H
