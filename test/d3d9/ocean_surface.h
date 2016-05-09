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

#include <D3DX9.h>

#include <GFSDK_WaveWorks.h>

//#define DEBUG_VS
//#define DEBUG_PS

// Ocean grid setting
#define BICOLOR_TEX_SIZE			256

struct OceanSurfaceParameters
{
	// Shading properties
	D3DXVECTOR4 sky_color;
    D3DXVECTOR4 waterbody_color;
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
	IDirect3DDevice9* m_pd3dDevice;
	LPDIRECT3DVERTEXDECLARATION9 m_pQuadDecl;
	
	// Color look up 1D texture
	LPDIRECT3DTEXTURE9 m_pBicolorMap;			// (RGBA8)
	LPDIRECT3DCUBETEXTURE9 m_pCubeMap;
	LPDIRECT3DTEXTURE9 m_pFoamIntensityMap;
	LPDIRECT3DTEXTURE9 m_pFoamDiffuseMap;

	ID3DXEffect* m_pOceanFX;

	// --------------------------------- Rendering routines -----------------------------------

	// Rendering
	void renderShaded(const D3DXMATRIX& matView, const D3DXMATRIX& matProj, GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate, BOOL freeze_cam);
	void renderWireframe(const D3DXMATRIX& matView, const D3DXMATRIX& matProj, GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate, BOOL freeze_cam);
	void getQuadTreeStats(GFSDK_WaveWorks_Quadtree_Stats& stats);

	// --------------------------------- Surface geometry -----------------------------------
	GFSDK_WaveWorks_QuadtreeHandle m_hOceanQuadTree;
	HRESULT initQuadTree(const GFSDK_WaveWorks_Quadtree_Params& params);

	UINT* m_pQuadTreeShaderInputMappings_Shaded;
	UINT* m_pQuadTreeShaderInputMappings_Wireframe;

	UINT* m_pSimulationShaderInputMappings_Shaded;
	UINT* m_pSimulationShaderInputMappings_Wireframe;

	D3DXMATRIX m_matView;
	D3DXMATRIX m_matProj;
};

#endif	// _OCEAN_WAVE_H
