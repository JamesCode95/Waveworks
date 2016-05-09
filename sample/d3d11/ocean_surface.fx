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

#define GFSDK_WAVEWORKS_SM5
#define GFSDK_WAVEWORKS_USE_TESSELLATION

#define GFSDK_WAVEWORKS_DECLARE_GEOM_VS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_GEOM_VS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_GEOM_VS_CBUFFER };

#define GFSDK_WAVEWORKS_DECLARE_GEOM_HS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_GEOM_HS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_GEOM_HS_CBUFFER };

#include "GFSDK_WaveWorks_Quadtree.fxh"

#define GFSDK_WAVEWORKS_DECLARE_ATTR_DS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_ATTR_DS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_ATTR_DS_CBUFFER };
#define GFSDK_WAVEWORKS_DECLARE_ATTR_DS_SAMPLER(Label,TextureLabel,Regoff) sampler Label; texture2D TextureLabel;

#define GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_ATTR_PS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_ATTR_PS_CBUFFER };
#define GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(Label,TextureLabel,Regoff) sampler Label; texture2D TextureLabel;

#include "GFSDK_WaveWorks_Attributes.fxh"
#include "common.fx"

//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

// Constant

float3      g_LightPosition;
float3      g_CameraPosition;
float4x4    g_ModelViewMatrix;
float4x4    g_ModelViewProjectionMatrix;
float4x4    g_LightModelViewProjectionMatrix;
float4x4    g_WorldToTopDownTextureMatrix;

float3      g_WaterTransmittance = {0.065,0.028,0.035}; // light absorption per meter for coastal water, taken from here: http://www.seafriends.org.nz/phgraph/water.htm http://www.seafriends.org.nz/phgraph/phdwg34.gif
float3      g_WaterScatterColor = {0.0,0.7,0.3};
float3      g_WaterSpecularColor = {1.1,0.8,0.5};
float       g_WaterScatterIntensity = 0.1;
float		g_WaterSpecularIntensity = 10.0f;

float3		g_FoamColor = {0.90f, 0.95f, 1.0f};
float3		g_FoamUnderwaterColor = {0.0,0.7,0.6};

float       g_WaterSpecularPower = 200.0;
float3      g_AtmosphereBrightColor = {1.1,0.9,0.6};
float3      g_AtmosphereDarkColor = {0.4,0.4,0.5};
float		g_FogDensity = 1.0f/1500.0f;

float4		g_WireframeColor = {1.0,1.0,1.0,1.0};

float2      g_WindDirection;

float2      g_ScreenSizeInv;
float		g_ZNear;
float		g_ZFar;
float		g_Time;

float		g_GerstnerSteepness;
float		g_BaseGerstnerAmplitude;
float		g_BaseGerstnerWavelength;
float		g_BaseGerstnerSpeed;
float		g_BaseGerstnerParallelness;
float		g_enableShoreEffects;

float		g_Wireframe;
float2		g_WinSize = {1920.0,1080.0};

//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
texture2D g_LogoTexture;
Texture2D g_ReflectionTexture;
Texture2D g_RefractionTexture;
Texture2D g_RefractionDepthTextureResolved;
Texture2D g_WaterNormalMapTexture;
Texture2D g_ShadowmapTexture;
Texture2D g_FoamIntensityTexture;
Texture2D g_FoamDiffuseTexture;
Texture2D g_DataTexture;

// Custom trilinear sampler clamping to custom border color
sampler SamplerTrilinearBorder =
sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Border;
	AddressV = Border;
	BorderColor = float4(20.0,-50.0,0.0,0.0);
};

struct VS_OUTPUT 
{
	float4								worldspace_position	: VSO ;
};

struct DS_OUTPUT
{
	float4								positionClip	 : SV_Position;
	GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT NV_ocean_interp;
	float3								displacementWS: TEXCOORD5;
	float3								positionWS: TEXCOORD6;
	float3								world_pos_undisplaced : TEXCOORD7;
	float3								gerstner_displacement : TEXCOORD8;
	float2								gerstner_sdfUV : TEXCOORD9;
	float								gerstner_multiplier : TEXCOORD10;
};

struct PS_INPUT
{
	float4								positionClip	 : SV_Position;
	GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT NV_ocean_interp;
	float3								displacementWS: TEXCOORD5;
	float3								positionWS: TEXCOORD6;
	float3								world_pos_undisplaced : TEXCOORD7;
	float3								gerstner_displacement : TEXCOORD8;
	float2								gerstner_sdfUV : TEXCOORD9;
	float								gerstner_multiplier : TEXCOORD10;
	noperspective float3				v_dist : TEXCOORD11;
};
 
struct HS_ConstantOutput
{
	float fTessFactor[3]    : SV_TessFactor; 
	float fInsideTessFactor : SV_InsideTessFactor;
};



static const float kTopDownDataPixelsPerMeter = 256.0f/700.0; // taken from SDF generation source code, the SDF texture size is 256x256, the viewport size is 700x700
static const float kMaxDepthBelowSea = 50.0f;
static const float kMaxDistance = 20.0f; // taken from SDF generation code
static const float kNumWaves = 1.0; // Total number of Gerster waves of different amplitude, speed etc to calculate, 
									// i+1-th wave has 20% smaller amplitude, 
								    // 20% smaller phase and group speed and 20% less parallelity
							        // Note that all the waves will share the same gerstnerMultiplierOut (lerping between ocean waves and Gerstner waves) for simplicity

void GetGerstnerVertexAttributes(float3 posWS, out float2 sdfUVOut, out float3 offsetOut, out float gerstnerMultiplierOut)
{
	// getting UV for fetching SDF texture 
	float4 topDownPosition = mul( float4( posWS.xyz, 1), g_WorldToTopDownTextureMatrix );
	float2 uv = mad( topDownPosition.xy/topDownPosition.w, 0.5f, 0.5f );
	uv.y = 1-uv.y;

	// initializing the outputs so we can exit early
	sdfUVOut = uv;
	offsetOut = float3 (0.0,0.0,0.0);
	gerstnerMultiplierOut = 0;

	// getting SDF
	const float4 tdData = g_DataTexture.SampleLevel(SamplerTrilinearBorder, uv, 0 );

	// early out without adding gerstner waves if far from shore
	if((tdData.x >= kMaxDistance - 0.1)) 
	{
		return;
	}

	// initializing variables common to all Gerstner waves
	float phaseShift = g_Time;
	float sdfPhase = tdData.x*kMaxDistance/kTopDownDataPixelsPerMeter; 
	float distanceMultiplier =  saturate(1.0-tdData.x); // Shore waves linearly fade in on the edges of SDF
	float depthMultiplier = saturate((g_BaseGerstnerWavelength*0.5 + tdData.y)*0.5); // Shore waves fade in when depth is less than half the wave length, we use 0.25 as this parameter also allows shore waves to heighten as the depth decreases
	gerstnerMultiplierOut = distanceMultiplier*depthMultiplier;

	// initializing variables to be changed along summing up the waves
	float gerstnerWavelength = g_BaseGerstnerWavelength;
	float gerstnerOmega = 2.0*3.141592 / g_BaseGerstnerWavelength; // angular speed of gerstner wave
	float gerstnerParallelness = g_BaseGerstnerParallelness; // "parallelness" of shore waves. 0 means the waves are parallel to shore, 1 means the waves are parallel to wind gradient
	float gerstnerSpeed = g_BaseGerstnerSpeed; // phase speed of gerstner waves
	float gerstnerAmplitude = g_BaseGerstnerAmplitude; 
	float2 windDirection = g_WindDirection;

	// summing up the waves
	for(float i = 0.0; i < kNumWaves; i+=1.0)
	{
		float windPhase = dot(windDirection, posWS.xz); 
		float gerstnerPhase = 2.0*3.141592*(lerp( sdfPhase, windPhase, gerstnerParallelness)/gerstnerWavelength); 
		float2 propagationDirection = normalize( lerp(-tdData.zw + windDirection * 0.000001f, g_WindDirection, gerstnerParallelness*gerstnerParallelness));
		float gerstnerGroupSpeedPhase = 2.0*3.141592*(lerp( sdfPhase, windPhase, gerstnerParallelness*3.0)/gerstnerWavelength); // letting the group speed phase to be non-parallel to propagation phase, so altering parallelness modificator fot this

		float groupSpeedMultiplier = 0.5 + 0.5*cos((gerstnerGroupSpeedPhase + gerstnerOmega*gerstnerSpeed*phaseShift/2.0)/2.7); // Group speed for water waves is half of the phase speed, we allow 2.7 wavelengths to be in wave group, not so much as breaking shore waves lose energy quickly
		float worldSpacePosMultiplier = 0.75 + 0.25*sin(phaseShift*0.3 + 0.5*posWS.x/gerstnerWavelength)*sin(phaseShift*0.4 + 0.5*posWS.y/gerstnerWavelength); // slowly crawling worldspace aligned checkerboard pattern that damps gerstner waves further
		float depthMultiplier = saturate((gerstnerWavelength*0.5 + tdData.y)*0.5); // Shore waves fade in when depth is less than half the wave length
		float gerstnerMultiplier = distanceMultiplier*depthMultiplier*groupSpeedMultiplier*worldSpacePosMultiplier; // final scale factor applied to base Gerstner amplitude and used to mix between ocean waves and shore waves
	
		float steepness = g_GerstnerSteepness;	
		float baseAmplitude = gerstnerMultiplier * gerstnerAmplitude; //amplitude gradually increases as wave runs over shallower seabed
		float breakerMultiplier = saturate((baseAmplitude*2.0*1.28 + tdData.y)/gerstnerAmplitude); // Wave height is 2*amplitude, a wave will start to break when it approximately reaches a water depth of 1.28 times the wave height, empirically: http://passyworldofmathematics.com/mathematics-of-ocean-waves-and-surfing/ 

		// calculating Gerstner offset
		float s,c;
		sincos(gerstnerPhase + gerstnerOmega*gerstnerSpeed*phaseShift, s, c);
		float waveVerticalOffset = s * baseAmplitude;
		offsetOut.y += waveVerticalOffset; 
		offsetOut.xz += c * propagationDirection * steepness * baseAmplitude; // trochoidal Gerstner wave
		offsetOut.xz -= propagationDirection * s * baseAmplitude * breakerMultiplier * 2.0; // adding wave forward skew due to its bottom slowing down, so the forward wave front gradually becomes vertical 
		float breakerPhase = gerstnerPhase + gerstnerOmega*gerstnerSpeed*phaseShift + 3.141592*0.05; 
		float fp = frac(breakerPhase/(3.141592*2.0));
		offsetOut.xz -= 0.5*baseAmplitude*propagationDirection*breakerMultiplier*(saturate(fp*10.0) - saturate(-1.0 + fp*10.0)); // moving breaking area of the wave further forward

		// updating the parameters for next wave
		gerstnerWavelength *= 0.66;
		gerstnerOmega /= 0.66;
		gerstnerSpeed *= 0.66;
		gerstnerAmplitude *= 0.66; 
		gerstnerParallelness *= 0.66;
		windDirection.xy *= float2(-1.0,1.0)*windDirection.yx; // rotating wind direction 
		
		offsetOut.y += baseAmplitude*1.2; // Adding vertical displacement as the wave increases while rolling on the shallow area
	}

}	

void GetGerstnerSurfaceAttributes( float2 sdfUV, float2 posWS, out float3 normalOut, out float breakerOut, out float foamTrailOut)
{
	// initializing the outputs
	normalOut = float3 (0.0,1.0,0.0);
	foamTrailOut = 0.0;
	breakerOut = 0.0;

	// getting SDF
	const float4 tdData = g_DataTexture.SampleLevel(SamplerTrilinearBorder, sdfUV, 0 );

	// initializing variables common to all Gerstner waves
	float phaseShift = g_Time;
	float sdfPhase = tdData.x*kMaxDistance/kTopDownDataPixelsPerMeter; 
	float distanceMultiplier = saturate(1.0-tdData.x); // Shore waves linearly fade in on the edges of SDF

	// initializing variables to be changed along summing up the waves
	float gerstnerWavelength = g_BaseGerstnerWavelength;
	float gerstnerOmega = 2.0*3.141592 / g_BaseGerstnerWavelength; // angular speed of gerstner wave
	float gerstnerParallelness = g_BaseGerstnerParallelness; // "parallelness" of shore waves. 0 means the waves are parallel to shore, 1 means the waves are parallel to wind gradient
	float gerstnerSpeed = g_BaseGerstnerSpeed; // phase speed of gerstner waves
	float gerstnerAmplitude = g_BaseGerstnerAmplitude; 
	float2 windDirection = g_WindDirection;

	// summing up the waves
	for(float i = 0.0; i < kNumWaves; i+=1.0)
	{
		float windPhase = dot(windDirection, posWS.xy); 
		float gerstnerPhase = 2.0*3.141592*(lerp( sdfPhase, windPhase, gerstnerParallelness)/gerstnerWavelength); 
		float2 propagationDirection = normalize( lerp(-tdData.zw + windDirection * 0.000001f, g_WindDirection, gerstnerParallelness*gerstnerParallelness));
		float gerstnerGroupSpeedPhase = 2.0*3.141592*(lerp( sdfPhase, windPhase, gerstnerParallelness*3.0)/gerstnerWavelength); // letting the group speed phase to be non-parallel to propagation phase, so altering parallelness modificator fot this

		float groupSpeedMultiplier = 0.5 + 0.5*cos((gerstnerGroupSpeedPhase + gerstnerOmega*gerstnerSpeed*phaseShift/2.0)/2.7); // Group speed for water waves is half of the phase speed, we allow 2.7 wavelengths to be in wave group, not so much as breaking shore waves lose energy quickly
		float worldSpacePosMultiplier = 0.75 + 0.25*sin(phaseShift*0.3 + 0.5*posWS.x/gerstnerWavelength)*sin(phaseShift*0.4 + 0.5*posWS.y/gerstnerWavelength); // slowly crawling worldspace aligned checkerboard pattern that damps gerstner waves further
		float depthMultiplier = saturate((gerstnerWavelength*0.5 + tdData.y)*0.5); // Shore waves fade in when depth is less than half the wave length
		float gerstnerMultiplier = distanceMultiplier*depthMultiplier*groupSpeedMultiplier*worldSpacePosMultiplier; // final scale factor applied to base Gerstner amplitude and used to mix between ocean waves and shore waves
	
		float steepness = g_GerstnerSteepness;	
		float baseAmplitude = gerstnerMultiplier * gerstnerAmplitude; //amplitude gradually increases as wave runs over shallower seabed

		// calculating normal
		float s,c;
		sincos(gerstnerPhase + gerstnerOmega*gerstnerSpeed*phaseShift, s, c);
		normalOut.y -= gerstnerOmega*steepness*baseAmplitude*s;
		normalOut.xz -= gerstnerOmega*baseAmplitude*c*propagationDirection;   // orienting normal according to direction of wave propagation. No need to normalize, it is unit length.

		// calculating foam parameters
		float breakerMultiplier = saturate((baseAmplitude*2.0*1.28 + tdData.y)/gerstnerAmplitude); // Wave height is 2*amplitude, a wave will start to break when it approximately reaches a water depth of 1.28 times the wave height, empirically: http://passyworldofmathematics.com/mathematics-of-ocean-waves-and-surfing/ 

		float foamTrailPhase = gerstnerPhase + gerstnerOmega*gerstnerSpeed*phaseShift + 3.141592*0.05; // delaying foam trail a bit so it's following the breaker
		float fp = frac(foamTrailPhase/(3.141592*2.0));
		foamTrailOut += gerstnerMultiplier*breakerMultiplier*(saturate(fp*10.0) - saturate(fp*1.1)); // only breaking waves leave foamy trails
		breakerOut += gerstnerMultiplier*breakerMultiplier*(saturate(fp*10.0) - saturate(-1.0 + fp*10.0)); // making narrow sawtooth pattern

		// updating the parameters for next wave
		gerstnerWavelength *= 0.66;
		gerstnerOmega /= 0.66;
		gerstnerSpeed *= 0.66;
		gerstnerAmplitude *= 0.66; 
		gerstnerParallelness *= 0.66;
		windDirection.xy *= float2(-1.0,1.0)*windDirection.yx; // rotating wind direction 
	}
}	

//-----------------------------------------------------------------------------
// Name: OceanWaveVS 
// Type: Vertex shader                                      
// Desc: 
//-----------------------------------------------------------------------------
VS_OUTPUT OceanWaveVS(GFSDK_WAVEWORKS_VERTEX_INPUT In)
{
	VS_OUTPUT Output;
	Output.worldspace_position = float4(GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition(In),0.0);
	return Output; 
}


//-----------------------------------------------------------------------------
// Name: HS_Constant
// Type: Hull shader                                      
// Desc: 
//-----------------------------------------------------------------------------
HS_ConstantOutput HS_Constant( InputPatch<VS_OUTPUT, 3> I )
{
	HS_ConstantOutput O;
	O.fTessFactor[0] = GFSDK_WaveWorks_GetEdgeTessellationFactor(I[1].worldspace_position,I[2].worldspace_position);
	O.fTessFactor[1] = GFSDK_WaveWorks_GetEdgeTessellationFactor(I[2].worldspace_position,I[0].worldspace_position);
	O.fTessFactor[2] = GFSDK_WaveWorks_GetEdgeTessellationFactor(I[0].worldspace_position,I[1].worldspace_position);
	O.fInsideTessFactor = (O.fTessFactor[0] + O.fTessFactor[1] + O.fTessFactor[2])/3.0f;
	return O;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[patchconstantfunc("HS_Constant")]
[outputcontrolpoints(3)]
VS_OUTPUT HS_FlatTriangles( InputPatch<VS_OUTPUT, 3> I, uint uCPID : SV_OutputControlPointID )
{
	VS_OUTPUT O = (VS_OUTPUT)I[uCPID];
	return O;
}

//--------------------------------------------------------------------------------------
// This domain shader applies contol point weighting to the barycentric coords produced by the FF tessellator 
//--------------------------------------------------------------------------------------
[domain("tri")]
DS_OUTPUT DS_FlatTriangles_Shore( HS_ConstantOutput HSConstantData, const OutputPatch<VS_OUTPUT, 3> I, float3 f3BarycentricCoords : SV_DomainLocation )
{
	DS_OUTPUT Output = (DS_OUTPUT)0;

	GFSDK_WAVEWORKS_VERTEX_OUTPUT NV_ocean = GFSDK_WaveWorks_GetDisplacedVertexAfterTessellation(I[0].worldspace_position, I[1].worldspace_position, I[2].worldspace_position, f3BarycentricCoords);

	float3 gerstnerDisplacement = float3(0,0,0);
	float2 sdfUV = float2(0,0);
	float  gerstnerMultiplier = 0;

	if(g_enableShoreEffects > 0)
	{
		GetGerstnerVertexAttributes(NV_ocean.pos_world_undisplaced.xzy, sdfUV, gerstnerDisplacement, gerstnerMultiplier);
	}

	NV_ocean.world_displacement *= 1.0 - 0.7*gerstnerMultiplier;
	NV_ocean.world_displacement += gerstnerDisplacement.xzy*gerstnerMultiplier;

	NV_ocean.pos_world = NV_ocean.pos_world_undisplaced + NV_ocean.world_displacement;
	Output.positionWS = NV_ocean.pos_world;
	Output.displacementWS = NV_ocean.world_displacement;
	Output.positionClip = mul(float4(NV_ocean.pos_world,1.0), g_ModelViewProjectionMatrix);
	Output.world_pos_undisplaced = NV_ocean.pos_world_undisplaced;
	Output.gerstner_displacement = gerstnerDisplacement.xzy;
	Output.gerstner_sdfUV = sdfUV;
	Output.gerstner_multiplier = gerstnerMultiplier;
	Output.NV_ocean_interp = NV_ocean.interp;

	return Output;
}

//--------------------------------------------------------------------------------------
// This geometry shader enables solid wireframe mode
//--------------------------------------------------------------------------------------
[maxvertexcount(3)]
void GSSolidWire( triangle DS_OUTPUT input[3], inout TriangleStream<PS_INPUT> outStream )
{
    PS_INPUT output;

	float2 p0 = g_WinSize * input[0].positionClip.xy/input[0].positionClip.w;
	float2 p1 = g_WinSize * input[1].positionClip.xy/input[1].positionClip.w;
	float2 p2 = g_WinSize * input[2].positionClip.xy/input[2].positionClip.w;
	float2 v0 = p2 - p1;
	float2 v1 = p2 - p0;
	float2 v2 = p1 - p0;
	float area = abs(v1.x*v2.y - v1.y * v2.x);



	// Generate vertices
    output.positionClip                = input[0].positionClip;
	output.NV_ocean_interp             = input[0].NV_ocean_interp;
	output.displacementWS              = input[0].displacementWS;
	output.positionWS                  = input[0].positionWS;
	output.world_pos_undisplaced       = input[0].world_pos_undisplaced;
	output.gerstner_displacement       = input[0].gerstner_displacement;
	output.gerstner_sdfUV              = input[0].gerstner_sdfUV;
	output.gerstner_multiplier         = input[0].gerstner_multiplier;
	output.v_dist                      = float3(area/length(v0),0,0);
    outStream.Append( output );
     
    output.positionClip                = input[1].positionClip;
	output.NV_ocean_interp             = input[1].NV_ocean_interp;
	output.displacementWS              = input[1].displacementWS;
	output.positionWS                  = input[1].positionWS;
	output.world_pos_undisplaced       = input[1].world_pos_undisplaced;
	output.gerstner_displacement       = input[1].gerstner_displacement;
	output.gerstner_sdfUV              = input[1].gerstner_sdfUV;
	output.gerstner_multiplier         = input[1].gerstner_multiplier;
	output.v_dist                      = float3(0,area/length(v1),0);
    outStream.Append( output );

    output.positionClip                = input[2].positionClip;
	output.NV_ocean_interp             = input[2].NV_ocean_interp;
	output.displacementWS              = input[2].displacementWS;
	output.positionWS                  = input[2].positionWS;
	output.world_pos_undisplaced       = input[2].world_pos_undisplaced;
	output.gerstner_displacement       = input[2].gerstner_displacement;
	output.gerstner_sdfUV              = input[2].gerstner_sdfUV;
	output.gerstner_multiplier         = input[2].gerstner_multiplier;
	output.v_dist                      = float3(0,0,area/length(v2));
    outStream.Append( output );

    outStream.RestartStrip();
}

float GetRefractionDepth(float2 position)
{
	return g_RefractionDepthTextureResolved.SampleLevel(SamplerLinearClamp,position,0).r;
}

// primitive simulation of non-uniform atmospheric fog
float3 CalculateFogColor(float3 pixel_to_light_vector, float3 pixel_to_eye_vector)
{
	return lerp(g_AtmosphereDarkColor,g_AtmosphereBrightColor,0.5*dot(pixel_to_light_vector,-pixel_to_eye_vector)+0.5);
}

//-----------------------------------------------------------------------------
// Name: OceanWaveShorePS
// Type: Pixel shader                                      
// Desc: 
//-----------------------------------------------------------------------------
float4 OceanWaveShorePS(PS_INPUT In) : SV_Target
{
	float3 color;
	float3 normal;
	float fresnel_factor;
	float specular_factor;
	float scatter_factor;
	float3 refraction_color;
	float3 reflection_color;
	float4 disturbance_eyespace;
	

	float water_depth;

	float3 water_vertex_positionWS = In.positionWS.xzy;
	
	float3 pixel_to_light_vector = normalize(g_LightPosition-water_vertex_positionWS);
	float3 pixel_to_eye_vector = normalize(g_CameraPosition-water_vertex_positionWS);

	GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES surface_attributes = GFSDK_WaveWorks_GetSurfaceAttributes(In.NV_ocean_interp);

	float3 gerstner_normal = float3(0.0,1.0,0.0);
	float gerstner_breaker = 0;
	float gerstner_foamtrail = 0;

	if(g_enableShoreEffects > 0)
	{
		if(In.gerstner_multiplier > 0)
		{
			GetGerstnerSurfaceAttributes( In.gerstner_sdfUV, In.world_pos_undisplaced.xy, gerstner_normal, gerstner_breaker, gerstner_foamtrail);
		}
		surface_attributes.normal = lerp(float3(0,1,0),surface_attributes.normal.xzy, 1.0-0.9*In.gerstner_multiplier*In.gerstner_multiplier); // Leaving just 10% of original open ocean normals in areas affected by shore waves
		surface_attributes.foam_turbulent_energy += gerstner_foamtrail*3.0; 
		surface_attributes.foam_wave_hats += gerstner_breaker*15.0;				// 15.0*breaker so the breaker foam has rough edges 

		// using PD normal combination
		normal = normalize(float3(surface_attributes.normal.xz*gerstner_normal.y + gerstner_normal.xz*surface_attributes.normal.y, surface_attributes.normal.y*gerstner_normal.y));
		normal = normal.xzy;
	}
	else
	{
		normal = surface_attributes.normal.xzy;
	}

	float3 reflected_eye_to_pixel_vector=-pixel_to_eye_vector+2*dot(pixel_to_eye_vector,normal)*normal;

	// calculating pixel position in light space
	float4 positionLS = mul(float4(water_vertex_positionWS,1),g_LightModelViewProjectionMatrix);
	positionLS.xyz/=positionLS.w;
	positionLS.x=(positionLS.x+1)*0.5;
	positionLS.y=(1-positionLS.y)*0.5;
	positionLS.z = min(0.99,positionLS.z);

	// calculating shadow multiplier to be applied to diffuse/scatter/specular light components
	float shadow_factor = g_ShadowmapTexture.SampleCmp(SamplerDepthAnisotropic,positionLS.xy,positionLS.z* 0.995f).r;

	// simulating scattering/double refraction: light hits the side of wave, travels some distance in water, and leaves wave on the other side
	// it's difficult to do it physically correct without photon mapping/ray tracing, so using simple but plausible emulation below
	
	// only the crests of water waves generate double refracted light
	scatter_factor = g_WaterScatterIntensity*
		// the waves that lie between camera and light projection on water plane generate maximal amount of double refracted light 
		pow(max(0.0,dot(normalize(float3(pixel_to_light_vector.x,0.0,pixel_to_light_vector.z)),-pixel_to_eye_vector)),2.0)*
		// the slopes of waves that are oriented back to light generate maximal amount of double refracted light 
		shadow_factor*pow(max(0.0,1.0-dot(pixel_to_light_vector,normal)),2.0);

	scatter_factor += g_WaterScatterIntensity*
		// the scattered light is best seen if observing direction is normal to slope surface
		max(0,dot(pixel_to_eye_vector,normal));//*
	
	// calculating fresnel factor 
	float r=(1.0 - 1.33)*(1.0 - 1.33)/((1.0 + 1.33)*(1.0 + 1.33));
	fresnel_factor = r + (1.0-r)*pow(saturate(1.0 - dot(normal,pixel_to_eye_vector)),5.0);


	// calculating specular factor
	specular_factor=shadow_factor*pow(max(0,dot(pixel_to_light_vector,reflected_eye_to_pixel_vector)),g_WaterSpecularPower);
	
	// calculating disturbance which has to be applied to planar reflections/refractions to give plausible results
	disturbance_eyespace=mul(float4(normal.x,normal.z,0,0),g_ModelViewMatrix);

	float2 reflection_disturbance = float2(disturbance_eyespace.x,disturbance_eyespace.z)*0.06;
	float2 refraction_disturbance = float2(-disturbance_eyespace.x,disturbance_eyespace.y)*0.9*
		// fading out refraction disturbance at distance so refraction doesn't look noisy at distance
		(100.0/(100+length(g_CameraPosition-water_vertex_positionWS)));
	
	// picking refraction depth at non-displaced point, need it to scale the refraction texture displacement amount according to water depth
	float refraction_depth = GetRefractionDepth(In.positionClip.xy*g_ScreenSizeInv);
	refraction_depth = g_ZFar*g_ZNear / (g_ZFar-refraction_depth*(g_ZFar-g_ZNear));
	float4 vertex_in_viewspace = mul(float4(In.positionWS.xyz,1),g_ModelViewMatrix);
	water_depth = refraction_depth-vertex_in_viewspace.z;

	if(water_depth < 0)
	{
		refraction_disturbance = 0;
	}
	water_depth = max(0,water_depth);
	refraction_disturbance *= min(1.0f,water_depth*0.03);
	
	// getting refraction depth again, at displaced point now
	refraction_depth = GetRefractionDepth(In.positionClip.xy*g_ScreenSizeInv+refraction_disturbance);
	refraction_depth = g_ZFar*g_ZNear / (g_ZFar-refraction_depth*(g_ZFar-g_ZNear));
	vertex_in_viewspace= mul(float4(In.positionWS.xyz,1),g_ModelViewMatrix);
	water_depth = max(water_depth,refraction_depth-vertex_in_viewspace.z);
	water_depth = max(0,water_depth);
	float depth_damper = min(1,water_depth*3.0);
	float depth_damper_sss = min(1,water_depth*0.5);

	// getting reflection and refraction color at disturbed texture coordinates
	reflection_color = g_ReflectionTexture.SampleLevel(SamplerLinearClamp,float2(In.positionClip.x*g_ScreenSizeInv.x,1.0-In.positionClip.y*g_ScreenSizeInv.y)+reflection_disturbance,0).rgb;
	refraction_color = g_RefractionTexture.SampleLevel(SamplerLinearClamp,In.positionClip.xy*g_ScreenSizeInv+refraction_disturbance,0).rgb;

	// fading fresnel factor to 0 to soften water surface edges
	fresnel_factor*=depth_damper;

	// fading fresnel factor to 0 for rays that reflect below water surface
	fresnel_factor*= 1.0 - 1.0*saturate(-2.0*reflected_eye_to_pixel_vector.y);

	// applying water absorbtion according to distance that refracted ray travels in water
	// note that we multiply this by 2 since light travels through water twice: from light to seafloor then from seafloor back to eye
	refraction_color.r *= exp(-1.0*water_depth*2.0*g_WaterTransmittance.r);
	refraction_color.g *= exp(-1.0*water_depth*2.0*g_WaterTransmittance.g);
	refraction_color.b *= exp(-1.0*water_depth*2.0*g_WaterTransmittance.b);

	// applying water scatter factor
	refraction_color += scatter_factor*shadow_factor*g_WaterScatterColor*depth_damper_sss;

	// adding milkiness due to mixed-in foam
	refraction_color += g_FoamUnderwaterColor*saturate(surface_attributes.foam_turbulent_energy*0.2)*depth_damper_sss;
	
	// combining final water color
	color = lerp(refraction_color, reflection_color, fresnel_factor);
	// adding specular
	color.rgb += specular_factor*g_WaterSpecularIntensity*g_WaterSpecularColor*shadow_factor*depth_damper;

	// applying surface foam provided by turbulent energy

	// low frequency foam map
	float foam_intensity_map_lf = 1.0*g_FoamIntensityTexture.Sample(SamplerLinearWrap, In.world_pos_undisplaced.xy*0.04*float2(1,1)).x - 1.0;

	// high frequency foam map
	float foam_intensity_map_hf = 1.0*g_FoamIntensityTexture.Sample(SamplerLinearWrap, In.world_pos_undisplaced.xy*0.15*float2(1,1)).x - 1.0;

	// ultra high frequency foam map
	float foam_intensity_map_uhf = 1.0*g_FoamIntensityTexture.Sample(SamplerLinearWrap, In.world_pos_undisplaced.xy*0.3*float2(1,1)).x;

	float foam_intensity;
	foam_intensity = saturate(foam_intensity_map_hf + min(3.5,1.0*surface_attributes.foam_turbulent_energy-0.2)); 
	foam_intensity += (foam_intensity_map_lf + min(1.5,1.0*surface_attributes.foam_turbulent_energy)); 

	
	foam_intensity -= 0.1*saturate(-surface_attributes.foam_surface_folding);
	
	foam_intensity = max(0,foam_intensity);

	foam_intensity *= 1.0+0.8*saturate(surface_attributes.foam_surface_folding);

	float foam_bubbles = g_FoamDiffuseTexture.Sample(SamplerLinearWrap, In.world_pos_undisplaced.xy*0.5).r;
	foam_bubbles = saturate(5.0*(foam_bubbles-0.8));

	// applying foam hats
	foam_intensity += max(0,foam_intensity_map_uhf*2.0*surface_attributes.foam_wave_hats);

	foam_intensity = pow(foam_intensity, 0.7);
	foam_intensity = saturate(foam_intensity*foam_bubbles*1.0);

	foam_intensity*=depth_damper;

	// foam diffuse color
	float foam_diffuse_factor = max(0,0.8+max(0,0.2*dot(pixel_to_light_vector,surface_attributes.normal)));

	color = lerp(color, foam_diffuse_factor*float3(1.0,1.0,1.0),foam_intensity);
	
	// applying atmospheric fog to water surface
	float fog_factor = min(1,exp(-length(g_CameraPosition-water_vertex_positionWS)*g_FogDensity));
	color = lerp(color, CalculateFogColor(normalize(g_LightPosition),pixel_to_eye_vector).rgb, fresnel_factor*(1.0-fog_factor));
	
	// applying solid wireframe
	float d = min(In.v_dist.x,min(In.v_dist.y,In.v_dist.z));
	float I = exp2(-2.0*d*d);
	return float4(color + g_Wireframe*I*0.5, 1.0);
}

//-----------------------------------------------------------------------------
// Name: OceanWaveTech
// Type: Technique
// Desc: 
//-----------------------------------------------------------------------------
technique11 RenderOceanSurfTech
{

	// With shoreline
	pass Pass_Solid_WithShoreline
	{
		SetVertexShader( CompileShader( vs_5_0, OceanWaveVS() ) );
		SetHullShader( CompileShader( hs_5_0, HS_FlatTriangles() ) );
		SetDomainShader( CompileShader( ds_5_0, DS_FlatTriangles_Shore() ) );
		SetGeometryShader( CompileShader( gs_5_0, GSSolidWire() ) );
		SetPixelShader( CompileShader( ps_5_0, OceanWaveShorePS() ) );

		SetDepthStencilState( DepthNormal, 0 );
		SetRasterizerState( CullBackMS );
		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
	}
}


struct VS_LOGO_OUTPUT
{
	float4 positionClip : SV_Position;   
	float2 tex_coord	: TEXCOORD1;  
};

//-----------------------------------------------------------------------------
// Name: DisplayLogoVS                                        
// Type: Vertex shader
//-----------------------------------------------------------------------------
VS_LOGO_OUTPUT DisplayLogoVS(float4 vPos : POSITION, float2 vTexCoord : TEXCOORD0)
{
	VS_LOGO_OUTPUT Output;
	Output.positionClip = vPos;
	Output.tex_coord = vTexCoord;
	return Output;
}

//-----------------------------------------------------------------------------
// Name: DisplayLogoPS                                        
// Type: Pixel shader
//-----------------------------------------------------------------------------
float4 DisplayLogoPS(VS_LOGO_OUTPUT In) : SV_Target
{
	return g_LogoTexture.Sample(SamplerLinearWrap, In.tex_coord);
}

//-----------------------------------------------------------------------------
// Name: DisplayBufferTech
// Type: Technique                                     
// Desc: Logo rendering
//-----------------------------------------------------------------------------
technique11 DisplayLogoTech
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_5_0, DisplayLogoVS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, DisplayLogoPS() ) );

		SetDepthStencilState( DepthAlways, 0 );
		SetRasterizerState( NoCullMS );
		SetBlendState( Translucent, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
}

float4 g_OriginPosition;
float4 g_ContactPosition;
float4 g_RayDirection;

struct RAY_CONTACT_VS_INPUT
{
	float4	PositionWS : POSITION;
};

struct RAY_CONTACT_VS_OUTPUT
{
	float4	PositionClip : SV_Position;
};

//-----------------------------------------------------------------------------
// Name: ContactVS
// Type: Vertex shader                                      
// Desc: 
//-----------------------------------------------------------------------------
RAY_CONTACT_VS_OUTPUT ContactVS(RAY_CONTACT_VS_INPUT In)
{
	RAY_CONTACT_VS_OUTPUT Output;
	Output.PositionClip = mul(float4(In.PositionWS.xzy*0.5 + g_ContactPosition.xyz, 1.0), g_ModelViewProjectionMatrix);
	return Output; 
}

//-----------------------------------------------------------------------------
// Name: RayVS
// Type: Vertex shader                                      
// Desc: 
//-----------------------------------------------------------------------------
RAY_CONTACT_VS_OUTPUT RayVS(RAY_CONTACT_VS_INPUT In)
{
	RAY_CONTACT_VS_OUTPUT Output;
	Output.PositionClip = mul(float4(g_OriginPosition.xzy + In.PositionWS.y*g_RayDirection.xzy,1.0), g_ModelViewProjectionMatrix);
	return Output; 
}

//-----------------------------------------------------------------------------
// Name: RayContactPS
// Type: Pixel shader                                      
// Desc: 
//-----------------------------------------------------------------------------
float4 RayContactPS(RAY_CONTACT_VS_OUTPUT In) : SV_Target
{
	return float4(0, 1.0, 0, 1.0); 
}

//-----------------------------------------------------------------------------
// Name: RenderRayContactTech
// Type: Technique
// Desc: 
//-----------------------------------------------------------------------------
technique11 RenderRayContactTech
{

	// Contact
	pass Pass_Contact
	{
		SetVertexShader( CompileShader( vs_5_0, ContactVS() ) );
		SetHullShader( NULL );
		SetDomainShader( NULL );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, RayContactPS() ) );

		SetDepthStencilState( DepthNormal, 0 );
		SetRasterizerState( NoCullMS );
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}

	// Ray
	pass Pass_Ray
	{
		SetVertexShader( CompileShader( vs_5_0, RayVS() ) );
		SetHullShader( NULL );
		SetDomainShader( NULL );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, RayContactPS() ) );

		SetDepthStencilState( DepthNormal, 0 );
		SetRasterizerState( Wireframe );
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
}
