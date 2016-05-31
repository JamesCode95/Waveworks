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
 
#include "DXUT.h"
#include "ocean_surface.h"
#include "SDKmisc.h"

#include "GFSDK_WaveWorks_D3D_Util.h"
#include "DDSTextureLoader.h"

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

	UINT NumQuadtreeShaderInputs = GFSDK_WaveWorks_Quadtree_GetShaderInputCountD3D11();
	UINT NumSimulationShaderInputs = GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D11();
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

	m_pd3dDevice = DXUTGetD3D11Device();

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
		return GFSDK_WaveWorks_Quadtree_CreateD3D11(params, m_pd3dDevice, &m_hOceanQuadTree);
	}
	else
	{
		return GFSDK_WaveWorks_Quadtree_UpdateParams(m_hOceanQuadTree, params);
	}
}

UINT GetShaderInputRegisterMapping(	ID3D11ShaderReflection* pReflectionVS,
									ID3D11ShaderReflection* pReflectionHS,
									ID3D11ShaderReflection* pReflectionDS,
									ID3D11ShaderReflection* pReflectionPS,
									const GFSDK_WaveWorks_ShaderInput_Desc& inputDesc
									)
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
	case GFSDK_WaveWorks_ShaderInput_Desc::HullShader_FloatConstant:
	case GFSDK_WaveWorks_ShaderInput_Desc::HullShader_Sampler:
	case GFSDK_WaveWorks_ShaderInput_Desc::HullShader_Texture:
	case GFSDK_WaveWorks_ShaderInput_Desc::HullShader_ConstantBuffer:
		pReflection = pReflectionHS;
		break;
	case GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_FloatConstant:
	case GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Sampler:
	case GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Texture:
	case GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_ConstantBuffer:
		pReflection = pReflectionDS;
		break;
	case GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant:
	case GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler:
	case GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture:
	case GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_ConstantBuffer:
		pReflection = pReflectionPS;
		break;
	default:
		pReflection = NULL;
		break;
	}

	assert(pReflection);
	D3D11_SHADER_INPUT_BIND_DESC desc;
	HRESULT hr = pReflection->GetResourceBindingDescByName(inputDesc.Name, &desc);
	if(FAILED(hr))
	{
		// Shader doesn't use this input, mark it as unused
		return 0xFFFFFFFF;
	}

	return desc.BindPoint;
}

ID3D11ShaderReflection* GetReflection(const D3DX11_PASS_SHADER_DESC& passShader)
{
	ID3D11ShaderReflection* pResult = NULL;

	D3DX11_EFFECT_SHADER_DESC shaderDesc;
	if(SUCCEEDED(passShader.pShaderVariable->GetShaderDesc(passShader.ShaderIndex, &shaderDesc)))
	{
		D3DReflect(shaderDesc.pBytecode, shaderDesc.BytecodeLength, IID_ID3D11ShaderReflection, (void**)&pResult);
	}

	return pResult;
}

HRESULT OceanSurface::init(const OceanSurfaceParameters& params)
{
	HRESULT hr = S_OK;

	if(NULL == m_pOceanFX)
	{
		TCHAR path[MAX_PATH];

		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("ocean_surface_d3d11.fxo")));
		V_RETURN(D3DX11CreateEffectFromFile(path, 0, m_pd3dDevice, &m_pOceanFX));

		// Hook up the shader mappings
		m_pRenderSurfaceTechnique = m_pOceanFX->GetTechniqueByName("RenderOceanSurfTech");
		m_pRenderSurfaceShadedPass = m_pRenderSurfaceTechnique->GetPassByName("Pass_Shaded");
		m_pRenderSurfaceWireframePass = m_pRenderSurfaceTechnique->GetPassByName("Pass_Wireframe");

		m_pRenderSurfaceMatViewProjVariable = m_pOceanFX->GetVariableByName("g_matViewProj")->AsMatrix();
		m_pRenderSurfaceSkyColorVariable = m_pOceanFX->GetVariableByName("g_SkyColor")->AsVector();
		m_pRenderSurfaceWaterColorVariable = m_pOceanFX->GetVariableByName("g_DeepColor")->AsVector();
		m_pRenderSurfacePatchColorVariable = m_pOceanFX->GetVariableByName("g_PatchColor")->AsVector();
		m_pRenderSurfaceColorMapVariable = m_pOceanFX->GetVariableByName("g_texColorMap")->AsShaderResource();
		m_pRenderSurfaceCubeMapVariable = m_pOceanFX->GetVariableByName("g_texCubeMap")->AsShaderResource();
		m_pRenderSurfaceFoamIntensityMapVariable = m_pOceanFX->GetVariableByName("g_texFoamIntensityMap")->AsShaderResource();
		m_pRenderSurfaceFoamDiffuseMapVariable = m_pOceanFX->GetVariableByName("g_texFoamDiffuseMap")->AsShaderResource();

		D3DX11_PASS_SHADER_DESC passShaderDesc;

		V_RETURN(m_pRenderSurfaceShadedPass->GetVertexShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pShadedReflectionVS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceShadedPass->GetHullShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pShadedReflectionHS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceShadedPass->GetDomainShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pShadedReflectionDS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceShadedPass->GetPixelShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pShadedReflectionPS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceWireframePass->GetVertexShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pWireframeReflectionVS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceWireframePass->GetHullShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pWireframeReflectionHS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceWireframePass->GetDomainShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pWireframeReflectionDS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceWireframePass->GetPixelShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pWireframeReflectionPS = GetReflection(passShaderDesc);

		UINT NumQuadtreeShaderInputs = GFSDK_WaveWorks_Quadtree_GetShaderInputCountD3D11();
		UINT NumSimulationShaderInputs = GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D11();

		for(UINT i = 0; i != NumQuadtreeShaderInputs; ++i)
		{
			GFSDK_WaveWorks_ShaderInput_Desc inputDesc;
			GFSDK_WaveWorks_Quadtree_GetShaderInputDescD3D11(i, &inputDesc);

			m_pQuadTreeShaderInputMappings_Shaded[i] = GetShaderInputRegisterMapping(pShadedReflectionVS, pShadedReflectionHS, pShadedReflectionDS, pShadedReflectionPS, inputDesc);
			m_pQuadTreeShaderInputMappings_Wireframe[i] = GetShaderInputRegisterMapping(pWireframeReflectionVS, pWireframeReflectionHS, pWireframeReflectionDS, pWireframeReflectionPS, inputDesc);
		}

		for(UINT i = 0; i != NumSimulationShaderInputs; ++i)
		{
			GFSDK_WaveWorks_ShaderInput_Desc inputDesc;
			GFSDK_WaveWorks_Simulation_GetShaderInputDescD3D11(i, &inputDesc);

			m_pSimulationShaderInputMappings_Shaded[i] = GetShaderInputRegisterMapping(pShadedReflectionVS, pShadedReflectionHS, pShadedReflectionDS, pShadedReflectionPS, inputDesc);
			m_pSimulationShaderInputMappings_Wireframe[i] = GetShaderInputRegisterMapping(pWireframeReflectionVS, pWireframeReflectionHS, pWireframeReflectionDS, pWireframeReflectionPS, inputDesc);
		}

		pShadedReflectionVS->Release();
		pWireframeReflectionVS->Release();
		pShadedReflectionPS->Release();
		pWireframeReflectionPS->Release();
		if(pShadedReflectionHS) pShadedReflectionHS->Release();
		if(pWireframeReflectionHS) pWireframeReflectionHS->Release();
		if(pShadedReflectionDS) pShadedReflectionDS->Release();
		if(pWireframeReflectionDS) pWireframeReflectionDS->Release();
	}

	if(NULL == m_pQuadLayout)
	{
		const D3D11_INPUT_ELEMENT_DESC quad_layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		const UINT num_layout_elements = sizeof(quad_layout)/sizeof(quad_layout[0]);

		ID3DX11EffectTechnique* pDisplayBufferTech = m_pOceanFX->GetTechniqueByName("DisplayBufferTech");

		D3DX11_PASS_DESC PassDesc;
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
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("reflect_cube.dds")));
		ID3D11Resource* pD3D11Resource = NULL;

		
		V_RETURN(DirectX::CreateDDSTextureFromFile(m_pd3dDevice, static_cast<const wchar_t *>(path), &pD3D11Resource, &m_pCubeMap));
//		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pCubeMap));
		SAFE_RELEASE(pD3D11Resource);
	}

	if(NULL == m_pFoamIntensityMap)
	{
		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("foam_intensity_perlin2.dds")));
		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(DirectX::CreateDDSTextureFromFile(m_pd3dDevice, static_cast<const wchar_t *>(path), &pD3D11Resource, &m_pFoamIntensityMap));
//		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pFoamIntensityMap));
		SAFE_RELEASE(pD3D11Resource);
	}

	if(NULL == m_pFoamDiffuseMap)
	{
		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("foam.dds")));
		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(DirectX::CreateDDSTextureFromFile(m_pd3dDevice, static_cast<const wchar_t *>(path), &pD3D11Resource, &m_pFoamDiffuseMap));
//		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pFoamDiffuseMap));
		SAFE_RELEASE(pD3D11Resource);
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

		// TODO: FIX ME - COMPLETELY BROKEN WITH new DirectXMath FresnelTerm thingy
// 		float cos_a = i / (FLOAT)BICOLOR_TEX_SIZE;
// 		float fresnel = XMFresnelTerm(cos_a, 1.33f);
// 		DWORD weight = (DWORD)(fresnel * 255);
// 
// 		fresnel = powf(1 / (1 + cos_a), m_params.sky_blending);
// 		DWORD refl_fresnel = (DWORD)(fresnel * 255);
// 
// 		buffer[i] = (weight) | (refl_fresnel << 8);
	}

	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = BICOLOR_TEX_SIZE;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA srd;
	srd.pSysMem = buffer;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;

	ID3D11Texture1D* pTextureResource = NULL;
	m_pd3dDevice->CreateTexture1D(&texDesc, &srd, &pTextureResource);
	m_pd3dDevice->CreateShaderResourceView(pTextureResource, NULL, &m_pBicolorMap);
	SAFE_RELEASE(pTextureResource);
}

void OceanSurface::renderWireframe(ID3D11DeviceContext* pDC, const XMMATRIX& matView, const XMMATRIX& matProj, GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate, BOOL freeze_cam)
{
	// Matrices
	XMMATRIX matVP = matView * matProj;
	m_pRenderSurfaceMatViewProjVariable->SetMatrix((FLOAT*)&matVP);

	// Wireframe color
	XMFLOAT4 patch_color(1, 1, 1, 1);
	m_pRenderSurfacePatchColorVariable->SetFloatVector((FLOAT*)&patch_color);

	D3D11_VIEWPORT vp;
	UINT NumViewports = 1;
	pDC->RSGetViewports(&NumViewports,&vp);

	if(!freeze_cam) {
		m_matView = matView;
		m_matProj = matProj;
	}

	m_pRenderSurfaceWireframePass->Apply(0,pDC);
	GFSDK_WaveWorks_Simulation_SetRenderStateD3D11(hSim, pDC, NvFromDX(m_matView), m_pSimulationShaderInputMappings_Wireframe, hSavestate);
	GFSDK_WaveWorks_Quadtree_DrawD3D11(m_hOceanQuadTree, pDC, NvFromDX(m_matView), NvFromDX(m_matProj), m_pQuadTreeShaderInputMappings_Wireframe, hSavestate);
	GFSDK_WaveWorks_Savestate_RestoreD3D11(hSavestate, pDC);
}

void OceanSurface::renderShaded(ID3D11DeviceContext* pDC, const XMMATRIX& matView, const XMMATRIX& matProj, GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate, BOOL freeze_cam)
{
	m_pRenderSurfaceColorMapVariable->SetResource(m_pBicolorMap);
	m_pRenderSurfaceCubeMapVariable->SetResource(m_pCubeMap);
	m_pRenderSurfaceFoamIntensityMapVariable->SetResource(m_pFoamIntensityMap);
	m_pRenderSurfaceFoamDiffuseMapVariable->SetResource(m_pFoamDiffuseMap);

	// Colors
	m_pRenderSurfaceSkyColorVariable->SetFloatVector((FLOAT*)&m_params.sky_color);
	m_pRenderSurfaceWaterColorVariable->SetFloatVector((FLOAT*)&m_params.waterbody_color);

	// Matrices
	XMFLOAT4X4 matVP;
	XMStoreFloat4x4(&matVP, matView * matProj);
	m_pRenderSurfaceMatViewProjVariable->SetMatrix((FLOAT*)&matVP);

	D3D11_VIEWPORT vp;
	UINT NumViewports = 1;
	pDC->RSGetViewports(&NumViewports,&vp);

	if(!freeze_cam) {
		m_matView = matView;
		m_matProj = matProj;
	}

	m_pRenderSurfaceShadedPass->Apply(0,pDC);
	GFSDK_WaveWorks_Simulation_SetRenderStateD3D11(hSim, pDC, NvFromDX(m_matView), m_pSimulationShaderInputMappings_Shaded, hSavestate);
	GFSDK_WaveWorks_Quadtree_DrawD3D11(m_hOceanQuadTree, pDC, NvFromDX(m_matView), NvFromDX(m_matProj), m_pQuadTreeShaderInputMappings_Shaded, hSavestate);
	GFSDK_WaveWorks_Savestate_RestoreD3D11(hSavestate, pDC);
}

void OceanSurface::getQuadTreeStats(GFSDK_WaveWorks_Quadtree_Stats& stats)
{
	GFSDK_WaveWorks_Quadtree_GetStats(m_hOceanQuadTree, stats);
}
