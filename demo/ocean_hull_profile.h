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

#ifndef _OCEAN_HULL_PROFILE_H
#define _OCEAN_HULL_PROFILE_H

#include <d3d11.h>

class OceanHullProfile {

public:

	OceanHullProfile() :
		m_pSRV(0)
	{
	}

	OceanHullProfile(ID3D11ShaderResourceView* pSRV) :
		m_pSRV(pSRV)
	{
		m_pSRV->AddRef();
	}

	OceanHullProfile(const OceanHullProfile& rhs)
	{
		*this = rhs;
	}

	OceanHullProfile& operator=(const OceanHullProfile& rhs)
	{
		if(this != &rhs) {
			SAFE_RELEASE(m_pSRV);
			m_pSRV = rhs.m_pSRV;
			m_WorldToProfileCoordsOffset = rhs.m_WorldToProfileCoordsOffset;
			m_WorldToProfileCoordsScale = rhs.m_WorldToProfileCoordsScale;
			m_ProfileToWorldHeightOffset = rhs.m_ProfileToWorldHeightOffset;
			m_ProfileToWorldHeightScale = rhs.m_ProfileToWorldHeightScale;
			m_TexelSizeInWorldSpace = rhs.m_TexelSizeInWorldSpace;

			if(m_pSRV) {
				m_pSRV->AddRef();
			}
		}

		return *this;
	}

	~OceanHullProfile()
	{
		SAFE_RELEASE(m_pSRV);
	}

	ID3D11ShaderResourceView* GetSRV() const { return m_pSRV; }

	// For generating lookup coords from world coords: uv = offset + scale * world_pos
	D3DXVECTOR2 m_WorldToProfileCoordsOffset;
	D3DXVECTOR2 m_WorldToProfileCoordsScale;

	// For generating a world height from lookup: world_height = offset + scale * lookup
	FLOAT m_ProfileToWorldHeightOffset;
	FLOAT m_ProfileToWorldHeightScale;

	// For choosing the right mip
	FLOAT m_TexelSizeInWorldSpace;

private:

	// The SRV for doing the lookup
	ID3D11ShaderResourceView* m_pSRV;
};

#endif	// _OCEAN_HULL_PROFILE_H
