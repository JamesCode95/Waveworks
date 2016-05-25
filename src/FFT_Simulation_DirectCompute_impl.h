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

#ifndef _NVWAVEWORKS_FFT_SIMULATION_DIRECTCOMPUTE_IMPL_H
#define _NVWAVEWORKS_FFT_SIMULATION_DIRECTCOMPUTE_IMPL_H

#include "FFT_Simulation.h"

#ifdef SUPPORT_DIRECTCOMPUTE

class NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl;
template<class T> class CircularFIFO;

class NVWaveWorks_FFT_Simulation_DirectCompute_Impl : public NVWaveWorks_FFT_Simulation
{
public:
	NVWaveWorks_FFT_Simulation_DirectCompute_Impl(NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl* pManager, const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params);
	~NVWaveWorks_FFT_Simulation_DirectCompute_Impl();

	// Mandatory NVWaveWorks_FFT_Simulation interface
    HRESULT initD3D11(ID3D11Device* pD3DDevice);
	HRESULT initNoGraphics() { return S_OK; }
	HRESULT reinit(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params);
	HRESULT addDisplacements(const gfsdk_float2* inSamplePoints, gfsdk_float4* outDisplacements, UINT numSamples);
	HRESULT addArchivedDisplacements(float coord, const gfsdk_float2* inSamplePoints, gfsdk_float4* outDisplacements, UINT numSamples);
	HRESULT getTimings(NVWaveWorks_FFT_Simulation_Timings&) const;
	gfsdk_U64 getDisplacementMapVersion() const { return m_DisplacementMapVersion; }
	ID3D11ShaderResourceView** GetDisplacementMapD3D11();
	GLuint GetDisplacementMapGL2();

	HRESULT kick(Graphics_Context* pGC, double dSimTime, gfsdk_U64 kickID);

	HRESULT collectSingleReadbackResult(bool blocking);
	bool getReadbackCursor(gfsdk_U64* pKickID);
	bool hasReadbacksInFlight() const;
	HRESULT canCollectSingleReadbackResultWithoutBlocking();
	HRESULT resetReadbacks();

	HRESULT archiveDisplacements();

private:

	NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl* m_pManager;

	GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade m_params;

	unsigned int m_resolution;  // m_params.fft_resolution
	unsigned int m_half_resolution_plus_one;

	bool m_avoid_frame_depedencies; // if SLI, currently always true (performance issue)
	bool m_GaussAndOmegaInitialised;
	bool m_H0Dirty;

	HRESULT allocateAllResources();
	void releaseAllResources();
	void releaseAll();
	HRESULT initGaussAndOmega();
	void updateConstantBuffer(double simTime) const;

	enum { NumReadbackSlots = 4 };	// 2 in-flight, one usable, one active
	enum { NumTimerSlots = 4 };	// 2 in-flight, one usable, one active

    int m_active_readback_slot;			// i.e. not in-flight
    int m_end_inflight_readback_slots;	// the first in-flight slot is always the one after active
	bool m_ReadbackInitialised;

	gfsdk_U64 m_readback_kickIDs[NumReadbackSlots];

	gfsdk_U64 m_DisplacementMapVersion;

	HRESULT consumeAvailableReadbackSlot(int& slot, gfsdk_U64 kickID);
	HRESULT waitForAllInFlightReadbacks();

	float m_timer_results[NumTimerSlots];
	gfsdk_U64 m_timer_kickIDs[NumReadbackSlots];
    int m_active_timer_slot;			// i.e. not in-flight
    int m_end_inflight_timer_slots;		// the first in-flight slot is always the one after active

	HRESULT consumeAvailableTimerSlot(int& slot, gfsdk_U64 kickID);
	HRESULT waitForAllInFlightTimers();

	void add_displacements_float16_d3d11(	ID3D11Texture2D* buffer,
											const gfsdk_float2* inSamplePoints,
											gfsdk_float4* outDisplacements,
											UINT numSamples,
											float multiplier
											);

	HRESULT addArchivedDisplacementsD3D11(	float coord,
											const gfsdk_float2* inSamplePoints,
											gfsdk_float4* outDisplacements,
											UINT numSamples
											);

	// D3D API handling
	nv_water_d3d_api m_d3dAPI;

    struct D3D11Objects
    {
		ID3D11Device* m_device;
		ID3D11DeviceContext* m_context;

		// The Gauss distribution used to generated H0 (size: N x N).
		ID3D11Buffer* m_buffer_Gauss;
		// Angular frequency (size: N/2+1 x N/2+1).
		ID3D11Buffer* m_buffer_Omega;
		// Initial height field H(0) generated by Phillips spectrum & Gauss distribution (size: N+1 x N+1).
		ID3D11Buffer* m_buffer_H0;
		// Height field H(t) in frequency domain, updated each frame (size: N/2+1 x N).
		ID3D11Buffer* m_buffer_Ht;
		// Choppy fields Dx(t) and Dy(t), updated each frame (size: N/2+1 x N).
		ID3D11Buffer* m_buffer_Dt;
		// Displacement/choppy field (size: N x N).
		ID3D11Texture2D* m_texture_Displacement;
		// per-frame constants (todo: only time is updated every frame, worth splitting?)
		ID3D11Buffer* m_buffer_constants;

		ID3D11ShaderResourceView* m_srv_Gauss;
		ID3D11ShaderResourceView* m_srv_H0;
		ID3D11ShaderResourceView* m_srv_Ht;
		ID3D11ShaderResourceView* m_srv_Dt;
		ID3D11ShaderResourceView* m_srv_Omega;
		ID3D11ShaderResourceView* m_srv_Displacement;	// (ABGR32F)

		ID3D11UnorderedAccessView* m_uav_H0;
		ID3D11UnorderedAccessView* m_uav_Ht;
		ID3D11UnorderedAccessView* m_uav_Dt;
		ID3D11UnorderedAccessView* m_uav_Displacement;

		// readback staging
		ID3D11Texture2D* m_readback_buffers[NumReadbackSlots];
		ID3D11Query* m_readback_queries[NumReadbackSlots];
		ID3D11Texture2D* m_active_readback_buffer;

		struct ReadbackFIFOSlot
		{
			gfsdk_U64 kickID;
			ID3D11Texture2D* buffer;
		};
		CircularFIFO<ReadbackFIFOSlot>* m_pReadbackFIFO;

		// timers
		ID3D11Query* m_frequency_queries[NumTimerSlots];
		ID3D11Query* m_start_queries[NumTimerSlots];
		ID3D11Query* m_end_queries[NumTimerSlots];

		// Shaders
		ID3D11ComputeShader* m_update_h0_shader;
		ID3D11ComputeShader* m_row_shader;
		ID3D11ComputeShader* m_column_shader;
    };

    union
    {
		D3D11Objects _11;
    } m_d3d;
};

#endif // SUPPORT_DIRECTCOMPUTE

#endif	// _NVWAVEWORKS_FFT_SIMULATION_DIRECTCOMPUTE_IMPL_H
