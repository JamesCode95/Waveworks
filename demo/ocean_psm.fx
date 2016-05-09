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

#include "ocean_shader_common.h"

//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

Texture3D g_PSMMap;
RWTexture3D<uint> g_PSMPropagationMapUAV;

//------------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------------
static const float2 kQuadCornerUVs[] = {
	float2(0.f,0.f),
	float2(0.f,1.f),
	float2(1.f,0.f),
	float2(1.f,1.f)
};

//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
SamplerState g_SamplerPSMUI
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
	AddressW = Clamp;
};

//--------------------------------------------------------------------------------------
// DepthStates
//--------------------------------------------------------------------------------------
DepthStencilState NoDepthStencil
{
    DepthEnable = FALSE;
    StencilEnable = FALSE;
};

//--------------------------------------------------------------------------------------
// RasterStates
//--------------------------------------------------------------------------------------
RasterizerState SolidNoCull
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

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
struct VS_UI_OUT
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD;
};

//--------------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------------
VS_UI_OUT QuadToUIVS(uint vID : SV_VertexID) {

	VS_UI_OUT o;
	o.uv = kQuadCornerUVs[vID % 4];
	o.position.xy = 0.5 * o.uv - 1.f;
	o.position.z = 0.5f;
	o.position.w = 1.f;
	
	return o;
}

float4 PSMToUIPS(VS_UI_OUT i) : SV_Target
{
    int PSMMapW, PSMMapH, PSMMapD;
    g_PSMMap.GetDimensions(PSMMapW, PSMMapH, PSMMapD);

	float tile_wh = ceil(sqrt(float(PSMMapD)));
	float2 tile_coord = i.uv * tile_wh;
	float2 tile_xy = floor(tile_coord);
	float2 within_tile_xy = frac(tile_coord);

	float slice = (tile_xy.y * tile_wh + tile_xy.x);

	return g_PSMMap.Sample(g_SamplerPSMUI,float3(within_tile_xy,slice/float(PSMMapD)));
}

[numthreads(PSMPropagationCSBlockSize,PSMPropagationCSBlockSize,1)]
void PSMPropagationCS( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
{
    uint PSMMapW, PSMMapH, PSMMapD;
    g_PSMPropagationMapUAV.GetDimensions(PSMMapW, PSMMapH, PSMMapD);

	// TBD: Would it be worth doing away with this by insisting on multiple-of-block-size dims?
	if(DTid.x >= PSMMapW || DTid.y >= PSMMapH)
		return;

	float AccumulatedOpacity = 1.f;
	uint2 PixelCoords = DTid.xy;

	/*
	// Early-out if there is no coverage on this coverage slice
	float Coverage = g_PSMCoverageTexture[int2(PixelCoords/PSM_COVERAGE_MULTIPLIER)].r;
	if(Coverage == 0.f)
		return;
		*/

	// Skip layer 0, which is reserved for coverage
	for(uint layer = 1; layer < PSMMapD; ++layer)
	{
		uint ReadVal = g_PSMPropagationMapUAV[int3(PixelCoords,layer)];
		uint RReadVal = ReadVal & 0x0000FFFF;
		uint GReadVal = ReadVal >> 16;
		float LocalOpacity1 = float(RReadVal) * 1.f/65535.f;
		float LocalOpacity2 = float(GReadVal) * 1.f/65535.f;
		float AccumulatedOpacityTemp = AccumulatedOpacity * LocalOpacity1;
		AccumulatedOpacity = AccumulatedOpacityTemp * LocalOpacity2;
		if(AccumulatedOpacity < 1.f)
		{
			uint RWriteVal = 65535.f * AccumulatedOpacityTemp;
			uint GWriteVal = 65535.f * AccumulatedOpacity;
			uint WriteVal = RWriteVal + (GWriteVal << 16);
			g_PSMPropagationMapUAV[int3(PixelCoords,layer)] = WriteVal;
		}
	}
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique11 PropagatePSMTech
{
    pass
    {          
        SetVertexShader( NULL );
        SetGeometryShader( NULL );
        SetHullShader( NULL );
        SetDomainShader( NULL );
        SetPixelShader( NULL );
		SetComputeShader( CompileShader( cs_5_0, PSMPropagationCS() ) );
    }
}

technique11 RenderPSMToUITech
{
    pass
    {          
        SetVertexShader( CompileShader( vs_4_0, QuadToUIVS() ) );
        SetGeometryShader( NULL );
        SetHullShader( NULL );
        SetDomainShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PSMToUIPS() ) );

        SetDepthStencilState( NoDepthStencil, 0 );
        SetRasterizerState( SolidNoCull );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}
