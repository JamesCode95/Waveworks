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
float g_numQuadsW;
float g_numQuadsH;
float4 g_quadScale;
float4 g_quadUVDims;
float4 g_srcUVToWorldScale;
float4 g_srcUVToWorldRot;
float4 g_srcUVToWorldOffset;
float4 g_worldToClipScale;
float4 g_clipToWorldRot;
float4 g_clipToWorldOffset;

float4 g_worldToUVScale;
float4 g_worldToUVOffset;
float4 g_worldToUVRot;
float4x4 g_matViewProj;
float4x3 g_matWorld;

//------------------------------------------------------------------------------------
// Water hooks
//------------------------------------------------------------------------------------
struct GFSDK_WAVEWORKS_VERTEX_INPUT
{
	float2 src_uv;
};

float2 rotate_2d(float2 v, float2 rot)
{
	return float2(v.x * rot.x + v.y * rot.y, v.x * -rot.y + v.y * rot.x);
}

float3 GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition(GFSDK_WAVEWORKS_VERTEX_INPUT In)
{
	return float3(g_srcUVToWorldOffset.xy+rotate_2d(In.src_uv*g_srcUVToWorldScale.xy,g_srcUVToWorldRot.xy),0.f);
}

#define GFSDK_WAVEWORKS_SM5
#define GFSDK_WAVEWORKS_USE_TESSELLATION

#define GFSDK_WAVEWORKS_DECLARE_ATTR_DS_SAMPLER(Label,TextureLabel,Regoff) sampler Label; texture2D TextureLabel;
#define GFSDK_WAVEWORKS_DECLARE_ATTR_DS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_ATTR_DS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_ATTR_DS_CBUFFER };

#define GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(Label,TextureLabel,Regoff) sampler Label; texture2D TextureLabel;
#define GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_ATTR_PS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_ATTR_PS_CBUFFER };

#include "GFSDK_WaveWorks_Attributes.fxh"

//------------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------------
static const float2 kQuadCornerUVs[] = {
	float2(0.f,0.f),
	float2(0.f,1.f),
	float2(1.f,0.f),
	float2(1.f,1.f)
};

static const float3 kMarkerCoords[] = {
	float3( 0.f, 0.f, 0.f),
	float3( 1.f, 1.f, 1.f),
	float3( 1.f,-1.f, 1.f),
	float3( 0.f, 0.f, 0.f),
	float3( 1.f,-1.f, 1.f),
	float3(-1.f,-1.f, 1.f),
	float3( 0.f, 0.f, 0.f),
	float3(-1.f,-1.f, 1.f),
	float3(-1.f, 1.f, 1.f),
	float3( 0.f, 0.f, 0.f),
	float3(-1.f, 1.f, 1.f),
	float3( 1.f, 1.f, 1.f)
};

static const float kMarkerSeparation = 5.f;

//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
texture2D g_texDiffuse;
sampler g_samplerDiffuse = sampler_state
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

texture2D g_texLookup;
sampler g_samplerLookup = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};


//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
struct VS_RENDER_LOOKUP_OUTPUT {
	float2 uv : TEXCOORD;
};

struct DS_RENDER_LOOKUP_OUTPUT {
	float4 clip_pos : SV_Position;
	float3 uvz : TEXCOORD;
};

struct VS_UI_OUT {
	float4 position : SV_Position;
	float2 uv       : TEXCOORD;
};

struct HS_RENDER_LOOKUP_OUTPUT_CONST
{
	float fTessFactor[4]       : SV_TessFactor;
	float fInsideTessFactor[2] : SV_InsideTessFactor;
};

struct VS_MARKER_OUTPUT {
	float4 position : SV_Position;
	float3 color : COLOR;
};

//--------------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------------
VS_RENDER_LOOKUP_OUTPUT RenderSurfaceToReverseLookupVS(uint vID : SV_VertexID)
{
	uint quadID = vID/4;
	float2 corner = kQuadCornerUVs[vID%4];

	uint quadX = quadID%int(g_numQuadsW);
	uint quadY = quadID/int(g_numQuadsW);

	VS_RENDER_LOOKUP_OUTPUT o;
	o.uv = g_quadUVDims.xy * (corner + float2(quadX,quadY));
	return o;
}

HS_RENDER_LOOKUP_OUTPUT_CONST RenderSurfaceToReverseLookupHS_Constant(InputPatch<VS_RENDER_LOOKUP_OUTPUT, 4> I)
{
	HS_RENDER_LOOKUP_OUTPUT_CONST O;
	O.fTessFactor[0] = 1;
	O.fTessFactor[1] = 1;
	O.fTessFactor[2] = 1;
	O.fTessFactor[3] = 1;
	O.fInsideTessFactor[0] = 1;
	O.fInsideTessFactor[1] = 1;
	return O;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[patchconstantfunc("RenderSurfaceToReverseLookupHS_Constant")]
[outputcontrolpoints(4)]
VS_RENDER_LOOKUP_OUTPUT RenderSurfaceToReverseLookupHS( InputPatch<VS_RENDER_LOOKUP_OUTPUT, 4> I, uint uCPID : SV_OutputControlPointID )
{
	VS_RENDER_LOOKUP_OUTPUT O;
	O = I[uCPID];
	return O;
}

[domain("quad")]
DS_RENDER_LOOKUP_OUTPUT RenderSurfaceToReverseLookupDS( HS_RENDER_LOOKUP_OUTPUT_CONST HSConstantData, const OutputPatch<VS_RENDER_LOOKUP_OUTPUT, 4> I, float2 f2BilerpCoords : SV_DomainLocation )
{
	float2 uv01 = lerp(I[0].uv,I[1].uv,f2BilerpCoords.y);
	float2 uv23 = lerp(I[2].uv,I[3].uv,f2BilerpCoords.y);

	GFSDK_WAVEWORKS_VERTEX_INPUT water_in;
	water_in.src_uv = lerp(uv01,uv23,f2BilerpCoords.x);

	float3 undisplaced_coords = GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition(water_in);

	GFSDK_WAVEWORKS_VERTEX_OUTPUT water_out = GFSDK_WaveWorks_GetDisplacedVertexAfterTessellation(float4(undisplaced_coords,1.f),0.f,0.f,float3(1,0,0));

	float2 clip_uv = rotate_2d(water_out.pos_world.xy-g_clipToWorldOffset.xy,float2(g_clipToWorldRot.x,-g_clipToWorldRot.y)) * g_worldToClipScale.xy;

	DS_RENDER_LOOKUP_OUTPUT output;
	output.clip_pos = float4(2.f*clip_uv-1.f,0.5f,1.f);
	output.clip_pos.y = -output.clip_pos.y;
	output.uvz = float3(undisplaced_coords.xy,water_out.pos_world.z);

	return output;
}

float4 RenderSurfaceToReverseLookupPS(DS_RENDER_LOOKUP_OUTPUT In) : SV_Target
{ 
	// Simply interpolate the source coords and heights into the map
	return float4(In.uvz,1.f);
}

VS_UI_OUT QuadToUIVS(uint vID : SV_VertexID) {

	VS_UI_OUT o;
	o.uv = kQuadCornerUVs[vID % 4];
	o.position.x =  2.0 * o.uv.x - 1.f;
	o.position.y =  2.0 * o.uv.y - 1.0f;
	o.position.z = 0.5f;
	o.position.w = 1.f;
	
	return o;
}

float4 QuadToUIPS(VS_UI_OUT i) : SV_Target
{
	return g_texDiffuse.Sample(g_samplerDiffuse,i.uv);
}

VS_MARKER_OUTPUT RenderMarkerVS(uint vID : SV_VertexID)
{
	uint markerID = vID/12;
	uint markerX = markerID%5;
	uint markerY = markerID/5;

	float2 markerOffset = float2((float(markerX)-2.f)*kMarkerSeparation,(float(markerY)-2.f)*kMarkerSeparation);

	float3 world_vertex;

	float3 local_vertex = kMarkerCoords[vID%12];
	world_vertex.y = mul(local_vertex.xzy,(float3x3)g_matWorld).y;	// z offset comes from water lookup

	local_vertex.xy += markerOffset;
	world_vertex.xz = mul(float4(local_vertex.xzy,1),g_matWorld).xz;

	float2 lookup_uv = g_worldToUVScale.xy*rotate_2d(world_vertex.xz+g_worldToUVOffset.xy,g_worldToUVRot.xy);
	float3 lookup_value = g_texLookup.SampleLevel(g_samplerLookup,lookup_uv,0).xyz;
	world_vertex.y += lookup_value.z;

	VS_MARKER_OUTPUT o;
	o.position = mul(float4(world_vertex,1.f),g_matViewProj);
	o.color = float3((lookup_uv > 1.f || lookup_uv < 0.f) ? float2(1,1) : float2(0,0),1.f);
	return o;
}

float4 RenderMarkerPS(VS_MARKER_OUTPUT In) : SV_Target
{
	return float4(In.color,1.f);
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

RasterizerState Wireframe
{
	FillMode = WIREFRAME;
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
// Techniques
//--------------------------------------------------------------------------------------
technique11 RenderSurfaceToReverseLookupTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_5_0, RenderSurfaceToReverseLookupVS() ) );
        SetGeometryShader( NULL );
        SetHullShader( CompileShader( hs_5_0, RenderSurfaceToReverseLookupHS() ) );
        SetDomainShader( CompileShader( ds_5_0, RenderSurfaceToReverseLookupDS() ) );
        SetPixelShader( CompileShader( ps_5_0, RenderSurfaceToReverseLookupPS() ) );

        SetDepthStencilState( NoDepthStencil, 0 );
        SetRasterizerState( SolidNoCull );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique11 RenderQuadToUITech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, QuadToUIVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, QuadToUIPS() ) );

        SetDepthStencilState( NoDepthStencil, 0 );
        SetRasterizerState( SolidNoCull );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique11 RenderMarkerTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, RenderMarkerVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, RenderMarkerPS() ) );

        SetDepthStencilState( EnableDepth, 0 );
        SetRasterizerState( SolidNoCull );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}