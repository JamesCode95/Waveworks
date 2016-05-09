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

#include "Common.fxh"

#ifdef GFSDK_WAVEWORKS_GL
#define DECLARE_ATTR_CONSTANT(Type,Label,Regoff) uniform Type Label
#define DECLARE_ATTR_SAMPLER(Label,TextureLabel,Regoff) \
	uniform sampler2D TextureLabel
#else
#define DECLARE_ATTR_CONSTANT(Type,Label,Regoff) Type Label : register(c##Regoff)
#define DECLARE_ATTR_SAMPLER(Label,TextureLabel,Regoff) \
	Texture2D Label : register(t##Regoff);	\
	SamplerState TextureLabel : register(s##Regoff)
#endif

//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

BEGIN_CBUFFER(nvsf_globals,0)
DECLARE_ATTR_CONSTANT(float4,nvsf_g_DissipationFactors,0); // x - the blur extents, y - the fadeout multiplier, z - the accumulation multiplier, w - foam generation threshold
DECLARE_ATTR_CONSTANT(float4,nvsf_g_SourceComponents  ,1); // xyzw - weights of energy map components to be sampled
DECLARE_ATTR_CONSTANT(float4,nvsf_g_UVOffsets         ,2); // xy - defines either horizontal offsets either vertical offsets
END_CBUFFER

DECLARE_ATTR_SAMPLER(nvsf_g_textureEnergyMap,nvsf_g_samplerEnergyMap,0);

#ifdef GFSDK_WAVEWORKS_GL
varying float2 nvsf_vInterpTexCoord;
#endif

#ifndef GFSDK_WAVEWORKS_OMIT_VS

#ifdef GFSDK_WAVEWORKS_GL
attribute float4 nvsf_vInPos;
attribute float2 nvsf_vInTexCoord;
#define nvsf_vOutPos gl_Position
void main()
#else
void vs(
	float4 nvsf_vInPos SEMANTIC(POSITION),
	float2 nvsf_vInTexCoord SEMANTIC(TEXCOORD0),
	out float2 nvsf_vInterpTexCoord SEMANTIC(TEXCOORD0),
	out float4 nvsf_vOutPos SEMANTIC(SV_Position)
)
#endif
{
    // No need to do matrix transform.
    nvsf_vOutPos = nvsf_vInPos;
    
	// Pass through general texture coordinate.
    nvsf_vInterpTexCoord = nvsf_vInTexCoord;
}

#endif // !GFSDK_WAVEWORKS_OMIT_VS

// at 1st rendering step, the folding and the accumulated foam values are being read from gradient map (components z and w),
// blurred by X, summed, faded and written to foam energy map 

// at 2nd rendering step, the accumulated foam values are being read from foam energy texture,
// blurred by Y and written to w component of gradient map

#ifndef GFSDK_WAVEWORKS_OMIT_PS

#ifdef GFSDK_WAVEWORKS_GL
#define nvsf_Output gl_FragColor
void main()
#else
void ps(
	float2 nvsf_vInterpTexCoord SEMANTIC(TEXCOORD0),
	out float4 nvsf_Output SEMANTIC(SV_Target)
)
#endif
{

	float2 nvsf_UVoffset = nvsf_g_UVOffsets.xy*nvsf_g_DissipationFactors.x;

	// blur with variable size kernel is done by doing 4 bilinear samples, 
	// each sample is slightly offset from the center point
	float nvsf_foamenergy1	= dot(nvsf_g_SourceComponents, SampleTex2D(nvsf_g_textureEnergyMap, nvsf_g_samplerEnergyMap, nvsf_vInterpTexCoord.xy + nvsf_UVoffset));
	float nvsf_foamenergy2	= dot(nvsf_g_SourceComponents, SampleTex2D(nvsf_g_textureEnergyMap, nvsf_g_samplerEnergyMap, nvsf_vInterpTexCoord.xy - nvsf_UVoffset));
	float nvsf_foamenergy3	= dot(nvsf_g_SourceComponents, SampleTex2D(nvsf_g_textureEnergyMap, nvsf_g_samplerEnergyMap, nvsf_vInterpTexCoord.xy + nvsf_UVoffset*2.0));
	float nvsf_foamenergy4	= dot(nvsf_g_SourceComponents, SampleTex2D(nvsf_g_textureEnergyMap, nvsf_g_samplerEnergyMap, nvsf_vInterpTexCoord.xy - nvsf_UVoffset*2.0));
	
	float nvsf_folding = max(0,SampleTex2D(nvsf_g_textureEnergyMap, nvsf_g_samplerEnergyMap, nvsf_vInterpTexCoord.xy).z);

	float nvsf_energy = nvsf_g_DissipationFactors.y*((nvsf_foamenergy1 + nvsf_foamenergy2 + nvsf_foamenergy3 + nvsf_foamenergy4)*0.25 + max(0,(1.0-nvsf_folding-nvsf_g_DissipationFactors.w))*nvsf_g_DissipationFactors.z);

	nvsf_energy = min(1.0,nvsf_energy);

	// Output
	nvsf_Output = float4(nvsf_energy,nvsf_energy,nvsf_energy,nvsf_energy);
}

#endif // !GFSDK_WAVEWORKS_OMIT_PS