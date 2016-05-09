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

#ifndef _OCEAN_VESSEL_H
#define _OCEAN_VESSEL_H

#include <D3DX10.h>

#include "GFSDK_WaveWorks_Types.h"

class OceanVessel
{
public:
	OceanVessel();
	~OceanVessel();

	HRESULT init(FLOAT w, FLOAT h, FLOAT l);
	void updateVesselMotion(GFSDK_WaveWorks_SimulationHandle hSim, FLOAT sea_level, FLOAT time_delta, FLOAT water_scale);
	void renderVessel(const CBaseCamera& camera);

	void setPosition(D3DXVECTOR2 pos);
	void setHeading(D3DXVECTOR2 heading);

	const D3DXMATRIX* getCameraXform() const { return &m_CameraToWorld; }

private:

	// ---------------------------------- GPU shading data ------------------------------------

	// D3D objects
	ID3D10Device* m_pd3dDevice;

	ID3D10Buffer* m_pIB;
	ID3D10Buffer* m_pVB;
	ID3D10InputLayout* m_pLayout;

	UINT m_NumTris;
	UINT m_NumVerts;

	ID3D10Effect* m_pFX;
	ID3D10EffectTechnique* m_pRenderTechnique;
	ID3D10EffectMatrixVariable* m_pMatWorldViewProjVariable;

	D3DXVECTOR2 m_Position;
	D3DXVECTOR2 m_Heading;
	FLOAT m_Draft;
	FLOAT m_KeelLength;
	FLOAT m_Beam;

	// Dynamic state
	FLOAT m_Pitch;
	FLOAT m_PitchRate;
	FLOAT m_Roll;
	FLOAT m_RollRate;
	FLOAT m_Height;
	FLOAT m_HeightRate;

	UINT m_DynamicsCountdown;
	void ResetDynamicState();

	FLOAT m_PitchSpringConstant;
	FLOAT m_PitchDampingConstant;
	FLOAT m_RollSpringConstant;
	FLOAT m_RollDampingConstant;
	FLOAT m_HeightSpringConstant;
	FLOAT m_HeightDampingConstant;

	D3DXMATRIX m_CameraToWorld;
	D3DXMATRIX m_LocalToWorld;
};

#endif	// _OCEAN_VESSEL_H
