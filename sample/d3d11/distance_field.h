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

#ifndef _SHORELINE_INTERACTION_H
#define _SHORELINE_INTERACTION_H

#include "terrain.h"
#include <D3DX11Effect.h>

#include <GFSDK_WaveWorks.h>

struct DistanceField
{
	explicit DistanceField( CTerrain* const pTerrainRenderer );
	~DistanceField();

	HRESULT Init( ID3D11Device* const pDevice );

	// --------------------------------- Accessors -----------------------------------
	ID3D11ShaderResourceView*	GetDataTextureSRV() const	{ return m_pTopDownDataSRV; } 

	void GetWorldToTopDownTextureMatrix(XMMATRIX &worldToTopDownMatrix);
	// --------------------------------- Rendering routines -----------------------------------
	void GenerateDataTexture(ID3D11DeviceContext* pDC );

private:
    DistanceField()
        : m_pTerrainRenderer( NULL )
    {}
    DistanceField& operator=(const DistanceField &tmp);

	// ---------------------------------- Not owned refrences ------------------------------------
	CTerrain* const			m_pTerrainRenderer;	// Not owned.

	// ---------------------------------- GPU shading data ------------------------------------
	ID3D11ShaderResourceView*	m_pTopDownDataSRV;
	ID3D11RenderTargetView*		m_pTopDownDataRTV;
	ID3D11Texture2D*			m_pTopDownDataTexture;
	ID3D11Texture2D*			m_pStagingTexture;

	// ---------------------------------- Top down camera data ------------------------------------
	XMFLOAT4	m_topDownViewPositionWS;
	XMFLOAT4	m_viewDirectionWS;
	XMFLOAT4X4	m_worldToViewMatrix;
	XMFLOAT4X4	m_viewToProjectionMatrix;

	bool m_shouldGenerateDataTexture;

	void renderTopDownData(ID3D11DeviceContext* pDC, const XMVECTOR eyePositionWS);
	void generateDistanceField(ID3D11DeviceContext* pDC);
	bool checkPixel( float* pTextureData, const int cx, const int cy, const int dx, const int dy) const;
	float FindNearestPixel( float* pTextureData, const int cx, const int cy, float&, float&);
};

#endif	// _SHORELINE_INTERACTION_H
