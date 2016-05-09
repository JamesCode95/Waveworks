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
#include "ocean_spray_common.h"
#include "atmospheric.fxh"
#include "shader_common.fxh"
#include "ocean_psm.fxh"

//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

float4x4	g_matProj;
float4x4	g_matView;
float4x4	g_matProjInv;
float3		g_LightDirection;
float3		g_LightColor;
float3		g_AmbientColor;
float		g_FogExponent;
float		g_InvParticleLifeTime;

float3		g_LightningPosition;
float3		g_LightningColor;
float		g_SimpleParticles;

int			g_LightsNum;
float4		g_SpotlightPosition[MaxNumSpotlights];
float4		g_SpotLightAxisAndCosAngle[MaxNumSpotlights];
float4		g_SpotlightColor[MaxNumSpotlights];

#if ENABLE_SHADOWS
float4x4    g_SpotlightMatrix[MaxNumSpotlights];
Texture2D   g_SpotlightResource[MaxNumSpotlights];
#endif

Buffer<float4> g_RenderInstanceData;
Buffer<float4> g_RenderOrientationAndDecimationData;
Buffer<float4> g_RenderVelocityAndTimeData;

float  g_PSMOpacityMultiplier;

// Data for GPU simulation
struct SprayParticleData
{
	float4 position_and_mass;
	float4 velocity_and_time;
};

int			g_ParticlesNum;
float		g_SimulationTime;
float3		g_WindSpeed;

// We use these for ensuring particles do not intersect ship
float4x4	g_worldToVessel;
float4x4	g_vesselToWorld;

// We use these to kill particles
float2		g_worldToHeightLookupScale;
float2		g_worldToHeightLookupOffset;
float2		g_worldToHeightLookupRot;
Texture2D   g_texHeightLookup;

// We use these to feed particles back into foam map
float4x4	g_matWorldToFoam;

static const float kVesselLength = 63.f;
static const float kVesselWidth = 9.f;
static const float kVesselDeckHeight = 0.f;
static const float kMaximumCollisionAcceleration = 10.f;
static const float kCollisionAccelerationRange = 0.5f;

static const float kSceneParticleTessFactor = 8.f;

struct DepthSortEntry {
	int ParticleIndex;
	float ViewZ;
};

StructuredBuffer <DepthSortEntry> g_ParticleDepthSortSRV;
StructuredBuffer<SprayParticleData> g_SprayParticleDataSRV;

RWStructuredBuffer <DepthSortEntry> g_ParticleDepthSortUAV			: register(u0);
AppendStructuredBuffer<SprayParticleData> g_SprayParticleData		: register(u1);

uint g_iDepthSortLevel;
uint g_iDepthSortLevelMask;
uint g_iDepthSortWidth;
uint g_iDepthSortHeight;

float4 g_AudioVisualizationRect; // litterbug
float2 g_AudioVisualizationMargin;
float g_AudioVisualizationLevel;

//------------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------------
static const float2 kParticleCornerCoords[4] = {
    {-1, 1},
    { 1, 1},
    {-1,-1},
    { 1,-1}
};

static const float3 kFoamColor = {0.5f, 0.5f, 0.5f};

//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
Texture2D	g_texSplash;

sampler g_SamplerTrilinearClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
	AddressV = Clamp;
};

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
struct VS_DUMMY_PARTICLE_OUTPUT
{
};

struct PARTICLE_INSTANCE_DATA {
	float4 position_and_mass			: PosMass;
	float3 orientation_and_decimation	: OriDec;
	float3 velocity						: Vel;
	float time							: T;
};

struct VS_SCENE_PARTICLE_OUTPUT {
	PARTICLE_INSTANCE_DATA InstanceData;
	float FogFactor               : FogFactor;
};

struct HS_PARTICLE_COORDS {
	float3 ViewPos;
	float3 TextureUVAndOpacity;
};

struct HS_SCENE_PARTICLE_OUTPUT {
	float3 ViewPos                : ViewPos;
    float3 TextureUVAndOpacity    : TEXCOORD0;
	// NOT USED float3 PSMCoords              : PSMCoords;
	float FogFactor               : FogFactor;
};

struct DS_SCENE_PARTICLE_OUTPUT {
	float4 Position               : SV_Position;
	float3 ViewPos                : ViewPos;
    float3 TextureUVAndOpacity    : TEXCOORD0;
	// NOT USED float3 PSMCoords              : PSMCoords;
	float FogFactor               : FogFactor;
	float3 Lighting               : LIGHTING;
};

struct HS_SCENE_PARTICLE_OUTPUT_CONST
{
	float fTessFactor[4]       : SV_TessFactor;
	float fInsideTessFactor[2] : SV_InsideTessFactor;
};

struct GS_FOAM_PARTICLE_OUTPUT {
	float4 Position               : SV_Position;
	float3 ViewPos                : ViewPos;
    float3 TextureUVAndOpacity    : TEXCOORD0;
	float  FoamAmount			  : FOAMAMOUNT;
};

struct GS_PSM_PARTICLE_OUTPUT
{
    float4 Position                      : SV_Position;
	nointerpolation uint LayerIndex      : SV_RenderTargetArrayIndex;
    float3 TextureUVAndOpacity           : TEXCOORD0;
	nointerpolation uint SubLayer        : TEXCOORD1;
};

//--------------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------------
PARTICLE_INSTANCE_DATA GetParticleInstanceData(in uint PrimID)
{
	uint particle_index = PrimID; // one day? - g_ParticleDepthSortSRV[PrimID].ParticleIndex;

	PARTICLE_INSTANCE_DATA result;
#if ENABLE_GPU_SIMULATION
#if SPRAY_PARTICLE_SORTING
	particle_index = g_ParticleDepthSortSRV[particle_index].ParticleIndex;
#endif
	SprayParticleData particleData = g_SprayParticleDataSRV[particle_index];
	result.position_and_mass = particleData.position_and_mass;
	result.orientation_and_decimation.xy = float2(1.0, 0.0);
	result.velocity = particleData.velocity_and_time.xyz;
	result.time = particleData.velocity_and_time.w;

	result.orientation_and_decimation.z = 1.f;
#else
	result.position_and_mass = g_RenderInstanceData.Load(particle_index);
	result.orientation_and_decimation = g_RenderOrientationAndDecimationData.Load(particle_index).xyz;
	result.velocity = 0;
#endif

	return result;
}

float CalcVelocityScale(float speed)
{
	return log2(speed * 0.2f + 2.0f);
}

float CalcTimeScale(float time)
{
	return 0.5+0.5*time;
}

void CalcParticleCoords( PARTICLE_INSTANCE_DATA InstanceData, in float2 CornerCoord, out float3 ViewPos, out float2 UV, out float Opacity)
{
	// Transform to camera space
	ViewPos = mul(float4(InstanceData.position_and_mass.xyz,1), g_matView).xyz;

	float2 coords = CornerCoord*CalcTimeScale(InstanceData.time);
	coords *= 0.7f; // Make particles a little smaller to keep the look crisp
	
	float3 velocityView = mul(InstanceData.velocity.xyz, (float3x3)g_matView).xyz;
	float velocityScale = CalcVelocityScale(length(velocityView.xy));
	coords.x /= (velocityScale * 0.25f + 0.75f);
	coords.y *= velocityScale;

	float angle = atan2(velocityView.x,velocityView.y);
	
	float2 orientation = float2(cos(angle), sin(angle));//InstanceData.orientation_and_decimation.xy;
	float2 rotatedCornerCoord;
	rotatedCornerCoord.x =  coords.x * orientation.x + coords.y * orientation.y;
	rotatedCornerCoord.y = -coords.x * orientation.y + coords.y * orientation.x;

	// Inflate corners, applying scale from instance data
	ViewPos.xy += g_SimpleParticles>0?CornerCoord*0.02:rotatedCornerCoord;

	const float mass = InstanceData.position_and_mass.w;
	float cosAngle = cos(mass * 8.0f);
	float sinAngle = sin(mass * 8.0f);

	float2 rotatedUV = CornerCoord;
	UV.x = rotatedUV.x * cosAngle + rotatedUV.y * sinAngle;
	UV.y =-rotatedUV.x * sinAngle + rotatedUV.y * cosAngle;

	UV = 0.5f * (UV + 1.f);

	Opacity = InstanceData.orientation_and_decimation.z;
}

HS_PARTICLE_COORDS CalcParticleCoords(in PARTICLE_INSTANCE_DATA InstanceData, int i)
{
	HS_PARTICLE_COORDS result;
	CalcParticleCoords( InstanceData, kParticleCornerCoords[i], result.ViewPos, result.TextureUVAndOpacity.xy, result.TextureUVAndOpacity.z);
	return result;
}

float4 GetParticleRGBA(SamplerState s, float2 uv, float alphaMult)
{
	float4 splash = g_texSplash.SampleBias(s, uv, -1.0);
	float alpha_threshold = 1.0 - alphaMult * 0.6;
	clip(splash.r-alpha_threshold);
	return float4(kFoamColor, 1.0);
}

VS_DUMMY_PARTICLE_OUTPUT DummyVS( )
{
    VS_DUMMY_PARTICLE_OUTPUT Output;
    return Output;
}

VS_SCENE_PARTICLE_OUTPUT RenderParticlesToSceneVS(in uint vID : SV_VertexID)
{
    VS_SCENE_PARTICLE_OUTPUT Output;
	Output.InstanceData = GetParticleInstanceData(vID);
	float3 CentreViewPos = mul(float4(Output.InstanceData.position_and_mass.xyz,1), g_matView).xyz;
	Output.FogFactor = exp(dot(CentreViewPos,CentreViewPos)*g_FogExponent);
    return Output;
}


HS_SCENE_PARTICLE_OUTPUT_CONST RenderParticlesToSceneHS_Constant(InputPatch<VS_SCENE_PARTICLE_OUTPUT, 1> I)
{
	HS_SCENE_PARTICLE_OUTPUT_CONST O;
	O.fTessFactor[0] = kSceneParticleTessFactor;
	O.fTessFactor[1] = kSceneParticleTessFactor;
	O.fTessFactor[2] = kSceneParticleTessFactor;
	O.fTessFactor[3] = kSceneParticleTessFactor;
	O.fInsideTessFactor[0] = kSceneParticleTessFactor;
	O.fInsideTessFactor[1] = kSceneParticleTessFactor;
	return O;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[patchconstantfunc("RenderParticlesToSceneHS_Constant")]
[outputcontrolpoints(4)]
HS_SCENE_PARTICLE_OUTPUT RenderParticlesToSceneHS( InputPatch<VS_SCENE_PARTICLE_OUTPUT, 1> I, uint uCPID : SV_OutputControlPointID )
{
	HS_PARTICLE_COORDS particleCoords = CalcParticleCoords(I[0].InstanceData,uCPID);

	// NOT USED float4 PSMCoords = mul(float4(particleCoords.ViewPos,1.f), g_matViewToPSM);

	HS_SCENE_PARTICLE_OUTPUT outvert;
	outvert.TextureUVAndOpacity = particleCoords.TextureUVAndOpacity;
	outvert.ViewPos = particleCoords.ViewPos;
	// NOT USED outvert.PSMCoords = float3(PSMCoords.xyz);
	outvert.FogFactor = I[0].FogFactor;

	return outvert;
}

#define QUAD_INTERP(member) (f0*I[0].member + f1*I[1].member + f2*I[2].member + f3*I[3].member)

[domain("quad")]
DS_SCENE_PARTICLE_OUTPUT RenderParticlesToSceneDS( HS_SCENE_PARTICLE_OUTPUT_CONST HSConstantData, const OutputPatch<HS_SCENE_PARTICLE_OUTPUT, 4> I, float2 f2BilerpCoords : SV_DomainLocation )
{
	float f0 = f2BilerpCoords.x * f2BilerpCoords.y;
	float f1 = (1.f-f2BilerpCoords.x) * f2BilerpCoords.y;
	float f2 = f2BilerpCoords.x * (1.f-f2BilerpCoords.y);
	float f3 = (1.f-f2BilerpCoords.x) * (1.f-f2BilerpCoords.y);

	DS_SCENE_PARTICLE_OUTPUT outvert;
	outvert.ViewPos = QUAD_INTERP(ViewPos); 
	outvert.TextureUVAndOpacity = QUAD_INTERP(TextureUVAndOpacity);
	outvert.FogFactor = QUAD_INTERP(FogFactor);
	outvert.Position = mul(float4(outvert.ViewPos,1.f), g_matProj);

	////////////////////////////////////////////////////////////////////////////////
	// Lighting calcs hoisted from PS from here...
	////////////////////////////////////////////////////////////////////////////////

	// randomize ppsition in view space to reduce shading banding
	float displacementScale = RND_1d(outvert.TextureUVAndOpacity.xy / 10);
	outvert.ViewPos.z += (displacementScale * 2.0f - 1.0f) * 0.25f;

	// Fake ship AO
	float3 vessel_pos = mul(float4(outvert.ViewPos,1),g_worldToVessel).xyz;
	float ao_profile = 0.5f*kVesselWidth*saturate((0.5f*kVesselLength-abs(vessel_pos.z))/(kVesselWidth));
	float ao_horizontal_range = abs(vessel_pos.x) - ao_profile;
	float ao_vertical_range = vessel_pos.y;
	float ao = 0.2f+ 0.8f*(1.f -saturate(1.f-ao_horizontal_range/5.f)* saturate(1.f-ao_vertical_range/5.f));

	float3 dynamic_lighting = ao*(g_LightColor + g_AmbientColor*2.0) + g_LightningColor;

	[unroll]
	for(int ix = 0; ix != g_LightsNum; ++ix) {
		float3 pixel_to_light = g_SpotlightPosition[ix].xyz - outvert.ViewPos;
		float3 pixel_to_light_nml = normalize(pixel_to_light);
		float beam_attn = saturate(1.f*(-dot(g_SpotLightAxisAndCosAngle[ix].xyz,pixel_to_light_nml)-g_SpotLightAxisAndCosAngle[ix].w)/(1.f-g_SpotLightAxisAndCosAngle[ix].w));
		beam_attn *= 1.f/dot(pixel_to_light,pixel_to_light);
		float shadow = 1.0f;
#if ENABLE_SHADOWS
		if (beam_attn * dot(g_SpotlightColor[ix].xyz, g_SpotlightColor[ix].xyz) > 0.01f)
		{
			shadow = GetShadowValue(g_SpotlightResource[ix], g_SpotlightMatrix[ix], outvert.ViewPos.xyz, true);
		}
#endif
		dynamic_lighting += beam_attn * g_SpotlightColor[ix].xyz * shadow;
	}

	outvert.Lighting = dynamic_lighting;

	return outvert;
}

#define USE_DOWNSAMPLING 0

#if USE_DOWNSAMPLING
Texture2D<float> g_texDepth;
#else
Texture2DMS<float> g_texDepth;
#endif

[earlydepthstencil]
float4 RenderParticlesToScenePS(DS_SCENE_PARTICLE_OUTPUT In) : SV_Target
{
	if(g_SimpleParticles>0)
	{
		return float4(0.5,0.0,0.0,1.0);
	}
	
	// disable PSM on spray dynamic_lighting *= CalcPSMShadowFactor(In.PSMCoords);
	float4 result = GetParticleRGBA(g_SamplerTrilinearClamp, In.TextureUVAndOpacity.xy, In.TextureUVAndOpacity.z);

	result.rgb *= In.Lighting;

	result.rgb = lerp(g_AmbientColor + g_LightningColor,result.rgb,In.FogFactor);
	result.a = 0.125 * In.TextureUVAndOpacity.z;

#if USE_DOWNSAMPLING
	float depth = g_texDepth[(int2)In.Position.xy]; // coarse depth
#else
	float depth = g_texDepth.Load((int2)In.Position.xy, 0); // fine depth
#endif

	float4 clipPos = float4(0, 0, depth, 1.0);
	float4 viewSpace = mul(clipPos, g_matProjInv);
	viewSpace.z /= viewSpace.w;

	result.a *= saturate(abs(In.ViewPos.z - viewSpace.z) * 0.5);

	return result;
}

[maxvertexcount(4)]
void RenderParticlesToPSMGS( in point VS_DUMMY_PARTICLE_OUTPUT dummy[1], in uint PrimID : SV_PrimitiveID, inout TriangleStream<GS_PSM_PARTICLE_OUTPUT> outstream )
{
	PARTICLE_INSTANCE_DATA InstanceData = GetParticleInstanceData(PrimID);
    float3 CentreViewPos = mul(float4(InstanceData.position_and_mass.xyz,1), g_matView).xyz;

	// Dispatch to PSM layer-slice
    float linearZ = mul(float4(CentreViewPos,1.f), g_matProj).z;
	float slice = g_PSMSlices * linearZ + 1.f;	// +1 because zero slice reserved for coverage
	uint sublayer = 2.f * frac(slice);

	if(slice < 0.f || slice > g_PSMSlices)
		return;

	slice = floor(slice);

	[unroll]
	for(uint i = 0; i != 4; ++i) {

		HS_PARTICLE_COORDS particleCoords = CalcParticleCoords(InstanceData,i);

		GS_PSM_PARTICLE_OUTPUT outvert;
		outvert.TextureUVAndOpacity = particleCoords.TextureUVAndOpacity;
		outvert.Position = mul(float4(particleCoords.ViewPos,1.f), g_matProj);
		outvert.SubLayer = sublayer;
		outvert.LayerIndex = slice;
		outstream.Append(outvert);
	}
	outstream.RestartStrip();
}

float4 RenderParticlesToPSMPS( GS_PSM_PARTICLE_OUTPUT In) : SV_Target
{ 
    float4 tex = GetParticleRGBA(g_SamplerTrilinearClamp,In.TextureUVAndOpacity.xy,In.TextureUVAndOpacity.z);

    float4 Output = tex.a;
	Output *= g_PSMOpacityMultiplier;
	if(In.SubLayer == 0)
		Output *= float4(1,0,0,0);
	else if(In.SubLayer == 1)
		Output *= float4(0,1,0,0);
	else if(In.SubLayer == 2)
		Output *= float4(0,0,1,0);
	else
		Output *= float4(0,0,0,1);

    return Output;
}

float2 rotate_2d(float2 v, float2 rot)
{
	return float2(v.x * rot.x + v.y * rot.y, v.x * -rot.y + v.y * rot.x);
}

[maxvertexcount(4)]
void RenderParticlesToFoamGS( in point VS_DUMMY_PARTICLE_OUTPUT dummy[1], in uint PrimID : SV_PrimitiveID, inout TriangleStream<GS_FOAM_PARTICLE_OUTPUT> outstream )
{
	PARTICLE_INSTANCE_DATA InstanceData = GetParticleInstanceData(PrimID);

	// Only downward-moving particles are eligible for foam injection
	if(InstanceData.velocity.y > 0.f)
		return;

	// Particles that fall outside our lookup cannot be rendered to foam, because we have no way of knowing when they hit the sea surface
	float2 lookup_uv = rotate_2d(InstanceData.position_and_mass.xz+g_worldToHeightLookupOffset,g_worldToHeightLookupRot)*g_worldToHeightLookupScale;
	if(lookup_uv.x < 0.f || lookup_uv.y < 0.f || lookup_uv.x > 1.f || lookup_uv.y > 1.f)
		return;

	float particle_scale = CalcTimeScale(InstanceData.time);
	float3 lookup_value = g_texHeightLookup.SampleLevel(g_SamplerTrilinearClamp,lookup_uv,0.f).xyz;
	float height_of_centre_above_water = InstanceData.position_and_mass.y-lookup_value.z;
	if( height_of_centre_above_water < -particle_scale || height_of_centre_above_water > particle_scale)
		return;	// No intersection

	// The proportion of foam applied depends on how much foam is falling through the surface on this update
	// Note that this does not take account of the normal or speed of the water surface, which would be
	// necessary for perfection here...
	float relative_penetration = 0.15f*(-InstanceData.velocity.y * g_SimulationTime)/particle_scale;

	[unroll]
	for(uint i = 0; i != 4; ++i) {

		HS_PARTICLE_COORDS particleCoords = CalcParticleCoords(InstanceData,i);

		// Lookup undisplaced coords
		float2 world_xy = float2(particleCoords.ViewPos.xy);
		// DISABLED: use world space float2 corner_lookup_uv = rotate_2d(world_xy+g_worldToHeightLookupOffset,g_worldToHeightLookupRot)*g_worldToHeightLookupScale;
		// DISABLED: use world space world_xy = g_texHeightLookup.SampleLevel(g_SamplerTrilinearClamp,corner_lookup_uv,0.f).xy;

		float2 clip_xy = mul(float4(world_xy.x,0.0,world_xy.y,1.0),g_matWorldToFoam).xz;
		clip_xy *= g_WakeTexScale;
		clip_xy += g_WakeTexOffset;
		clip_xy *= 2.f;
		clip_xy -= 1.f;

		GS_FOAM_PARTICLE_OUTPUT outvert;
		outvert.TextureUVAndOpacity = particleCoords.TextureUVAndOpacity;
		outvert.Position = float4(clip_xy,0.5f,1.f);
		outvert.ViewPos = particleCoords.ViewPos;
		outvert.FoamAmount = relative_penetration;
		outstream.Append(outvert);
	}
	outstream.RestartStrip();
}

float4 RenderParticlesToFoamPS( GS_FOAM_PARTICLE_OUTPUT In) : SV_Target
{
	return In.FoamAmount * GetParticleRGBA(g_SamplerTrilinearClamp, In.TextureUVAndOpacity.xy, In.TextureUVAndOpacity.z).a;
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

DepthStencilState ReadDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ZERO;
    DepthFunc = LESS_EQUAL;
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

BlendState Translucent
{
	BlendEnable[0] = TRUE;
	RenderTargetWriteMask[0] = 0xF;

	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
	BlendOp = Add;

	SrcBlendAlpha = ZERO;
	DestBlendAlpha = INV_SRC_ALPHA;
	BlendOpAlpha = Add;
};

BlendState AddBlend
{
	BlendEnable[0] = TRUE;
	RenderTargetWriteMask[0] = 0xF;

	SrcBlend = ONE;
	DestBlend = ONE;
	BlendOp = Add;
};

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
technique10 RenderSprayToSceneTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_5_0, RenderParticlesToSceneVS() ) );
        SetGeometryShader( NULL );
        SetHullShader( CompileShader( hs_5_0, RenderParticlesToSceneHS() ) );
        SetDomainShader( CompileShader( ds_5_0, RenderParticlesToSceneDS() ) );
        SetPixelShader( CompileShader( ps_5_0, RenderParticlesToScenePS() ) );

        SetDepthStencilState( ReadDepth, 0 );
        SetRasterizerState( SolidNoCull );
        SetBlendState( Translucent, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
 
}

technique10 RenderSprayToFoamTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, DummyVS() ) );
        SetGeometryShader( CompileShader( gs_4_0, RenderParticlesToFoamGS() ) );
        SetHullShader( NULL );
        SetDomainShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, RenderParticlesToFoamPS() ) );

        SetDepthStencilState( NoDepthStencil, 0 );
        SetRasterizerState( SolidNoCull );
        SetBlendState( AddBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique11 RenderSprayToPSMTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, DummyVS() ) );
        SetGeometryShader( CompileShader( gs_4_0, RenderParticlesToPSMGS() ) );
        SetHullShader( NULL );
        SetDomainShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, RenderParticlesToPSMPS() ) );

        SetDepthStencilState( NoDepthStencil, 0 );
        SetRasterizerState( SolidNoCull );
        SetBlendState( PSMBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

//--------------------------------------------------------------------------------------
// Compute shaders
//--------------------------------------------------------------------------------------

cbuffer DispatchArguments : register(b2)
{
	uint g_ParticleCount;
}

float3 CollisionAvoidanceAcceleration(float3 position)
{
	float3 vessel_pos = mul(float4(position,1),g_worldToVessel).xyz;
	float3 accel_vessel_direction = 0.f;
	float accel_magnitude = 0.f;

	// Apply at all heights
	{
		float collison_profile = 0.5f*kVesselWidth*saturate((0.5f*kVesselLength-vessel_pos.z)/(kVesselWidth));
		float rear_exclude = (vessel_pos.z < -20.f && vessel_pos.y > 0.f) ? 0.f : 1.f;
		float range = abs(vessel_pos.x) - collison_profile;
		float range_mult = saturate(1.f-range/kCollisionAccelerationRange);
		accel_magnitude = kMaximumCollisionAcceleration * range_mult * rear_exclude;
		accel_vessel_direction = float3(vessel_pos.x/abs(vessel_pos.x),0,0);
	}

	return accel_magnitude * mul(accel_vessel_direction,(float3x3)g_vesselToWorld);
}

[numthreads(SprayParticlesCSBlocksSize, 1, 1)]
void InitSprayParticlesCS(uint3 globalIdx : SV_DispatchThreadID)
{
	uint particleID = globalIdx.x;

	if (particleID >= g_ParticlesNum) return;

	SprayParticleData particleData;
	particleData.position_and_mass = g_RenderInstanceData.Load(particleID);
	particleData.velocity_and_time = g_RenderVelocityAndTimeData.Load(particleID);

	particleData.velocity_and_time.w = 0;

	g_SprayParticleData.Append(particleData);
}

[numthreads(SprayParticlesCSBlocksSize, 1, 1)]
void SimulateSprayParticlesCS(uint3 globalIdx : SV_DispatchThreadID)
{
	uint particleID = globalIdx.x;

	if (particleID >= min(g_ParticleCount, SPRAY_PARTICLE_COUNT)) return;

	SprayParticleData particleData = g_SprayParticleDataSRV[particleID];

	int num_steps = ceil(g_SimulationTime / kMaxSimulationTimeStep);
	num_steps = min(num_steps, 3);
	float time_step = g_SimulationTime / float(num_steps);

	// kill particles by life-time
	if (particleData.velocity_and_time.w + time_step * num_steps > kParticleTTL) return;

	// Kill rules:
	//  1/ particle must be heading down
	//  2/ particle must be its own size inside the surface
	if(particleData.velocity_and_time.y < 0.f) {
		float2 lookup_uv = rotate_2d(particleData.position_and_mass.xz+g_worldToHeightLookupOffset,g_worldToHeightLookupRot)*g_worldToHeightLookupScale;
		if(lookup_uv.x > 0.f && lookup_uv.y > 0.f && lookup_uv.x < 1.f && lookup_uv.y < 1.f) {
			float3 lookup_value = g_texHeightLookup.SampleLevel(g_SamplerTrilinearClamp,lookup_uv,0.f).xyz;
			float depth_of_centre_below = lookup_value.z - particleData.position_and_mass.y;
			float conservative_velocity_scale = CalcVelocityScale(length(particleData.velocity_and_time.xyz)) * CalcTimeScale(particleData.velocity_and_time.w);
			if(depth_of_centre_below > conservative_velocity_scale) {
				return;	// KILLED!
			}
		}
	}

	// updating particle times
	particleData.velocity_and_time.w += time_step * num_steps;
	for(int step = 0; step != num_steps; ++step)
	{
		// updating spray particles positions
		float3 positionDelta = particleData.velocity_and_time.xyz * time_step;
		particleData.position_and_mass.xyz += positionDelta;

		// updating spray particles speeds
		float3 accel = -kParticleDrag * (particleData.velocity_and_time.xyz - g_WindSpeed)/particleData.position_and_mass.w;
		accel.y -= kGravity;//*1.1*particleData.position_and_mass.w;
		accel += CollisionAvoidanceAcceleration(particleData.position_and_mass.xyz);
		particleData.velocity_and_time.xyz += accel * time_step;
	}

	g_SprayParticleData.Append(particleData);
}

[numthreads(SprayParticlesCSBlocksSize, 1, 1)]
void InitSortCS(uint3 globalIdx : SV_DispatchThreadID)
{
	uint particle_index = globalIdx.x;

	SprayParticleData particleData = g_SprayParticleDataSRV[particle_index];
	float view_z = max(0,mul(float4(particleData.position_and_mass.xyz,1), g_matView).z);

	if (particle_index >= g_ParticleCount) view_z = 0;

	g_ParticleDepthSortUAV[particle_index].ParticleIndex = particle_index;
	g_ParticleDepthSortUAV[particle_index].ViewZ = view_z;
}

RWBuffer<uint4> u_DispatchArgumentsBuffer : register(u0);

[numthreads(1, 1, 1)]
void DispatchArgumentsCS(uint3 globalIdx : SV_DispatchThreadID)
{
	uint blocksNum = ceil((float)g_ParticleCount / SimulateSprayParticlesCSBlocksSize);

	u_DispatchArgumentsBuffer[0] = uint4(blocksNum, 1, 1, 0);
}

groupshared DepthSortEntry shared_data[BitonicSortCSBlockSize];

[numthreads(BitonicSortCSBlockSize, 1, 1)]
void BitonicSortCS(	uint3 Gid : SV_GroupID,
					uint3 DTid : SV_DispatchThreadID,
					uint3 GTid : SV_GroupThreadID,
					uint GI : SV_GroupIndex )
{
	// Load shared data
	shared_data[GI] = g_ParticleDepthSortUAV[DTid.x];
	GroupMemoryBarrierWithGroupSync();

	// Sort the shared data
	for (unsigned int j = g_iDepthSortLevel >> 1 ; j > 0 ; j >>= 1)
	{
		DepthSortEntry result;
		if((bool)(shared_data[GI & ~j].ViewZ > shared_data[GI | j].ViewZ) == (bool)(g_iDepthSortLevelMask & DTid.x))
			result = shared_data[GI ^ j];
		else
			result = shared_data[GI];
		GroupMemoryBarrierWithGroupSync();
		shared_data[GI] = result;
		GroupMemoryBarrierWithGroupSync();
	}

	// Store shared data
	g_ParticleDepthSortUAV[DTid.x] = shared_data[GI];
}

groupshared DepthSortEntry transpose_shared_data[TransposeCSBlockSize * TransposeCSBlockSize];

[numthreads(TransposeCSBlockSize, TransposeCSBlockSize, 1)]
void MatrixTransposeCS(	uint3 Gid : SV_GroupID,
						uint3 DTid : SV_DispatchThreadID,
						uint3 GTid : SV_GroupThreadID,
						uint GI : SV_GroupIndex )
{
	transpose_shared_data[GI] = g_ParticleDepthSortSRV[DTid.y * g_iDepthSortWidth + DTid.x];
	GroupMemoryBarrierWithGroupSync();
	uint2 XY = DTid.yx - GTid.yx + GTid.xy;
	g_ParticleDepthSortUAV[XY.y * g_iDepthSortHeight + XY.x] = transpose_shared_data[GTid.x * TransposeCSBlockSize + GTid.y];
}

technique11 InitSprayParticles
{
	pass
	{
		SetVertexShader( NULL );
		SetGeometryShader( NULL );
		SetHullShader( NULL );
		SetDomainShader( NULL );
		SetPixelShader( NULL );

		SetComputeShader( CompileShader( cs_5_0, InitSprayParticlesCS() ) );
	}
}

technique11 SimulateSprayParticles
{
	pass
	{
		SetVertexShader( NULL );
		SetGeometryShader( NULL );
		SetHullShader( NULL );
		SetDomainShader( NULL );
		SetPixelShader( NULL );

		SetComputeShader( CompileShader( cs_5_0, SimulateSprayParticlesCS() ) );
	}
}

technique11 PrepareDispatchArguments
{
	pass
	{
		SetVertexShader( NULL );
		SetGeometryShader( NULL );
		SetHullShader( NULL );
		SetDomainShader( NULL );
		SetPixelShader( NULL );

		SetComputeShader( CompileShader( cs_5_0, DispatchArgumentsCS() ) );
	}
}

technique11 InitSortTech
{
	pass
	{
		SetVertexShader( NULL );
		SetGeometryShader( NULL );
		SetHullShader( NULL );
		SetDomainShader( NULL );
		SetPixelShader( NULL );

		SetComputeShader( CompileShader( cs_5_0, InitSortCS() ) );
	}
}

technique11 BitonicSortTech
{
	pass
	{
		SetVertexShader( NULL );
		SetGeometryShader( NULL );
		SetHullShader( NULL );
		SetDomainShader( NULL );
		SetPixelShader( NULL );
		SetComputeShader( CompileShader( cs_5_0, BitonicSortCS() ) );
	}
}

technique11 MatrixTransposeTech
{
	pass
	{
		SetVertexShader( NULL );
		SetGeometryShader( NULL );
		SetHullShader( NULL );
		SetDomainShader( NULL );
		SetPixelShader( NULL );
		SetComputeShader( CompileShader( cs_5_0, MatrixTransposeCS() ) );
	}
}

//--------------------------------------------------------------------------------------
// Sensor visualization shaders
//--------------------------------------------------------------------------------------

RasterizerState Solid
{
	FillMode = SOLID;
	CullMode = NONE;

	MultisampleEnable = True;
};

struct SENSOR_VISUALIZATION_VS_OUT
{
	float4 position : SV_Position;
	float3 color : SENSOR_COLOR;
	float2 uv : TEXCOORD;
};

SENSOR_VISUALIZATION_VS_OUT SensorVisualizationVS(float3 vPos : POSITION, float intensity : INTENSITY, float2 uv : TEXCOORD)
{
	SENSOR_VISUALIZATION_VS_OUT output;
	float4x4 matViewProj = mul(g_matView,g_matProj);
	float c_intensity = abs(intensity*0.7)+0.3;
	output.position =  mul(float4(vPos,1.0), matViewProj);
	output.color = intensity>0?float3(c_intensity,c_intensity,c_intensity) : float3(c_intensity,c_intensity,0);
	output.uv = uv;
	return output;
}

float4 SensorVisualizationPS( SENSOR_VISUALIZATION_VS_OUT i) : SV_Target
{
	float r = length(i.uv - float2(0.5,0.5));
	clip(0.5f-r);
	return float4(i.color,1.f);
}

//--------------------------------------------------------------------------------------
// Sensor visualization technique
//--------------------------------------------------------------------------------------

technique10 SensorVisualizationTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, SensorVisualizationVS() ) );
        SetGeometryShader( NULL );
        SetHullShader( NULL );
        SetDomainShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, SensorVisualizationPS() ) );
        
        SetDepthStencilState( EnableDepth, 0 );
		SetRasterizerState( Solid );
		SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
   }
}

//--------------------------------------------------------------------------------------
// Audio trigger visualization
//--------------------------------------------------------------------------------------

static const float2 kAudioVizCornerCoords[6] = {
    { 0, 0},
    { 1, 1},
    { 1, 0},
    { 0, 0},
	{ 0, 1},
	{ 1, 1}
};


struct AUDIO_VISUALIZATION_VS_OUT
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD;
	float element_index : ELIX;
};

AUDIO_VISUALIZATION_VS_OUT AudioVisualizationVS(uint vID : SV_VertexID)
{
	AUDIO_VISUALIZATION_VS_OUT Out;
	Out.uv = kAudioVizCornerCoords[vID%6];
	float4 element_rect = g_AudioVisualizationRect;
	if(vID<6) {
		// Background element
		Out.element_index = 0.f;
	} else {
		// Level meter element
		Out.element_index = 1.f;
		element_rect.x += g_AudioVisualizationMargin.x;
		element_rect.y -= g_AudioVisualizationMargin.y;
		element_rect.z -= g_AudioVisualizationMargin.x;
		element_rect.w += g_AudioVisualizationMargin.y;
		Out.uv.y *= g_AudioVisualizationLevel;
	}

	Out.position = float4(	element_rect.x + (element_rect.z-element_rect.x)*Out.uv.x,
							element_rect.w - (element_rect.w-element_rect.y)*Out.uv.y,
							0.5f,
							1.f
							);

	return Out;
}

float4 AudioVisualizationPS( AUDIO_VISUALIZATION_VS_OUT i) : SV_Target
{
	float sin_pattern = 0.5f+0.5f*sin(400.f*i.uv.y);
	return float4(	i.element_index * (i.uv.y >  0.5f ? sin_pattern : 0.f),
					i.element_index * (i.uv.y <= 0.5f ? sin_pattern : 0.f),
					0.f,
					0.5f+0.5f*i.element_index);
}

technique10 AudioVisualizationTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, AudioVisualizationVS() ) );
        SetGeometryShader( NULL );
        SetHullShader( NULL );
        SetDomainShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, AudioVisualizationPS() ) );
        
        SetDepthStencilState( NoDepthStencil, 0 );
		SetRasterizerState( SolidNoCull );
		SetBlendState( Translucent, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
   }
}