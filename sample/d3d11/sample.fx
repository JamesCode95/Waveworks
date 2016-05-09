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

#include "common.fx"

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// static textures
Texture2D g_HeightfieldTexture;
Texture2D g_LayerdefTexture;
Texture2D g_RockBumpTexture;
Texture2D g_FoamIntensityTexture;
Texture2D g_FoamDiffuseTexture;

// rendertarget textures
Texture2D g_SkyTexture;
Texture2D g_ShadowmapTexture;
Texture2D g_MainTexture;
Texture2DMS<float,1> g_RefractionDepthTextureMS1;
Texture2DMS<float,2> g_RefractionDepthTextureMS2;
Texture2DMS<float,4> g_RefractionDepthTextureMS4;



//--------------------------------------------------------------------------------------
// Shader Inputs/Outputs
//--------------------------------------------------------------------------------------
struct VSIn_Diffuse
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal   : NORMAL;
};

struct PSIn_Diffuse
{
    float4			position     : SV_Position;
    centroid float3 normal       : NORMAL;
    centroid float3 positionWS   : TEXCOORD0;
	centroid float4 layerdef	 : TEXCOORD1;
	centroid float  brightness   : TEXCOORD2;
};

struct PSIn_Quad
{
    float4 position     : SV_Position;
    float2 texcoord     : TEXCOORD0;
};

struct VSIn_Default
{
	float4 position : POSITION;
    float2 texcoord  : TEXCOORD;
};


struct DUMMY
{
	float Dummmy : DUMMY;
};

struct HSIn_Heightfield
{
    float2 origin   : ORIGIN;
    float2 size     : SIZE;
};


struct PatchData
{
    float Edges[4]  : SV_TessFactor;
    float Inside[2]	: SV_InsideTessFactor;

	float2 origin   : ORIGIN;
    float2 size     : SIZE;
};

//--------------------------------------------------------------------------------------
// Global variables 
//--------------------------------------------------------------------------------------

// rendering control variables
float       g_DynamicTessFactor;
float		g_HalfSpaceCullSign;
float		g_HalfSpaceCullPosition;
int			g_MSSamples;

// view/time dependent variables
float4x4    g_ModelViewMatrix;
float4x4    g_ModelViewProjectionMatrix;
float4x4	g_ModelViewProjectionMatrixInv;
float4x4    g_LightModelViewProjectionMatrix;
float4x4    g_LightModelViewProjectionMatrixInv;
float3      g_CameraPosition;
float3      g_CameraDirection;

float3      g_LightPosition;
float2      g_ScreenSizeInv;
float	    g_MainBufferSizeMultiplier;
float		g_ZNear;
float		g_ZFar;
float		g_ApplyFog;

// constants defining visual appearance
float2		g_RockBumpTexcoordScale={10.0,10.0};
float		g_RockBumpHeightScale=3.0;
float3      g_AtmosphereBrightColor={1.1,0.9,0.6};
float3      g_AtmosphereDarkColor={0.4,0.4,0.5};
float		g_FogDensity = 1.0f/1500.0f;
float2		g_HeightFieldOrigin = float2(0, 0);
float		g_HeightFieldSize = 512;

// Shoreline rendering related variables
float4x4    g_WorldToTopDownTextureMatrix;
Texture2D   g_DataTexture;
float		g_Time;
float		g_GerstnerSteepness;
float		g_BaseGerstnerAmplitude;
float		g_BaseGerstnerWavelength;
float		g_BaseGerstnerSpeed;
float		g_BaseGerstnerParallelness;
float2      g_WindDirection;
float		g_enableShoreEffects;

//--------------------------------------------------------------------------------------
// Misc functions
//--------------------------------------------------------------------------------------

// calculating tessellation factor. 
float CalculateTessellationFactor(float distance)
{
	return g_DynamicTessFactor*(1.0/(0.015*distance));
}

// to avoid vertex swimming while tessellation varies, one can use mipmapping for displacement maps
// it's not always the best choice, but it effificiently suppresses high frequencies at zero cost
float CalculateMIPLevelForDisplacementTextures(float distance)
{
	return log2(128/CalculateTessellationFactor(distance));
}

// primitive simulation of non-uniform atmospheric fog
float3 CalculateFogColor(float3 pixel_to_light_vector, float3 pixel_to_eye_vector)
{
	return lerp(g_AtmosphereDarkColor,g_AtmosphereBrightColor,0.5*dot(pixel_to_light_vector,-pixel_to_eye_vector)+0.5);
}

//--------------------------------------------------------------------------------------
// Sky shaders
//--------------------------------------------------------------------------------------

struct PSIn_Sky
{
    float4			position     : SV_Position;
    centroid float2 texcoord     : TEXCOORD0;
    centroid float3 positionWS   : TEXCOORD1;
};

PSIn_Sky SkyVS(VSIn_Default input)
{
    PSIn_Sky output;

    output.position = mul(input.position, g_ModelViewProjectionMatrix);
	output.positionWS=input.position.xyz;
    output.texcoord = input.texcoord;
    return output;
}

float4 SkyPS(PSIn_Sky input) : SV_Target
{
	float4 color;
	float3 acolor;
	float3 pixel_to_eye_vector = normalize(g_CameraPosition-input.positionWS);

	color=1.5*g_SkyTexture.Sample(SamplerLinearWrap,float2(input.texcoord.x,input.texcoord.y))-0.3;
	acolor =CalculateFogColor(normalize(g_LightPosition),pixel_to_eye_vector);
	color.rgb = lerp(color.rgb,acolor,pow(saturate(input.texcoord.y),3));
	color.a =1;
	return color;
}

//--------------------------------------------------------------------------------------
// Heightfield shaders
//--------------------------------------------------------------------------------------

HSIn_Heightfield PassThroughVS(float4 PatchParams : PATCH_PARAMETERS)
{
    HSIn_Heightfield output;
    output.origin = PatchParams.xy;
    output.size = PatchParams.zw;
    return output;
}

PatchData PatchConstantHS( InputPatch<HSIn_Heightfield, 1> inputPatch )
{    
    PatchData output;

	float distance_to_camera;
	float tesselation_factor;
	float inside_tessellation_factor=0;
	float in_frustum=0;

	output.origin = inputPatch[0].origin;
	output.size = inputPatch[0].size;

	float2 texcoord0to1 = (inputPatch[0].origin + inputPatch[0].size/2.0)/g_HeightFieldSize;
	texcoord0to1.y=1-texcoord0to1.y;
	
	// conservative frustum culling
	float3 patch_center=float3(inputPatch[0].origin.x+inputPatch[0].size.x*0.5,g_HeightfieldTexture.SampleLevel(SamplerLinearWrap, texcoord0to1,0).w,inputPatch[0].origin.y+inputPatch[0].size.y*0.5);
	float3 camera_to_patch_vector =  patch_center-g_CameraPosition;
	float3 patch_to_camera_direction_vector = g_CameraDirection*dot(camera_to_patch_vector,g_CameraDirection)-camera_to_patch_vector;
	float3 patch_center_realigned=patch_center+normalize(patch_to_camera_direction_vector)*min(2*inputPatch[0].size.x,length(patch_to_camera_direction_vector));
	float4 patch_screenspace_center = mul(float4(patch_center_realigned, 1.0), g_ModelViewProjectionMatrix);

	if(((patch_screenspace_center.x/patch_screenspace_center.w>-1.0) && (patch_screenspace_center.x/patch_screenspace_center.w<1.0) 
		&& (patch_screenspace_center.y/patch_screenspace_center.w>-1.0) && (patch_screenspace_center.y/patch_screenspace_center.w<1.0)
		&& (patch_screenspace_center.w>0)) || (length(patch_center-g_CameraPosition)<2*inputPatch[0].size.x))
	{
		in_frustum=1;
	}

	if(in_frustum) 
	{
		distance_to_camera=length(g_CameraPosition.xz-inputPatch[0].origin-float2(0,inputPatch[0].size.y*0.5));
		tesselation_factor=CalculateTessellationFactor(distance_to_camera);
		output.Edges[0] =  tesselation_factor;
		inside_tessellation_factor+=tesselation_factor;


		distance_to_camera=length(g_CameraPosition.xz-inputPatch[0].origin-float2(inputPatch[0].size.x*0.5,0));
		tesselation_factor=CalculateTessellationFactor(distance_to_camera);
		output.Edges[1] =  tesselation_factor;
		inside_tessellation_factor+=tesselation_factor;

		distance_to_camera=length(g_CameraPosition.xz-inputPatch[0].origin-float2(inputPatch[0].size.x,inputPatch[0].size.y*0.5));
		tesselation_factor=CalculateTessellationFactor(distance_to_camera);
		output.Edges[2] =  tesselation_factor;
		inside_tessellation_factor+=tesselation_factor;

		distance_to_camera=length(g_CameraPosition.xz-inputPatch[0].origin-float2(inputPatch[0].size.x*0.5,inputPatch[0].size.y));
		tesselation_factor=CalculateTessellationFactor(distance_to_camera);
		output.Edges[3] =  tesselation_factor;
		inside_tessellation_factor+=tesselation_factor;
		output.Inside[0] = output.Inside[1] = inside_tessellation_factor*0.25;
	}
	else
	{
		output.Edges[0]=-1;
		output.Edges[1]=-1;
		output.Edges[2]=-1;
		output.Edges[3]=-1;
		output.Inside[0]=-1;
		output.Inside[1]=-1;
	}

    return output;
}

[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(1)]
[patchconstantfunc("PatchConstantHS")]
DUMMY PatchHS( InputPatch<HSIn_Heightfield, 1> inputPatch )
{
    return (DUMMY)0;
}

[domain("quad")]
PSIn_Diffuse HeightFieldPatchDS(    PatchData input, 
                                    float2 uv : SV_DomainLocation,
                                    OutputPatch<DUMMY, 1> inputPatch )
{
    PSIn_Diffuse output;
	float3 vertexPosition;
	float4 base_texvalue;
	float2 texcoord0to1 = (input.origin + uv * input.size)/g_HeightFieldSize;
	float3 base_normal;
	float3 detail_normal;
	float3 detail_normal_rotated;
	float4 detail_texvalue;
	float detail_height;
	float3x3 normal_rotation_matrix;
	float4 layerdef;
	float distance_to_camera;
	float detailmap_miplevel;
	texcoord0to1.y=1-texcoord0to1.y;
	
	// fetching base heightmap,normal and moving vertices along y axis
	base_texvalue=g_HeightfieldTexture.SampleLevel(SamplerLinearWrap, texcoord0to1,0);
    base_normal=base_texvalue.xyz;
	base_normal.z=-base_normal.z;
	vertexPosition.xz = input.origin + uv * input.size;
    vertexPosition.y = base_texvalue.w;

	// calculating MIP level for detail texture fetches
	distance_to_camera=length(g_CameraPosition-vertexPosition);
	detailmap_miplevel= CalculateMIPLevelForDisplacementTextures(distance_to_camera);
	
	// fetching layer definition texture
	layerdef=g_LayerdefTexture.SampleLevel(SamplerLinearWrap, texcoord0to1,0);
	
	// rock detail texture
	detail_texvalue = g_RockBumpTexture.SampleLevel(SamplerLinearWrap, texcoord0to1*g_RockBumpTexcoordScale,detailmap_miplevel).rbga;
	detail_normal = normalize(lerp(float3(0.0,1.0,0.0),2.0*detail_texvalue.xyz-float3(1,1,1),layerdef.w));
	detail_height = (detail_texvalue.w-0.5)*g_RockBumpHeightScale*layerdef.w;

	// moving vertices by detail height along base normal
	vertexPosition+=base_normal*detail_height;

	//calculating base normal rotation matrix
	normal_rotation_matrix[1]=base_normal;
	normal_rotation_matrix[2]=normalize(cross(float3(-1.0,0.0,0.0),normal_rotation_matrix[1]));
	normal_rotation_matrix[0]=normalize(cross(normal_rotation_matrix[2],normal_rotation_matrix[1]));

	//applying base rotation matrix to detail normal
	detail_normal_rotated = mul(detail_normal,normal_rotation_matrix);

	// writing output params
    output.position = mul(float4(vertexPosition, 1.0), g_ModelViewProjectionMatrix);
	output.normal = detail_normal_rotated;
	output.positionWS = vertexPosition;
	output.layerdef = layerdef;
	output.brightness = detail_height;
    return output;
}

static const float kTopDownDataPixelsPerMeter = 256.0f/700.0; // taken from SDF generation source code, the SDF texture size is 256x256, the viewport size is 700x700
static const float kMaxDepthBelowSea = 50.0f;
static const float kMaxDistance = 20.0f; // taken from SDF generation code
static const float kNumWaves = 1.0; // Total number of Gerster waves of different amplitude, speed etc to calculate, 
									// i+1-th wave has 20% smaller amplitude, 
								    // 20% smaller phase and group speed and 20% less parallelity
							        // Note that all the waves will share the same gerstnerMultiplierOut (lerping between ocean waves and Gerstner waves) for simplicity
static const float kBackWaveSpeed = 0.5;  // the speed of wave rolling back from shore, in vertical dimension, in meters/sec

void GetGerstnerShoreAttributes(float3 posWS, out float waveOut, out float3 normalOut, out float foamTrailOut, out float2 foamWSShift, out float waterLayerOut, out float waterLayerSlowOut)
{
	// getting UV for fetching SDF texture 
	float4 topDownPosition = mul( float4( posWS.xyz, 1), g_WorldToTopDownTextureMatrix );
	float2 uv = mad( topDownPosition.xy/topDownPosition.w, 0.5f, 0.5f );
	uv.y = 1-uv.y;

	// initializing the outputs
	normalOut = float3(0.0,1.0,0.0);
	waveOut = 0;
	foamWSShift = float2(0.0,0.0);
	foamTrailOut = 0;
	waterLayerOut = 0;
	waterLayerSlowOut = 0;

	// getting SDF
	const float4 tdData = g_DataTexture.SampleLevel(SamplerLinearBorder, uv, 0 );
	
	// getting terrain altitude gradient in y meters per xz meter
	float terrain_dy = 0.25*(tdData.y - g_DataTexture.SampleLevel(SamplerLinearBorder, uv - kTopDownDataPixelsPerMeter*float2(tdData.z,-tdData.w)/256.0, 0 ).y);

	// initializing variables common to all Gerstner waves
	float phaseShift = g_Time;
	float sdfPhase = tdData.x*kMaxDistance/kTopDownDataPixelsPerMeter; 
	float distanceMultiplier = saturate(1.0-tdData.x); // Shore waves linearly fade in on the edges of SDF
	float depthMultiplier = saturate((g_BaseGerstnerWavelength*0.5 + tdData.y)); // Shore waves fade in when depth is less than half the wave length

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
		float gerstnerMultiplier = distanceMultiplier*depthMultiplier*worldSpacePosMultiplier*groupSpeedMultiplier; 
	
		float steepness = gerstnerMultiplier * g_GerstnerSteepness;	// steepness gradually increases as wave runs over shallower seabed
		float baseAmplitude = gerstnerMultiplier * gerstnerAmplitude; //amplitude gradually increases as wave runs over shallower seabed
		float skewMultiplier = saturate((baseAmplitude*2.0*1.28 + tdData.y)/gerstnerAmplitude); // Wave height is 2*amplitude, a wave will start to break when it approximately reaches a water depth of 1.28 times the wave height, empirically: http://passyworldofmathematics.com/mathematics-of-ocean-waves-and-surfing/ 
		float breakerMultiplier = saturate((baseAmplitude*2.0*1.28 + tdData.y)/gerstnerAmplitude); // Wave height is 2*amplitude, a wave will start to break when it approximately reaches a water depth of 1.28 times the wave height, empirically: http://passyworldofmathematics.com/mathematics-of-ocean-waves-and-surfing/ 

		// calculating Gerstner offset
		float s,c;
		sincos(gerstnerPhase + gerstnerOmega*gerstnerSpeed*phaseShift, s, c);
		float waveVerticalOffset = (s*0.5+0.5)*(s*0.5+0.5);

		// calculating normal
		normalOut.y -= gerstnerOmega*steepness*baseAmplitude*s;
		normalOut.xz -= gerstnerOmega*baseAmplitude*c*propagationDirection;   // orienting normal according to direction of wave propagation. No need to normalize, it is unit length.

		// calculating foam parameters
		foamTrailOut += gerstnerMultiplier*breakerMultiplier;

		// calculating wave falling edges moving slow and fast
		float foamTrailPhase = gerstnerPhase + gerstnerOmega*gerstnerSpeed*phaseShift + 3.141592*0.05; // delaying foam trail a bit so it's following the breaker
		float fp = frac(foamTrailPhase/(3.141592*2.0));

		float k = kBackWaveSpeed*terrain_dy/((gerstnerSpeed/gerstnerWavelength)*baseAmplitude);
		float sawtooth = 1.0 - k + k*(saturate(fp*10.0) - saturate(fp*1.1));
		waterLayerOut += sawtooth*baseAmplitude + baseAmplitude;

		k = kBackWaveSpeed/(gerstnerOmega*gerstnerSpeed);
		sawtooth = k*(saturate(fp*10.0) - saturate(fp*1.1));

		foamWSShift += 10.0*sawtooth*propagationDirection*gerstnerAmplitude;

		k = 0.33*kBackWaveSpeed*terrain_dy/((gerstnerSpeed/gerstnerWavelength)*baseAmplitude);
		sawtooth = 1.0 - k + k*(saturate(fp*10.0) - saturate(fp*1.1));
		waterLayerSlowOut += sawtooth*baseAmplitude + baseAmplitude;

		waveOut += waveVerticalOffset*baseAmplitude;
		
		// updating the parameters for next wave
		gerstnerWavelength *= 0.66;
		gerstnerOmega /= 0.66;
		gerstnerSpeed *= 0.66;
		gerstnerAmplitude *= 0.66; 
		gerstnerParallelness *= 0.66;
		windDirection.xy *= float2(-1.0,1.0)*windDirection.yx; // rotating wind direction 
		
	}
}	

float4 HeightFieldPatchPS(PSIn_Diffuse input) : SV_Target
{
	float3 color;
	float3 pixel_to_light_vector = normalize(g_LightPosition-input.positionWS);
	float3 pixel_to_eye_vector = normalize(g_CameraPosition-input.positionWS);

	// culling halfspace if needed
	clip(g_HalfSpaceCullSign*(input.positionWS.y-g_HalfSpaceCullPosition));
	
	float darkening_change_rate = min(1.0,1.0/(3.0*g_BaseGerstnerAmplitude)); 
	float shore_darkening_factor = saturate((input.positionWS.y + 1.0)*darkening_change_rate);

	// getting diffuse color
	color = float3(0.3,0.3,0.3);

	// adding per-vertex lighting defined by displacement of vertex 
	color*=0.5+0.5*min(1.0,max(0.0, input.brightness/3.0f+0.5f));

	// calculating pixel position in light view space
	float4 positionLS = mul(float4(input.positionWS,1),g_LightModelViewProjectionMatrix);
	positionLS.xyz/=positionLS.w;
	positionLS.x=(positionLS.x+1)*0.5;
	positionLS.y=(1-positionLS.y)*0.5;

	// fetching shadowmap and shading
	float dsf = 0.66f/4096.0f;
	float shadow_factor = 0.2*g_ShadowmapTexture.SampleCmp(SamplerDepthAnisotropic,positionLS.xy,positionLS.z* 0.99f).r;
	shadow_factor+=0.2*g_ShadowmapTexture.SampleCmp(SamplerDepthAnisotropic,positionLS.xy+float2(dsf,dsf),positionLS.z* 0.99f).r;
	shadow_factor+=0.2*g_ShadowmapTexture.SampleCmp(SamplerDepthAnisotropic,positionLS.xy+float2(-dsf,dsf),positionLS.z* 0.99f).r;
	shadow_factor+=0.2*g_ShadowmapTexture.SampleCmp(SamplerDepthAnisotropic,positionLS.xy+float2(dsf,-dsf),positionLS.z* 0.99f).r;
	shadow_factor+=0.2*g_ShadowmapTexture.SampleCmp(SamplerDepthAnisotropic,positionLS.xy+float2(-dsf,-dsf),positionLS.z* 0.99f).r;
	color *= g_AtmosphereBrightColor*max(0,dot(pixel_to_light_vector,input.normal))*shadow_factor;

	// making all brighter
	color*=2.0;
	// adding light from the sky
	color += (0.0+0.2*max(0,(dot(float3(0,1,0),input.normal))))*g_AtmosphereDarkColor;


	// calculating shore effects
	if((g_enableShoreEffects > 0) && (shore_darkening_factor < 1.0))
	{
		float3 normal;
		float foam_trail;
		float water_layer;
		float water_layer_slow;
		float wave_pos;
		float2 foamWSShift;

		GetGerstnerShoreAttributes(input.positionWS, wave_pos, normal, foam_trail, foamWSShift, water_layer, water_layer_slow);

		float waterlayer_change_rate = max(2.0,1.0/(0.1 + water_layer_slow - water_layer));

		float underwater_factor = saturate((input.positionWS.y - wave_pos + 2.0)*5.0);
		float darkening_factor = saturate((input.positionWS.y - g_BaseGerstnerAmplitude*2.0 + 2.0)*1.0);
		float fresnel_damp_factor = saturate((input.positionWS.y + 0.1 - wave_pos + 2.0)*5.0);
		float shore_waterlayer_factor_windy = saturate((input.positionWS.y - water_layer + 2.0)*waterlayer_change_rate);
		float shore_waterlayer_factor_calm = saturate((input.positionWS.y + 2.0)*10.0);
		float shore_waterlayer_factor = lerp(shore_waterlayer_factor_calm, shore_waterlayer_factor_windy, saturate(g_BaseGerstnerAmplitude*5.0));
		float shore_foam_lower_bound_factor = saturate((input.positionWS.y + g_BaseGerstnerAmplitude - wave_pos + 2.0)*min(3.0,3.0/(2.0*g_BaseGerstnerAmplitude)));
	

		float3 reflected_eye_to_pixel_vector=-pixel_to_eye_vector+2*dot(pixel_to_eye_vector,input.normal)*input.normal;
		float specular_light = pow(max(0,dot(reflected_eye_to_pixel_vector,pixel_to_light_vector)),40.0);

		// calculating fresnel factor 
		float r = (1.0 - 1.33)*(1.0 - 1.33)/((1.0 + 1.33)*(1.0 + 1.33));
		float fresnel_factor = r + (1.0-r)*pow(saturate(1.0 - dot(input.normal,pixel_to_eye_vector)),5.0);

		fresnel_factor *= (1.0-shore_waterlayer_factor)*fresnel_damp_factor; 

		// darkening the terrain close to water
		color *= 0.6 + 0.4*darkening_factor;
		// darkening terrain underwater
		color *= min(1.0,exp((input.positionWS.y + 2.0)));

		// adding specular
		color += 5.0*g_AtmosphereBrightColor*specular_light*shadow_factor*fresnel_factor;

		// calculating reflection color
		float3 reflection_color = CalculateFogColor(pixel_to_light_vector,-reflected_eye_to_pixel_vector);

		color = lerp(color, reflection_color.rgb, fresnel_factor);

		// adding foam
		float2 positionWS_shifted = input.positionWS.xz + foamWSShift;
		float foam_intensity_map_lf = g_FoamIntensityTexture.Sample(SamplerLinearWrap, positionWS_shifted*0.04*float2(1,1)).x - 1.0;
		float foam_intensity_map_hf = g_FoamIntensityTexture.Sample(SamplerLinearWrap, positionWS_shifted*0.15*float2(1,1)).x - 1.0;

		float foam_intensity;
		float k = 1.5;
		float ff2 = (2.0/g_BaseGerstnerAmplitude)*saturate(input.positionWS.y - water_layer*0.8 + 2.0);
		float ff = (1.0-ff2)*shore_foam_lower_bound_factor*foam_trail;
		foam_intensity = saturate(foam_intensity_map_hf + min(3.5,k*ff-0.2)); 
		foam_intensity += (foam_intensity_map_lf + min(1.5,k*ff)); 
		foam_intensity = max(0.0, foam_intensity);
		float foam_bubbles = g_FoamDiffuseTexture.Sample(SamplerLinearWrap, positionWS_shifted*0.5).r;
		foam_bubbles = saturate(5.0*(foam_bubbles-0.8));
		foam_intensity = pow(foam_intensity, 0.7);
		foam_intensity = saturate(foam_intensity*foam_bubbles*1.0);

		// foam diffuse color
		float foam_diffuse_factor = max(0,0.8+max(0,0.2*dot(pixel_to_light_vector,normal)));

		color = lerp(color, foam_diffuse_factor*float3(1.0,1.0,1.0),foam_intensity);
	}

	// applying fog
	if(g_ApplyFog > 0)
	{
		color = lerp(CalculateFogColor(pixel_to_light_vector,pixel_to_eye_vector).rgb, color, min(1,exp(-length(g_CameraPosition-input.positionWS)*g_FogDensity)));
	}


	return float4(color, length(g_CameraPosition-input.positionWS));
}

float4 HeightFieldPatchDataPS( PSIn_Diffuse input ) : SV_Target
{
	float y_biased = input.positionWS.y + 2.0;
	return float4( y_biased, y_biased, 0, 0 );
}


//--------------------------------------------------------------------------------------
// Fullscreen shaders
//--------------------------------------------------------------------------------------

PSIn_Quad FullScreenQuadVS(uint VertexId: SV_VertexID)
{
    PSIn_Quad output;

	output.position = float4(QuadVertices[VertexId],0,1);
    output.texcoord = QuadTexCoordinates[VertexId];
    
    return output;
}

float4 MainToBackBufferPS(PSIn_Quad input) : SV_Target
{
	float4 color;
	color.rgb = g_MainTexture.SampleLevel(SamplerLinearWrap,float2((input.texcoord.x-0.5)/g_MainBufferSizeMultiplier+0.5f,(input.texcoord.y-0.5)/g_MainBufferSizeMultiplier+0.5f),0).rgb;
	color.a=0;
	return color;
}

float RefractionDepthManualResolvePS1(PSIn_Quad input) : SV_Target
{
	return g_RefractionDepthTextureMS1.Load(input.position.xy,0,int2(0,0)).r;
}

float RefractionDepthManualResolvePS2(PSIn_Quad input) : SV_Target
{
	return g_RefractionDepthTextureMS2.Load(input.position.xy,0,int2(0,0)).r;
}

float RefractionDepthManualResolvePS4(PSIn_Quad input) : SV_Target
{
	return g_RefractionDepthTextureMS4.Load(input.position.xy,0,int2(0,0)).r;
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique11 RefractionDepthManualResolve
{
    pass MS1
    {
        SetRasterizerState(NoCullMS);
        SetDepthStencilState(NoDepthStencil, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_4_0, FullScreenQuadVS()));
        SetHullShader(NULL);
        SetDomainShader(NULL);
        SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, RefractionDepthManualResolvePS1()));
    }
    pass MS2
    {
        SetRasterizerState(NoCullMS);
        SetDepthStencilState(NoDepthStencil, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_4_0, FullScreenQuadVS()));
        SetHullShader(NULL);
        SetDomainShader(NULL);
        SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, RefractionDepthManualResolvePS2()));
    }
    pass MS4
    {
        SetRasterizerState(NoCullMS);
        SetDepthStencilState(NoDepthStencil, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_4_0, FullScreenQuadVS()));
        SetHullShader(NULL);
        SetDomainShader(NULL);
        SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, RefractionDepthManualResolvePS4()));
    }
}

technique11 MainToBackBuffer
{
    pass Solid
    {
        SetRasterizerState(NoCullMS);
        SetDepthStencilState(NoDepthStencil, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

        SetVertexShader(CompileShader(vs_4_0, FullScreenQuadVS()));
        SetHullShader(NULL);
        SetDomainShader(NULL);
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, MainToBackBufferPS()));
    }
}

technique11 RenderHeightfield
{
    pass Solid
    {
        SetRasterizerState(CullBackMS);
        SetDepthStencilState(DepthNormal, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

        SetVertexShader(CompileShader(vs_4_0, PassThroughVS()));
        SetHullShader(CompileShader(hs_5_0, PatchHS()));
        SetDomainShader(CompileShader(ds_5_0, HeightFieldPatchDS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, HeightFieldPatchPS()));
    }
    pass Wireframe
    {
        SetRasterizerState(WireframeMS);
        SetDepthStencilState(DepthNormal, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

        SetVertexShader(CompileShader(vs_4_0, PassThroughVS()));
        SetHullShader(CompileShader(hs_5_0, PatchHS()));
        SetDomainShader(CompileShader(ds_5_0, HeightFieldPatchDS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, ColorPS(float4(1.0f, 1.0f, 1.0f, 0.0f))));
    }

    pass DepthOnly
    {
        SetRasterizerState(CullBackMS);
        SetDepthStencilState(DepthNormal, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

        SetVertexShader(CompileShader(vs_4_0, PassThroughVS()));
        SetHullShader(CompileShader(hs_5_0, PatchHS()));
        SetDomainShader(CompileShader(ds_5_0, HeightFieldPatchDS()));
        SetGeometryShader(NULL);
        SetPixelShader(NULL);
    }

	pass DataPass
    {
        SetRasterizerState(CullBackMS);
        SetDepthStencilState(DepthNormal, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

        SetVertexShader(CompileShader(vs_4_0, PassThroughVS()));
        SetHullShader(CompileShader(hs_5_0, PatchHS()));
        SetDomainShader(CompileShader(ds_5_0, HeightFieldPatchDS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, HeightFieldPatchDataPS()));
    }
}


technique11 RenderSky
{
    pass Solid
    {
        SetRasterizerState(NoCullMS);
        SetDepthStencilState(DepthNormal, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

        SetVertexShader(CompileShader(vs_4_0, SkyVS()));
        SetHullShader(NULL);
        SetDomainShader(NULL);
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, SkyPS()));
    }
    pass Wireframe
    {
        SetRasterizerState(WireframeMS);
        SetDepthStencilState(DepthNormal, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

        SetVertexShader(CompileShader(vs_4_0, SkyVS()));
        SetHullShader(NULL);
        SetDomainShader(NULL);
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, ColorPS(float4(1.0f, 1.0f, 1.0f, 0.0f))));
    }
}
