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
 
#include "dxstdafx.h"
#include "ocean_surface.h"
#include "SDKmisc.h"

#include "GFSDK_WaveWorks_D3D_Util.h"

#pragma warning(disable:4127)

OceanSurface::OceanSurface()
{
	m_pBicolorMap	= NULL;
	m_pCubeMap		= NULL;
	m_pFoamDiffuseMap = NULL;
	m_pFoamIntensityMap = NULL;
	m_pQuadLayout		= NULL;
	m_pOceanFX		= NULL;
	m_pRenderSurfaceTechnique = NULL;
	m_pRenderSurfaceShadedPass = NULL;
	m_pRenderSurfaceWireframePass = NULL;
	m_hOceanQuadTree	= NULL;

	UINT NumQuadtreeShaderInputs = GFSDK_WaveWorks_Quadtree_GetShaderInputCountD3D10();
	UINT NumSimulationShaderInputs = GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D10();
	m_pQuadTreeShaderInputMappings_Shaded = new UINT [NumQuadtreeShaderInputs];
	m_pQuadTreeShaderInputMappings_Wireframe = new UINT [NumQuadtreeShaderInputs];
	m_pSimulationShaderInputMappings_Shaded = new UINT [NumSimulationShaderInputs];
	m_pSimulationShaderInputMappings_Wireframe = new UINT [NumSimulationShaderInputs];

	m_pRenderSurfaceMatViewProjVariable = NULL;
	m_pRenderSurfaceSkyColorVariable = NULL;
	m_pRenderSurfaceWaterColorVariable = NULL;
	m_pRenderSurfacePatchColorVariable = NULL;
	m_pRenderSurfaceColorMapVariable = NULL;
	m_pRenderSurfaceCubeMapVariable = NULL;

	m_pd3dDevice = DXUTGetD3D10Device();

	memset(&m_params, 0, sizeof(m_params));
}

OceanSurface::~OceanSurface()
{
	if(m_hOceanQuadTree)
	{
		GFSDK_WaveWorks_Quadtree_Destroy(m_hOceanQuadTree);
		m_hOceanQuadTree = NULL;
	}

	SAFE_RELEASE(m_pQuadLayout);
	SAFE_RELEASE(m_pOceanFX);

	SAFE_RELEASE(m_pBicolorMap);
	SAFE_RELEASE(m_pCubeMap);

	SAFE_RELEASE(m_pFoamDiffuseMap);
	SAFE_RELEASE(m_pFoamIntensityMap);

	SAFE_DELETE_ARRAY(m_pQuadTreeShaderInputMappings_Shaded);
	SAFE_DELETE_ARRAY(m_pQuadTreeShaderInputMappings_Wireframe);
	SAFE_DELETE_ARRAY(m_pSimulationShaderInputMappings_Shaded);
	SAFE_DELETE_ARRAY(m_pSimulationShaderInputMappings_Wireframe);
}

HRESULT OceanSurface::initQuadTree(const GFSDK_WaveWorks_Quadtree_Params& params)
{
	if(NULL == m_hOceanQuadTree)
{
		return GFSDK_WaveWorks_Quadtree_CreateD3D10(params, m_pd3dDevice, &m_hOceanQuadTree);
	}
	else
	{
		return GFSDK_WaveWorks_Quadtree_UpdateParams(m_hOceanQuadTree, params);
	}
}

UINT GetShaderInputRegisterMapping(ID3D11ShaderReflection* pReflectionVS, ID3D11ShaderReflection* pReflectionPS, const GFSDK_WaveWorks_ShaderInput_Desc& inputDesc)
{
	ID3D11ShaderReflection* pReflection = NULL;
	switch(inputDesc.Type)
	{
	case GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_FloatConstant:
	case GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler:
	case GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture:
	case GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_ConstantBuffer:
		pReflection = pReflectionVS;
		break;
	default:
		pReflection = pReflectionPS;
		break;
	}

	D3D11_SHADER_INPUT_BIND_DESC desc;
	HRESULT hr = pReflection->GetResourceBindingDescByName(inputDesc.Name, &desc);
	if(FAILED(hr))
	{
		// Shader doesn't use this input, mark it as unused
		return 0xFFFFFFFF;
	}

	return desc.BindPoint;
}

ID3D11ShaderReflection* GetReflection(const D3D10_PASS_SHADER_DESC& passShader)
{
	D3D10_EFFECT_SHADER_DESC shaderDesc;
	passShader.pShaderVariable->GetShaderDesc(passShader.ShaderIndex, &shaderDesc);
	ID3D11ShaderReflection* pResult = NULL;
	D3DReflect(shaderDesc.pBytecode, shaderDesc.BytecodeLength, IID_ID3D11ShaderReflection, (void**)&pResult);

	return pResult;
}

HRESULT OceanSurface::init(const OceanSurfaceParameters& params)
{
	HRESULT hr = S_OK;

	if(NULL == m_pOceanFX)
	{
		// HLSL
		DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
	#if defined( DEBUG ) || defined( _DEBUG )
		// Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows 
		// the shaders to be optimized and to run exactly the way they will run in 
		// the release configuration of this program.
		dwShaderFlags |= D3D10_SHADER_DEBUG;
		#endif

		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\ocean_surface_d3d10.fxo")));
		V_RETURN( D3DX10CreateEffectFromFile(	path, NULL, NULL,
												"fx_4_0", dwShaderFlags, 0, m_pd3dDevice, NULL,
												NULL, &m_pOceanFX, NULL, NULL ) );

		// Hook up the shader mappings
		m_pRenderSurfaceTechnique = m_pOceanFX->GetTechniqueByName("RenderOceanSurfTech");
		m_pRenderSurfaceShadedPass = m_pRenderSurfaceTechnique->GetPassByName("Pass_PatchVS_WavePS");
		m_pRenderSurfaceWireframePass = m_pRenderSurfaceTechnique->GetPassByName("Pass_PatchWireframe");

		m_pRenderSurfaceMatViewProjVariable = m_pOceanFX->GetVariableByName("g_matViewProj")->AsMatrix();
		m_pRenderSurfaceSkyColorVariable = m_pOceanFX->GetVariableByName("g_SkyColor")->AsVector();
		m_pRenderSurfaceWaterColorVariable = m_pOceanFX->GetVariableByName("g_DeepColor")->AsVector();
		m_pRenderSurfacePatchColorVariable = m_pOceanFX->GetVariableByName("g_PatchColor")->AsVector();
		m_pRenderSurfaceColorMapVariable = m_pOceanFX->GetVariableByName("g_texColorMap")->AsShaderResource();
		m_pRenderSurfaceCubeMapVariable = m_pOceanFX->GetVariableByName("g_texCubeMap")->AsShaderResource();
		m_pRenderSurfaceFoamIntensityMapVariable = m_pOceanFX->GetVariableByName("g_texFoamIntensityMap")->AsShaderResource();
		m_pRenderSurfaceFoamDiffuseMapVariable = m_pOceanFX->GetVariableByName("g_texFoamDiffuseMap")->AsShaderResource();

		D3D10_PASS_SHADER_DESC passShaderDesc;

		V_RETURN(m_pRenderSurfaceShadedPass->GetVertexShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pShadedReflectionVS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceShadedPass->GetPixelShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pShadedReflectionPS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceWireframePass->GetVertexShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pWireframeReflectionVS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceWireframePass->GetPixelShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pWireframeReflectionPS = GetReflection(passShaderDesc);

		UINT NumQuadtreeShaderInputs = GFSDK_WaveWorks_Quadtree_GetShaderInputCountD3D10();
		UINT NumSimulationShaderInputs = GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D10();

		for(UINT i = 0; i != NumQuadtreeShaderInputs; ++i)
		{
			GFSDK_WaveWorks_ShaderInput_Desc inputDesc;
			GFSDK_WaveWorks_Quadtree_GetShaderInputDescD3D10(i, &inputDesc);

			m_pQuadTreeShaderInputMappings_Shaded[i] = GetShaderInputRegisterMapping(pShadedReflectionVS, pShadedReflectionPS, inputDesc);
			m_pQuadTreeShaderInputMappings_Wireframe[i] = GetShaderInputRegisterMapping(pWireframeReflectionVS, pWireframeReflectionPS, inputDesc);
		}

		for(UINT i = 0; i != NumSimulationShaderInputs; ++i)
		{
			GFSDK_WaveWorks_ShaderInput_Desc inputDesc;
			GFSDK_WaveWorks_Simulation_GetShaderInputDescD3D10(i, &inputDesc);

			m_pSimulationShaderInputMappings_Shaded[i] = GetShaderInputRegisterMapping(pShadedReflectionVS, pShadedReflectionPS, inputDesc);
			m_pSimulationShaderInputMappings_Wireframe[i] = GetShaderInputRegisterMapping(pWireframeReflectionVS, pWireframeReflectionPS, inputDesc);
		}

		pShadedReflectionVS->Release();
		pWireframeReflectionVS->Release();
		pShadedReflectionPS->Release();
		pWireframeReflectionPS->Release();
	}

	if(NULL == m_pQuadLayout)
	{
		const D3D10_INPUT_ELEMENT_DESC quad_layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		};
		const UINT num_layout_elements = sizeof(quad_layout)/sizeof(quad_layout[0]);

		ID3D10EffectTechnique* pDisplayBufferTech = m_pOceanFX->GetTechniqueByName("DisplayBufferTech");

		D3D10_PASS_DESC PassDesc;
		V_RETURN(pDisplayBufferTech->GetPassByIndex(0)->GetDesc(&PassDesc));

		V_RETURN(m_pd3dDevice->CreateInputLayout(	quad_layout, num_layout_elements,
													PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize,
													&m_pQuadLayout
													));
	}

	// Textures
	if(NULL == m_pCubeMap)
	{
		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\reflect_cube.dds")));
		ID3D10Resource* pD3D10Resource = NULL;
		V_RETURN(D3DX10CreateTextureFromFile(m_pd3dDevice, path, NULL, NULL, &pD3D10Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D10Resource, NULL, &m_pCubeMap));
		SAFE_RELEASE(pD3D10Resource);
	}

	if(NULL == m_pFoamIntensityMap)
	{
		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\foam_intensity_perlin2.dds")));
		ID3D10Resource* pD3D10Resource = NULL;
		V_RETURN(D3DX10CreateTextureFromFile(m_pd3dDevice, path, NULL, NULL, &pD3D10Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D10Resource, NULL, &m_pFoamIntensityMap));
		SAFE_RELEASE(pD3D10Resource);
	}

	if(NULL == m_pFoamDiffuseMap)
	{
		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\foam.dds")));
		ID3D10Resource* pD3D10Resource = NULL;
		V_RETURN(D3DX10CreateTextureFromFile(m_pd3dDevice, path, NULL, NULL, &pD3D10Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D10Resource, NULL, &m_pFoamDiffuseMap));
		SAFE_RELEASE(pD3D10Resource);
	}

	BOOL recreateFresnelMap = FALSE;
	if(params.sky_blending != m_params.sky_blending)
	{
		recreateFresnelMap = TRUE;
	}

	m_params = params;

	if(recreateFresnelMap)
		createFresnelMap();

    return S_OK;
}

void OceanSurface::createFresnelMap()
{
	SAFE_RELEASE(m_pBicolorMap);

	DWORD buffer[BICOLOR_TEX_SIZE];
	for (int i = 0; i < BICOLOR_TEX_SIZE; i++)
	{
		// R channel: fresnel term
		// G channel: sky/envmap blend term
		// B channel: 
		// A channel: 

		float cos_a = i / (FLOAT)BICOLOR_TEX_SIZE;
		float fresnel = D3DXFresnelTerm(cos_a, 1.33f);
		DWORD weight = (DWORD)(fresnel * 255);

		fresnel = powf(1 / (1 + cos_a), m_params.sky_blending);
		DWORD refl_fresnel = (DWORD)(fresnel * 255);

		buffer[i] = (weight) | (refl_fresnel << 8);
	}

	D3D10_TEXTURE1D_DESC texDesc;
	texDesc.Width = BICOLOR_TEX_SIZE;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D10_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	D3D10_SUBRESOURCE_DATA srd;
	srd.pSysMem = buffer;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;

	ID3D10Texture1D* pTextureResource = NULL;
	m_pd3dDevice->CreateTexture1D(&texDesc, &srd, &pTextureResource);
	m_pd3dDevice->CreateShaderResourceView(pTextureResource, NULL, &m_pBicolorMap);
	SAFE_RELEASE(pTextureResource);
}

void OceanSurface::renderWireframe(const D3DXMATRIX& matView, const D3DXMATRIX& matProj, GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate, BOOL freeze_cam)
{
	// Matrices
	D3DXMATRIX matVP =  matView * matProj;
	m_pRenderSurfaceMatViewProjVariable->SetMatrix((FLOAT*)&matVP);

	// Wireframe color
	D3DXVECTOR4 patch_color(1, 1, 1, 1);
	m_pRenderSurfacePatchColorVariable->SetFloatVector((FLOAT*)&patch_color);

	D3D10_VIEWPORT vp;
	UINT NumViewports = 1;
	m_pd3dDevice->RSGetViewports(&NumViewports,&vp);

	if(!freeze_cam) {
		m_matView = matView;
		m_matProj = matProj;
	}

	m_pRenderSurfaceWireframePass->Apply(0);
	GFSDK_WaveWorks_Simulation_SetRenderStateD3D10(hSim, NvFromDX(m_matView), m_pSimulationShaderInputMappings_Wireframe, hSavestate);
	GFSDK_WaveWorks_Quadtree_DrawD3D10(m_hOceanQuadTree, NvFromDX(m_matView), NvFromDX(m_matProj), m_pQuadTreeShaderInputMappings_Wireframe, hSavestate);
	GFSDK_WaveWorks_Savestate_RestoreD3D10(hSavestate);
}

void OceanSurface::renderShaded(const D3DXMATRIX& matView, const D3DXMATRIX& matProj, GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate, BOOL freeze_cam)
{
	m_pRenderSurfaceColorMapVariable->SetResource(m_pBicolorMap);
	m_pRenderSurfaceCubeMapVariable->SetResource(m_pCubeMap);
	m_pRenderSurfaceFoamIntensityMapVariable->SetResource(m_pFoamIntensityMap);
	m_pRenderSurfaceFoamDiffuseMapVariable->SetResource(m_pFoamDiffuseMap);

	// Colors
	m_pRenderSurfaceSkyColorVariable->SetFloatVector((FLOAT*)&m_params.sky_color);
	m_pRenderSurfaceWaterColorVariable->SetFloatVector((FLOAT*)&m_params.waterbody_color);

	// Matrices
	D3DXMATRIX matVP =  matView * matProj;
	m_pRenderSurfaceMatViewProjVariable->SetMatrix((FLOAT*)&matVP);

	D3D10_VIEWPORT vp;
	UINT NumViewports = 1;
	m_pd3dDevice->RSGetViewports(&NumViewports,&vp);

	if(!freeze_cam) {
		m_matView = matView;
		m_matProj = matProj;
	}

	m_pRenderSurfaceShadedPass->Apply(0);
	GFSDK_WaveWorks_Simulation_SetRenderStateD3D10(hSim, NvFromDX(m_matView), m_pSimulationShaderInputMappings_Shaded, hSavestate);
	GFSDK_WaveWorks_Quadtree_DrawD3D10(m_hOceanQuadTree, NvFromDX(m_matView), NvFromDX(m_matProj), m_pQuadTreeShaderInputMappings_Shaded, hSavestate);
	GFSDK_WaveWorks_Savestate_RestoreD3D10(hSavestate);
}

void OceanSurface::getQuadTreeStats(GFSDK_WaveWorks_Quadtree_Stats& stats)
{
	GFSDK_WaveWorks_Quadtree_GetStats(m_hOceanQuadTree, stats);
}
