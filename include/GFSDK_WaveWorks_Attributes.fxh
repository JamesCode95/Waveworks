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
	
GFSDK_WAVEWORKS_BEGIN_ATTR_DISPLACEMENT_CBUFFER(attr_vs_buffer)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float3, g_WorldEye, 0)
#if defined( GFSDK_WAVEWORKS_GL )
	GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float, g_UseTextureArrays, 1)
#else
	GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float, g_Pad1, 1)
#endif
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_CONSTANT(float4, g_UVScaleCascade0123, 2)
GFSDK_WAVEWORKS_END_ATTR_DISPLACEMENT_CBUFFER

GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(g_samplerDisplacementMap0, g_textureDisplacementMap0, 0)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(g_samplerDisplacementMap1, g_textureDisplacementMap1, 1)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(g_samplerDisplacementMap2, g_textureDisplacementMap2, 2)
GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER(g_samplerDisplacementMap3, g_textureDisplacementMap3, 3)

#if defined( GFSDK_WAVEWORKS_GL )
	GFSDK_WAVEWORKS_DECLARE_ATTR_DISPLACEMENT_SAMPLER_TEXTUREARRAY(g_samplerDisplacementMapTextureArray, g_textureArrayDisplacementMap, 4)
#endif

GFSDK_WAVEWORKS_BEGIN_ATTR_PS_CBUFFER(attr_ps_buffer)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, g_TexelLength_x2_PS, 0)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, g_Cascade1Scale_PS, 1)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, g_Cascade1TexelScale_PS, 2)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, g_Cascade1UVOffset_PS, 3)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, g_Cascade2Scale_PS, 4)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, g_Cascade2TexelScale_PS, 5)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, g_Cascade2UVOffset_PS, 6)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, g_Cascade3Scale_PS, 7)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, g_Cascade3TexelScale_PS, 8)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(float, g_Cascade3UVOffset_PS, 9)
GFSDK_WAVEWORKS_END_ATTR_PS_CBUFFER

GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(g_samplerGradientMap0, g_textureGradientMap0, 0)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(g_samplerGradientMap1, g_textureGradientMap1, 1)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(g_samplerGradientMap2, g_textureGradientMap2, 2)
GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(g_samplerGradientMap3, g_textureGradientMap3, 3)

#if defined( GFSDK_WAVEWORKS_GL )
	GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER_TEXTUREARRAY(g_samplerGradientMapTextureArray, g_textureArrayGradientMap, 4)
#endif

struct GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT
{
    float4 tex_coord_cascade01			SEMANTIC(TEXCOORD0);
    float4 tex_coord_cascade23			SEMANTIC(TEXCOORD1);
    float4 blend_factor_cascade0123	SEMANTIC(TEXCOORD2);
    float3 eye_vec						SEMANTIC(TEXCOORD3);
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
	float3 pos_world_undisplaced = GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition(In);
	float  distance = length(g_WorldEye - pos_world_undisplaced);
	
	// UVs
	float2 uv_world_cascade0 = pos_world_undisplaced.xy * g_UVScaleCascade0123.x;
	float2 uv_world_cascade1 = pos_world_undisplaced.xy * g_UVScaleCascade0123.y;
	float2 uv_world_cascade2 = pos_world_undisplaced.xy * g_UVScaleCascade0123.z;
	float2 uv_world_cascade3 = pos_world_undisplaced.xy * g_UVScaleCascade0123.w;
	
	// cascade blend factors
	float4 blendfactors;
	float4 cascade_spatial_size = 1.0/g_UVScaleCascade0123.xyzw;
	blendfactors.x = 1.0;
	blendfactors.yzw = saturate(0.25*(cascade_spatial_size.yzw*24.0-distance)/cascade_spatial_size.yzw);
	blendfactors.yzw *= blendfactors.yzw;


	// Displacement map
	#if defined(GFSDK_WAVEWORKS_GL)
		float3 displacement;
		if(g_UseTextureArrays > 0)
		{
			displacement =  blendfactors.x * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade0, 0.0), 0).xyz;
			displacement += blendfactors.y==0? float3(0,0,0) : blendfactors.y * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade1, 1.0), 0).xyz;
			displacement += blendfactors.z==0? float3(0,0,0) : blendfactors.z * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade2, 2.0), 0).xyz;
			displacement += blendfactors.w==0? float3(0,0,0) : blendfactors.w * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade3, 3.0), 0).xyz;
		}
		else
		{
			displacement =  blendfactors.x * SampleTex2Dlod(g_textureDisplacementMap0, g_samplerDisplacementMap0, uv_world_cascade0, 0).xyz;
			displacement += blendfactors.y==0? float3(0,0,0) : blendfactors.y * SampleTex2Dlod(g_textureDisplacementMap1, g_samplerDisplacementMap1, uv_world_cascade1, 0).xyz;
			displacement += blendfactors.z==0? float3(0,0,0) : blendfactors.z * SampleTex2Dlod(g_textureDisplacementMap2, g_samplerDisplacementMap2, uv_world_cascade2, 0).xyz;
			displacement += blendfactors.w==0? float3(0,0,0) : blendfactors.w * SampleTex2Dlod(g_textureDisplacementMap3, g_samplerDisplacementMap3, uv_world_cascade3, 0).xyz;
		}
	#else
		float3 displacement =  blendfactors.x * SampleTex2Dlod(g_textureDisplacementMap0, g_samplerDisplacementMap0, uv_world_cascade0, 0).xyz;
			   displacement += blendfactors.y==0? float3(0,0,0) : blendfactors.y * SampleTex2Dlod(g_textureDisplacementMap1, g_samplerDisplacementMap1, uv_world_cascade1, 0).xyz;
			   displacement += blendfactors.z==0? float3(0,0,0) : blendfactors.z * SampleTex2Dlod(g_textureDisplacementMap2, g_samplerDisplacementMap2, uv_world_cascade2, 0).xyz;
			   displacement += blendfactors.w==0? float3(0,0,0) : blendfactors.w * SampleTex2Dlod(g_textureDisplacementMap3, g_samplerDisplacementMap3, uv_world_cascade3, 0).xyz;
	#endif

	float3 pos_world = pos_world_undisplaced + displacement;

	// Output
	GFSDK_WAVEWORKS_VERTEX_OUTPUT Output;
	Output.interp.eye_vec = g_WorldEye - pos_world;
	Output.interp.tex_coord_cascade01.xy = uv_world_cascade0;
	Output.interp.tex_coord_cascade01.zw = uv_world_cascade1;
	Output.interp.tex_coord_cascade23.xy = uv_world_cascade2;
	Output.interp.tex_coord_cascade23.zw = uv_world_cascade3;
	Output.interp.blend_factor_cascade0123 = blendfactors;
	Output.pos_world = pos_world;
	Output.pos_world_undisplaced = pos_world_undisplaced;
	Output.world_displacement = displacement;
	return Output; 
}

GFSDK_WAVEWORKS_VERTEX_OUTPUT GFSDK_WaveWorks_GetDisplacedVertexAfterTessellation(float4 In0, float4 In1, float4 In2, float3 BarycentricCoords)
{
	// Get starting position
	float3 tessellated_ws_position =	In0.xyz * BarycentricCoords.x + 
											In1.xyz * BarycentricCoords.y + 
											In2.xyz * BarycentricCoords.z;
	float3 pos_world_undisplaced = tessellated_ws_position;
	

	// blend factors for cascades	
	float4 blendfactors;
	float distance = length(g_WorldEye - pos_world_undisplaced);
	float4 cascade_spatial_size = 1.0/g_UVScaleCascade0123.xyzw;
	blendfactors.x = 1.0;
	blendfactors.yzw = saturate(0.25*(cascade_spatial_size.yzw*24.0-distance)/cascade_spatial_size.yzw);
	blendfactors.yzw *= blendfactors.yzw;

	// UVs
	float2 uv_world_cascade0 = pos_world_undisplaced.xy * g_UVScaleCascade0123.x;
	float2 uv_world_cascade1 = pos_world_undisplaced.xy * g_UVScaleCascade0123.y;
	float2 uv_world_cascade2 = pos_world_undisplaced.xy * g_UVScaleCascade0123.z;
	float2 uv_world_cascade3 = pos_world_undisplaced.xy * g_UVScaleCascade0123.w;

	// Displacement map
	#if defined(GFSDK_WAVEWORKS_GL)
		float3 displacement;
		if(g_UseTextureArrays > 0)
		{
			displacement =  blendfactors.x * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade0, 0.0), 0).xyz;
			displacement += blendfactors.y==0? float3(0,0,0) : blendfactors.y * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade1, 1.0), 0).xyz;
			displacement += blendfactors.z==0? float3(0,0,0) : blendfactors.z * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade2, 2.0), 0).xyz;
			displacement += blendfactors.w==0? float3(0,0,0) : blendfactors.w * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade3, 3.0), 0).xyz;
		}
		else
		{
			displacement =  blendfactors.x * SampleTex2Dlod(g_textureDisplacementMap0, g_samplerDisplacementMap0, uv_world_cascade0, 0).xyz;
			displacement += blendfactors.y==0? float3(0,0,0) : blendfactors.y * SampleTex2Dlod(g_textureDisplacementMap1, g_samplerDisplacementMap1, uv_world_cascade1, 0).xyz;
			displacement += blendfactors.z==0? float3(0,0,0) : blendfactors.z * SampleTex2Dlod(g_textureDisplacementMap2, g_samplerDisplacementMap2, uv_world_cascade2, 0).xyz;
			displacement += blendfactors.w==0? float3(0,0,0) : blendfactors.w * SampleTex2Dlod(g_textureDisplacementMap3, g_samplerDisplacementMap3, uv_world_cascade3, 0).xyz;
		}
	#else
		float3 displacement =  blendfactors.x * SampleTex2Dlod(g_textureDisplacementMap0, g_samplerDisplacementMap0, uv_world_cascade0, 0).xyz;
			   displacement += blendfactors.y==0? float3(0,0,0) : blendfactors.y * SampleTex2Dlod(g_textureDisplacementMap1, g_samplerDisplacementMap1, uv_world_cascade1, 0).xyz;
			   displacement += blendfactors.z==0? float3(0,0,0) : blendfactors.z * SampleTex2Dlod(g_textureDisplacementMap2, g_samplerDisplacementMap2, uv_world_cascade2, 0).xyz;
			   displacement += blendfactors.w==0? float3(0,0,0) : blendfactors.w * SampleTex2Dlod(g_textureDisplacementMap3, g_samplerDisplacementMap3, uv_world_cascade3, 0).xyz;
	#endif

	float3 pos_world = pos_world_undisplaced + displacement;
	
	// Output
	GFSDK_WAVEWORKS_VERTEX_OUTPUT Output;
	Output.interp.eye_vec = g_WorldEye - pos_world;
	Output.interp.tex_coord_cascade01.xy = uv_world_cascade0;
	Output.interp.tex_coord_cascade01.zw = uv_world_cascade1;
	Output.interp.tex_coord_cascade23.xy = uv_world_cascade2;
	Output.interp.tex_coord_cascade23.zw = uv_world_cascade3;
	Output.interp.blend_factor_cascade0123 = blendfactors;
	Output.pos_world = pos_world;
	Output.pos_world_undisplaced = pos_world_undisplaced;
	Output.world_displacement = displacement;
	return Output; 
}

GFSDK_WAVEWORKS_VERTEX_OUTPUT GFSDK_WaveWorks_GetDisplacedVertexAfterTessellationQuad(float4 In0, float4 In1, float4 In2, float4 In3, float2 UV)
{
	// Get starting position
	float3 tessellated_ws_position =	In2.xyz*UV.x*UV.y + 
											In0.xyz*(1.0-UV.x)*UV.y + 
											In1.xyz*(1.0-UV.x)*(1.0-UV.y) + 
											In3.xyz*UV.x*(1.0-UV.y);
	float3 pos_world_undisplaced = tessellated_ws_position;
	
	// blend factors for cascades	
	float4 blendfactors;
	float distance = length(g_WorldEye - pos_world_undisplaced);
	float4 cascade_spatial_size = 1.0/g_UVScaleCascade0123.xyzw;
	blendfactors.x = 1.0;
	blendfactors.yzw = saturate(0.25*(cascade_spatial_size.yzw*24.0-distance)/cascade_spatial_size.yzw);
	blendfactors.yzw *= blendfactors.yzw;

	// UVs
	float2 uv_world_cascade0 = pos_world_undisplaced.xy * g_UVScaleCascade0123.x;
	float2 uv_world_cascade1 = pos_world_undisplaced.xy * g_UVScaleCascade0123.y;
	float2 uv_world_cascade2 = pos_world_undisplaced.xy * g_UVScaleCascade0123.z;
	float2 uv_world_cascade3 = pos_world_undisplaced.xy * g_UVScaleCascade0123.w;

	// Displacement map
	#if defined(GFSDK_WAVEWORKS_GL)
		float3 displacement;
		if(g_UseTextureArrays > 0)
		{
			displacement =  blendfactors.x * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade0, 0.0), 0).xyz;
			displacement += blendfactors.y==0? float3(0,0,0) : blendfactors.y * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade1, 1.0), 0).xyz;
			displacement += blendfactors.z==0? float3(0,0,0) : blendfactors.z * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade2, 2.0), 0).xyz;
			displacement += blendfactors.w==0? float3(0,0,0) : blendfactors.w * SampleTex2Dlod(g_textureArrayDisplacementMap, g_samplerDisplacementMapTextureArray, vec3(uv_world_cascade3, 3.0), 0).xyz;
		}
		else
		{
			displacement =  blendfactors.x * SampleTex2Dlod(g_textureDisplacementMap0, g_samplerDisplacementMap0, uv_world_cascade0, 0).xyz;
			displacement += blendfactors.y==0? float3(0,0,0) : blendfactors.y * SampleTex2Dlod(g_textureDisplacementMap1, g_samplerDisplacementMap1, uv_world_cascade1, 0).xyz;
			displacement += blendfactors.z==0? float3(0,0,0) : blendfactors.z * SampleTex2Dlod(g_textureDisplacementMap2, g_samplerDisplacementMap2, uv_world_cascade2, 0).xyz;
			displacement += blendfactors.w==0? float3(0,0,0) : blendfactors.w * SampleTex2Dlod(g_textureDisplacementMap3, g_samplerDisplacementMap3, uv_world_cascade3, 0).xyz;
		}
	#else
		float3 displacement =  blendfactors.x * SampleTex2Dlod(g_textureDisplacementMap0, g_samplerDisplacementMap0, uv_world_cascade0, 0).xyz;
			   displacement += blendfactors.y==0? float3(0,0,0) : blendfactors.y * SampleTex2Dlod(g_textureDisplacementMap1, g_samplerDisplacementMap1, uv_world_cascade1, 0).xyz;
			   displacement += blendfactors.z==0? float3(0,0,0) : blendfactors.z * SampleTex2Dlod(g_textureDisplacementMap2, g_samplerDisplacementMap2, uv_world_cascade2, 0).xyz;
			   displacement += blendfactors.w==0? float3(0,0,0) : blendfactors.w * SampleTex2Dlod(g_textureDisplacementMap3, g_samplerDisplacementMap3, uv_world_cascade3, 0).xyz;
	#endif

	float3 pos_world = pos_world_undisplaced + displacement;
	
	// Output
	GFSDK_WAVEWORKS_VERTEX_OUTPUT Output;
	Output.interp.eye_vec = g_WorldEye - pos_world;
	Output.interp.tex_coord_cascade01.xy = uv_world_cascade0;
	Output.interp.tex_coord_cascade01.zw = uv_world_cascade1;
	Output.interp.tex_coord_cascade23.xy = uv_world_cascade2;
	Output.interp.tex_coord_cascade23.zw = uv_world_cascade3;
	Output.interp.blend_factor_cascade0123 = blendfactors;
	Output.pos_world = pos_world;
	Output.pos_world_undisplaced = pos_world_undisplaced;
	Output.world_displacement = displacement;
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
	// Beware: 'eye_vec' is a large number, 32bit floating point required.
	float3 eye_dir = normalize(In.eye_vec);

	// --------------- Water body color

	float4 grad_fold0;
	float4 grad_fold1;
	float4 grad_fold2;
	float4 grad_fold3;
	
	#if defined(GFSDK_WAVEWORKS_GL)
		float3 displacement;
		if(g_UseTextureArrays > 0)
		{
			grad_fold0 = SampleTex2D(g_textureArrayGradientMap, g_samplerGradientMapTextureArray, vec3(In.tex_coord_cascade01.xy, 0.0));
			grad_fold1 = SampleTex2D(g_textureArrayGradientMap, g_samplerGradientMapTextureArray, vec3(In.tex_coord_cascade01.zw, 1.0));
			grad_fold2 = SampleTex2D(g_textureArrayGradientMap, g_samplerGradientMapTextureArray, vec3(In.tex_coord_cascade23.xy, 2.0));
			grad_fold3 = SampleTex2D(g_textureArrayGradientMap, g_samplerGradientMapTextureArray, vec3(In.tex_coord_cascade23.zw, 3.0));
		}
		else
		{
			grad_fold0 = SampleTex2D(g_textureGradientMap0, g_samplerGradientMap0, In.tex_coord_cascade01.xy);
			grad_fold1 = SampleTex2D(g_textureGradientMap1, g_samplerGradientMap1, In.tex_coord_cascade01.zw);
			grad_fold2 = SampleTex2D(g_textureGradientMap2, g_samplerGradientMap2, In.tex_coord_cascade23.xy);
			grad_fold3 = SampleTex2D(g_textureGradientMap3, g_samplerGradientMap3, In.tex_coord_cascade23.zw);
		}
	#else
	
		grad_fold0 = SampleTex2D(g_textureGradientMap0, g_samplerGradientMap0, In.tex_coord_cascade01.xy);
		grad_fold1 = SampleTex2D(g_textureGradientMap1, g_samplerGradientMap1, In.tex_coord_cascade01.zw);
		grad_fold2 = SampleTex2D(g_textureGradientMap2, g_samplerGradientMap2, In.tex_coord_cascade23.xy);
		grad_fold3 = SampleTex2D(g_textureGradientMap3, g_samplerGradientMap3, In.tex_coord_cascade23.zw);
	#endif

	float2 grad;
	grad.xy = grad_fold0.xy*In.blend_factor_cascade0123.x + 
				   grad_fold1.xy*In.blend_factor_cascade0123.y*g_Cascade1TexelScale_PS + 
				   grad_fold2.xy*In.blend_factor_cascade0123.z*g_Cascade2TexelScale_PS + 
				   grad_fold3.xy*In.blend_factor_cascade0123.w*g_Cascade3TexelScale_PS;

	float c2c_scale = 0.25; // larger cascaded cover larger areas, so foamed texels cover larger area, thus, foam intensity on these needs to be scaled down for uniform foam look

	float foam_turbulent_energy = 
					  // accumulated foam energy with blendfactors
					  100.0*grad_fold0.w *  
					  lerp(c2c_scale, grad_fold1.w, In.blend_factor_cascade0123.y)*
					  lerp(c2c_scale, grad_fold2.w, In.blend_factor_cascade0123.z)*
					  lerp(c2c_scale, grad_fold3.w, In.blend_factor_cascade0123.w);


	float foam_surface_folding = 
						// folding for foam "clumping" on folded areas
    				   max(-100,
					  (1.0-grad_fold0.z) +
					  (1.0-grad_fold1.z) +
					  (1.0-grad_fold2.z) +
					  (1.0-grad_fold3.z));
	
	// Calculate normal here.
	float3 normal = normalize(float3(grad, g_TexelLength_x2_PS));

	float hats_c2c_scale = 0.5;		// the larger is the wave, the higher is the chance to start breaking at high folding, so folding for smaller cascade s is decreased
	float foam_wave_hats =  
      				   10.0*(-0.55 + // this allows hats to appear on breaking places only. Can be tweaked to represent Beaufort scale better
					  (1.0-grad_fold0.z) +
					  hats_c2c_scale*(1.0-grad_fold1.z) +
					  hats_c2c_scale*hats_c2c_scale*(1.0-grad_fold2.z) +
					  hats_c2c_scale*hats_c2c_scale*hats_c2c_scale*(1.0-grad_fold3.z));


	// Output
	GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES Output;
	Output.normal = normal;
	Output.eye_dir = eye_dir;
	Output.foam_surface_folding = foam_surface_folding;
	Output.foam_turbulent_energy = log(1.0 + foam_turbulent_energy);
	Output.foam_wave_hats = foam_wave_hats;
	return Output;
}


#endif /* _GFSDK_WAVEWORKS_ATTRIBUTES_FX */
