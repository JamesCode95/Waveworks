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

#ifndef _TERRAIN_H
#define _TERRAIN_H

#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKMisc.h"
#include "d3dx11effect.h"
#include "d3d11.h"
#include "DirectXMath.h"

using namespace DirectX;

#define terrain_gridpoints					512
#define terrain_numpatches_1d				64
#define terrain_geometry_scale				1.0f
#define terrain_maxheight					35.0f 
#define terrain_minheight					-30.0f 
#define terrain_fractalfactor				0.51f;
#define terrain_fractalinitialvalue			100.0f
#define terrain_smoothfactor1				0.9f
#define terrain_smoothfactor2				-0.03f
#define terrain_rockfactor					0.97f
#define terrain_smoothsteps					40

#define terrain_height_underwater_start		-100.0f
#define terrain_height_underwater_end		-8.0f
#define terrain_height_sand_start			-30.0f
#define terrain_height_sand_end				1.7f
#define terrain_height_grass_start			1.7f
#define terrain_height_grass_end			30.0f
#define terrain_height_rocks_start			-2.0f
#define terrain_height_trees_start			4.0f
#define terrain_height_trees_end			30.0f
#define terrain_slope_grass_start			0.96f
#define terrain_slope_rocks_start			0.85f

#define terrain_far_range terrain_gridpoints*terrain_geometry_scale

#define shadowmap_resource_buffer_size_xy				4096
#define water_normalmap_resource_buffer_size_xy			2048
#define terrain_layerdef_map_texture_size				1024
#define terrain_depth_shadow_map_texture_size			512

#define sky_gridpoints						20
#define sky_texture_angle					0.34f

#define main_buffer_size_multiplier			1.1f
#define reflection_buffer_size_multiplier   1.1f
#define refraction_buffer_size_multiplier   1.1f

#define scene_z_near						1.0f
#define scene_z_far							25000.0f
#define camera_fov							110.0f

class CTerrain
{
	public:


		void Initialize(ID3D11Device*, ID3DX11Effect*);
		void DeInitialize();
		void ReCreateBuffers();
		HRESULT LoadTextures();
		void Render(CFirstPersonCamera *);
		void RenderTerrainToHeightField(ID3D11DeviceContext* const pContext, const XMMATRIX& worldToViewMatrix, const XMMATRIX& viewToProjectionMatrix, const XMVECTOR eyePositionWS, const XMVECTOR viewDirectionWS);
		void CreateTerrain();

		float DynamicTesselationFactor;
		void SetupNormalView(CFirstPersonCamera *);
		void SetupReflectionView(CFirstPersonCamera *);
		void SetupRefractionView(CFirstPersonCamera *);
		void SetupLightView(CFirstPersonCamera * );
		float BackbufferWidth;
		float BackbufferHeight;

		UINT MultiSampleCount;
		UINT MultiSampleQuality;

		ID3D11ShaderResourceView	*rock_bump_textureSRV;

		ID3D11ShaderResourceView *sky_textureSRV;	

		ID3D11ShaderResourceView *foam_intensity_textureSRV;

		ID3D11ShaderResourceView *foam_diffuse_textureSRV;

		ID3D11Texture2D			 *reflection_color_resource;
		ID3D11ShaderResourceView *reflection_color_resourceSRV;
		ID3D11RenderTargetView   *reflection_color_resourceRTV;
		ID3D11Texture2D			 *refraction_color_resource;
		ID3D11ShaderResourceView *refraction_color_resourceSRV;
		ID3D11RenderTargetView   *refraction_color_resourceRTV;

		ID3D11Texture2D			 *shadowmap_resource;
		ID3D11ShaderResourceView *shadowmap_resourceSRV;
		ID3D11DepthStencilView   *shadowmap_resourceDSV;

		ID3D11Texture2D			 *reflection_depth_resource;
		ID3D11DepthStencilView   *reflection_depth_resourceDSV;

		ID3D11Texture2D			 *refraction_depth_resource;
		ID3D11RenderTargetView   *refraction_depth_resourceRTV;
		ID3D11ShaderResourceView *refraction_depth_resourceSRV;

		ID3D11Texture2D			 *main_color_resource;
		ID3D11ShaderResourceView *main_color_resourceSRV;
		ID3D11RenderTargetView   *main_color_resourceRTV;
		ID3D11Texture2D			 *main_depth_resource;
		ID3D11DepthStencilView   *main_depth_resourceDSV;
		ID3D11ShaderResourceView *main_depth_resourceSRV;
		ID3D11Texture2D			 *main_color_resource_resolved;
		ID3D11ShaderResourceView *main_color_resource_resolvedSRV;

		ID3D11Device* pDevice;
		ID3DX11Effect* pEffect;




		float				height[terrain_gridpoints+1][terrain_gridpoints+1];
		XMFLOAT3			normal[terrain_gridpoints+1][terrain_gridpoints+1];
		XMFLOAT3			tangent[terrain_gridpoints + 1][terrain_gridpoints + 1];
		XMFLOAT3			binormal[terrain_gridpoints + 1][terrain_gridpoints + 1];

		ID3D11Texture2D		*heightmap_texture;
		ID3D11ShaderResourceView *heightmap_textureSRV;

		ID3D11Texture2D		*layerdef_texture;
		ID3D11ShaderResourceView *layerdef_textureSRV;

		ID3D11Texture2D		*depthmap_texture;
		ID3D11ShaderResourceView *depthmap_textureSRV;

		ID3D11Buffer		*heightfield_vertexbuffer;
		ID3D11Buffer		*sky_vertexbuffer;

		ID3D11InputLayout   *heightfield_inputlayout;
		ID3D11InputLayout   *trianglestrip_inputlayout;

};

float bilinear_interpolation(float fx, float fy, float a, float b, float c, float d);

#endif // _TERRAIN_H