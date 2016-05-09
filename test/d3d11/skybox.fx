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
    Filter = MIN_MAG_MIP_LINEAR;
   
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VS_OUTPUT
{
    float4 Position	 : SV_Position;
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
	Output.EyeVec = vPos.xyz - g_EyePos;

	return Output; 
}




//-----------------------------------------------------------------------------
// Name: SkyboxPS
// Type: Pixel shader                                      
// Desc: 
//-----------------------------------------------------------------------------
float4 SkyboxPS(VS_OUTPUT In) : SV_Target
{
	return g_texSkyCube.Sample(g_samplerSkyCube, normalize(In.EyeVec));
}

//--------------------------------------------------------------------------------------
// DepthStates
//--------------------------------------------------------------------------------------
DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
    StencilEnable = FALSE;
};

//--------------------------------------------------------------------------------------
// RasterStates
//--------------------------------------------------------------------------------------
RasterizerState Solid
{
	FillMode = SOLID;
	CullMode = NONE;
	MultisampleEnable = True;
};

//--------------------------------------------------------------------------------------
// BlendStates
//--------------------------------------------------------------------------------------
BlendState Opaque
{
	BlendEnable[0] = FALSE;
	RenderTargetWriteMask[0] = 0xF;
};

//-----------------------------------------------------------------------------
// Name: OceanWaveTech
// Type: Technique
// Desc: 
//-----------------------------------------------------------------------------
technique11 SkyboxTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, SkyboxVS() ) );
		SetHullShader( NULL );
		SetDomainShader( NULL );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, SkyboxPS() ) );

        SetDepthStencilState( EnableDepth, 0 );
        SetRasterizerState( Solid );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}
