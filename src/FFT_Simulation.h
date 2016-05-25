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

#ifndef _NVWAVEWORKS_FFT_SIMULATION_H
#define _NVWAVEWORKS_FFT_SIMULATION_H

#if WAVEWORKS_ENABLE_GNM
#include <gnm\texture.h>
#else
namespace sce { namespace Gnm { struct Texture; } }
#endif

class NVWaveWorks_GFX_Timer_Impl;

typedef struct IDirect3DTexture9* LPDIRECT3DTEXTURE9;
struct ID3D10ShaderResourceView;
struct ID3D11ShaderResourceView;

struct NVWaveWorks_FFT_Simulation_Timings
{
	float GPU_simulation_time;					 // GPU time spent on simulation
	float GPU_FFT_simulation_time;				 // GPU simulation time spent on simulation
};

class NVWaveWorks_FFT_Simulation
{
public:

	virtual ~NVWaveWorks_FFT_Simulation() {};

    virtual HRESULT initD3D11(ID3D11Device* pD3DDevice) = 0;
	virtual HRESULT initGnm() { return S_FALSE; };
	virtual HRESULT initGL2(void* /*pGLContext*/) { return S_FALSE; };
	virtual HRESULT initNoGraphics() = 0;

	virtual HRESULT reinit(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params) = 0;

	virtual HRESULT addDisplacements(	const gfsdk_float2* inSamplePoints,
										gfsdk_float4* outDisplacements,
										UINT numSamples
										) = 0;

	virtual HRESULT addArchivedDisplacements(	float coord,
												const gfsdk_float2* inSamplePoints,
												gfsdk_float4* outDisplacements,
												UINT numSamples
												) = 0;

	virtual HRESULT getTimings(NVWaveWorks_FFT_Simulation_Timings&) const = 0;

	virtual gfsdk_U64 getDisplacementMapVersion() const = 0;	// Returns the kickID of the last time the displacement map was updated

	// NB: None of these AddRef's the underlying D3D resource
	virtual ID3D11ShaderResourceView** GetDisplacementMapD3D11() = 0;
	virtual sce::Gnm::Texture* GetDisplacementMapGnm() { return NULL; }
	virtual GLuint					   GetDisplacementMapGL2() = 0;
};

#endif	// _NVWAVEWORKS_FFT_SIMULATION_H
