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
#include "distance_field.h"

class OceanSurface
{
public:
	OceanSurface();
	~OceanSurface();

	HRESULT init();

	// ---------------------------------- GPU shading data ------------------------------------

	// D3D objects
	ID3D11Device*			m_pd3dDevice;

	ID3DX11Effect*			m_pOceanFX;
	ID3DX11EffectTechnique* m_pRenderSurfaceTechnique;
	ID3DX11EffectPass*		m_pRenderSurfaceShadedWithShorelinePass;
	ID3DX11EffectPass*		m_pRenderSurfaceWireframeWithShorelinePass;

	ID3D11InputLayout*		m_pQuadLayout;
	ID3D11InputLayout*		m_pRayContactLayout;
	ID3DX11EffectTechnique*	m_pRenderRayContactTechnique;
	ID3D11Buffer*			m_pContactVB;
	ID3D11Buffer*			m_pContactIB;

	DistanceField*							pDistanceFieldModule; // Not owned!
	void AttachDistanceFieldModule( DistanceField* pDistanceField ) { pDistanceFieldModule = pDistanceField; }

	// --------------------------------- Rendering routines -----------------------------------
	// Rendering
	void renderShaded(	ID3D11DeviceContext* pDC, 
						const XMMATRIX& matView, 
						const XMMATRIX& matProj,
						GFSDK_WaveWorks_SimulationHandle hSim, 
						GFSDK_WaveWorks_SavestateHandle hSavestate, 
						const XMFLOAT2& windDir, 
						const float steepness, 
						const float amplitude, 
						const float wavelength, 
						const float speed, 
						const float parallelness,
                        const float totalTime
						);
	void getQuadTreeStats(GFSDK_WaveWorks_Quadtree_Stats& stats);

	// --------------------------------- Surface geometry -----------------------------------
	GFSDK_WaveWorks_QuadtreeHandle m_hOceanQuadTree;
	HRESULT initQuadTree(const GFSDK_WaveWorks_Quadtree_Params& params);

	UINT* m_pQuadTreeShaderInputMappings_Shore;
	UINT* m_pSimulationShaderInputMappings_Shore;
};

#endif	// _OCEAN_WAVE_H
