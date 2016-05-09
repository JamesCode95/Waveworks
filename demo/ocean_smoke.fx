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

#include "ocean_psm.fxh"
#include "inoise.fxh"

//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

float4x4	g_matProj;
float4x4	g_matView;
float3		g_LightDirection;
float3		g_LightColor;
float3		g_AmbientColor;
float		g_FogExponent;
float2		g_ParticleBeginEndScale;
float		g_InvParticleLifeTime;
float		g_NoiseTime;
Buffer<float4> g_RenderInstanceData;

float3		g_LightningPosition;
float3		g_LightningColor;

uint g_ParticleIndexOffset;
uint g_ParticleCount;
float g_TimeStep;
float g_PreRollEndTime;

RWBuffer<float> g_SimulationInstanceData;
RWBuffer<float> g_SimulationVelocities;
float4x3 g_CurrEmitterMatrix;
float4x3 g_PrevEmitterMatrix;
float2 g_EmitAreaScale;
float3 g_EmitMinMaxVelocityAndSpread;
float2 g_EmitInterpScaleAndOffset;
float4 g_WindVectorAndNoiseMult;
float3 g_BuoyancyParams;
float g_WindDrag;

float g_NoiseSpatialScale;
float g_NoiseTimeScale;

Buffer<float2> g_RandomUV;
uint g_RandomOffset;

float  g_PSMOpacityMultiplier;
float  g_PSMFadeMargin;

struct DepthSortEntry {
	int ParticleIndex;
	float ViewZ;
};

RWStructuredBuffer <DepthSortEntry> g_ParticleDepthSortUAV;
StructuredBuffer <DepthSortEntry> g_ParticleDepthSortSRV;

uint g_iDepthSortLevel;
uint g_iDepthSortLevelMask;
uint g_iDepthSortWidth;
uint g_iDepthSortHeight;

//------------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------------
static const float2 kParticleCornerCoords[4] = {
    {-1, 1},
    { 1, 1},
    {-1,-1},
    { 1,-1}
};

static const float PI = 3.141592654f;

//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
texture2D g_texDiffuse;
sampler g_samplerDiffuse = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

//--------------------------------------------------------------------------------------
// DepthStates
//--------------------------------------------------------------------------------------
DepthStencilState ReadOnlyDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ZERO;
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

//--------------------------------------------------------------------------------------
// BlendStates
//--------------------------------------------------------------------------------------
BlendState TranslucentBlendRGB
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

BlendState Opaque
{
	BlendEnable[0] = FALSE;
	RenderTargetWriteMask[0] = 0xF;
};

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
struct VS_DUMMY_PARTICLE_OUTPUT
{
};

struct GS_SCENE_PARTICLE_OUTPUT
{
    float4 Position               : SV_Position;
    float3 TextureUVAndOpacity    : TEXCOORD0;
	float3 PSMCoords              : PSMCoords;
	float  FogFactor              : FogFactor;
};

struct GS_PSM_PARTICLE_OUTPUT
{
    float4 Position                      : SV_Position;
	nointerpolation uint LayerIndex      : SV_RenderTargetArrayIndex;
    float3 TextureUVAndOpacity           : TEXCOORD0;
	nointerpolation uint SubLayer        : TEXCOORD1;
};

struct GS_PARTICLE_COORDS {
	float3 ViewPos;
	float3 TextureUVAndOpacity;
};

//--------------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------------
float4 GetParticleInstanceData(in uint PrimID)
{
	uint particle_index = g_ParticleDepthSortSRV[PrimID].ParticleIndex;
    return g_RenderInstanceData.Load(particle_index);
}

void CalcParticleCoords( float4 InstanceData, in float2 CornerCoord, out float3 ViewPos, out float2 UV, out float Opacity)
{
	const float life_param = g_InvParticleLifeTime * InstanceData.w;

    // Transform to camera space
    ViewPos = mul(float4(InstanceData.xyz,1), g_matView).xyz;

    // Inflate corners, applying scale from instance data
	const float scale = lerp(g_ParticleBeginEndScale.x,g_ParticleBeginEndScale.y,life_param);
    ViewPos.xy += CornerCoord * scale;

    UV = 0.5f * (CornerCoord + 1.f);
    
    Opacity = 1.f - life_param * life_param;
}

GS_PARTICLE_COORDS CalcParticleCoords(in float4 InstanceData, int i)
{
	GS_PARTICLE_COORDS result;
	CalcParticleCoords( InstanceData, kParticleCornerCoords[i], result.ViewPos, result.TextureUVAndOpacity.xy, result.TextureUVAndOpacity.z);
	return result;
}

float4 GetParticleRGBA(SamplerState s, float2 uv, float alphaMult)
{
    const float base_alpha = 16.f/255.f;

    float4 raw_tex = g_texDiffuse.Sample(s, uv);
    raw_tex.a = saturate((raw_tex.a - base_alpha)/(1.f - base_alpha));
    raw_tex.a *= alphaMult * 0.3f;
    return raw_tex;
}

float noise_3_octave(float4 pos_time)
{
	return inoise(pos_time) + 0.5f * inoise(2.f*pos_time) + 0.25f * inoise(4.f*pos_time);
}

static const float4 noise_r_offset = float4(0.01,0.02,0.03,0.04);
static const float4 noise_g_offset = float4(0.05,0.06,0.07,0.08);
static const float4 noise_b_offset = float4(0.09,0.10,0.11,0.12);

float3 xyz_noise_3_octave(float4 pos_time)
{
	float3 result;
	result.x = noise_3_octave(pos_time + noise_r_offset);
	result.y = noise_3_octave(pos_time + noise_g_offset);
	result.z = noise_3_octave(pos_time + noise_b_offset);
	return result;
}

float3 wind_potential(float4 pos_time)
{
	float4 noise_coord;
	noise_coord.xyz = pos_time.xyz * g_NoiseSpatialScale;
	noise_coord.w = pos_time.w * g_NoiseTimeScale;
	float3 noise_component = g_WindVectorAndNoiseMult.w * xyz_noise_3_octave(noise_coord);
	float3 gross_wind_component = -cross(pos_time.xyz,g_WindVectorAndNoiseMult.xyz);
	return noise_component + gross_wind_component;
}

float3 wind_function(float4 pos_time)
{
	const float delta = 0.001f;
	float4 dx = float4(delta,0.f,0.f,0.f);
	float4 dy = float4(0.f,delta,0.f,0.f);
	float4 dz = float4(0.f,0.f,delta,0.f);

	float3 dx_pos = wind_potential(pos_time + dx);
	float3 dx_neg = wind_potential(pos_time - dx);

	float3 dy_pos = wind_potential(pos_time + dy);
	float3 dy_neg = wind_potential(pos_time - dy);

	float3 dz_pos = wind_potential(pos_time + dz);
	float3 dz_neg = wind_potential(pos_time - dz);

	float x = dy_pos.z - dy_neg.z - dz_pos.y + dz_neg.y;
	float y = dz_pos.x - dz_neg.x - dx_pos.z + dx_neg.z;
	float z = dx_pos.y - dx_neg.y - dy_pos.x + dy_neg.x;

	return float3(x,y,z)/(2.f*delta);
}

void simulate(inout float4 instance_data, inout float4 velocity, float elapsed_time)
{
	float3 wind_velocity = wind_function(float4(instance_data.xyz,g_NoiseTime));

	instance_data.xyz += elapsed_time * velocity.xyz;
	instance_data.w += elapsed_time;

	float3 relative_velocity = velocity.xyz - wind_velocity;
	float3 accel = -relative_velocity * g_WindDrag;
	accel.y += velocity.w;	// buoyancy
	velocity.xyz += accel * elapsed_time;

	// Reduce buoyancy with heat loss
	velocity.w *= exp(g_BuoyancyParams.z * elapsed_time);
}

VS_DUMMY_PARTICLE_OUTPUT DummyVS( )
{
    VS_DUMMY_PARTICLE_OUTPUT Output;
    return Output;
}

float CalcPSMFadeFactor(float3 PSMCoords)
{
	// Find the minimum distance from a PSM bounds face, and fade accordingly
	// This will be in normalized PSM coords, but that seems to work well
	float min_dist_from_bounds_face;
	min_dist_from_bounds_face = PSMCoords.x;
	min_dist_from_bounds_face = min(min_dist_from_bounds_face, 1.f-PSMCoords.x);
	min_dist_from_bounds_face = min(min_dist_from_bounds_face,PSMCoords.y);
	min_dist_from_bounds_face = min(min_dist_from_bounds_face, 1.f-PSMCoords.y);
	min_dist_from_bounds_face = min(min_dist_from_bounds_face,PSMCoords.z);
	min_dist_from_bounds_face = min(min_dist_from_bounds_face, 1.f-PSMCoords.z);

	float result = smoothstep(0.f,g_PSMFadeMargin,min_dist_from_bounds_face);
	result *= result;

	return result;
}

[maxvertexcount(4)]
void RenderParticlesToSceneGS( in point VS_DUMMY_PARTICLE_OUTPUT dummy[1], in uint PrimID : SV_PrimitiveID, inout TriangleStream<GS_SCENE_PARTICLE_OUTPUT> outstream )
{
	float4 InstanceData = GetParticleInstanceData(PrimID);

	// Fade particles that are near the bounds of the PSM, and kill any that go outside
	float3 CentreViewPos = mul(float4(InstanceData.xyz,1), g_matView).xyz;
	float3 CentrePSMCoords = mul(float4(CentreViewPos,1.f), g_matViewToPSM).xyz;
	float PSMFade = CalcPSMFadeFactor(CentrePSMCoords);
	if(PSMFade <= 0.f)
		return;

	float fog_factor = exp(dot(CentreViewPos,CentreViewPos)*g_FogExponent);

	[unroll]
	for(uint i = 0; i != 4; ++i) {

		GS_PARTICLE_COORDS particleCoords = CalcParticleCoords(InstanceData,i);

		float4 PSMCoords = mul(float4(particleCoords.ViewPos,1.f), g_matViewToPSM);

		GS_SCENE_PARTICLE_OUTPUT outvert;
		outvert.TextureUVAndOpacity = particleCoords.TextureUVAndOpacity;
		outvert.TextureUVAndOpacity.z *= PSMFade;
		outvert.Position = mul(float4(particleCoords.ViewPos,1.f), g_matProj);
		outvert.PSMCoords = float3(PSMCoords.xyz);
		outvert.FogFactor = fog_factor;
		outstream.Append(outvert);
	}
	outstream.RestartStrip();
}

float4 RenderParticlesToScenePS( GS_SCENE_PARTICLE_OUTPUT In) : SV_Target
{ 
    float3 illumination = g_LightColor*0.5 + g_AmbientColor*0.5 + g_LightningColor*0.5;
	illumination *= CalcPSMShadowFactor(In.PSMCoords);
    
    float4 tex = GetParticleRGBA(g_samplerDiffuse,In.TextureUVAndOpacity.xy,In.TextureUVAndOpacity.z);

    float4 Output = float4(illumination * tex.rgb, tex.a);
	Output.rgb = lerp(g_AmbientColor*0.5 + g_LightningColor*0.5,Output.rgb,In.FogFactor);

    return Output;
}

[maxvertexcount(4)]
void RenderParticlesToPSMGS( in point VS_DUMMY_PARTICLE_OUTPUT dummy[1], in uint PrimID : SV_PrimitiveID, inout TriangleStream<GS_PSM_PARTICLE_OUTPUT> outstream )
{
	float4 InstanceData = GetParticleInstanceData(PrimID);
    float3 CentreViewPos = mul(float4(InstanceData.xyz,1), g_matView).xyz;

	// Dispatch to PSM layer-slice
    float linearZ = mul(float4(CentreViewPos,1.f), g_matProj).z;
	float slice = g_PSMSlices * linearZ + 1.f;	// +1 because zero slice reserved for coverage
	uint sublayer = 2.f * frac(slice);

	if(slice < 0.f || slice > g_PSMSlices)
		return;

	slice = floor(slice);

	[unroll]
	for(uint i = 0; i != 4; ++i) {

		GS_PARTICLE_COORDS particleCoords = CalcParticleCoords(InstanceData,i);

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
    float4 tex = GetParticleRGBA(g_samplerDiffuse,In.TextureUVAndOpacity.xy,In.TextureUVAndOpacity.z);

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

[numthreads(EmitParticlesCSBlocksSize,1,1)]
void EmitParticlesCS( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
{
	if(DTid.x >= g_ParticleCount)
		return;

	uint random_wrap_index;
	g_RandomUV.GetDimensions(random_wrap_index);
	uint random_index = (g_RandomOffset + 2 * DTid.x) % random_wrap_index;

	float emit_interp = g_EmitInterpScaleAndOffset.y + g_EmitInterpScaleAndOffset.x * float(DTid.x);

	float2 random_uv_0 = g_RandomUV.Load(random_index);
	float2 random_uv_1 = g_RandomUV.Load(random_index+1);
	float r = sqrt(random_uv_0.x);
	float theta_pos = 2.f * PI * random_uv_0.y;
	float2 random_unit_circle = float2(r * cos(theta_pos), r *sin(theta_pos));
	float4 emit_pos_local = float4(g_EmitAreaScale.x * random_unit_circle.x,0.f,g_EmitAreaScale.y * random_unit_circle.y,1.f);

	float3 prev_emit_pos_world = mul(emit_pos_local, g_PrevEmitterMatrix);
	float3 curr_emit_pos_world = mul(emit_pos_local, g_CurrEmitterMatrix);

	float3 emit_velocity_local;
	emit_velocity_local.y = 1.f;
	emit_velocity_local.xz = g_EmitMinMaxVelocityAndSpread.z * random_unit_circle;
	emit_velocity_local *= lerp(g_EmitMinMaxVelocityAndSpread.x,g_EmitMinMaxVelocityAndSpread.y,random_uv_1.x);

	float3 prev_emit_vel_world = mul(emit_velocity_local, (float3x3)g_PrevEmitterMatrix);
	float3 curr_emit_vel_world = mul(emit_velocity_local, (float3x3)g_CurrEmitterMatrix);

	float3 emit_pos_world = lerp(prev_emit_pos_world,curr_emit_pos_world,emit_interp);
	float3 emit_vel_world = lerp(prev_emit_vel_world,curr_emit_vel_world,emit_interp);

	// Add in the velocity of the emitter
	emit_vel_world += (curr_emit_vel_world - prev_emit_vel_world)/g_TimeStep;

	// Random buoyancy
	float initial_buoyancy = lerp(g_BuoyancyParams.x,g_BuoyancyParams.y,random_uv_1.y);

	// Pre-roll
	float pre_roll_time = lerp(g_TimeStep,0.f,emit_interp) - g_PreRollEndTime;
	float4 velocity = float4(emit_vel_world,initial_buoyancy);
	float4 instance_data = float4(emit_pos_world,0.f);
	simulate(instance_data, velocity, pre_roll_time);

	// Calc particle index
	uint particle_wrap_index;
	g_SimulationInstanceData.GetDimensions(particle_wrap_index);
	uint particle_index = (4 * (g_ParticleIndexOffset + DTid.x)) % particle_wrap_index;

	// Calc view-space z for depth sort (clamp to 0 to ensure pow-2 pad entries sort to end)
	float view_z = max(0,mul(float4(instance_data.xyz,1), g_matView).z);

	// Write results
	g_SimulationInstanceData[particle_index+0] = instance_data.x;
	g_SimulationInstanceData[particle_index+1] = instance_data.y;
	g_SimulationInstanceData[particle_index+2] = instance_data.z;
	g_SimulationInstanceData[particle_index+3] = instance_data.w;

	g_SimulationVelocities[particle_index+0] = velocity.x;
	g_SimulationVelocities[particle_index+1] = velocity.y;
	g_SimulationVelocities[particle_index+2] = velocity.z;
	g_SimulationVelocities[particle_index+3] = velocity.w;

	int depth_sort_index = particle_index/4;
	g_ParticleDepthSortUAV[depth_sort_index].ParticleIndex = depth_sort_index;
	g_ParticleDepthSortUAV[depth_sort_index].ViewZ = view_z;
}

[numthreads(SimulateParticlesCSBlocksSize,1,1)]
void SimulateParticlesCS( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
{
	if(DTid.x >= g_ParticleCount)
		return;

	uint particle_wrap_index;
	g_SimulationInstanceData.GetDimensions(particle_wrap_index);
	uint particle_index = (4 * (g_ParticleIndexOffset + DTid.x)) % particle_wrap_index;

	float4 velocity;
	velocity.x = g_SimulationVelocities[particle_index+0];
	velocity.y = g_SimulationVelocities[particle_index+1];
	velocity.z = g_SimulationVelocities[particle_index+2];
	velocity.w = g_SimulationVelocities[particle_index+3];

	float4 instance_data;
	instance_data.x = g_SimulationInstanceData[particle_index+0];
	instance_data.y = g_SimulationInstanceData[particle_index+1];
	instance_data.z = g_SimulationInstanceData[particle_index+2];
	instance_data.w = g_SimulationInstanceData[particle_index+3];

	simulate(instance_data, velocity, g_TimeStep);

	// Calc view-space z for depth sort (clamp to 0 to ensure pow-2 pad entries sort to end)
	float view_z = max(0,mul(float4(instance_data.xyz,1), g_matView).z);

	g_SimulationInstanceData[particle_index+0] = instance_data.x;
	g_SimulationInstanceData[particle_index+1] = instance_data.y;
	g_SimulationInstanceData[particle_index+2] = instance_data.z;
	g_SimulationInstanceData[particle_index+3] = instance_data.w;

	g_SimulationVelocities[particle_index+0] = velocity.x;
	g_SimulationVelocities[particle_index+1] = velocity.y;
	g_SimulationVelocities[particle_index+2] = velocity.z;
	g_SimulationVelocities[particle_index+3] = velocity.w;

	int depth_sort_index = particle_index/4;
	g_ParticleDepthSortUAV[depth_sort_index].ParticleIndex = depth_sort_index;
	g_ParticleDepthSortUAV[depth_sort_index].ViewZ = view_z;
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

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
technique11 RenderSmokeToSceneTech
{
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, DummyVS() ) );
        SetGeometryShader( CompileShader( gs_4_0, RenderParticlesToSceneGS() ) );
        SetHullShader( NULL );
        SetDomainShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, RenderParticlesToScenePS() ) );

        SetDepthStencilState( ReadOnlyDepth, 0 );
        SetRasterizerState( SolidNoCull );
        SetBlendState( TranslucentBlendRGB, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

technique11 RenderSmokeToPSMTech
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

technique11 EmitParticlesTech
{
    pass
    {          
        SetVertexShader( NULL );
        SetGeometryShader( NULL );
        SetHullShader( NULL );
        SetDomainShader( NULL );
        SetPixelShader( NULL );
		SetComputeShader( CompileShader( cs_5_0, EmitParticlesCS() ) );
    }
}

technique11 SimulateParticlesTech
{
    pass
    {          
        SetVertexShader( NULL );
        SetGeometryShader( NULL );
        SetHullShader( NULL );
        SetDomainShader( NULL );
        SetPixelShader( NULL );
		SetComputeShader( CompileShader( cs_5_0, SimulateParticlesCS() ) );
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
