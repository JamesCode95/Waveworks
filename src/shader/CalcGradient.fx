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

BEGIN_CBUFFER(nvsf_globals,0)
DECLARE_ATTR_CONSTANT(float4,nvsf_g_Scales,        0); // was: float nvsf_g_ChoppyScale, nvsf_g_GradMap2TexelWSScale
DECLARE_ATTR_CONSTANT(float4,nvsf_g_OneTexel_Left, 1);
DECLARE_ATTR_CONSTANT(float4,nvsf_g_OneTexel_Right,2);
DECLARE_ATTR_CONSTANT(float4,nvsf_g_OneTexel_Back, 3);
DECLARE_ATTR_CONSTANT(float4,nvsf_g_OneTexel_Front,4);
END_CBUFFER

DECLARE_ATTR_SAMPLER(nvsf_g_textureDisplacementMap,nvsf_g_samplerDisplacementMap,0);

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
	// Sample neighbour texels
	float3 nvsf_displace_left	= SampleTex2D(nvsf_g_textureDisplacementMap, nvsf_g_samplerDisplacementMap, nvsf_vInterpTexCoord.xy + nvsf_g_OneTexel_Left.xy).rgb;
	float3 nvsf_displace_right	= SampleTex2D(nvsf_g_textureDisplacementMap, nvsf_g_samplerDisplacementMap, nvsf_vInterpTexCoord.xy + nvsf_g_OneTexel_Right.xy).rgb;
	float3 nvsf_displace_back	= SampleTex2D(nvsf_g_textureDisplacementMap, nvsf_g_samplerDisplacementMap, nvsf_vInterpTexCoord.xy + nvsf_g_OneTexel_Back.xy).rgb;
	float3 nvsf_displace_front	= SampleTex2D(nvsf_g_textureDisplacementMap, nvsf_g_samplerDisplacementMap, nvsf_vInterpTexCoord.xy + nvsf_g_OneTexel_Front.xy).rgb;
	
	// -------- Do not store the actual normal value, instead, it preserves two differential values.
	float2 nvsf_gradient = float2(-(nvsf_displace_right.z - nvsf_displace_left.z) / max(0.01,1.0 + nvsf_g_Scales.y*(nvsf_displace_right.x - nvsf_displace_left.x)), -(nvsf_displace_front.z - nvsf_displace_back.z) / max(0.01,1.0+nvsf_g_Scales.y*(nvsf_displace_front.y - nvsf_displace_back.y)));
	//float2 nvsf_gradient = {-(nvsf_displace_right.z - nvsf_displace_left.z), -(nvsf_displace_front.z - nvsf_displace_back.z) };
	
	// Calculate Jacobian corelation from the partial differential of displacement field
	float2 nvsf_Dx = (nvsf_displace_right.xy - nvsf_displace_left.xy) * nvsf_g_Scales.x;
	float2 nvsf_Dy = (nvsf_displace_front.xy - nvsf_displace_back.xy) * nvsf_g_Scales.x;
	float nvsf_J = (1.0f + nvsf_Dx.x) * (1.0f + nvsf_Dy.y) - nvsf_Dx.y * nvsf_Dy.x;

	// Output
	nvsf_Output = float4(nvsf_gradient, nvsf_J, 0);
}

#endif // !GFSDK_WAVEWORKS_OMIT_PS
