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

#ifndef _OCEAN_WAVE_H
#define _OCEAN_WAVE_H

#include <D3DX11Effect.h>

#include <GFSDK_WaveWorks.h>
#include <DirectXMath.h>

using namespace DirectX;

//#define DEBUG_VS
//#define DEBUG_PS

// Ocean grid setting
#define BICOLOR_TEX_SIZE			256

struct OceanSurfaceParameters
{
	// Shading properties
	XMFLOAT4 sky_color;
	XMFLOAT4 waterbody_color;
    float sky_blending;
};

class OceanSurface
{
public:
	OceanSurface();
	~OceanSurface();

	OceanSurfaceParameters m_params;

	HRESULT init(const OceanSurfaceParameters& params);

	// create color/fresnel lookup table.
	void createFresnelMap();

	// ---------------------------------- GPU shading data ------------------------------------

	// D3D objects
	ID3D11Device* m_pd3dDevice;
	ID3D11InputLayout* m_pQuadLayout;
	
	// Color look up 1D texture
	ID3D11ShaderResourceView* m_pBicolorMap;			// (RGBA8)
	ID3D11ShaderResourceView* m_pCubeMap;
	ID3D11ShaderResourceView* m_pFoamIntensityMap;
	ID3D11ShaderResourceView* m_pFoamDiffuseMap;

	ID3DX11Effect* m_pOceanFX;
	ID3DX11EffectTechnique* m_pRenderSurfaceTechnique;
	ID3DX11EffectPass* m_pRenderSurfaceShadedPass;
	ID3DX11EffectPass* m_pRenderSurfaceWireframePass;
	ID3DX11EffectMatrixVariable* m_pRenderSurfaceMatViewProjVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceSkyColorVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceWaterColorVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfacePatchColorVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceColorMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceCubeMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceFoamIntensityMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceFoamDiffuseMapVariable;

	// --------------------------------- Rendering routines -----------------------------------

	// Rendering
	void renderShaded(ID3D11DeviceContext* pDC, const XMMATRIX& matView, const XMMATRIX& matProj, GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate, BOOL freeze_cam);
	void renderWireframe(ID3D11DeviceContext* pDC, const XMMATRIX& matView, const XMMATRIX& matProj, GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate, BOOL freeze_cam);
	void getQuadTreeStats(GFSDK_WaveWorks_Quadtree_Stats& stats);

	// --------------------------------- Surface geometry -----------------------------------
	GFSDK_WaveWorks_QuadtreeHandle m_hOceanQuadTree;
	HRESULT initQuadTree(const GFSDK_WaveWorks_Quadtree_Params& params);

	UINT* m_pQuadTreeShaderInputMappings_Shaded;
	UINT* m_pQuadTreeShaderInputMappings_Wireframe;

	UINT* m_pSimulationShaderInputMappings_Shaded;
	UINT* m_pSimulationShaderInputMappings_Wireframe;

	XMMATRIX m_matView;
	XMMATRIX m_matProj;
};

#endif	// _OCEAN_WAVE_H
