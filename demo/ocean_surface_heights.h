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

#ifndef _OCEAN_SURFACE_HEIGHTS_H
#define _OCEAN_SURFACE_HEIGHTS_H

#include "GFSDK_WaveWorks_Types.h"

#include <D3DX11Effect.h>
#include <d3d11.h>

class OceanSurfaceHeights
{
public:
	OceanSurfaceHeights(int max_num_samples, const gfsdk_float2& UVToWorldScale);
	~OceanSurfaceHeights();

	HRESULT init();

	void updateHeights(ID3D11DeviceContext* pDC, GFSDK_WaveWorks_SimulationHandle hSim, const gfsdk_float2& UVToWorldRotation, const gfsdk_float2& worldCentroid);
	void updateGPUHeights(ID3D11DeviceContext* pDC, GFSDK_WaveWorks_SimulationHandle hSim, const D3DXMATRIX& matView);

	void getDisplacements(const gfsdk_float2* inSamplePoints, gfsdk_float4* outDisplacements, gfsdk_U32 numSamples) const;

	void getGradients(const gfsdk_float2* inSamplePoints, gfsdk_float2* outGradients, gfsdk_U32 numSamples) const;

	void renderTextureToUI(ID3D11DeviceContext* pDC) const;

	void renderMarkerArray(ID3D11DeviceContext* pDC, const D3DXMATRIX& matViewProj, const D3DXMATRIX& matWorld) const;

	void getGPUWorldToUVTransform(gfsdk_float2& offset, gfsdk_float2& rot, gfsdk_float2& scale) const;
	ID3D11ShaderResourceView* getGPULookupSRV() const { return m_pLookupSRV; }

private:

	int converge_scratch(GFSDK_WaveWorks_SimulationHandle hSim, int num_samples, float initial_conv_amt);

	void updateGradients();

	gfsdk_float2 m_UVToWorldScale;
	gfsdk_float2 m_UVToWorldRotation;
	gfsdk_float2 m_UVToWorldOffset;

	int m_num_samples_w;
	int m_num_samples_l;
	int m_num_gpu_samples_w;
	int m_num_gpu_samples_l;
	int m_max_num_intervals_stride;

	gfsdk_float2* m_pTargetSamplePositions;
	gfsdk_float2* m_pAdjustedSamplePositions;
	gfsdk_float4* m_pSampleDisplacements;
	gfsdk_float2* m_pSampleGradients;

	gfsdk_float2* m_pScratchTargetSampleInputs;
	gfsdk_float2* m_pScratchAdjustedSampleInputs;
	gfsdk_float4* m_pScratchSampleDisplacements;

	// FX objects
	ID3DX11Effect* m_pFX;
	ID3DX11EffectTechnique* m_pRenderSurfaceToReverseLookupTechnique;
    ID3DX11EffectTechnique* m_pRenderQuadToUITechnique;
	ID3DX11EffectTechnique* m_pRenderMarkerTechnique;

	ID3DX11EffectScalarVariable* m_pNumQuadsWVariable;
	ID3DX11EffectScalarVariable* m_pNumQuadsHVariable;
	ID3DX11EffectVectorVariable* m_pQuadUVDimsVariable;
	ID3DX11EffectVectorVariable* m_pSrcUVToWorldScaleVariable;
	ID3DX11EffectVectorVariable* m_pSrcUVToWorldRotationVariable;
	ID3DX11EffectVectorVariable* m_pSrcUVToWorldOffsetVariable;
	ID3DX11EffectVectorVariable* m_pWorldToClipScaleVariable;
	ID3DX11EffectVectorVariable* m_pClipToWorldRotationVariable;
	ID3DX11EffectVectorVariable* m_pClipToWorldOffsetVariable;
	
	// UI rendering
	ID3DX11EffectShaderResourceVariable* m_pTexDiffuseVariable;

	// Marker rendering
	ID3DX11EffectShaderResourceVariable* m_pTexLookupVariable;
	ID3DX11EffectMatrixVariable* m_pMatViewProjVariable;
	ID3DX11EffectMatrixVariable* m_pMatWorldVariable;
	ID3DX11EffectVectorVariable* m_pWorldToUVScaleVariable;
	ID3DX11EffectVectorVariable* m_pWorldToUVRotationVariable;
	ID3DX11EffectVectorVariable* m_pWorldToUVOffsetVariable;

	UINT* m_pWaterSimulationShaderInputMappings;

	// D3D objects
	ID3D11Device* m_pd3dDevice;
	ID3D11ShaderResourceView* m_pLookupSRV;
	ID3D11RenderTargetView* m_pLookupRTV;
};

#endif	// _OCEAN_SURFACE_HEIGHTS_H
