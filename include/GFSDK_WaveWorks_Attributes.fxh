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
 * NVIDIA Corporation products are not authorized for use as critic
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

#ifndef _GFSDK_WAVEWORKS_ATTRIBUTES_FX
#define _GFSDK_WAVEWORKS_ATTRIBUTES_FX

/*
 *
 *
 */

#include "GFSDK_WaveWorks_Common.fxh"

/*
 *
 *
 */

#if defined(GFSDK_WAVEWORKS_SM3) || defined(GFSDK_WAVEWORKS_GL)
	#define GFSDK_WAVEWORKS_BEGIN_ATTR_VS_CBUFFER(Label)
	#define GFSDK_WAVEWORKS_END_ATTR_VS_CBUFFER
	#define GFSDK_WAVEWORKS_BEGIN_ATTR_PS_CBUFFER(Label) 
	#define GFSDK_WAVEWORKS_END_ATTR_PS_CBUFFER
#endif


#if defined( GFSDK_WAVEWORKS_USE_TESSELLATION )
	#define GFSDK_WAVEWORKS_BEGIN_ATTR_DISPLACEMENT_CBUFFER(Label) GFSDK_WAVEWORKS_BEGIN_ATTR_DS_CBUFFER(Label)
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(Type,Label,Regoff) GFSDK_WAVEWORKS_DECLARE_ATTR_DS_CONSTANT(Type,Label,Regoff) 
	#define GFSDK_WAVEWORKS_END_ATTR_DISPLACEMENT_CBUFFER GFSDK_WAVEWORKS_END_ATTR_DS_CBUFFER
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(SampLabel,TexLabel,Regoff) GFSDK_WAVEWORKS_DECLARE_ATTR_DS_SAMPLER(SampLabel,TexLabel,Regoff)
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER_TEXTUREARRAY(SampLabel,TexLabel,Regoff) GFSDK_WAVEWORKS_DECLARE_ATTR_DS_SAMPLER_TEXTUREARRAY(SampLabel,TexLabel,Regoff)
#else
	#define GFSDK_WAVEWORKS_BEGIN_ATTR_DISPLACEMENT_CBUFFER(Label) GFSDK_WAVEWORKS_BEGIN_ATTR_VS_CBUFFER(Label)
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(Type,Label,Regoff)  GFSDK_WAVEWORKS_DECLARE_ATTR_VS_CONSTANT(Type,Label,Regoff) 
	#define GFSDK_WAVEWORKS_END_ATTR_DISPLACEMENT_CBUFFER GFSDK_WAVEWORKS_END_ATTR_VS_CBUFFER
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(SampLabel,TexLabel,Regoff) GFSDK_WAVEWORKS_DECLARE_ATTR_VS_SAMPLER(SampLabel,TexLabel,Regoff)
	#define GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER_TEXTUREARRAY(SampLabel,TexLabel,Regoff) GFSDK_WAVEWORKS_DECLARE_ATTR_VS_SAMPLER_TEXTUREARRAY(SampLabel,TexLabel,Regoff)
#endif
	
GFSDK_WAVEWORKS_BEGIN_ATTR_DISPLACEMENT_CBUFFER(nvsf_attr_vs_buffer)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float3, nvsf_g_WorldEye, 0)
#if defined( GFSDK_WAVEWORKS_GL )
	GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float, nvsf_g_UseTextureArrays, 1)
#else
	GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float, nvsf_g_Pad1, 1)
#endif
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float4, nvsf_g_UVScaleCascade0123, 2)
GFSDK_WAVEWORKS_END_ATTR_DISPLACEMENT_CBUFFER

GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(nvsf_g_samplerDisplacementMap0, nvsf_g_textureDisplacementMap0, 0)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(nvsf_g_samplerDisplacementMap1, nvsf_g_textureDisplacementMap1, 1)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(nvsf_g_samplerDisplacementMap2, nvsf_g_textureDisplacementMap2, 2)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(nvsf_g_samplerDisplacementMap3, nvsf_g_textureDisplacementMap3, 3)

#if defined( GFSDK_WAVEWORKS_GL )
	GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER_TEXTUREARRAY(nvsf_g_samplerDisplacementMapTextureArray, nvsf_g_textureArrayDisplacementMap, 4)
#endif

GFSDK_WAVEWORKS_BEGIN_ATTR_PS_CBUFFER(nvsf_attr_ps_buffer)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nvsf_g_TexelLength_x2_PS, 0)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nvsf_g_Cascade1Scale_PS, 1)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nvsf_g_Cascade1TexelScale_PS, 2)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nvsf_g_Cascade1UVOffset_PS, 3)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nvsf_g_Cascade2Scale_PS, 4)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nvsf_g_Cascade2TexelScale_PS, 5)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nvsf_g_Cascade2UVOffset_PS, 6)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nvsf_g_Cascade3Scale_PS, 7)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nvsf_g_Cascade3TexelScale_PS, 8)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, nvsf_g_Cascade3UVOffset_PS, 9)
GFSDK_WAVEWORKS_END_ATTR_PS_CBUFFER

GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(nvsf_g_samplerGradientMap0, nvsf_g_textureGradientMap0, 0)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(nvsf_g_samplerGradientMap1, nvsf_g_textureGradientMap1, 1)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(nvsf_g_samplerGradientMap2, nvsf_g_textureGradientMap2, 2)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(nvsf_g_samplerGradientMap3, nvsf_g_textureGradientMap3, 3)

#if defined( GFSDK_WAVEWORKS_GL )
	GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER_TEXTUREARRAY(nvsf_g_samplerGradientMapTextureArray, nvsf_g_textureArrayGradientMap, 4)
#endif

struct GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT
{
    float4 nvsf_tex_coord_cascade01			SEMANTIC(TEXCOORD0);
    float4 nvsf_tex_coord_cascade23			SEMANTIC(TEXCOORD1);
    float4 nvsf_blend_factor_cascade0123	SEMANTIC(TEXCOORD2);
    float3 nvsf_eye_vec						SEMANTIC(TEXCOORD3);
};

struct GFSDK_WAVEWORKS_VERTEX_OUTPUT
{
    centroid GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT interp;
    float3 pos_world;
    float3 pos_world_undisplaced;
    float3 world_displacement;
};

GFSDK_WAVEWORKS_VERTEX_OUTPUT GFSDK_WaveWorks_GetDisplacedVertex(GFSDK_WAVEWORKS_VERTEX_INPUT In)
{
	// Get starting position and distance to camera
	float3 nvsf_pos_world_undisplaced = GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition(In);
	float  nvsf_distance = length(nvsf_g_WorldEye - nvsf_pos_world_undisplaced);
	
	// UVs
	float2 nvsf_uv_world_cascade0 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.x;
	float2 nvsf_uv_world_cascade1 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.y;
	float2 nvsf_uv_world_cascade2 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.z;
	float2 nvsf_uv_world_cascade3 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.w;
	
	// cascade blend factors
	float4 nvsf_blendfactors;
	float4 nvsf_cascade_spatial_size = 1.0/nvsf_g_UVScaleCascade0123.xyzw;
	nvsf_blendfactors.x = 1.0;
	nvsf_blendfactors.yzw = saturate(0.25*(nvsf_cascade_spatial_size.yzw*24.0-nvsf_distance)/nvsf_cascade_spatial_size.yzw);
	nvsf_blendfactors.yzw *= nvsf_blendfactors.yzw;


	// Displacement map
	#if defined(GFSDK_WAVEWORKS_GL)
		float3 nvsf_displacement;
		if(nvsf_g_UseTextureArrays > 0)
		{
			nvsf_displacement =  nvsf_blendfactors.x * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade0, 0.0), 0).xyz;
			nvsf_displacement += nvsf_blendfactors.y==0? float3(0,0,0) : nvsf_blendfactors.y * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade1, 1.0), 0).xyz;
			nvsf_displacement += nvsf_blendfactors.z==0? float3(0,0,0) : nvsf_blendfactors.z * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade2, 2.0), 0).xyz;
			nvsf_displacement += nvsf_blendfactors.w==0? float3(0,0,0) : nvsf_blendfactors.w * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade3, 3.0), 0).xyz;
		}
		else
		{
			nvsf_displacement =  nvsf_blendfactors.x * SampleTex2Dlod(nvsf_g_textureDisplacementMap0, nvsf_g_samplerDisplacementMap0, nvsf_uv_world_cascade0, 0).xyz;
			nvsf_displacement += nvsf_blendfactors.y==0? float3(0,0,0) : nvsf_blendfactors.y * SampleTex2Dlod(nvsf_g_textureDisplacementMap1, nvsf_g_samplerDisplacementMap1, nvsf_uv_world_cascade1, 0).xyz;
			nvsf_displacement += nvsf_blendfactors.z==0? float3(0,0,0) : nvsf_blendfactors.z * SampleTex2Dlod(nvsf_g_textureDisplacementMap2, nvsf_g_samplerDisplacementMap2, nvsf_uv_world_cascade2, 0).xyz;
			nvsf_displacement += nvsf_blendfactors.w==0? float3(0,0,0) : nvsf_blendfactors.w * SampleTex2Dlod(nvsf_g_textureDisplacementMap3, nvsf_g_samplerDisplacementMap3, nvsf_uv_world_cascade3, 0).xyz;
		}
	#else
		float3 nvsf_displacement =  nvsf_blendfactors.x * SampleTex2Dlod(nvsf_g_textureDisplacementMap0, nvsf_g_samplerDisplacementMap0, nvsf_uv_world_cascade0, 0).xyz;
			   nvsf_displacement += nvsf_blendfactors.y==0? float3(0,0,0) : nvsf_blendfactors.y * SampleTex2Dlod(nvsf_g_textureDisplacementMap1, nvsf_g_samplerDisplacementMap1, nvsf_uv_world_cascade1, 0).xyz;
			   nvsf_displacement += nvsf_blendfactors.z==0? float3(0,0,0) : nvsf_blendfactors.z * SampleTex2Dlod(nvsf_g_textureDisplacementMap2, nvsf_g_samplerDisplacementMap2, nvsf_uv_world_cascade2, 0).xyz;
			   nvsf_displacement += nvsf_blendfactors.w==0? float3(0,0,0) : nvsf_blendfactors.w * SampleTex2Dlod(nvsf_g_textureDisplacementMap3, nvsf_g_samplerDisplacementMap3, nvsf_uv_world_cascade3, 0).xyz;
	#endif

	float3 nvsf_pos_world = nvsf_pos_world_undisplaced + nvsf_displacement;

	// Output
	GFSDK_WAVEWORKS_VERTEX_OUTPUT Output;
	Output.interp.nvsf_eye_vec = nvsf_g_WorldEye - nvsf_pos_world;
	Output.interp.nvsf_tex_coord_cascade01.xy = nvsf_uv_world_cascade0;
	Output.interp.nvsf_tex_coord_cascade01.zw = nvsf_uv_world_cascade1;
	Output.interp.nvsf_tex_coord_cascade23.xy = nvsf_uv_world_cascade2;
	Output.interp.nvsf_tex_coord_cascade23.zw = nvsf_uv_world_cascade3;
	Output.interp.nvsf_blend_factor_cascade0123 = nvsf_blendfactors;
	Output.pos_world = nvsf_pos_world;
	Output.pos_world_undisplaced = nvsf_pos_world_undisplaced;
	Output.world_displacement = nvsf_displacement;
	return Output; 
}

GFSDK_WAVEWORKS_VERTEX_OUTPUT GFSDK_WaveWorks_GetDisplacedVertexAfterTessellation(float4 In0, float4 In1, float4 In2, float3 BarycentricCoords)
{
	// Get starting position
	float3 nvsf_tessellated_ws_position =	In0.xyz * BarycentricCoords.x + 
											In1.xyz * BarycentricCoords.y + 
											In2.xyz * BarycentricCoords.z;
	float3 nvsf_pos_world_undisplaced = nvsf_tessellated_ws_position;
	

	// blend factors for cascades	
	float4 nvsf_blendfactors;
	float nvsf_distance = length(nvsf_g_WorldEye - nvsf_pos_world_undisplaced);
	float4 nvsf_cascade_spatial_size = 1.0/nvsf_g_UVScaleCascade0123.xyzw;
	nvsf_blendfactors.x = 1.0;
	nvsf_blendfactors.yzw = saturate(0.25*(nvsf_cascade_spatial_size.yzw*24.0-nvsf_distance)/nvsf_cascade_spatial_size.yzw);
	nvsf_blendfactors.yzw *= nvsf_blendfactors.yzw;

	// UVs
	float2 nvsf_uv_world_cascade0 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.x;
	float2 nvsf_uv_world_cascade1 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.y;
	float2 nvsf_uv_world_cascade2 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.z;
	float2 nvsf_uv_world_cascade3 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.w;

	// Displacement map
	#if defined(GFSDK_WAVEWORKS_GL)
		float3 nvsf_displacement;
		if(nvsf_g_UseTextureArrays > 0)
		{
			nvsf_displacement =  nvsf_blendfactors.x * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade0, 0.0), 0).xyz;
			nvsf_displacement += nvsf_blendfactors.y==0? float3(0,0,0) : nvsf_blendfactors.y * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade1, 1.0), 0).xyz;
			nvsf_displacement += nvsf_blendfactors.z==0? float3(0,0,0) : nvsf_blendfactors.z * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade2, 2.0), 0).xyz;
			nvsf_displacement += nvsf_blendfactors.w==0? float3(0,0,0) : nvsf_blendfactors.w * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade3, 3.0), 0).xyz;
		}
		else
		{
			nvsf_displacement =  nvsf_blendfactors.x * SampleTex2Dlod(nvsf_g_textureDisplacementMap0, nvsf_g_samplerDisplacementMap0, nvsf_uv_world_cascade0, 0).xyz;
			nvsf_displacement += nvsf_blendfactors.y==0? float3(0,0,0) : nvsf_blendfactors.y * SampleTex2Dlod(nvsf_g_textureDisplacementMap1, nvsf_g_samplerDisplacementMap1, nvsf_uv_world_cascade1, 0).xyz;
			nvsf_displacement += nvsf_blendfactors.z==0? float3(0,0,0) : nvsf_blendfactors.z * SampleTex2Dlod(nvsf_g_textureDisplacementMap2, nvsf_g_samplerDisplacementMap2, nvsf_uv_world_cascade2, 0).xyz;
			nvsf_displacement += nvsf_blendfactors.w==0? float3(0,0,0) : nvsf_blendfactors.w * SampleTex2Dlod(nvsf_g_textureDisplacementMap3, nvsf_g_samplerDisplacementMap3, nvsf_uv_world_cascade3, 0).xyz;
		}
	#else
		float3 nvsf_displacement =  nvsf_blendfactors.x * SampleTex2Dlod(nvsf_g_textureDisplacementMap0, nvsf_g_samplerDisplacementMap0, nvsf_uv_world_cascade0, 0).xyz;
			   nvsf_displacement += nvsf_blendfactors.y==0? float3(0,0,0) : nvsf_blendfactors.y * SampleTex2Dlod(nvsf_g_textureDisplacementMap1, nvsf_g_samplerDisplacementMap1, nvsf_uv_world_cascade1, 0).xyz;
			   nvsf_displacement += nvsf_blendfactors.z==0? float3(0,0,0) : nvsf_blendfactors.z * SampleTex2Dlod(nvsf_g_textureDisplacementMap2, nvsf_g_samplerDisplacementMap2, nvsf_uv_world_cascade2, 0).xyz;
			   nvsf_displacement += nvsf_blendfactors.w==0? float3(0,0,0) : nvsf_blendfactors.w * SampleTex2Dlod(nvsf_g_textureDisplacementMap3, nvsf_g_samplerDisplacementMap3, nvsf_uv_world_cascade3, 0).xyz;
	#endif

	float3 nvsf_pos_world = nvsf_pos_world_undisplaced + nvsf_displacement;
	
	// Output
	GFSDK_WAVEWORKS_VERTEX_OUTPUT Output;
	Output.interp.nvsf_eye_vec = nvsf_g_WorldEye - nvsf_pos_world;
	Output.interp.nvsf_tex_coord_cascade01.xy = nvsf_uv_world_cascade0;
	Output.interp.nvsf_tex_coord_cascade01.zw = nvsf_uv_world_cascade1;
	Output.interp.nvsf_tex_coord_cascade23.xy = nvsf_uv_world_cascade2;
	Output.interp.nvsf_tex_coord_cascade23.zw = nvsf_uv_world_cascade3;
	Output.interp.nvsf_blend_factor_cascade0123 = nvsf_blendfactors;
	Output.pos_world = nvsf_pos_world;
	Output.pos_world_undisplaced = nvsf_pos_world_undisplaced;
	Output.world_displacement = nvsf_displacement;
	return Output; 
}

GFSDK_WAVEWORKS_VERTEX_OUTPUT GFSDK_WaveWorks_GetDisplacedVertexAfterTessellationQuad(float4 In0, float4 In1, float4 In2, float4 In3, float2 UV)
{
	// Get starting position
	float3 nvsf_tessellated_ws_position =	In2.xyz*UV.x*UV.y + 
											In0.xyz*(1.0-UV.x)*UV.y + 
											In1.xyz*(1.0-UV.x)*(1.0-UV.y) + 
											In3.xyz*UV.x*(1.0-UV.y);
	float3 nvsf_pos_world_undisplaced = nvsf_tessellated_ws_position;
	
	// blend factors for cascades	
	float4 nvsf_blendfactors;
	float nvsf_distance = length(nvsf_g_WorldEye - nvsf_pos_world_undisplaced);
	float4 nvsf_cascade_spatial_size = 1.0/nvsf_g_UVScaleCascade0123.xyzw;
	nvsf_blendfactors.x = 1.0;
	nvsf_blendfactors.yzw = saturate(0.25*(nvsf_cascade_spatial_size.yzw*24.0-nvsf_distance)/nvsf_cascade_spatial_size.yzw);
	nvsf_blendfactors.yzw *= nvsf_blendfactors.yzw;

	// UVs
	float2 nvsf_uv_world_cascade0 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.x;
	float2 nvsf_uv_world_cascade1 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.y;
	float2 nvsf_uv_world_cascade2 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.z;
	float2 nvsf_uv_world_cascade3 = nvsf_pos_world_undisplaced.xy * nvsf_g_UVScaleCascade0123.w;

	// Displacement map
	#if defined(GFSDK_WAVEWORKS_GL)
		float3 nvsf_displacement;
		if(nvsf_g_UseTextureArrays > 0)
		{
			nvsf_displacement =  nvsf_blendfactors.x * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade0, 0.0), 0).xyz;
			nvsf_displacement += nvsf_blendfactors.y==0? float3(0,0,0) : nvsf_blendfactors.y * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade1, 1.0), 0).xyz;
			nvsf_displacement += nvsf_blendfactors.z==0? float3(0,0,0) : nvsf_blendfactors.z * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade2, 2.0), 0).xyz;
			nvsf_displacement += nvsf_blendfactors.w==0? float3(0,0,0) : nvsf_blendfactors.w * SampleTex2Dlod(nvsf_g_textureArrayDisplacementMap, nvsf_g_samplerDisplacementMapTextureArray, vec3(nvsf_uv_world_cascade3, 3.0), 0).xyz;
		}
		else
		{
			nvsf_displacement =  nvsf_blendfactors.x * SampleTex2Dlod(nvsf_g_textureDisplacementMap0, nvsf_g_samplerDisplacementMap0, nvsf_uv_world_cascade0, 0).xyz;
			nvsf_displacement += nvsf_blendfactors.y==0? float3(0,0,0) : nvsf_blendfactors.y * SampleTex2Dlod(nvsf_g_textureDisplacementMap1, nvsf_g_samplerDisplacementMap1, nvsf_uv_world_cascade1, 0).xyz;
			nvsf_displacement += nvsf_blendfactors.z==0? float3(0,0,0) : nvsf_blendfactors.z * SampleTex2Dlod(nvsf_g_textureDisplacementMap2, nvsf_g_samplerDisplacementMap2, nvsf_uv_world_cascade2, 0).xyz;
			nvsf_displacement += nvsf_blendfactors.w==0? float3(0,0,0) : nvsf_blendfactors.w * SampleTex2Dlod(nvsf_g_textureDisplacementMap3, nvsf_g_samplerDisplacementMap3, nvsf_uv_world_cascade3, 0).xyz;
		}
	#else
		float3 nvsf_displacement =  nvsf_blendfactors.x * SampleTex2Dlod(nvsf_g_textureDisplacementMap0, nvsf_g_samplerDisplacementMap0, nvsf_uv_world_cascade0, 0).xyz;
			   nvsf_displacement += nvsf_blendfactors.y==0? float3(0,0,0) : nvsf_blendfactors.y * SampleTex2Dlod(nvsf_g_textureDisplacementMap1, nvsf_g_samplerDisplacementMap1, nvsf_uv_world_cascade1, 0).xyz;
			   nvsf_displacement += nvsf_blendfactors.z==0? float3(0,0,0) : nvsf_blendfactors.z * SampleTex2Dlod(nvsf_g_textureDisplacementMap2, nvsf_g_samplerDisplacementMap2, nvsf_uv_world_cascade2, 0).xyz;
			   nvsf_displacement += nvsf_blendfactors.w==0? float3(0,0,0) : nvsf_blendfactors.w * SampleTex2Dlod(nvsf_g_textureDisplacementMap3, nvsf_g_samplerDisplacementMap3, nvsf_uv_world_cascade3, 0).xyz;
	#endif

	float3 nvsf_pos_world = nvsf_pos_world_undisplaced + nvsf_displacement;
	
	// Output
	GFSDK_WAVEWORKS_VERTEX_OUTPUT Output;
	Output.interp.nvsf_eye_vec = nvsf_g_WorldEye - nvsf_pos_world;
	Output.interp.nvsf_tex_coord_cascade01.xy = nvsf_uv_world_cascade0;
	Output.interp.nvsf_tex_coord_cascade01.zw = nvsf_uv_world_cascade1;
	Output.interp.nvsf_tex_coord_cascade23.xy = nvsf_uv_world_cascade2;
	Output.interp.nvsf_tex_coord_cascade23.zw = nvsf_uv_world_cascade3;
	Output.interp.nvsf_blend_factor_cascade0123 = nvsf_blendfactors;
	Output.pos_world = nvsf_pos_world;
	Output.pos_world_undisplaced = nvsf_pos_world_undisplaced;
	Output.world_displacement = nvsf_displacement;
	return Output; 
}

struct GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES
{
	float3 normal;
	float3 eye_dir;
	float foam_surface_folding;
	float foam_turbulent_energy;
	float foam_wave_hats;
};

GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES GFSDK_WaveWorks_GetSurfaceAttributes(GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT In)
{
	// Calculate eye vector.
	// Beware: 'nvsf_eye_vec' is a large number, 32bit floating point required.
	float3 nvsf_eye_dir = normalize(In.nvsf_eye_vec);

	// --------------- Water body color

	float4 nvsf_grad_fold0;
	float4 nvsf_grad_fold1;
	float4 nvsf_grad_fold2;
	float4 nvsf_grad_fold3;
	
	#if defined(GFSDK_WAVEWORKS_GL)
		float3 nvsf_displacement;
		if(nvsf_g_UseTextureArrays > 0)
		{
			nvsf_grad_fold0 = SampleTex2D(nvsf_g_textureArrayGradientMap, nvsf_g_samplerGradientMapTextureArray, vec3(In.nvsf_tex_coord_cascade01.xy, 0.0));
			nvsf_grad_fold1 = SampleTex2D(nvsf_g_textureArrayGradientMap, nvsf_g_samplerGradientMapTextureArray, vec3(In.nvsf_tex_coord_cascade01.zw, 1.0));
			nvsf_grad_fold2 = SampleTex2D(nvsf_g_textureArrayGradientMap, nvsf_g_samplerGradientMapTextureArray, vec3(In.nvsf_tex_coord_cascade23.xy, 2.0));
			nvsf_grad_fold3 = SampleTex2D(nvsf_g_textureArrayGradientMap, nvsf_g_samplerGradientMapTextureArray, vec3(In.nvsf_tex_coord_cascade23.zw, 3.0));
		}
		else
		{
			nvsf_grad_fold0 = SampleTex2D(nvsf_g_textureGradientMap0, nvsf_g_samplerGradientMap0, In.nvsf_tex_coord_cascade01.xy);
			nvsf_grad_fold1 = SampleTex2D(nvsf_g_textureGradientMap1, nvsf_g_samplerGradientMap1, In.nvsf_tex_coord_cascade01.zw);
			nvsf_grad_fold2 = SampleTex2D(nvsf_g_textureGradientMap2, nvsf_g_samplerGradientMap2, In.nvsf_tex_coord_cascade23.xy);
			nvsf_grad_fold3 = SampleTex2D(nvsf_g_textureGradientMap3, nvsf_g_samplerGradientMap3, In.nvsf_tex_coord_cascade23.zw);
		}
	#else
	
		nvsf_grad_fold0 = SampleTex2D(nvsf_g_textureGradientMap0, nvsf_g_samplerGradientMap0, In.nvsf_tex_coord_cascade01.xy);
		nvsf_grad_fold1 = SampleTex2D(nvsf_g_textureGradientMap1, nvsf_g_samplerGradientMap1, In.nvsf_tex_coord_cascade01.zw);
		nvsf_grad_fold2 = SampleTex2D(nvsf_g_textureGradientMap2, nvsf_g_samplerGradientMap2, In.nvsf_tex_coord_cascade23.xy);
		nvsf_grad_fold3 = SampleTex2D(nvsf_g_textureGradientMap3, nvsf_g_samplerGradientMap3, In.nvsf_tex_coord_cascade23.zw);
	#endif

	float2 nvsf_grad;
	nvsf_grad.xy = nvsf_grad_fold0.xy*In.nvsf_blend_factor_cascade0123.x + 
				   nvsf_grad_fold1.xy*In.nvsf_blend_factor_cascade0123.y*nvsf_g_Cascade1TexelScale_PS + 
				   nvsf_grad_fold2.xy*In.nvsf_blend_factor_cascade0123.z*nvsf_g_Cascade2TexelScale_PS + 
				   nvsf_grad_fold3.xy*In.nvsf_blend_factor_cascade0123.w*nvsf_g_Cascade3TexelScale_PS;

	float nvsf_c2c_scale = 0.25; // larger cascaded cover larger areas, so foamed texels cover larger area, thus, foam intensity on these needs to be scaled down for uniform foam look

	float nvsf_foam_turbulent_energy = 
					  // accumulated foam energy with blendfactors
					  100.0*nvsf_grad_fold0.w *  
					  lerp(nvsf_c2c_scale, nvsf_grad_fold1.w, In.nvsf_blend_factor_cascade0123.y)*
					  lerp(nvsf_c2c_scale, nvsf_grad_fold2.w, In.nvsf_blend_factor_cascade0123.z)*
					  lerp(nvsf_c2c_scale, nvsf_grad_fold3.w, In.nvsf_blend_factor_cascade0123.w);


	float nvsf_foam_surface_folding = 
						// folding for foam "clumping" on folded areas
    				   max(-100,
					  (1.0-nvsf_grad_fold0.z) +
					  (1.0-nvsf_grad_fold1.z) +
					  (1.0-nvsf_grad_fold2.z) +
					  (1.0-nvsf_grad_fold3.z));
	
	// Calculate normal here.
	float3 nvsf_normal = normalize(float3(nvsf_grad, nvsf_g_TexelLength_x2_PS));

	float nvsf_hats_c2c_scale = 0.5;		// the larger is the wave, the higher is the chance to start breaking at high folding, so folding for smaller cascade s is decreased
	float nvsf_foam_wave_hats =  
      				   10.0*(-0.55 + // this allows hats to appear on breaking places only. Can be tweaked to represent Beaufort scale better
					  (1.0-nvsf_grad_fold0.z) +
					  nvsf_hats_c2c_scale*(1.0-nvsf_grad_fold1.z) +
					  nvsf_hats_c2c_scale*nvsf_hats_c2c_scale*(1.0-nvsf_grad_fold2.z) +
					  nvsf_hats_c2c_scale*nvsf_hats_c2c_scale*nvsf_hats_c2c_scale*(1.0-nvsf_grad_fold3.z));


	// Output
	GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES Output;
	Output.normal = nvsf_normal;
	Output.eye_dir = nvsf_eye_dir;
	Output.foam_surface_folding = nvsf_foam_surface_folding;
	Output.foam_turbulent_energy = log(1.0 + nvsf_foam_turbulent_energy);
	Output.foam_wave_hats = nvsf_foam_wave_hats;
	return Output;
}


#endif /* _GFSDK_WAVEWORKS_ATTRIBUTES_FX */
