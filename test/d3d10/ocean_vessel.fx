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
// Copyright � 2008- 2013 NVIDIA Corporation. All rights reserved.
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
float4x4	g_matWorldViewProj;

struct VS_OUT
{
	float4 position : SV_Position;
	float4 color : COLOR;
};

VS_OUT VesselVS(float4 vPos : POSITION, float4 vCol : COLOR)
{
	VS_OUT o;
	o.position = mul(vPos, g_matWorldViewProj);
	o.color = vCol;
	return o;
}

float4 VesselPS(VS_OUT i) : SV_Target
{
	return i.color * i.color;
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

technique10 RenderVesselTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, VesselVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, VesselPS() ) );

        SetDepthStencilState( EnableDepth, 0 );
        SetRasterizerState( Solid );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}
