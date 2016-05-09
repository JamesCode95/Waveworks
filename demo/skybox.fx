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

#include "atmospheric.fxh"

//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

// XForm Matrix
float4x4	g_matViewProj;
float4x4	g_matProjInv;
float3		g_FogColor;
float		g_FogExponent;
float3		g_LightningColor;
float		g_CloudFactor;

static const float kEarthRadius = 6400000;
static const float kCloudbaseHeight = 250;
static const float kMaxCloudbaseDistance = 2000;
static const float kMinFogFactor = 0.f;


float3 g_LightPos;												// The direction to the light source

//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
textureCUBE	g_texSkyCube0;
textureCUBE	g_texSkyCube1;
float g_SkyCubeBlend;
float2 g_SkyCube0RotateSinCos;
float2 g_SkyCube1RotateSinCos;
float4 g_SkyCubeMult;

Texture2D g_texColor;
Texture2DMS<float> g_texDepthMS;
Texture2D<float> g_texDepth;

float3 rotateXY(float3 xyz, float2 sc)
{
	float3 result = xyz;
	float s = sc.x;
	float c = sc.y;
	result.x = xyz.x * c - xyz.y * s;
	result.y = xyz.x * s + xyz.y * c;
	return result;
}

// Displacement map for height and choppy field
sampler g_samplerSkyCube =
sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
   
    AddressU = Clamp;
    AddressV = Clamp;
};

SamplerState DefaultSampler
{
};

struct VS_OUTPUT
{
    float4 Position	 : SV_Position;
    float3 EyeVec	 : TEXCOORD0;
    float3 PosWorld  : TEXCOORD1;
};

struct QUAD_OUTPUT
{
    float4 Position	: SV_Position;
    float2 TexCoord	: TEXCOORD0;
};

static const float2 kCornerCoords[4] = {
    {-1, 1},
    { 1, 1},
    {-1,-1},
    { 1,-1}
};

QUAD_OUTPUT QuadVS(uint index : SV_VertexID)
{
	QUAD_OUTPUT output;

	output.Position = float4(kCornerCoords[index], 0, 1.0f);
	output.TexCoord = kCornerCoords[index] * 0.5f + 0.5f;
	output.TexCoord.y = 1.0f - output.TexCoord.y;

	return output;
}

//-----------------------------------------------------------------------------
// Name: SkyboxVS
// Type: Vertex shader                                      
// Desc: 
//-----------------------------------------------------------------------------
VS_OUTPUT SkyboxVS(float4 vPos : POSITION)
{
	VS_OUTPUT Output;

	Output.Position = mul(vPos, g_matViewProj);
	Output.Position.z = Output.Position.w;
	Output.EyeVec = normalize(vPos.xyz);
	Output.PosWorld = vPos.xyz;
	return Output; 
}

//-----------------------------------------------------------------------------
// Name: SkyboxPS
// Type: Pixel shader                                      
// Desc: 
//-----------------------------------------------------------------------------
float4 SkyboxPS(VS_OUTPUT In) : SV_Target
{
	float3 n = normalize(In.EyeVec);
	float4 lower = g_texSkyCube0.Sample(g_samplerSkyCube, rotateXY(n,g_SkyCube0RotateSinCos));
	float4 upper = g_texSkyCube1.Sample(g_samplerSkyCube, rotateXY(n,g_SkyCube1RotateSinCos));
	float4 sky_color = g_SkyCubeMult * lerp(lower,upper,g_SkyCubeBlend);

	float zr = n.z * kEarthRadius;
	float distance_to_cloudbase = sqrt(zr * zr + 2.f * kEarthRadius * kCloudbaseHeight + kCloudbaseHeight * kCloudbaseHeight) - zr;
	distance_to_cloudbase = min(kMaxCloudbaseDistance,distance_to_cloudbase);

	float fog_factor = exp(distance_to_cloudbase*distance_to_cloudbase*g_FogExponent);
	fog_factor = kMinFogFactor + (1.f - kMinFogFactor) * fog_factor;
	sky_color.rgb = lerp(g_FogColor + g_LightningColor*0.5,sky_color.rgb,fog_factor);

	AtmosphereColorsType AtmosphereColors = CalculateAtmosphericScattering(In.EyeVec,g_LightPos, 15.0);
	float3 clear_color= AtmosphereColors.RayleighColor + AtmosphereColors.MieColor*5.0;
	float3 result = lerp(clear_color, sky_color, g_CloudFactor);
	return float4(result,1.0);
}

float DownsampleDepthPS(QUAD_OUTPUT In) : SV_Depth
{
	int2 iCoords = (int2)In.Position.xy;
	iCoords *= 2;

	return g_texDepthMS.Load(iCoords, 0);
}

float RND_1d(float2 x)
{
    uint n = asuint(x.y * 6435.1392 + x.x * 45.97345);
    n = (n<<13)^n;
    n = n * (n*n*15731u + 789221u) + 1376312589u;
    n = (n>>9u) | 0x3F800000;

    return 2.0 - asfloat(n);
}

float4 UpsampleParticlesPS(QUAD_OUTPUT In) : SV_Target
{
	float4 pixelColor = 0;
	uint sampleCount = 0;

	float4 linearColor = g_texColor.Sample(DefaultSampler, In.TexCoord);

	for (uint s=0; s<4; ++s)
	{
		float sampleDepth = g_texDepthMS.Load((int2)In.Position.xy, s);

		float4 sampleClipPos = float4(0, 0, sampleDepth, 1.0);
		float4 sampleViewSpace = mul(sampleClipPos, g_matProjInv);
		sampleViewSpace.z /= sampleViewSpace.w;

		float4 combinedColor = 0;
		float combinedWeight = 0;

		int radius = 1;
		for (int i=-radius; i<=radius; ++i)
		{
			for (int j=-radius; j<=radius; ++j)
			{
				int2 iCoarseCoord = int2(In.Position.xy) / 2 + int2(i, j);
				float4 color = g_texColor[iCoarseCoord];
				float depth = g_texDepth[iCoarseCoord];

				float4 clipPos = float4(0, 0, depth, 1.0);
				float4 viewSpace = mul(clipPos, g_matProjInv);
				viewSpace.z /= viewSpace.w;

				float depthDifference = abs(sampleViewSpace.z - viewSpace.z);
				float weight = 1.0f / (abs(sampleViewSpace.z - viewSpace.z) + 0.001f);

				combinedColor += color * weight;
				combinedWeight += weight;
			}
		}

		if(combinedWeight > 0.00001)
		{
			pixelColor += combinedColor / combinedWeight;
			++sampleCount;
		}
	}

	if (!sampleCount) discard;

	float4 finalColor = pixelColor / sampleCount;
	finalColor = lerp(linearColor, finalColor, finalColor.a);

	return finalColor;
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

DepthStencilState WriteDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = ALWAYS;
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

BlendState Additive
{
	BlendEnable[0] = TRUE;
	RenderTargetWriteMask[0] = 0xF;

	SrcBlend = ONE;
	DestBlend = SRC_ALPHA;
	BlendOp = Add;
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

technique11 DownsampleDepthTech
{
	pass
	{
		SetVertexShader( CompileShader( vs_4_0, QuadVS() ) );
		SetHullShader( NULL );
		SetDomainShader( NULL );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, DownsampleDepthPS() ) );

		SetDepthStencilState( WriteDepth, 0 );
		SetRasterizerState( Solid );
		SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
}

technique11 UpsampleParticlesTech
{
	pass
	{
		SetVertexShader( CompileShader( vs_4_0, QuadVS() ) );
		SetHullShader( NULL );
		SetDomainShader( NULL );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, UpsampleParticlesPS() ) );

		SetDepthStencilState( NULL, 0 );
		SetRasterizerState( Solid );
		SetBlendState( Additive, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		//SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
}
