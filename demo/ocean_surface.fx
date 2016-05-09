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

#define GFSDK_WAVEWORKS_SM5
#define GFSDK_WAVEWORKS_USE_TESSELLATION

#define GFSDK_WAVEWORKS_DECLARE_GEOM_VS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_GEOM_VS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_GEOM_VS_CBUFFER };

#define GFSDK_WAVEWORKS_DECLARE_GEOM_HS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_GEOM_HS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_GEOM_HS_CBUFFER };

#include "GFSDK_WaveWorks_Quadtree.fxh"

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
// Global variables
//------------------------------------------------------------------------------------

// Constant

float		g_ZNear = 1.0;
float		g_ZFar = 20000.0;


float4x4	g_matViewProj;
float4x4	g_matView;
float4x4	g_matWorldToShip;

float4      g_ViewRight;
float4      g_ViewUp;
float3      g_ViewForward;

float4		g_GustUV;

float4		g_ScreenSizeInv;
float3		g_SkyColor;
float3		g_DeepColor;
float3		g_BendParam = {0.1f, -0.4f, 0.2f};

float3		g_LightningPosition;
float3		g_LightningColor;
float4x4	g_matSceneToShadowMap;

float3		g_LightDir;
float3		g_LightColor;
float3      g_WaterDeepColor={0.0,0.04,0.09};
float3      g_WaterScatterColor={0.0,0.05,0.025};
float		g_WaterSpecularIntensity = 0.4;
float       g_WaterSpecularPower=100.0;
float       g_WaterLightningSpecularPower=20.0;

float		g_ShowSpraySim=0.0;
float		g_ShowFoamSim=0.0;

int			g_LightsNum;
float4		g_SpotlightPosition[MaxNumSpotlights];
float4		g_SpotLightAxisAndCosAngle[MaxNumSpotlights];
float4		g_SpotlightColor[MaxNumSpotlights];

float3		foam_underwater_color = {0.81f, 0.90f, 1.0f};

float		g_GlobalFoamFade;

float4		g_HullProfileCoordOffsetAndScale[MaxNumVessels];
float4		g_HullProfileHeightOffsetAndHeightScaleAndTexelSize[MaxNumVessels];

float		g_CubeBlend;

float2		g_SkyCube0RotateSinCos;
float2		g_SkyCube1RotateSinCos;

float4		g_SkyCubeMult;

float		g_FogExponent;

float       g_TimeStep = 0.1f;

float		g_CloudFactor;

bool		g_bGustsEnabled;
bool		g_bWakeEnabled;

#if ENABLE_SHADOWS
float4x4    g_SpotlightMatrix[MaxNumSpotlights];
Texture2D   g_SpotlightResource[MaxNumSpotlights];
#endif

float3 rotateXY(float3 xyz, float2 sc)
{
	float3 result = xyz;
	float s = sc.x;
	float c = sc.y;
	result.x = xyz.x * c - xyz.y * s;
	result.y = xyz.x * s + xyz.y * c;
	return result;
}

#define FRESNEL_TERM_SUPERSAMPLES_RADIUS 1
#define FRESNEL_TERM_SUPERSAMPLES_INTERVALS (1 + 2*FRESNEL_TERM_SUPERSAMPLES_RADIUS)

//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
texture1D	g_texColorMap;
texture2D	g_texBufferMap;
textureCUBE	g_texCubeMap0;
textureCUBE	g_texCubeMap1;
texture2D	g_texFoamIntensityMap;
texture2D	g_texFoamDiffuseMap;
texture2D	g_texHullProfileMap[MaxNumVessels];

texture2D	g_texWakeMap;
texture2D	g_texShipFoamMap;
texture2D	g_texGustMap;
texture2D	g_texLocalFoamMap;

texture2D	g_texReflection;
texture2D	g_texReflectionPos;

struct SprayParticleData
{
    float4 position;
    float4 velocity;
};

AppendStructuredBuffer<SprayParticleData> g_SprayParticleData : register(u1);
StructuredBuffer<SprayParticleData>       g_SprayParticleDataSRV;

// Blending map for ocean color
sampler g_samplerColorMap =
sampler_state
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = Clamp;
};

// Environment map
sampler g_samplerCubeMap =
sampler_state
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
	AddressW = Clamp;
};

// Standard trilinear sampler
sampler g_samplerTrilinear =
sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;//ANISOTROPIC;
    AddressU = Wrap;
	AddressV = Wrap;
	MaxAnisotropy = 1;
};

// Standard anisotropic sampler
sampler g_samplerAnisotropic =
sampler_state
{
    Filter = ANISOTROPIC;
    AddressU = Wrap;
	AddressV = Wrap;
	MaxAnisotropy = 16;
};

// Hull profile sampler
sampler g_samplerHullProfile = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
	AddressV = Clamp;
};

sampler g_samplerHullProfileBorder
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Border;
	AddressV = Border;
    BorderColor = float4(0, 0, 0, 0);
};

sampler g_samplerTrilinearClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
	AddressV = Clamp;
};

sampler g_samplerBilinearClamp
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = Clamp;
	AddressV = Clamp;
};

sampler g_samplerPointClamp
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
	AddressV = Clamp;
};

struct VS_OUTPUT
{
	float4								worldspace_position	: VSO ;
	float								hull_proximity : HULL_PROX;
};

struct DS_OUTPUT
{
    precise float4								pos_clip	 : SV_Position;
    GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT NV_ocean_interp;
	float3								world_displacement: TEXCOORD4;
	float3								world_pos_undisplaced: TEXCOORD5;
	float3								world_pos: TEXCOORD6;
	float3								eye_pos: TEXCOORD7;
	float2								wake_uv: TEXCOORD8;
	float2								foam_uv: TEXCOORD9;
    float                               penetration : PENETRATION; 
};

struct HS_ConstantOutput
{
	// Tess factor for the FF HW block
	float fTessFactor[3]    : SV_TessFactor;
	float fInsideTessFactor : SV_InsideTessFactor;
	float fTargetEdgeLength[3] : TargetEdgeLength;
	float fHullProxMult[3] : HullProxMult;
};

struct Empty
{
};

struct InParticlePS
{
    float4 position : SV_Position;
    float4 color    : COLOR;
};

void ParticleVS()
{

}

static const float2 ParticleOffsets[] = 
{
    float2(-1,  1),
    float2(-1, -1),
    float2( 1,  1),
    float2( 1, -1)
};

[maxvertexcount(4)]
void ParticleGS(point Empty input[1], inout TriangleStream<InParticlePS> particleStream, uint particleID : SV_PrimitiveID)
{
    InParticlePS output;

    SprayParticleData particleData = g_SprayParticleDataSRV[particleID];

    float3 position = particleData.position.xyz;
    position -= g_ViewForward.xyz * 0.75f;
    float particleSize = particleData.position.w;

    output.color = float4(0, 0, 1.0f, 0.25f * particleSize);

    [unroll]
    for (int i=0; i<4; ++i)
    {
        float3 vertexPos = position + (ParticleOffsets[i].x * g_ViewRight.xyz + ParticleOffsets[i].y * g_ViewUp.xyz) * particleSize;
        output.position = mul(float4(vertexPos, 1.0f), g_matViewProj);
        particleStream.Append(output);
    }
}

float4 ParticlePS(InParticlePS input) : SV_Target
{
    return input.color;
}

//-----------------------------------------------------------------------------
// Name: OceanWaveVS
// Type: Vertex shader                                      
// Desc: 
//-----------------------------------------------------------------------------
VS_OUTPUT OceanWaveVS(GFSDK_WAVEWORKS_VERTEX_INPUT In)
{
	GFSDK_WAVEWORKS_VERTEX_OUTPUT wvo = GFSDK_WaveWorks_GetDisplacedVertex(In);

	VS_OUTPUT Output;
	Output.worldspace_position = float4(wvo.pos_world_undisplaced,0.0);

	Output.hull_proximity = 0.f;
	[unroll]
	for(int i = 0; i != MaxNumVessels; ++i) {

		// Probably we could calc this elegantly, but easier to hard-code right now
		float mip_level = 6;

		// Sample the vessel hull profile and depress the surface where necessary
		float2 hull_profile_uv = g_HullProfileCoordOffsetAndScale[i].xy + wvo.pos_world.xy * g_HullProfileCoordOffsetAndScale[i].zw;
		float4 hull_profile_sample = g_texHullProfileMap[i].SampleLevel(g_samplerHullProfile, hull_profile_uv, mip_level);

		float hull_profile_height = g_HullProfileHeightOffsetAndHeightScaleAndTexelSize[i].x + g_HullProfileHeightOffsetAndHeightScaleAndTexelSize[i].y * hull_profile_sample.x;
		Output.hull_proximity += hull_profile_sample.y;
	}

	return Output; 
}

float CalcTessFactorMultFromProximity(float proximity)
{
	const float max_proximity = 0.5f;
	const float max_mult = 4.f;
	return 1.f + (max_mult-1.f) * saturate(proximity/max_proximity);
}

//--------------------------------------------------------------------------------------
// This hull shader passes the tessellation factors through to the HW tessellator, 
// and the 10 (geometry), 6 (normal) control points of the PN-triangular patch to the domain shader
//--------------------------------------------------------------------------------------
HS_ConstantOutput HS_PNTrianglesConstant( InputPatch<VS_OUTPUT, 3> I )
{
	HS_ConstantOutput O;

	O.fHullProxMult[0] = CalcTessFactorMultFromProximity(max(I[1].hull_proximity,I[2].hull_proximity));
	O.fHullProxMult[1] = CalcTessFactorMultFromProximity(max(I[2].hull_proximity,I[0].hull_proximity));
	O.fHullProxMult[2] = CalcTessFactorMultFromProximity(max(I[0].hull_proximity,I[1].hull_proximity));

	O.fTessFactor[0] = GFSDK_WaveWorks_GetEdgeTessellationFactor(I[1].worldspace_position,I[2].worldspace_position) * O.fHullProxMult[0];
	O.fTessFactor[1] = GFSDK_WaveWorks_GetEdgeTessellationFactor(I[2].worldspace_position,I[0].worldspace_position) * O.fHullProxMult[1];
	O.fTessFactor[2] = GFSDK_WaveWorks_GetEdgeTessellationFactor(I[0].worldspace_position,I[1].worldspace_position) * O.fHullProxMult[2];

	O.fInsideTessFactor = (O.fTessFactor[0] + O.fTessFactor[1] + O.fTessFactor[2])/3.0f;

	O.fTargetEdgeLength[0] = GFSDK_WaveWorks_GetVertexTargetTessellatedEdgeLength(I[0].worldspace_position.xyz);
	O.fTargetEdgeLength[1] = GFSDK_WaveWorks_GetVertexTargetTessellatedEdgeLength(I[1].worldspace_position.xyz);
	O.fTargetEdgeLength[2] = GFSDK_WaveWorks_GetVertexTargetTessellatedEdgeLength(I[2].worldspace_position.xyz);
	return O;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[patchconstantfunc("HS_PNTrianglesConstant")]
[outputcontrolpoints(3)]
VS_OUTPUT HS_PNTriangles( InputPatch<VS_OUTPUT, 3> I, uint uCPID : SV_OutputControlPointID )
{
	VS_OUTPUT O = (VS_OUTPUT)I[uCPID];
	return O;
}


//--------------------------------------------------------------------------------------
// This domain shader applies contol point weighting to the barycentric coords produced by the FF tessellator 
//--------------------------------------------------------------------------------------
[domain("tri")]
DS_OUTPUT DS_PNTriangles( HS_ConstantOutput HSConstantData, const OutputPatch<VS_OUTPUT, 3> I, float3 f3BarycentricCoords : SV_DomainLocation )
{
	DS_OUTPUT Output = (DS_OUTPUT)0;

	GFSDK_WAVEWORKS_VERTEX_OUTPUT NV_ocean = GFSDK_WaveWorks_GetDisplacedVertexAfterTessellation(I[0].worldspace_position, I[1].worldspace_position, I[2].worldspace_position, f3BarycentricCoords);

	float3 pos_world = NV_ocean.pos_world;
	Output.world_pos_undisplaced = NV_ocean.pos_world - NV_ocean.world_displacement;

	float fTargetEdgeLength = HSConstantData.fTargetEdgeLength[0] * f3BarycentricCoords.x / HSConstantData.fHullProxMult[0] +
		HSConstantData.fTargetEdgeLength[1] * f3BarycentricCoords.y / HSConstantData.fHullProxMult[1] +
		HSConstantData.fTargetEdgeLength[2] * f3BarycentricCoords.z / HSConstantData.fHullProxMult[2];

	// calculating texcoords for wake maps
	float2 wake_uv = mul(float4(NV_ocean.pos_world.x,0.0,NV_ocean.pos_world.y,1.0),g_matWorldToShip).xz;
	wake_uv *= g_WakeTexScale;
	wake_uv += g_WakeTexOffset;
	Output.wake_uv = wake_uv;	

	float2 foam_uv = mul(float4(Output.world_pos_undisplaced.x,0.0,Output.world_pos_undisplaced.y,1.0),g_matWorldToShip).xz;
	foam_uv *= g_WakeTexScale;
	foam_uv += g_WakeTexOffset;
	Output.foam_uv = wake_uv;	

	if(g_bWakeEnabled) {
		// fetching wakes
		float4 wake = g_texWakeMap.SampleLevel(g_samplerTrilinearClamp, wake_uv,0);
		
		// applying displacement added by wakes
		float3 wake_displacement;
		wake_displacement.z = 2.0*(wake.a-0.5);
		wake_displacement.xy = float2(0,0);
		
		NV_ocean.world_displacement.z += wake_displacement.z;
		NV_ocean.pos_world.z += wake_displacement.z;
		pos_world.z += wake_displacement.z;
	}

	[unroll]
	for(int i = 0; i != MaxNumVessels; ++i) {

		// Calculate the mip level for sampling the hull profile (aim for one pixel per tessellated tri)
		// float mip_level = log2(fTargetEdgeLength/g_HullProfileHeightOffsetAndHeightScaleAndTexelSize[i].z);

		// Use the most detailed mip data available
		float mip_level = 0;

	// Sample the vessel hull profile and depress the surface where necessary
		float2 hull_profile_uv = g_HullProfileCoordOffsetAndScale[i].xy + NV_ocean.pos_world.xy * g_HullProfileCoordOffsetAndScale[i].zw;
		float4 hull_profile_sample = g_texHullProfileMap[i].SampleLevel(g_samplerHullProfile, hull_profile_uv, mip_level);
		float hull_profile_height = g_HullProfileHeightOffsetAndHeightScaleAndTexelSize[i].x + g_HullProfileHeightOffsetAndHeightScaleAndTexelSize[i].y * hull_profile_sample.x;
		float hull_profile_blend = hull_profile_sample.y;

		if(hull_profile_height < NV_ocean.pos_world.z && hull_profile_blend > 0.f)
        {
            Output.penetration += abs(pos_world.z - ((1.f-hull_profile_blend) * pos_world.z + hull_profile_blend * hull_profile_height));
            
            pos_world.z = (1.f-hull_profile_blend) * pos_world.z + hull_profile_blend * hull_profile_height;
		}
	}


	Output.NV_ocean_interp = NV_ocean.interp;
	Output.pos_clip = mul(float4(pos_world,1), g_matViewProj);
	Output.world_displacement = NV_ocean.world_displacement;
	Output.world_pos = pos_world;
	Output.eye_pos = mul(float4(pos_world,1), (float4x3)g_matView);
	return Output; 
}

//-----------------------------------------------------------------------------
// Name: OceanWavePS
// Type: Pixel shader                                      
// Desc: 
//-----------------------------------------------------------------------------
float4 OceanWavePS(DS_OUTPUT In, uint isFrontFace : SV_IsFrontFace) : SV_Target
{
	GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES surface_attributes = GFSDK_WaveWorks_GetSurfaceAttributes(In.NV_ocean_interp);

	float fresnel_factor;
	float diffuse_factor;
	float specular_factor;
	float scatter_factor;
	
	float lightning_diffuse_factor;
	float lightning_specular_factor;
	float lightning_scatter_factor;

	// applying wake normal to surface normal
	float4 wake = g_texWakeMap.Sample(g_samplerTrilinearClamp, In.wake_uv).rgba-float4(0.5,0.5,0.0,0.5);
	wake.rgb *= 2.0;
	wake.rgb = (mul(g_matWorldToShip,float4(wake.rbg,0.0))).rbg;
	
	if(g_bWakeEnabled) {
		surface_attributes.normal = normalize(surface_attributes.normal + (surface_attributes.normal.b/wake.b)*float3(wake.rg,0));
	}

	// fetching wake energy
	float4 wake_energy = g_bWakeEnabled ? g_texShipFoamMap.Sample(g_samplerTrilinearClamp, In.wake_uv) : 0.f;


	float3 pixel_to_lightning_vector=normalize(g_LightningPosition - In.world_pos);
	float3 pixel_to_light_vector=g_LightDir;
	float3 pixel_to_eye_vector=surface_attributes.eye_dir;
	float3 reflected_eye_to_pixel_vector = reflect(-surface_attributes.eye_dir, surface_attributes.normal);

	// Super-sample the fresnel term
	float3 ramp = 0.f;
	[unroll]
	float4 attr36_ddx = ddx(In.NV_ocean_interp.nv_waveworks_attr36);
	float4 attr36_ddy = ddy(In.NV_ocean_interp.nv_waveworks_attr36);
	float4 attr37_ddx = ddx(In.NV_ocean_interp.nv_waveworks_attr37);
	float4 attr37_ddy = ddy(In.NV_ocean_interp.nv_waveworks_attr37);
	for(int sx = -FRESNEL_TERM_SUPERSAMPLES_RADIUS; sx <= FRESNEL_TERM_SUPERSAMPLES_RADIUS; ++sx)
	{
		float fx = float(sx)/float(FRESNEL_TERM_SUPERSAMPLES_INTERVALS);

		GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT ssx_interp = In.NV_ocean_interp;
		ssx_interp.nv_waveworks_attr36 += fx * attr36_ddx;
		ssx_interp.nv_waveworks_attr37 += fx * attr37_ddx;

		[unroll]
		for(int sy = -FRESNEL_TERM_SUPERSAMPLES_RADIUS; sy <= FRESNEL_TERM_SUPERSAMPLES_RADIUS; ++sy)
		{
			float fy = float(sy)/float(FRESNEL_TERM_SUPERSAMPLES_INTERVALS);

			GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT ssxy_interp = ssx_interp;
			ssxy_interp.nv_waveworks_attr36 += fy * attr36_ddy;
			ssxy_interp.nv_waveworks_attr37 += fy * attr37_ddy;

			GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES ss_surface_attributes = GFSDK_WaveWorks_GetSurfaceAttributes(ssxy_interp);

			float cos_angle = dot(ss_surface_attributes.normal, surface_attributes.eye_dir);

			// ramp.x for fresnel term. ramp.y for atmosphere blending
			ramp += g_texColorMap.Sample(g_samplerColorMap, cos_angle).xyz;
		}
	}

	ramp *= 1.f/float(FRESNEL_TERM_SUPERSAMPLES_INTERVALS*FRESNEL_TERM_SUPERSAMPLES_INTERVALS);

	/* Disabled - contributes to objectionable shimmering
	// A worksaround to deal with "indirect reflection vectors" (which are rays requiring multiple
	// reflections to reach the sky).
	if (reflected_eye_to_pixel_vector.z < g_BendParam.x)
		ramp = lerp(ramp, g_BendParam.z, (g_BendParam.x - reflected_eye_to_pixel_vector.z)/(g_BendParam.x - g_BendParam.y));
		*/

	reflected_eye_to_pixel_vector.z = max(0, reflected_eye_to_pixel_vector.z);
	ramp = saturate(ramp);



	// simulating scattering/double refraction
	scatter_factor=5.0*max(0,In.world_displacement.z*0.05+0.3);
	scatter_factor*=pow(max(0.0,dot(normalize(float3(pixel_to_light_vector.x,pixel_to_light_vector.y,0)),-pixel_to_eye_vector)),2.0);
	scatter_factor*=pow(max(0.0,0.5-0.5*dot(pixel_to_light_vector,surface_attributes.normal)),2.0);
	scatter_factor+=3.0*max(0,In.world_displacement.z*0.05+0.3)* max(0,dot(pixel_to_eye_vector,surface_attributes.normal));


	//scattering from lightning
	lightning_scatter_factor=5.0*max(0,In.world_displacement.z*0.05+0.3);
	lightning_scatter_factor*=pow(max(0.0,dot(normalize(float3(pixel_to_lightning_vector.x,pixel_to_lightning_vector.y,0)),-pixel_to_eye_vector)),2.0);
	lightning_scatter_factor*=pow(max(0.0,0.5-0.5*dot(pixel_to_lightning_vector,surface_attributes.normal)),2.0);
	lightning_scatter_factor+=3.0*max(0,In.world_displacement.z*0.05+0.3)*max(0,dot(pixel_to_eye_vector,surface_attributes.normal));
	
	
	// calculating fresnel factor 
	//float r=(1.2-1.0)*(1.2-1.0)/(1.2+1.0);
	//fresnel_factor = max(0.0,min(1.0,  1.0/pow(r+(1.0-r)*dot(surface_attributes.normal,pixel_to_eye_vector),7.0)  ));

	//float r=(1.0 - 1.13)*(1.0 - 1.13)/(1.0 + 1.13);
	//fresnel_factor = r + (1.0-r)*pow(saturate(1.0 - dot(surface_attributes.normal,pixel_to_eye_vector)),4.0);

	fresnel_factor=ramp.x;
	
	if(g_bGustsEnabled) {
		// applying wind gust map
		// local gust map
		float gust_factor = g_texGustMap.Sample(g_samplerAnisotropic,(In.world_pos.xy + g_GustUV.xy)*0.0003).r;
		gust_factor *= g_texGustMap.Sample(g_samplerAnisotropic,(In.world_pos.xy + g_GustUV.zw)*0.001).r;	
		
		// distant gusts kicking in at very steep angles	
		gust_factor += 3.0*g_texGustMap.Sample(g_samplerAnisotropic,(In.world_pos.xy + g_GustUV.zw)*0.0001).r
						   *saturate(10.0*(-pixel_to_eye_vector.z+0.05));			 
		
		fresnel_factor *= (1.0 - 0.4*gust_factor);
	}

	// calculating diffuse intensity of water surface itself
	diffuse_factor=0.3*max(0,dot(pixel_to_light_vector,surface_attributes.normal));
	lightning_diffuse_factor=max(0,dot(pixel_to_lightning_vector,surface_attributes.normal));

	float3 dynamic_lighting = g_LightColor;
	
	float3 surface_lighting = diffuse_factor * g_LightColor;

	for(int ix = 0; ix != g_LightsNum; ++ix) {
		float3 pixel_to_light = g_SpotlightPosition[ix].xyz - In.eye_pos;
		float3 pixel_to_light_nml = normalize(pixel_to_light);
		float beam_attn = saturate(1.f*(-dot(g_SpotLightAxisAndCosAngle[ix].xyz,pixel_to_light_nml)-g_SpotLightAxisAndCosAngle[ix].w)/(1.f-g_SpotLightAxisAndCosAngle[ix].w));
		beam_attn *= 1.f/dot(pixel_to_light,pixel_to_light);
        float shadow = 1.0f;
#if ENABLE_SHADOWS
        if (beam_attn * dot(g_SpotlightColor[ix].xyz, g_SpotlightColor[ix].xyz) > 0.01f)
        {
            shadow = GetShadowValue(g_SpotlightResource[ix], g_SpotlightMatrix[ix], In.eye_pos.xyz);
        }
#endif
        surface_lighting += beam_attn * g_SpotlightColor[ix].xyz * saturate(dot(pixel_to_light_nml,surface_attributes.normal)) * shadow;
		dynamic_lighting += beam_attn * g_SpotlightColor[ix].xyz * shadow;
	}

	surface_lighting += g_SkyColor + g_LightningColor;
	float3 refraction_color=surface_lighting*g_WaterDeepColor;
	
	// adding color that provide foam bubbles spread in water 
	refraction_color += g_GlobalFoamFade*(surface_lighting*foam_underwater_color*saturate(surface_attributes.foam_turbulent_energy*0.2) + 0.1*wake_energy.r*surface_lighting);

	// adding scatter light component
	refraction_color += g_WaterScatterColor*scatter_factor*dynamic_lighting + g_WaterScatterColor*lightning_scatter_factor*g_LightningColor;

	// reflection color
	float3 cube_map_sample_vector = reflected_eye_to_pixel_vector;
	float3 refl_lower = g_texCubeMap0.Sample(g_samplerCubeMap, rotateXY(cube_map_sample_vector,g_SkyCube0RotateSinCos)).xyz;
	float3 refl_upper = g_texCubeMap1.Sample(g_samplerCubeMap, rotateXY(cube_map_sample_vector,g_SkyCube1RotateSinCos)).xyz;
	float3 cloudy_reflection_color = g_LightColor*lerp(g_SkyColor,g_SkyCubeMult.xyz * lerp(refl_lower,refl_upper,g_CubeBlend), ramp.y);
	
	AtmosphereColorsType AtmosphereColors;
	AtmosphereColors = CalculateAtmosphericScattering(reflected_eye_to_pixel_vector,g_LightDir, 15.0);
	float3 clear_reflection_color = AtmosphereColors.RayleighColor + AtmosphereColors.MieColor*5.0*(1.0f-g_CloudFactor);
	
	float3 reflection_color = lerp(clear_reflection_color,cloudy_reflection_color,g_CloudFactor);
	
    float2 reflection_disturbance_viewspace = mul(float4(surface_attributes.normal.x,surface_attributes.normal.y,0,0),g_matView).xz * 0.05;

    float2 reflectionCoords = In.pos_clip.xy * g_ScreenSizeInv.xy + reflection_disturbance_viewspace;

    float4 planar_reflection = g_texReflection.SampleLevel(g_samplerPointClamp, reflectionCoords,0);
    float reflectionFactor = 0;

    //if (planar_reflection.a)
    {
        float3 planar_reflection_pos = g_texReflectionPos.SampleLevel(g_samplerPointClamp, reflectionCoords, 0).xyz;

        float pixelDistance = dot(g_ViewForward.xzy, planar_reflection_pos.xyz - In.world_pos.xzy);
        pixelDistance = pixelDistance > 0 ? 1.0 : 0.0;

        reflected_eye_to_pixel_vector = normalize(reflected_eye_to_pixel_vector);
        float3 pixel_to_reflection = normalize(planar_reflection_pos - In.world_pos.xzy);

        reflectionFactor = max(dot(reflected_eye_to_pixel_vector.xzy, pixel_to_reflection), 0);
        reflectionFactor = min(pow(reflectionFactor, 8.0) * 8.0, 1.0) * pixelDistance;
    }

    reflection_color = lerp(reflection_color,planar_reflection.rgb * reflectionFactor, any(planar_reflection.a * reflectionFactor));

	//adding static foam map for ship
	surface_attributes.foam_turbulent_energy += 1.0*wake_energy.g*(1.0 + surface_attributes.foam_surface_folding*0.6-0.3);
	surface_attributes.foam_turbulent_energy += 1.0*wake_energy.b;

	if(g_bWakeEnabled) {
		surface_attributes.foam_surface_folding += 10.0*wake.a;
		surface_attributes.foam_wave_hats += 30.0*wake.a;
	}

	//adding local foam generated by spray (uses same UV as wake map and static foam map) 
	surface_attributes.foam_turbulent_energy += 0.2*g_texLocalFoamMap.Sample(g_samplerTrilinearClamp, float2(In.foam_uv.x,1.0-In.foam_uv.y)).r;

    float hullFoamFactor = 0;
    
    /* NO NEED TO DO THIS SINCE WE HAVE SPRAY FOAM  SIMULATION - tim*/
    /* NO LOOKS LIKE WE STILL NEED THIS BUT TUNED DOWN A BIT - tim - after prolonged meditation*/
	[unroll]
	for(int i = 0; i != MaxNumVessels; ++i) {

	    // Sample the vessel hull profile and depress the surface where necessary
		float2 hull_profile_uv = g_HullProfileCoordOffsetAndScale[i].xy + In.world_pos.xy * g_HullProfileCoordOffsetAndScale[i].zw;
		float4 hull_profile_sample = g_texHullProfileMap[i].SampleLevel(g_samplerHullProfileBorder, hull_profile_uv, 6);
		float hull_profile_height = g_HullProfileHeightOffsetAndHeightScaleAndTexelSize[i].x + g_HullProfileHeightOffsetAndHeightScaleAndTexelSize[i].y * hull_profile_sample.x;
		float hull_profile_blend = hull_profile_sample.y;

	    hullFoamFactor += pow(hull_profile_blend * 2.3f, 2.0f);
	}
	/* */
	
	// low frequency foam map
	float foam_intensity_map_lf = 1.0*g_texFoamIntensityMap.Sample(g_samplerTrilinear, In.world_pos_undisplaced.xy*0.1*float2(1,1)).x - 1.0;

	// high frequency foam map
	float foam_intensity_map_hf = 1.0*g_texFoamIntensityMap.Sample(g_samplerTrilinear, In.world_pos_undisplaced.xy*0.4*float2(1,1)).x - 1.0;

	// ultra high frequency foam map
	float foam_intensity_map_uhf = 1.0*g_texFoamIntensityMap.Sample(g_samplerTrilinear, In.world_pos_undisplaced.xy*0.7*float2(1,1)).x;

	float foam_intensity;
	foam_intensity = saturate(foam_intensity_map_hf + min(3.5,1.0*(surface_attributes.foam_turbulent_energy + hullFoamFactor) -0.2));

	foam_intensity += (foam_intensity_map_lf + min(1.5,1.0*surface_attributes.foam_turbulent_energy)); 
	
	foam_intensity -= 0.1*saturate(-surface_attributes.foam_surface_folding);
	
	foam_intensity = max(0,foam_intensity);

	foam_intensity *= 1.0+0.8*saturate(surface_attributes.foam_surface_folding);

    float foam_bubbles = g_texFoamDiffuseMap.Sample(g_samplerTrilinear, In.world_pos_undisplaced.xy*0.25).r;
	foam_bubbles = saturate(5.0*(foam_bubbles-0.8));

	// applying foam hats
	foam_intensity += max(0,foam_intensity_map_uhf*2.0*surface_attributes.foam_wave_hats);

	foam_intensity = pow(foam_intensity, 0.7);
	foam_intensity = saturate(foam_intensity*foam_bubbles*1.0);

	// foam diffuse color
	float foam_diffuse_factor=max(0,0.6+max(0,0.4*dot(pixel_to_light_vector,surface_attributes.normal)));
	float foam_lightning_diffuse_factor=max(0,0.6+max(0,0.4*dot(pixel_to_lightning_vector,surface_attributes.normal)));
	float3 foam_lighting = dynamic_lighting * foam_diffuse_factor + g_LightningColor*foam_lightning_diffuse_factor;
	foam_lighting += g_SkyColor;

	// fading reflection a bit in foamy areas
	reflection_color *= 1.0 - 0.5*foam_intensity;

	// applying Fresnel law
	float3 water_color = lerp(refraction_color,reflection_color,fresnel_factor);

	foam_intensity *= g_GlobalFoamFade;

	water_color = lerp(water_color,foam_lighting,foam_intensity);
	
	// applying specular 
	specular_factor=0;//pow(max(0,dot(pixel_to_light_vector,reflected_eye_to_pixel_vector)),g_WaterSpecularPower)*g_LightColor;
	
	
	lightning_specular_factor=pow(max(0,dot(pixel_to_lightning_vector,reflected_eye_to_pixel_vector)),g_WaterLightningSpecularPower);

	water_color += g_WaterSpecularIntensity*(1.0-foam_intensity)*(specular_factor + lightning_specular_factor*g_LightningColor);

	float fog_factor = exp(dot(In.eye_pos,In.eye_pos)*g_FogExponent*g_CloudFactor*g_CloudFactor);
	
	// overriding output if simplified techniques are requested
	if(g_ShowFoamSim + g_ShowSpraySim >0)
	{
		float3 simple_water_diffuse_color = float3(0.1,0.1,0.1)*(0.2+0.8*fresnel_factor);
		float injected_foam = 0.2*g_texLocalFoamMap.Sample(g_samplerTrilinearClamp, float2(In.foam_uv.x,1.0-In.foam_uv.y)).r;
		simple_water_diffuse_color.r += injected_foam*injected_foam;
		
		if(g_ShowFoamSim>0)
		{
			simple_water_diffuse_color.g += saturate(0.4*surface_attributes.foam_turbulent_energy);
			simple_water_diffuse_color.b += saturate(0.4*surface_attributes.foam_wave_hats);
		}
		return float4(simple_water_diffuse_color,1.0);
	}
	
	
	

	return float4(lerp(g_SkyColor + g_LightningColor*0.5,water_color,fog_factor), 1);

}

float4 g_PatchColor;

float4 OceanWireframePS(DS_OUTPUT In) : SV_Target
{
	return g_PatchColor;
}

//--------------------------------------------------------------------------------------
// DepthStates
//--------------------------------------------------------------------------------------

DepthStencilState DisableDepth
{
	DepthEnable = FALSE;
	DepthWriteMask = ALL;
	DepthFunc = ALWAYS;
	StencilEnable = FALSE;
};

DepthStencilState EnableDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
	DepthFunc = LESS_EQUAL;
	StencilEnable = FALSE; 
};

DepthStencilState AlwaysDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
	DepthFunc = ALWAYS;
	StencilEnable = FALSE;
};

DepthStencilState DepthCompare
{
	DepthEnable = TRUE;
	DepthWriteMask = ZERO;
	DepthFunc = LESS_EQUAL;
};

//--------------------------------------------------------------------------------------
// RasterStates
//--------------------------------------------------------------------------------------
RasterizerState Solid
{
	FillMode = SOLID;
	CullMode = FRONT;

	MultisampleEnable = True;
};

RasterizerState Wireframe
{
	FillMode = WIREFRAME;
	CullMode = FRONT;

	MultisampleEnable = True;
};

RasterizerState ParticleRS
{
	FillMode = SOLID;
	CullMode = None;

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

BlendState Translucent
{
	BlendEnable[0] = TRUE;
	RenderTargetWriteMask[0] = 0xF;

	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
	BlendOp = Add;
};

//-----------------------------------------------------------------------------
// Name: OceanWaveTech
// Type: Technique
// Desc: 
//-----------------------------------------------------------------------------
technique11 RenderOceanSurfTech
{
	// Solid
	pass Pass_PatchSolid
	{
		SetVertexShader( CompileShader( vs_5_0, OceanWaveVS() ) );
		SetHullShader( CompileShader( hs_5_0, HS_PNTriangles() ) );
		SetDomainShader( CompileShader( ds_5_0, DS_PNTriangles() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, OceanWavePS() ) );

		SetDepthStencilState( EnableDepth, 0 );
		SetRasterizerState( Solid );
		SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}

	// Wireframe
	pass Pass_PatchWireframe
	{
		SetVertexShader( CompileShader( vs_5_0, OceanWaveVS() ) );
		SetHullShader( CompileShader( hs_5_0, HS_PNTriangles() ) );
		SetDomainShader( CompileShader( ds_5_0, DS_PNTriangles() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, OceanWireframePS() ) );

		SetDepthStencilState( EnableDepth, 0 );
		SetRasterizerState( Wireframe);
		SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
}



// Buffer selector
int g_BufferType;

//-----------------------------------------------------------------------------
// Following only for debug
//-----------------------------------------------------------------------------
struct VS_DEBUG_OUTPUT
{
	float4 pos_clip : SV_Position;   
	float2 tex_coord	: TEXCOORD1;  
};

//-----------------------------------------------------------------------------
// Name: DebugTextureVS                                        
// Type: Vertex shader
//-----------------------------------------------------------------------------
VS_DEBUG_OUTPUT DisplayBufferVS(float4 vPos : POSITION, float2 vTexCoord : TEXCOORD0)
{
	VS_DEBUG_OUTPUT Output;

	Output.pos_clip = vPos;
	Output.tex_coord = vTexCoord;

	return Output;
}

//-----------------------------------------------------------------------------
// Name: DisplayBufferPS                                        
// Type: Pixel shader
//-----------------------------------------------------------------------------
float4 DisplayBufferPS(VS_DEBUG_OUTPUT In) : SV_Target
{
	// FXC in Mar09 DXSDK can't compile the following code correctly.

	//if (g_BufferType == 1)
	//	return tex2Dlod(g_samplerHeightMap, float4(In.tex_coord, 0, 0)) * 0.005f + 0.5f;
	//else if (g_BufferType == 2)
	//{
	//	float2 grad = tex2Dlod(g_samplerGradientMap, float4(In.tex_coord, 0, 0)).xy;
	//	float3 normal = float3(grad, g_TexelLength_x2);
	//	return float4(normalize(normal) * 0.5f + 0.5f, 0);
	//}
	//else if (g_BufferType == 3)
	//{
	//	float fold = tex2D(g_samplerGradientMap, In.tex_coord).w;
	//	return fold * 0.5f;
	//}
	//else
	//	return 0;

	//float4 height = tex2Dlod(g_samplerHeightMap, float4(In.tex_coord, 0, 0)) * 0.005f + 0.5f;

	//float4 grad = tex2Dlod(g_samplerGradientMap, float4(In.tex_coord, 0, 0));
	//float4 normal = float4(normalize(float3(grad.xy, g_TexelLength_x2)) * 0.5f + 0.5f, 0);

	//float4 fold = grad.w * 0.5f;

	//float4 color = (g_BufferType < 2) ? height : ((g_BufferType < 3) ? normal : fold);
	//return color;
	return g_texBufferMap.Sample(g_samplerColorMap, In.tex_coord);
}


//-----------------------------------------------------------------------------
// Name: DisplayBufferTech
// Type: Technique                                     
// Desc: For debug and performance tuning purpose: outputs a floating-point
//    on screen.
//-----------------------------------------------------------------------------
technique10 DisplayBufferTech
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_4_0, DisplayBufferVS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_4_0, DisplayBufferPS() ) );

		SetDepthStencilState( AlwaysDepth, 0 );
		SetRasterizerState( Solid );
		SetBlendState( Translucent, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
}


//-----------------------------------------------------------------------------
// Local foam maps technique
//-----------------------------------------------------------------------------

float4 g_UVOffsetBlur;
float g_FadeAmount;
texture2D g_texLocalFoamSource;

static const float2 kQuadCornerUVs[] = {
	float2(0.f,0.f),
	float2(0.f,1.f),
	float2(1.f,0.f),
	float2(1.f,1.f)
};

struct LOCALFOAMMAPS_VERTEX_OUTPUT
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD;
};

LOCALFOAMMAPS_VERTEX_OUTPUT ShiftFadeBlurLocalFoamVertexShader(uint vID : SV_VertexID) {

	LOCALFOAMMAPS_VERTEX_OUTPUT o;
	o.uv = kQuadCornerUVs[vID % 4];
	o.position.x =  2.0 * o.uv.x - 1.0f;
	o.position.y = -2.0 * o.uv.y + 1.0f;
	o.position.z = 0.0f;
	o.position.w = 1.f;
	
	return o;
}


float4 ShiftFadeBlurLocalFoamPixelShader(LOCALFOAMMAPS_VERTEX_OUTPUT In) : SV_Target
{
	float2 UVOffset = g_UVOffsetBlur.xy;
	float2 BlurUV = g_UVOffsetBlur.zw;
	float  Fade = g_FadeAmount;

	// blur with variable size kernel is done by doing 4 bilinear samples, 
	// each sample is slightly offset from the center point
	float foam1	= g_texLocalFoamSource.Sample(g_samplerTrilinearClamp, In.uv + UVOffset + BlurUV).r;
	float foam2	= g_texLocalFoamSource.Sample(g_samplerTrilinearClamp, In.uv + UVOffset - BlurUV).r;
	float foam3	= g_texLocalFoamSource.Sample(g_samplerTrilinearClamp, In.uv + UVOffset + BlurUV*2.0).r;
	float foam4	= g_texLocalFoamSource.Sample(g_samplerTrilinearClamp, In.uv + UVOffset - BlurUV*2.0).r;		
	float sum = min(5.0,(foam1 + foam2 + foam3 + foam4)*0.25*Fade); // added clamping to 5
	return float4(sum,sum,sum,sum);
}



//-----------------------------------------------------------------------------
// Name: LocalFoamMaps
// Type: Technique
// Desc: 
//-----------------------------------------------------------------------------
technique11 LocalFoamMapTech
{
	// Solid
	pass Pass_Solid
	{
		SetVertexShader( CompileShader( vs_5_0, ShiftFadeBlurLocalFoamVertexShader() ) );
		SetHullShader( NULL );
		SetDomainShader( NULL );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, ShiftFadeBlurLocalFoamPixelShader() ) );

		SetDepthStencilState( DisableDepth, 0 );
		SetRasterizerState( Solid );
		SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
}
