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

#ifndef _NVWaveWorks_FFT_Simulation_CPU_Impl_H
#define _NVWaveWorks_FFT_Simulation_CPU_Impl_H

#include "FFT_Simulation.h"

struct Task;
template<class T> class CircularFIFO;

typedef float complex[2];


class NVWaveWorks_FFT_Simulation_CPU_Impl : public NVWaveWorks_FFT_Simulation
{
public:
	NVWaveWorks_FFT_Simulation_CPU_Impl(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params);
	~NVWaveWorks_FFT_Simulation_CPU_Impl();

	// Simulation primitives
	bool UpdateH0(int row);			// Returns true if this is the last row to be updated
	bool UpdateHt(int row);			// Returns true if this is the last row to be updated
	bool UpdateTexture(int row);	// Returns true if this is the last row to be updated

	// FFT simulation primitives - 2 paths here:
	// - the 'legacy' path models the entire NxN 2D FFT as a single task
	// - the new path models each group of N-wide 1D FFT's as a single task
	bool ComputeFFT_XY_NxN(int index);		// Returns true if this is the last FFT to be processed
	bool ComputeFFT_X(int XYZindex, int subIndex);
	bool ComputeFFT_Y(int XYZindex, int subIndex);

	int GetNumRowsIn_FFT_X() const;
	int GetNumRowsIn_FFT_Y() const;

	HRESULT OnInitiateSimulationStep(Graphics_Context* pGC, double dSimTime);
	void OnCompleteSimulationStep(gfsdk_U64 kickID);

	// Mandatory NVWaveWorks_FFT_Simulation interface
    HRESULT initD3D9(IDirect3DDevice9* pD3DDevice);
    HRESULT initD3D10(ID3D10Device* pD3DDevice);
    HRESULT initD3D11(ID3D11Device* pD3DDevice);
	HRESULT initGnm();
	HRESULT initGL2(void* pGLContext);
	HRESULT initNoGraphics();
	HRESULT reinit(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params);
	HRESULT addDisplacements(const gfsdk_float2* inSamplePoints, gfsdk_float4* outDisplacements, UINT numSamples);
	HRESULT addArchivedDisplacements(float coord, const gfsdk_float2* inSamplePoints, gfsdk_float4* outDisplacements, UINT numSamples);
	HRESULT getTimings(NVWaveWorks_FFT_Simulation_Timings&) const;
	gfsdk_U64 getDisplacementMapVersion() const { return m_DisplacementMapVersion; }
	LPDIRECT3DTEXTURE9 GetDisplacementMapD3D9();
	ID3D10ShaderResourceView** GetDisplacementMapD3D10();
	ID3D11ShaderResourceView** GetDisplacementMapD3D11();
	sce::Gnm::Texture* GetDisplacementMapGnm();
	GLuint					   GetDisplacementMapGL2();

	const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& GetParams() const { return m_params; }

	bool IsH0UpdateRequired() const { return m_H0UpdateRequired; }
	void SetH0UpdateNotRequired() { m_H0UpdateRequired = false; }

	HRESULT archiveDisplacements(gfsdk_U64 kickID);

	void calcReinit(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params, bool& bRelease, bool& bAllocate, bool& bReinitH0, bool& bReinitGaussAndOmega);
	void pipelineNextReinit() { m_pipelineNextReinit = true; }

private:

	GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade m_next_params;
	GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade m_params;
	bool m_params_are_dirty;

	HRESULT allocateAllResources();
	void releaseAllResources();

	void releaseAll();

	HRESULT initGaussAndOmega();

	// D3D API handling
	nv_water_d3d_api m_d3dAPI;

#if WAVEWORKS_ENABLE_D3D9
    struct D3D9Objects
    {
		IDirect3DDevice9* m_pd3d9Device;
        LPDIRECT3DTEXTURE9 m_pd3d9DisplacementMapTexture[2];			// (ABGR32F)
    };
#endif

#if WAVEWORKS_ENABLE_D3D10
    struct D3D10Objects
    {
		ID3D10Device* m_pd3d10Device;
        ID3D10Texture2D* m_pd3d10DisplacementMapTexture[2];
        ID3D10ShaderResourceView* m_pd3d10DisplacementMapTextureSRV[2];	// (ABGR32F)
    };
#endif

#if WAVEWORKS_ENABLE_D3D11
    struct D3D11Objects
    {
		ID3D11Device* m_pd3d11Device;
		ID3D11Texture2D* m_pd3d11DisplacementMapTexture[2];
		ID3D11ShaderResourceView* m_pd3d11DisplacementMapTextureSRV[2];
		ID3D11DeviceContext* m_pDC;
    };
#endif
#if WAVEWORKS_ENABLE_GNM
	struct GnmObjects
	{
		enum { NumGnmTextures = 3 };
		int m_mapped_gnm_texture_index;	// We triple-buffer on PS4, because there is no driver/runtime to handle buffer renaming
		sce::Gnm::Texture m_pGnmDisplacementMapTexture[NumGnmTextures];
	};
#endif
#if WAVEWORKS_ENABLE_GL	
	struct GL2Objects
	{
		void* m_pGLContext;
		GLuint m_GLDisplacementMapTexture[2];
		GLuint m_GLDisplacementMapPBO[2];
	};
#endif
	struct NoGraphicsObjects
	{
		void* m_pnogfxDisplacementMap[2];
		size_t m_nogfxDisplacementMapRowPitch;
	};

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
#if WAVEWORKS_ENABLE_GNM
		GnmObjects _gnm;
#endif
#if WAVEWORKS_ENABLE_GL
		GL2Objects _GL2;	
#endif
		NoGraphicsObjects _noGFX;
    } m_d3d;

	//initial spectrum data
	float2* m_gauss_data;		// We cache the Gaussian distribution which underlies h0 in order to avoid having to re-run the
								// random number generator when we re-calculate h0 (e.g. when windspeed changes)
	float2* m_h0_data;
	float* m_omega_data;
	float* m_sqrt_table; //pre-computed coefficient for speed-up computation of update spectrum
	
	//in-out buffer for FFTCPU, it holds 3 FFT images sequentially
	complex* m_fftCPU_io_buffer;

	// "safe" buffers with data for readbacks, filled by working threads
	gfsdk_float4* m_readback_buffer[2];
	gfsdk_float4* m_active_readback_buffer;	// The readback buffer currently being served - this can potentially be a different buffer from the
											// double-buffered pair in m_readback_buffer[], since one of those could have been swapped for one
											// from the FIFO when an archiving operation occured

	struct ReadbackFIFOSlot
	{
		gfsdk_U64 kickID;
		gfsdk_float4* buffer;
	};
	CircularFIFO<ReadbackFIFOSlot>* m_pReadbackFIFO;

	volatile LONG m_ref_count_update_h0, m_ref_count_update_ht, m_ref_count_FFT_X, m_ref_count_FFT_Y, m_ref_count_update_texture;

	// current index of a texture that is mapped and filled by working threads
	// can be 0 or 1. Other texture is returned to user and can be safely used for rendering
	int m_mapped_texture_index;

	BYTE* m_mapped_texture_ptr; //pointer to a mapped texture that is filling by working threads
	size_t m_mapped_texture_row_pitch;

	friend void UpdateH0(const Task& task);
	friend void UpdateHt(const Task& task);
	friend void ComputeFFT(const Task& task);
	friend void UpdateTexture(const Task& task);

	double m_doubletime;

	bool m_H0UpdateRequired;

	gfsdk_U64 m_DisplacementMapVersion;

	bool m_pipelineNextReinit;
};

#endif	// _NVWaveWorks_FFT_Simulation_CPU_Impl_H
