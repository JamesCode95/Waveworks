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
DECLARE_ATTR_CONSTANT(float4,g_Scales,        0); // was: float g_ChoppyScale, g_GradMap2TexelWSScale
DECLARE_ATTR_CONSTANT(float4,g_OneTexel_Left, 1);
DECLARE_ATTR_CONSTANT(float4,g_OneTexel_Right,2);
DECLARE_ATTR_CONSTANT(float4,g_OneTexel_Back, 3);
DECLARE_ATTR_CONSTANT(float4,g_OneTexel_Front,4);
END_CBUFFER

DECLARE_ATTR_SAMPLER(g_textureDisplacementMap,g_samplerDisplacementMap,0);

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
	// Sample neighbour texels
	float3 displace_left	= SampleTex2D(g_textureDisplacementMap, g_samplerDisplacementMap, vInterpTexCoord.xy + g_OneTexel_Left.xy).rgb;
	float3 displace_right	= SampleTex2D(g_textureDisplacementMap, g_samplerDisplacementMap, vInterpTexCoord.xy + g_OneTexel_Right.xy).rgb;
	float3 displace_back	= SampleTex2D(g_textureDisplacementMap, g_samplerDisplacementMap, vInterpTexCoord.xy + g_OneTexel_Back.xy).rgb;
	float3 displace_front	= SampleTex2D(g_textureDisplacementMap, g_samplerDisplacementMap, vInterpTexCoord.xy + g_OneTexel_Front.xy).rgb;
	
	// -------- Do not store the actual normal value, instead, it preserves two differential values.
	float2 gradient = float2(-(displace_right.z - displace_left.z) / max(0.01,1.0 + g_Scales.y*(displace_right.x - displace_left.x)), -(displace_front.z - displace_back.z) / max(0.01,1.0+g_Scales.y*(displace_front.y - displace_back.y)));
	//float2 gradient = {-(displace_right.z - displace_left.z), -(displace_front.z - displace_back.z) };
	
	// Calculate Jacobian corelation from the partial differential of displacement field
	float2 Dx = (displace_right.xy - displace_left.xy) * g_Scales.x;
	float2 Dy = (displace_front.xy - displace_back.xy) * g_Scales.x;
	float J = (1.0f + Dx.x) * (1.0f + Dy.y) - Dx.y * Dy.x;

	// Output
	Output = float4(gradient, J, 0);
}

#endif // !GFSDK_WAVEWORKS_OMIT_PS
