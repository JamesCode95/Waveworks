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

cbuffer QuadObject
{
    static const float2 QuadVertices[4] =
    {
        {-1.0, -1.0},
        { 1.0, -1.0},
        {-1.0,  1.0},
        { 1.0,  1.0}
    };

    static const float2 QuadTexCoordinates[4] =
    {
        {0.0, 1.0},
        {1.0, 1.0},
        {0.0, 0.0},
        {1.0, 0.0}
    };
}

SamplerState SamplerPointClamp
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

SamplerState SamplerLinearClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

SamplerState SamplerLinearWrap
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState SamplerAnisotropicWrap
{
    Filter = ANISOTROPIC;
    AddressU = Wrap;
    AddressV = Wrap;
	MaxAnisotropy = 16;
};

SamplerState SamplerCube
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
	AddressW = Clamp;
};

SamplerState SamplerLinearMirror
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Mirror;
    AddressV = Mirror;
};

SamplerState SamplerLinearBorderBlack
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Border;
    AddressV = Border;
    AddressW = Border;
    BorderColor = float4(0, 0, 0, 0);
};

SamplerState SamplerLinearBorder
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Border;
	AddressV = Border;
};

SamplerComparisonState SamplerBackBufferDepth
{
    Filter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    AddressU = Border;
    AddressV = Border;
    BorderColor = float4(1, 1, 1, 1);
    ComparisonFunc = LESS_EQUAL;
};

SamplerComparisonState SamplerDepthAnisotropic
{
    Filter = COMPARISON_ANISOTROPIC;
    AddressU = Border;
    AddressV = Border;
    ComparisonFunc = LESS;
    BorderColor = float4(1, 1, 1, 0);
    MaxAnisotropy = 16;
};

RasterizerState CullBack
{
    CullMode = Back;
    FrontCounterClockwise = TRUE;
};

RasterizerState CullBackMS
{
    CullMode = Back;
    FrontCounterClockwise = TRUE;
    MultisampleEnable = TRUE;
};

RasterizerState CullFrontNoClip
{
    CullMode = Front;
    FrontCounterClockwise = TRUE;
    DepthClipEnable = FALSE;
};

RasterizerState CullFrontMS
{
    CullMode = Front;
    FrontCounterClockwise = TRUE;
    MultisampleEnable = TRUE;
};

RasterizerState NoCull
{
    CullMode = NONE;
};

RasterizerState NoCullMS
{
    CullMode = NONE;
    MultisampleEnable = TRUE;
};

RasterizerState Wireframe
{
    CullMode = NONE;
    FillMode = WIREFRAME;
};

RasterizerState WireframeMS
{
    CullMode = NONE;
    FillMode = WIREFRAME;
    MultisampleEnable = TRUE;
};

DepthStencilState DepthNormal
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
	DepthFunc = LESS_EQUAL;
	StencilEnable = FALSE;
};

DepthStencilState NoDepthStencil
{
	DepthEnable = FALSE;
	StencilEnable = FALSE;
};

DepthStencilState DepthAlways
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
	DepthFunc = ALWAYS;
	StencilEnable = FALSE;
};


BlendState NoBlending
{
    BlendEnable[0] = FALSE;
};

BlendState Translucent
{
	BlendEnable[0] = TRUE;
	RenderTargetWriteMask[0] = 0xF;

	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
	BlendOp = Add;
};


float4 ColorPS(uniform float4 color) : SV_Target
{
    return color;
}

technique11 Default
{
    pass p0
    {
        SetRasterizerState(NoCull);
        SetDepthStencilState(DepthNormal, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetVertexShader(NULL);
        SetHullShader(NULL);
        SetDomainShader(NULL);
        SetGeometryShader(NULL);
        SetPixelShader(NULL);
    }
}
