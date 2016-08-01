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

#include "GFSDK_WaveWorks_Common.fxh"

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

BEGIN_CBUFFER(globals,0)
DECLARE_ATTR_CONSTANT(float4,g_DissipationFactors,0); // x - the blur extents, y - the fadeout multiplier, z - the accumulation multiplier, w - foam generation threshold
DECLARE_ATTR_CONSTANT(float4,g_SourceComponents  ,1); // xyzw - weights of energy map components to be sampled
DECLARE_ATTR_CONSTANT(float4,g_UVOffsets         ,2); // xy - defines either horizontal offsets either vertical offsets
END_CBUFFER

DECLARE_ATTR_SAMPLER(g_textureEnergyMap,g_samplerEnergyMap,0);

#ifdef GFSDK_WAVEWORKS_GL
varying float2 vInterpTexCoord;
#endif

#ifndef GFSDK_WAVEWORKS_OMIT_VS

#ifdef GFSDK_WAVEWORKS_GL
attribute float4 vInPos;
attribute float2 vInTexCoord;
#define vOutPos gl_Position
void main()
#else
void vs(
	float4 vInPos SEMANTIC(POSITION),
	float2 vInTexCoord SEMANTIC(TEXCOORD0),
	out float2 vInterpTexCoord SEMANTIC(TEXCOORD0),
	out float4 vOutPos SEMANTIC(SV_Position)
)
#endif
{
    // No need to do matrix transform.
    vOutPos = vInPos;
    
	// Pass through general texture coordinate.
    vInterpTexCoord = vInTexCoord;
}

#endif // !GFSDK_WAVEWORKS_OMIT_VS

// at 1st rendering step, the folding and the accumulated foam values are being read from gradient map (components z and w),
// blurred by X, summed, faded and written to foam energy map 

// at 2nd rendering step, the accumulated foam values are being read from foam energy texture,
// blurred by Y and written to w component of gradient map

#ifndef GFSDK_WAVEWORKS_OMIT_PS

#ifdef GFSDK_WAVEWORKS_GL
#define Output gl_FragColor
void main()
#else
void ps(
	float2 vInterpTexCoord SEMANTIC(TEXCOORD0),
	out float4 Output SEMANTIC(SV_Target)
)
#endif
{

	float2 UVoffset = g_UVOffsets.xy*g_DissipationFactors.x;

	// blur with variable size kernel is done by doing 4 bilinear samples, 
	// each sample is slightly offset from the center point
	float foamenergy1	= dot(g_SourceComponents, SampleTex2D(g_textureEnergyMap, g_samplerEnergyMap, vInterpTexCoord.xy + UVoffset));
	float foamenergy2	= dot(g_SourceComponents, SampleTex2D(g_textureEnergyMap, g_samplerEnergyMap, vInterpTexCoord.xy - UVoffset));
	float foamenergy3	= dot(g_SourceComponents, SampleTex2D(g_textureEnergyMap, g_samplerEnergyMap, vInterpTexCoord.xy + UVoffset*2.0));
	float foamenergy4	= dot(g_SourceComponents, SampleTex2D(g_textureEnergyMap, g_samplerEnergyMap, vInterpTexCoord.xy - UVoffset*2.0));
	
	float folding = max(0,SampleTex2D(g_textureEnergyMap, g_samplerEnergyMap, vInterpTexCoord.xy).z);

	float energy = g_DissipationFactors.y*((foamenergy1 + foamenergy2 + foamenergy3 + foamenergy4)*0.25 + max(0,(1.0-folding-g_DissipationFactors.w))*g_DissipationFactors.z);

	energy = min(1.0,energy);

	// Output
	Output = float4(energy,energy,energy,energy);
}

#endif // !GFSDK_WAVEWORKS_OMIT_PS