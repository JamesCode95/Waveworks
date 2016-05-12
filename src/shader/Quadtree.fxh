/*
 * This code contains NVIDIA Confidential Information and is disclosed 
 * under the Mutual Non-Disclosure Agreement. 
 * 
 * Notice 
 * ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
 * NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
 * THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
 * MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
 * 
 * NVIDIA Corporation assumes no responsibility for the consequences of use of such 
 * information or for any infringement of patents or other rights of third parties that may 
 * result from its use. No license is granted by implication or otherwise under any patent 
 * or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
 * expressly authorized by NVIDIA.  Details are subject to change without notice. 
 * This code supersedes and replaces all information previously supplied. 
 * NVIDIA Corporation products are not authorized for use as critical 
 * components in life support devices or systems without express written approval of 
 * NVIDIA Corporation. 
 * 
 * Copyright © 2008- 2013 NVIDIA Corporation. All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property and proprietary
 * rights in and to this software and related documentation and any modifications thereto.
 * Any use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation is
 * strictly prohibited.
 */

#include "Common.fxh"

/*
 *
 *
 */
 
#if defined(GFSDK_WAVEWORKS_SM3) || defined(GFSDK_WAVEWORKS_GL)
	#define GFSDK_WAVEWORKS_BEGIN_GEOM_VS_CBUFFER(Label)
	#define GFSDK_WAVEWORKS_END_GEOM_VS_CBUFFER
#endif

#if defined( GFSDK_WAVEWORKS_USE_TESSELLATION )
	GFSDK_WAVEWORKS_BEGIN_GEOM_HS_CBUFFER(nvsf_eyepos_buffer)
	GFSDK_WAVEWORKS_DECLARE_GEOM_HS_CONSTANT(float4, nvsf_g_hsWorldEye, 0)
	GFSDK_WAVEWORKS_DECLARE_GEOM_HS_CONSTANT(float4, nvsf_g_tessellationParams, 1)
	GFSDK_WAVEWORKS_END_GEOM_HS_CBUFFER
#endif

GFSDK_WAVEWORKS_BEGIN_GEOM_VS_CBUFFER(nvsf_geom_buffer)
GFSDK_WAVEWORKS_DECLARE_GEOM_VS_CONSTANT(float4x3, nvsf_g_matLocalWorld, 0)
GFSDK_WAVEWORKS_DECLARE_GEOM_VS_CONSTANT(float4, nvsf_g_vsEyePos, 3)
GFSDK_WAVEWORKS_DECLARE_GEOM_VS_CONSTANT(float4, nvsf_g_MorphParam, 4)
GFSDK_WAVEWORKS_END_GEOM_VS_CBUFFER


struct GFSDK_WAVEWORKS_VERTEX_INPUT
{
	float4 nvsf_vPos SEMANTIC(POSITION);
};

#if !defined(GFSDK_WAVEWORKS_USE_TESSELLATION)
float3 GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition(GFSDK_WAVEWORKS_VERTEX_INPUT In)
{
	float2 nvsf_vpos = In.nvsf_vPos.xy;
	
	// Use multiple levels of geo-morphing to smooth away LOD boundaries
	float nvsf_geomorph_scale = 0.25f;

	float2 nvsf_geomorph_offset = float2(nvsf_g_MorphParam.w,nvsf_g_MorphParam.w);
	float2 nvsf_vpos_src = nvsf_vpos;
	float2 nvsf_vpos_target = nvsf_vpos_src;
	float nvsf_geomorph_amount = 0.f;
	
	for(int nvsf_geomorph_level = 0; nvsf_geomorph_level != 4; ++nvsf_geomorph_level) {

		float2 nvsf_intpart;
		float2 nvsf_rempart = modf(nvsf_geomorph_scale*nvsf_vpos_src.xy,nvsf_intpart);

		float2 nvsf_mirror = float2(1.0f, 1.0f);
		
		if(nvsf_rempart.x >  0.5f)
		{
			nvsf_rempart.x = 1.0f - nvsf_rempart.x;
			nvsf_mirror.x = -nvsf_mirror.x;
		}
		if(nvsf_rempart.y >  0.5f)
		{
			nvsf_rempart.y = 1.0f - nvsf_rempart.y;
			nvsf_mirror.y = -nvsf_mirror.y;
		}
		

		if(0.25f == nvsf_rempart.x && 0.25f == nvsf_rempart.y) nvsf_vpos_target.xy = nvsf_vpos_src.xy - nvsf_geomorph_offset*nvsf_mirror;
		else if(0.25f == nvsf_rempart.x) nvsf_vpos_target.x = nvsf_vpos_src.x + nvsf_geomorph_offset.x*nvsf_mirror.x;
		else if(0.25f == nvsf_rempart.y) nvsf_vpos_target.y = nvsf_vpos_src.y + nvsf_geomorph_offset.y*nvsf_mirror.y;

		float3 nvsf_eyevec = mul(float4(nvsf_vpos_target,0.f,1.f), nvsf_g_matLocalWorld) - nvsf_g_vsEyePos.xyz;
		float nvsf_d = length(nvsf_eyevec);
		float nvsf_geomorph_target_level = log2(nvsf_d * nvsf_g_MorphParam.x) + 1.f;
		nvsf_geomorph_amount = saturate(2.0*(nvsf_geomorph_target_level - float(nvsf_geomorph_level)));
		if(nvsf_geomorph_amount < 1.f) 
		{
			break;
		} 
		else 
		{
			nvsf_vpos_src = nvsf_vpos_target;
			nvsf_geomorph_scale *= 0.5f;
			nvsf_geomorph_offset *= -2.f;
		}
	}

	nvsf_vpos.xy = lerp(nvsf_vpos_src, nvsf_vpos_target, nvsf_geomorph_amount);
	return mul(float4(nvsf_vpos,In.nvsf_vPos.zw), nvsf_g_matLocalWorld);
}
#endif


#if defined(GFSDK_WAVEWORKS_USE_TESSELLATION)
float3 GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition(GFSDK_WAVEWORKS_VERTEX_INPUT In)
{
	float2 nvsf_vpos = In.nvsf_vPos.xy;
	// Use multiple levels of geo-morphing to smooth away LOD boundaries
	float nvsf_geomorph_scale = 0.5f;
	float nvsf_geomorph_offset = abs(nvsf_g_MorphParam.w);
	float2 nvsf_vpos_src = nvsf_vpos;
	float2 nvsf_vpos_target = nvsf_vpos_src;
	float nvsf_geomorph_amount = 0.f;
	
	//nvsf_vpos_target.x += 0.25*nvsf_geomorph_offset;
	//nvsf_vpos_src.x += 0.25*nvsf_geomorph_offset;
	
	for(int nvsf_geomorph_level = 0; nvsf_geomorph_level != 4; ++nvsf_geomorph_level) {

		float2 nvsf_intpart;
		float2 nvsf_rempart = modf(nvsf_geomorph_scale*nvsf_vpos_src.xy,nvsf_intpart);
		if(0.5f == nvsf_rempart.x) 
		{
			nvsf_vpos_target.x = nvsf_vpos_src.x + nvsf_geomorph_offset;
		}

		if(0.5f == nvsf_rempart.y) 
		{
			nvsf_vpos_target.y = nvsf_vpos_src.y + nvsf_geomorph_offset;
		}
		
		float3 nvsf_eyevec = mul(float4(nvsf_vpos_target,0.f,1.f), nvsf_g_matLocalWorld) - nvsf_g_vsEyePos.xyz;
		float nvsf_d = length(nvsf_eyevec);
		float nvsf_geomorph_target_level = log2(nvsf_d * nvsf_g_MorphParam.x) + 1.f;
		nvsf_geomorph_amount = saturate(3.0*(nvsf_geomorph_target_level - float(nvsf_geomorph_level)));
		if(nvsf_geomorph_amount < 1.f) {
			break;
		} else {
			nvsf_vpos_src = nvsf_vpos_target;
			nvsf_geomorph_scale *= 0.5f;
			nvsf_geomorph_offset *= 2.f;
		}
	}
	nvsf_vpos.xy = lerp(nvsf_vpos_src, nvsf_vpos_target, nvsf_geomorph_amount);
	return mul(float4(nvsf_vpos,In.nvsf_vPos.zw), nvsf_g_matLocalWorld);
}

float GFSDK_WaveWorks_GetEdgeTessellationFactor(float4 vertex1, float4 vertex2)
{
	float3 nvsf_edge_center = 0.5*(vertex1.xyz + vertex2.xyz);
	float nvsf_edge_length = length (vertex1.xyz - vertex2.xyz);
	float nvsf_edge_distance = length(nvsf_g_hsWorldEye.xyz - nvsf_edge_center.xyz);
	return nvsf_g_tessellationParams.x * nvsf_edge_length / nvsf_edge_distance;
}

float GFSDK_WaveWorks_GetVertexTargetTessellatedEdgeLength(float3 vertex)
{
	float nvsf_vertex_distance = length(nvsf_g_hsWorldEye.xyz - vertex.xyz);
	return nvsf_vertex_distance / nvsf_g_tessellationParams.x;
}

#endif

