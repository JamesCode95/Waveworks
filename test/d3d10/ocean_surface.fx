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
 
#define GFSDK_WAVEWORKS_SM4

#define GFSDK_WAVEWORKS_DECLARE_GEOM_VS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_GEOM_VS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_GEOM_VS_CBUFFER };
#include "GFSDK_WaveWorks_Quadtree.fxh"

#define GFSDK_WAVEWORKS_DECLARE_ATTR_VS_SAMPLER(Label,TextureLabel,Regoff) sampler Label; texture2D TextureLabel;
#define GFSDK_WAVEWORKS_DECLARE_ATTR_VS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_ATTR_VS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_ATTR_VS_CBUFFER };

#define GFSDK_WAVEWORKS_DECLARE_ATTR_PS_SAMPLER(Label,TextureLabel,Regoff) sampler Label; texture2D TextureLabel;
#define GFSDK_WAVEWORKS_DECLARE_ATTR_PS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_ATTR_PS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_ATTR_PS_CBUFFER };
#include "GFSDK_WaveWorks_Attributes.fxh"

//------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------

// Constant
float4x4	g_matViewProj;

float3		g_SkyColor;
float3		g_DeepColor;
float3		g_BendParam = {0.1f, -0.4f, 0.2f};

float3		g_SunDir = {0.936016f, -0.343206f, 0.0780013f};
float3		g_SunColor = {1.0f, 1.0f, 0.6f};
float		g_Shineness = 20.0f;
float3      g_WaterDeepColor={0.0,0.4,0.6};
float3      g_WaterScatterColor={0.0,0.7,0.6};
float3      g_WaterSpecularColor={1,0.8,0.5};
float2      g_WaterColorIntensity={0.2,0.1};
float		g_WaterSpecularIntensity = 0.7f;

float3		g_FoamColor = {0.90f, 0.95f, 1.0f};
float3		foam_underwater_color = {0.90f, 0.95f, 1.0f};

//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
texture1D	g_texColorMap;
texture2D	g_texBufferMap;
textureCUBE	g_texCubeMap;
texture2D	g_texFoamIntensityMap;
texture2D	g_texFoamDiffuseMap;

// Blending map for ocean color
sampler g_samplerColorMap =
sampler_state
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = Clamp;
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

// Environment map
sampler g_samplerCubeMap =
sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};

struct VS_OUTPUT
{
    float4								pos_clip	 : SV_Position;
    GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT NV_ocean_interp;
	float3								world_displacement: TEXCOORD4;
	float3								world_pos_undisplaced: TEXCOORD5;
};

//-----------------------------------------------------------------------------
// Name: OceanWaveVS
// Type: Vertex shader                                      
// Desc: 
//-----------------------------------------------------------------------------
VS_OUTPUT OceanWaveVS(GFSDK_WAVEWORKS_VERTEX_INPUT In)
{
	GFSDK_WAVEWORKS_VERTEX_OUTPUT NV_ocean = GFSDK_WaveWorks_GetDisplacedVertex(In);
	VS_OUTPUT Output;

	Output.NV_ocean_interp = NV_ocean.interp;
	Output.pos_clip = mul(float4(NV_ocean.pos_world,1), g_matViewProj);
	Output.world_displacement = NV_ocean.world_displacement;
	Output.world_pos_undisplaced = NV_ocean.pos_world - NV_ocean.world_displacement;
	return Output; 
}

//-----------------------------------------------------------------------------
// Name: OceanWavePS
// Type: Pixel shader                                      
// Desc: 
//-----------------------------------------------------------------------------
float4 OceanWavePS(VS_OUTPUT In) : SV_Target
{
	GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES surface_attributes = GFSDK_WaveWorks_GetSurfaceAttributes(In.NV_ocean_interp);

	float fresnel_factor;
	float diffuse_factor;
	float specular_factor;
	float scatter_factor;

	float3 pixel_to_light_vector=g_SunDir;
	float3 pixel_to_eye_vector=surface_attributes.eye_dir;
	float3 reflected_eye_to_pixel_vector = reflect(-surface_attributes.eye_dir, surface_attributes.normal);

	
	float cos_angle = dot(surface_attributes.normal, surface_attributes.eye_dir);
	// ramp.x for fresnel term. ramp.y for atmosphere blending
	float3 ramp = g_texColorMap.Sample(g_samplerColorMap, cos_angle).xyz;
	// A worksaround to deal with "indirect reflection vectors" (which are rays requiring multiple
	// reflections to reach the sky).
	if (reflected_eye_to_pixel_vector.z < g_BendParam.x)
		ramp = lerp(ramp, g_BendParam.z, (g_BendParam.x - reflected_eye_to_pixel_vector.z)/(g_BendParam.x - g_BendParam.y));
	reflected_eye_to_pixel_vector.z = max(0, reflected_eye_to_pixel_vector.z);



	// simulating scattering/double refraction: light hits the side of wave, travels some distance in water, and leaves wave on the other side
	// it's difficult to do it physically correct without photon mapping/ray tracing, so using simple but plausible emulation below
	
	// only the crests of water waves generate double refracted light
	scatter_factor=0.01*max(0,In.world_displacement.z*0.001+0.3);

	
	// the waves that lie between camera and light projection on water plane generate maximal amount of double refracted light 
	scatter_factor*=pow(max(0.0,dot(normalize(float3(pixel_to_light_vector.x,0.0,pixel_to_light_vector.z)),-pixel_to_eye_vector)),2.0);
	
	// the slopes of waves that are oriented back to light generate maximal amount of double refracted light 
	scatter_factor*=pow(max(0.0,0.5-0.5*dot(pixel_to_light_vector,surface_attributes.normal)),3.0);
	
	
	// water crests gather more light than lobes, so more light is scattered under the crests
	scatter_factor+=2.0*g_WaterColorIntensity.y*max(0,In.world_displacement.z*0.001+0.3)*
		// the scattered light is best seen if observing direction is normal to slope surface
		max(0,dot(pixel_to_eye_vector,surface_attributes.normal));

	
	// calculating fresnel factor 
	//float r=(1.2-1.0)/(1.2+1.0);
	//fresnel_factor = max(0.0,min(1.0,r+(1.0-r)*pow(1.0-dot(surface_attributes.normal,pixel_to_eye_vector),2.0)));

	//float r=(1.0 - 1.13)*(1.0 - 1.13)/(1.0 + 1.13);
	//fresnel_factor = r + (1.0-r)*pow(saturate(1.0 - dot(surface_attributes.normal,pixel_to_eye_vector)),4.0);

	fresnel_factor=ramp.x;

	// calculating diffuse intensity of water surface itself
	diffuse_factor=g_WaterColorIntensity.x+g_WaterColorIntensity.y*max(0,dot(pixel_to_light_vector,surface_attributes.normal));
	
	float3 refraction_color=diffuse_factor*g_WaterDeepColor;
	
	// adding color that provide foam bubbles spread in water 
	refraction_color += foam_underwater_color*saturate(surface_attributes.foam_turbulent_energy*0.2);

	// adding scatter light component
	refraction_color+=g_WaterScatterColor*scatter_factor;

	// reflection color
	float3 reflection_color = lerp(g_SkyColor,g_texCubeMap.Sample(g_samplerCubeMap, reflected_eye_to_pixel_vector).xyz, ramp.y);

	// applying Fresnel law
	float3 water_color = lerp(refraction_color,reflection_color,fresnel_factor);
	

	// applying surface foam provided by turbulent energy

	// low frequency foam map
	float foam_intensity_map_lf = 1.0*g_texFoamIntensityMap.Sample(g_samplerTrilinear, In.world_pos_undisplaced.xy*0.04*float2(1,1)).x - 1.0;

	// high frequency foam map
	float foam_intensity_map_hf = 1.0*g_texFoamIntensityMap.Sample(g_samplerTrilinear, In.world_pos_undisplaced.xy*0.15*float2(1,1)).x - 1.0;

	// ultra high frequency foam map
	float foam_intensity_map_uhf = 1.0*g_texFoamIntensityMap.Sample(g_samplerTrilinear, In.world_pos_undisplaced.xy*0.3*float2(1,1)).x;

	float foam_intensity;
	foam_intensity = saturate(foam_intensity_map_hf + min(3.5,1.0*surface_attributes.foam_turbulent_energy-0.2)); 
	foam_intensity += (foam_intensity_map_lf + min(1.5,1.0*surface_attributes.foam_turbulent_energy)); 

	
	foam_intensity -= 0.1*saturate(-surface_attributes.foam_surface_folding);
	
	foam_intensity = max(0,foam_intensity);

	foam_intensity *= 1.0+0.8*saturate(surface_attributes.foam_surface_folding);

	float foam_bubbles = g_texFoamDiffuseMap.Sample(g_samplerTrilinear, In.world_pos_undisplaced.xy).r;
	foam_bubbles = saturate(5.0*(foam_bubbles-0.8));

	// applying foam hats
	foam_intensity += max(0,foam_intensity_map_uhf*2.0*surface_attributes.foam_wave_hats);//*(1.0 + surface_attributes.foam_surface_folding*0.5);

	foam_intensity = pow(foam_intensity, 0.7);
	foam_intensity = saturate(foam_intensity*foam_bubbles*1.0);// + 0.1*foam_bubbles*saturate(surface_attributes.foam_surface_folding)));


	// foam diffuse color
	float foam_diffuse_factor=max(0,0.8+max(0,0.2*dot(pixel_to_light_vector,surface_attributes.normal)));


	water_color = lerp(water_color,foam_diffuse_factor*float3(1.0,1.0,1.0),foam_intensity);
	
	// calculating specular factor
	reflected_eye_to_pixel_vector=-pixel_to_eye_vector+2.0*dot(pixel_to_eye_vector,surface_attributes.normal)*surface_attributes.normal;
	specular_factor=pow(max(0,dot(pixel_to_light_vector,reflected_eye_to_pixel_vector)),g_Shineness);

	// adding specular component
	//water_color+=g_WaterSpecularIntensity*specular_factor*g_WaterSpecularColor*/*fresnel_factor*/saturate(1.0-5.0*foam_intensity);

	return float4(water_color, 1);
	

}

float4 g_PatchColor;

float4 OceanWireframePS(VS_OUTPUT In) : SV_Target
{
	return g_PatchColor;
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

DepthStencilState AlwaysDepth
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
// RasterStates
//--------------------------------------------------------------------------------------
RasterizerState CullFront
{
	FillMode = SOLID;
	CullMode = Front;
	
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
technique10 RenderOceanSurfTech
{
    pass Pass_PatchVS_WavePS
    {
        SetVertexShader( CompileShader( vs_4_0, OceanWaveVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, OceanWavePS() ) );

        SetDepthStencilState( EnableDepth, 0 );
        SetRasterizerState( CullFront );
        SetBlendState( Opaque, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }

    // Wireframe
    pass Pass_PatchWireframe
    {
        SetVertexShader( CompileShader( vs_4_0, OceanWaveVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, OceanWireframePS() ) );

		SetDepthStencilState( EnableDepth, 0 );
		SetRasterizerState( Wireframe );
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
