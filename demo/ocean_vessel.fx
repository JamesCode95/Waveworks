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
#include "shader_common.fxh"
#include "atmospheric.fxh"

//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

// XForm Matrix
float4x4	g_matWorldViewProj;
float4x4	g_matWorldView;
float4x4	g_matWorld;
float4		g_DiffuseColor;
float3		g_LightDirection;
float3		g_LightColor;
float3		g_AmbientColor;
float3		g_LightningPosition;
float3		g_LightningColor;

float		g_FogExponent;

int			g_LightsNum;
float4		g_SpotlightPosition[MaxNumSpotlights];
float4		g_SpotLightAxisAndCosAngle[MaxNumSpotlights];
float4		g_SpotlightColor[MaxNumSpotlights];

#if ENABLE_SHADOWS
float4x4    g_SpotlightMatrix[MaxNumSpotlights];
Texture2D   g_SpotlightResource[MaxNumSpotlights];
#endif

//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
texture2D g_texDiffuse;
texture2D g_texRustMap;
texture2D g_texRust;
texture2D g_texBump;

sampler g_samplerDiffuse = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

sampler g_samplerImageProcess = sampler_state
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
};

struct VS_OUT
{
	float4 position : SV_Position;
	float3 world_normal : NORMAL;
	float3 world_pos : WORLD_POS;
	float3 eye_pos : EYE_POS;
	float2 rust_uv : TEXCOORD;
	float2 rustmap_uv : TEXCOORD2;
	float  model_z : Z;
};

VS_OUT VesselVS(float4 vPos : POSITION, float2 vTex : TEXCOORD, float3 vNml : NORMAL)
{
	VS_OUT o;
	o.position = mul(vPos, g_matWorldViewProj);
	o.world_normal = mul(vNml, (float3x3)g_matWorld);
	o.world_pos = mul(vPos, (float4x3)g_matWorld);
	o.eye_pos = mul(vPos, (float4x3)g_matWorldView);
	o.rust_uv = vPos.yz*0.006;
	o.rustmap_uv = float2(-vPos.z*0.000345 + 0.495,0.945-vPos.y*0.000345+  vPos.z*0.00001);//vTex;
	o.model_z = vPos.y - vPos.z*vPos.z*0.00005 + vPos.z*0.05 - 80.0;
	return o;
}

struct PS_OUT
{
    float4 color    : SV_Target;
    float4 position : SV_Target1;
};

PS_OUT VesselToScenePS(VS_OUT i)
{
    PS_OUT output;

    float3 nml = normalize(i.world_normal);
	float4 result = g_DiffuseColor;
	float rustmap = 1.0-g_texRustMap.Sample(g_samplerDiffuse,i.rustmap_uv).r;
	rustmap = 0.7*rustmap*abs(nml.z)*1.0*saturate(0.6-i.model_z*0.02);
	float4 rust = g_texRust.Sample(g_samplerDiffuse,i.rust_uv);
	result = lerp(result, result*rust,saturate(rustmap));
		
	float hemisphere_term = 0.5f + 0.25f * nml.y;	// Hemisphere lighting against world 'up'
	float3 lighting = hemisphere_term * (g_AmbientColor + g_LightningColor*0.5) + 2.0*g_LightColor * saturate(dot(nml.xzy,g_LightDirection));
	
	// adding lightnings
	lighting += 2.0 * g_LightningColor * saturate(dot(nml,normalize(g_LightningPosition.xzy-i.world_pos)));

	for(int ix = 0; ix != g_LightsNum; ++ix) {
		float3 pixel_to_light = g_SpotlightPosition[ix].xyz - i.eye_pos;
		float3 pixel_to_light_nml = normalize(pixel_to_light);
		float attn = saturate(dot(pixel_to_light_nml,nml));
		attn *= 1.f/dot(pixel_to_light,pixel_to_light);
		attn *= saturate(1.f*(-dot(g_SpotLightAxisAndCosAngle[ix].xyz,pixel_to_light_nml)-g_SpotLightAxisAndCosAngle[ix].w)/(1.f-g_SpotLightAxisAndCosAngle[ix].w));
        float shadow = 1.0f;
#if ENABLE_SHADOWS
        if (attn * dot(g_SpotlightColor[ix].xyz, g_SpotlightColor[ix].xyz) > 0.01f)
        {
            shadow = GetShadowValue(g_SpotlightResource[ix], g_SpotlightMatrix[ix], i.eye_pos.xyz);
        }
#endif
        lighting += attn * g_SpotlightColor[ix].xyz * shadow;
    }

	lighting *=2.0;
	result.rgb *= lighting;

	float fog_factor = exp(dot(i.eye_pos,i.eye_pos)*g_FogExponent);
	result.rgb = lerp(g_AmbientColor + g_LightningColor*0.5,result.rgb,fog_factor);

    output.color = result;
    output.position = float4(i.world_pos.xyz, 0);

	return output;
}

float4 VesselToHullProfilePS(VS_OUT i) : SV_Target
{
	float4 result;
	result.r = i.position.z;	// r is set to depth
	result.gba = 1.f;			// gba is set to 1.f to indicate 'occupied'
	return result;
}

static const float2 kQuadCornerUVs[] = {
	float2(0.f,0.f),
	float2(0.f,1.f),
	float2(1.f,0.f),
	float2(1.f,1.f)
};

struct VS_UI_OUT
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD;
};

VS_UI_OUT QuadToUIVS(uint vID : SV_VertexID) {

	VS_UI_OUT o;
	o.uv = kQuadCornerUVs[vID % 4];
	o.position.x =  1.f * o.uv.x - 1.f;
	o.position.y = -1.f * o.uv.y - 0.f;
	o.position.z = 0.5f;
	o.position.w = 1.f;
	
	return o;
}

float4 QuadToUIPS(VS_UI_OUT i) : SV_Target
{
	return g_texDiffuse.Sample(g_samplerDiffuse,i.uv);
}

struct VS_CRACKFIX_OUT
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD;
};

VS_CRACKFIX_OUT QuadToCrackFixVS(uint vID : SV_VertexID) {

	VS_CRACKFIX_OUT o;
	o.uv = kQuadCornerUVs[vID % 4];
	o.position.x =  2.f * o.uv.x - 1.f;
	o.position.y = -2.f * o.uv.y + 1.f;
	o.position.z = 0.5f;
	o.position.w = 1.f;
	
	return o;
}

bool lt_hull_prof(float2 a, float2 b)
{
	if(a.y < b.y) return true;
	else if(a.y > b.y) return false;
	else if(a.x < b.x) return true;
	else return false;
}

void swap(inout float2 a, inout float2 b)
{
	float2 temp = a;
	a = b;
	b = temp;
}

float4 QuadToCrackFixPS(VS_CRACKFIX_OUT i) : SV_Target
{
	float2 samples[9];
	samples[0] = g_texDiffuse.Sample(g_samplerImageProcess,i.uv,int2(-1,-1)).xy;
	samples[1] = g_texDiffuse.Sample(g_samplerImageProcess,i.uv,int2( 0,-1)).xy;
	samples[2] = g_texDiffuse.Sample(g_samplerImageProcess,i.uv,int2( 1,-1)).xy;
	samples[3] = g_texDiffuse.Sample(g_samplerImageProcess,i.uv,int2(-1, 0)).xy;
	samples[4] = g_texDiffuse.Sample(g_samplerImageProcess,i.uv,int2( 0, 0)).xy;
	samples[5] = g_texDiffuse.Sample(g_samplerImageProcess,i.uv,int2( 1, 0)).xy;
	samples[6] = g_texDiffuse.Sample(g_samplerImageProcess,i.uv,int2(-1, 1)).xy;
	samples[7] = g_texDiffuse.Sample(g_samplerImageProcess,i.uv,int2( 0, 1)).xy;
	samples[8] = g_texDiffuse.Sample(g_samplerImageProcess,i.uv,int2( 1, 1)).xy;

	float coverage_count = 0.f;
	[unroll]
	for (int i = 0; i != 9; ++i) {
		coverage_count += samples[i].y;
	}

	#define TwoSort(a,b) { if(lt_hull_prof(b,a)) swap(a,b);  }
	[unroll]
	for (int n = 8; n ; --n) {
	  [unroll]
	  for (int i = 0; i < n; ++i) {
		TwoSort (samples[i], samples[i+1]);
	  }
	}

	return coverage_count > 4.5f ? samples[8.5f - 0.5f * coverage_count].xyyy : samples[0.5f * coverage_count - 0.5f].xyyy;
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

DepthStencilState DisableDepth
{
    DepthEnable = FALSE;
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
	CullMode = Back;
	MultisampleEnable = True;
};

RasterizerState SolidWireframe
{
	FillMode = WIREFRAME;
	CullMode = Back;
	MultisampleEnable = True;
};

RasterizerState SolidNoCulling
{
	FillMode = SOLID;
	CullMode = None;
	
	MultisampleEnable = True;
};

RasterizerState ShadowRS
{
	FillMode = SOLID;
	CullMode = NONE;

    SlopeScaledDepthBias = 4.0f;
};

//--------------------------------------------------------------------------------------
// BlendStates
//--------------------------------------------------------------------------------------
BlendState Opaque
{
	BlendEnable[0] = FALSE;
	RenderTargetWriteMask[0] = 0xF;
};

technique10 RenderVesselToSceneTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, VesselVS() ) );
        SetHullShader( NULL );
		SetDomainShader( NULL );
		SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, VesselToScenePS() ) );

        SetDepthStencilState( EnableDepth, 0 );
        SetRasterizerState( Solid );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, VesselVS() ) );
        SetHullShader( NULL );
		SetDomainShader( NULL );
		SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, VesselToScenePS() ) );

        SetDepthStencilState( EnableDepth, 0 );
        SetRasterizerState( SolidNoCulling );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique10 RenderVesselToShadowMapTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, VesselVS() ) );
        SetHullShader( NULL );
		SetDomainShader( NULL );
		SetGeometryShader( NULL );
        SetPixelShader( NULL );

        SetDepthStencilState( EnableDepth, 0 );
        SetRasterizerState( ShadowRS );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique10 RenderVesselToHullProfileTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, VesselVS() ) );
        SetHullShader( NULL );
		SetDomainShader( NULL );
		SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, VesselToHullProfilePS() ) );

        SetDepthStencilState( EnableDepth, 0 );
        SetRasterizerState( Solid );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique10 RenderQuadToUITech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, QuadToUIVS() ) );
        SetHullShader( NULL );
		SetDomainShader( NULL );
		SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, QuadToUIPS() ) );

        SetDepthStencilState( DisableDepth, 0 );
        SetRasterizerState( SolidNoCulling );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique10 RenderQuadToCrackFixTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, QuadToCrackFixVS() ) );
        SetHullShader( NULL );
		SetDomainShader( NULL );
		SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, QuadToCrackFixPS() ) );

        SetDepthStencilState( DisableDepth, 0 );
        SetRasterizerState( SolidNoCulling );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique10 WireframeOverrideTech
{
    pass
    {
        SetRasterizerState( SolidWireframe );
    }
}