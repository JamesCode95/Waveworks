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

//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------
float4x4 g_matViewToPSM;
float  g_PSMSlices;
float3 g_PSMTint;
Texture3D g_PSMMap;

//------------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
SamplerState g_SamplerPSM
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
	AddressW = Clamp;
};

//--------------------------------------------------------------------------------------
// DepthStates
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// RasterStates
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// BlendStates
//--------------------------------------------------------------------------------------
BlendState PSMBlend
{
    BlendEnable[0] = TRUE;
    RenderTargetWriteMask[0] = 0xF;
    
    SrcBlend = Zero;
    DestBlend = Inv_Src_Color;
    BlendOp = Add;

    SrcBlendAlpha = Zero;
    DestBlendAlpha = Inv_Src_Alpha;
    BlendOpAlpha = Add;
};

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------------
float3 CalcPSMShadowFactor(float3 PSMCoords)
{
    int PSMMapW, PSMMapH, PSMMapD;
    g_PSMMap.GetDimensions(PSMMapW, PSMMapH, PSMMapD);

	float NumSlices = g_PSMSlices;
	float slice = NumSlices * saturate(PSMCoords.z) + 1.f; // +1.f because zero slice reserved for coverage
	slice = min(slice, NumSlices + 0.5f);
	float slice_upper = ceil(slice);
	float slice_lower = slice_upper - 1;
	
	float3 lower_uvz = float3(PSMCoords.xy,(0.5f+slice_lower)/float(PSMMapD));
	float3 upper_uvz = float3(PSMCoords.xy,(0.5f+slice_upper)/float(PSMMapD));
	
	float4 raw_lower_vals = slice_lower < 1.f ? 1.f : g_PSMMap.SampleLevel(g_SamplerPSM, lower_uvz, 0);
	float raw_upper_val = slice_upper < 1.f ? 1.f : g_PSMMap.SampleLevel(g_SamplerPSM, upper_uvz, 0).r;
	float lower_val;
	float upper_val;
	float slice_lerp = 2.f * frac(slice);
	if(slice_lerp >= 1.f) {
		lower_val = raw_lower_vals.g;
		upper_val = raw_upper_val;
	} else if(slice_lerp < 1.f) {
		lower_val = raw_lower_vals.r;
		upper_val = raw_lower_vals.g;
	}

	float shadow_factor = lerp(lower_val,upper_val,frac(slice_lerp));

	return pow(shadow_factor,g_PSMTint);
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
