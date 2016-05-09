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

#ifndef _OCEAN_ENV_H
#define _OCEAN_ENV_H

#include <d3d11.h>
#include "ocean_shader_common.h"

struct OceanSkyMapInfo;

struct OceanEnvironment {
	D3DVECTOR main_light_direction;
	D3DVECTOR main_light_color;
	D3DVECTOR ambient_light_color;
	D3DVECTOR sky_color;
	D3DVECTOR sky_map_color_mult;
	const OceanSkyMapInfo* pSky0;
	const OceanSkyMapInfo* pSky1;
	float sky_interp;
	float fog_exponent;

	D3DXVECTOR4 spotlight_position[MaxNumSpotlights];
	D3DXVECTOR4 spotlight_axis_and_cos_angle[MaxNumSpotlights];
	D3DXVECTOR4 spotlight_color[MaxNumSpotlights];

	D3DXMATRIX spotlights_to_world_matrix;
	D3DXMATRIX spotlight_shadow_matrix[MaxNumSpotlights];
	ID3D11ShaderResourceView* spotlight_shadow_resource[MaxNumSpotlights];
	ID3D11ShaderResourceView* pPlanarReflectionSRV;
	ID3D11ShaderResourceView* pPlanarReflectionDepthSRV;
	ID3D11ShaderResourceView* pPlanarReflectionPosSRV;

	int activeLightsNum;
	int objectID[MaxNumSpotlights];
	int lightFilter;

	ID3D11ShaderResourceView* pSceneShadowmapSRV;
	D3DVECTOR lightning_light_position;
	D3DVECTOR lightning_light_intensity;
	float lightning_time_to_next_strike;
	float lightning_time_to_next_cstrike;
	int   lightning_num_of_cstrikes;
	int   lightning_current_cstrike;

	float cloud_factor;

	D3DXVECTOR4 gust_UV;
};


#endif	// _OCEAN_ENV_H
