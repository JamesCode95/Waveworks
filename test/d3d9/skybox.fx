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

// XForm Matrix
float4x4	g_matViewProj;

float3		g_EyePos;

//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
textureCUBE	g_texSkyCube;

// Displacement map for height and choppy field
sampler g_samplerSkyCube =
sampler_state
{
    texture = (g_texSkyCube);
    MipFilter = Linear;
    MinFilter = Linear;
    MagFilter = Linear;
   
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VS_OUTPUT
{
    float4 Position	 : POSITION;
    float3 EyeVec	 : TEXCOORD0;
};


//-----------------------------------------------------------------------------
// Name: SkyboxVS
// Type: Vertex shader                                      
// Desc: 
//-----------------------------------------------------------------------------
VS_OUTPUT SkyboxVS(float4 vPos : POSITION)
{
	VS_OUTPUT Output;

	Output.Position = mul(vPos, g_matViewProj);
	Output.EyeVec = vPos - g_EyePos;

	return Output; 
}




//-----------------------------------------------------------------------------
// Name: SkyboxPS
// Type: Pixel shader                                      
// Desc: 
//-----------------------------------------------------------------------------
float4 SkyboxPS(VS_OUTPUT In) : COLOR
{
	return texCUBE(g_samplerSkyCube, normalize(In.EyeVec));
}



//-----------------------------------------------------------------------------
// Name: OceanWaveTech
// Type: Technique
// Desc: 
//-----------------------------------------------------------------------------
technique SkyboxTech
{
    pass P0
    {
        VertexShader = compile vs_3_0 SkyboxVS();
        PixelShader  = compile ps_3_0 SkyboxPS();

        ZEnable				= True;
        ZFunc				= LessEqual;
        ZWriteEnable		= True; 
		FillMode			= Solid;
        CullMode			= None;
		AlphaBlendEnable	= False;
		AlphaTestEnable		= False;
		ColorWriteEnable	= 0x0f;
		StencilEnable		= False;
    }
}
