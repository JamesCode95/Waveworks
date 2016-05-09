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

#include "GFSDK_WaveWorks_D3D_Util.h"

#pragma warning(disable:4127)

enum OceanFX_Pass
{
	Pass_PatchVS_WavePS		= 0,
	Pass_PatchWireframe		= 1,
};

OceanSurface::OceanSurface()
{
	m_pBicolorMap	= NULL;
	m_pCubeMap		= NULL;
	m_pFoamDiffuseMap = NULL;
	m_pFoamIntensityMap = NULL;
	m_pQuadDecl		= NULL;
	m_pOceanFX		= NULL;
	m_hOceanQuadTree	= NULL;

	UINT NumQuadtreeShaderInputs = GFSDK_WaveWorks_Quadtree_GetShaderInputCountD3D9();
	UINT NumSimulationShaderInputs = GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D9();
	m_pQuadTreeShaderInputMappings_Shaded = new UINT [NumQuadtreeShaderInputs];
	m_pQuadTreeShaderInputMappings_Wireframe = new UINT [NumQuadtreeShaderInputs];
	m_pSimulationShaderInputMappings_Shaded = new UINT [NumSimulationShaderInputs];
	m_pSimulationShaderInputMappings_Wireframe = new UINT [NumSimulationShaderInputs];

	m_pd3dDevice = DXUTGetD3DDevice();

	memset(&m_params, 0, sizeof(m_params));
}

OceanSurface::~OceanSurface()
{
	if(m_hOceanQuadTree)
	{
		GFSDK_WaveWorks_Quadtree_Destroy(m_hOceanQuadTree);
		m_hOceanQuadTree = NULL;
	}

	SAFE_RELEASE(m_pQuadDecl);
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
		return GFSDK_WaveWorks_Quadtree_CreateD3D9(params, m_pd3dDevice, &m_hOceanQuadTree);
	}
	else
	{
		return GFSDK_WaveWorks_Quadtree_UpdateParams(m_hOceanQuadTree, params);
	}
}

UINT GetShaderInputRegisterMapping(ID3DXConstantTable* pConstTableVS, ID3DXConstantTable* pConstTablePS, const GFSDK_WaveWorks_ShaderInput_Desc& inputDesc)
{
	ID3DXConstantTable* pConstTable = NULL;
	switch(inputDesc.Type)
	{
	case GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_FloatConstant:
	case GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler:
		pConstTable = pConstTableVS;
		break;
	default:
		pConstTable = pConstTablePS;
		break;
	}

	D3DXCONSTANT_DESC desc;
	D3DXHANDLE hConstant = pConstTable->GetConstantByName(NULL,inputDesc.Name);
	if(NULL == hConstant)
	{
		// Shader doesn't use this input, mark it as unused
		return 0xFFFFFFFF;
	}

	UINT NumConstants = 1;
	pConstTable->GetConstantDesc(hConstant, &desc, &NumConstants);

	return desc.RegisterIndex;
}

HRESULT OceanSurface::init(const OceanSurfaceParameters& params)
{
	HRESULT hr = S_OK;

	if(NULL == m_pOceanFX)
	{
		// HLSL
		DWORD dwShaderFlags = 0;
	#if defined( DEBUG ) || defined( _DEBUG )
		dwShaderFlags |= D3DXSHADER_DEBUG;
	#endif
	#ifdef DEBUG_VS
		dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
	#endif
	#ifdef DEBUG_PS
		dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
	#endif

		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\ocean_surface_d3d9.fxo")));
		V_RETURN(D3DXCreateEffectFromFile(m_pd3dDevice, path, NULL, NULL, dwShaderFlags, NULL, &m_pOceanFX, NULL));

		// Hook up the shader mappings
		D3DXHANDLE hMainTechnique = m_pOceanFX->GetTechniqueByName("RenderOceanSurfTech");
		D3DXHANDLE hShadedPass = m_pOceanFX->GetPass(hMainTechnique, Pass_PatchVS_WavePS);
		D3DXHANDLE hWireframePass = m_pOceanFX->GetPass(hMainTechnique, Pass_PatchWireframe);

		D3DXPASS_DESC shadedPassDesc;
		m_pOceanFX->GetPassDesc(hShadedPass, &shadedPassDesc);
		ID3DXConstantTable* pShadedConstTableVS = NULL;
		ID3DXConstantTable* pShadedConstTablePS = NULL;
		D3DXGetShaderConstantTable(shadedPassDesc.pVertexShaderFunction, &pShadedConstTableVS);
		D3DXGetShaderConstantTable(shadedPassDesc.pPixelShaderFunction, &pShadedConstTablePS);

		D3DXPASS_DESC wireframePassDesc;
		m_pOceanFX->GetPassDesc(hWireframePass, &wireframePassDesc);
		ID3DXConstantTable* pWireframeConstTableVS = NULL;
		ID3DXConstantTable* pWireframeConstTablePS = NULL;
		D3DXGetShaderConstantTable(wireframePassDesc.pVertexShaderFunction, &pWireframeConstTableVS);
		D3DXGetShaderConstantTable(wireframePassDesc.pPixelShaderFunction, &pWireframeConstTablePS);

		UINT NumQuadtreeShaderInputs = GFSDK_WaveWorks_Quadtree_GetShaderInputCountD3D9();
		UINT NumSimulationShaderInputs = GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D9();

		for(UINT i = 0; i != NumQuadtreeShaderInputs; ++i)
		{
			GFSDK_WaveWorks_ShaderInput_Desc inputDesc;
			GFSDK_WaveWorks_Quadtree_GetShaderInputDescD3D9(i, &inputDesc);

			m_pQuadTreeShaderInputMappings_Shaded[i] = GetShaderInputRegisterMapping(pShadedConstTableVS, pShadedConstTablePS, inputDesc);
			m_pQuadTreeShaderInputMappings_Wireframe[i] = GetShaderInputRegisterMapping(pWireframeConstTableVS, pWireframeConstTablePS, inputDesc);
		}

		for(UINT i = 0; i != NumSimulationShaderInputs; ++i)
		{
			GFSDK_WaveWorks_ShaderInput_Desc inputDesc;
			GFSDK_WaveWorks_Simulation_GetShaderInputDescD3D9(i, &inputDesc);

			m_pSimulationShaderInputMappings_Shaded[i] = GetShaderInputRegisterMapping(pShadedConstTableVS, pShadedConstTablePS, inputDesc);
			m_pSimulationShaderInputMappings_Wireframe[i] = GetShaderInputRegisterMapping(pWireframeConstTableVS, pWireframeConstTablePS, inputDesc);
		}


		pShadedConstTableVS->Release();
		pWireframeConstTableVS->Release();
		pShadedConstTablePS->Release();
		pWireframeConstTablePS->Release();
	}

	if(NULL == m_pQuadDecl)
	{
		D3DVERTEXELEMENT9 quad_decl[] =
		{
			{0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
			{0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
			D3DDECL_END()
		};
		V_RETURN(m_pd3dDevice->CreateVertexDeclaration(quad_decl, &m_pQuadDecl));
	}

	// Textures
	if(NULL == m_pCubeMap)
	{
		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\reflect_cube.dds")));
		V_RETURN(D3DXCreateCubeTextureFromFileEx(m_pd3dDevice, path, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &m_pCubeMap));
	}

	if(NULL == m_pFoamDiffuseMap)
	{
		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\foam.dds")));
		V_RETURN(D3DXCreateTextureFromFileEx(m_pd3dDevice, path, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &m_pFoamDiffuseMap));
	}	
	
	if(NULL == m_pFoamIntensityMap)
	{
		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\foam_intensity_perlin2.dds")));
		V_RETURN(D3DXCreateTextureFromFileEx(m_pd3dDevice, path, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &m_pFoamIntensityMap));
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
	if (m_pBicolorMap == NULL)
	{
		m_pd3dDevice->CreateTexture(BICOLOR_TEX_SIZE, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pBicolorMap, NULL);
		if (m_pBicolorMap == NULL)
			return;
	}

	LPDIRECT3DTEXTURE9 pBicolorMapStaging = NULL;
	m_pd3dDevice->CreateTexture(BICOLOR_TEX_SIZE, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pBicolorMapStaging, NULL);
	if (pBicolorMapStaging == NULL)
		return;

	D3DLOCKED_RECT rect;
	pBicolorMapStaging->LockRect(0, &rect, NULL, 0);
	DWORD* buffer = (DWORD*)rect.pBits;

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

		buffer[i] = (weight << 16) | (refl_fresnel << 8);
	}

	pBicolorMapStaging->UnlockRect(0);

	m_pd3dDevice->UpdateTexture(pBicolorMapStaging, m_pBicolorMap);

	SAFE_RELEASE(pBicolorMapStaging);
}

void OceanSurface::renderWireframe(const D3DXMATRIX& matView, const D3DXMATRIX& matProj, GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate, BOOL freeze_cam)
{
	m_pOceanFX->SetTechnique("RenderOceanSurfTech");

	m_pOceanFX->Begin(0, 0);
	{
		// Matrices
		D3DXMATRIX matVP =  matView * matProj;
		m_pOceanFX->SetMatrix("g_matViewProj", &matVP);

		// Wireframe color
		D3DXVECTOR4 patch_color(1, 1, 1, 1);
		m_pOceanFX->SetVector("g_PatchColor", &patch_color);

		D3DVIEWPORT9 vp;
		m_pd3dDevice->GetViewport(&vp);

		if(!freeze_cam) {
			m_matView = matView;
			m_matProj = matProj;
		}

		m_pOceanFX->BeginPass(Pass_PatchWireframe);
		GFSDK_WaveWorks_Simulation_SetRenderStateD3D9(hSim, NvFromDX(m_matView), m_pSimulationShaderInputMappings_Wireframe, hSavestate);
		GFSDK_WaveWorks_Quadtree_DrawD3D9(m_hOceanQuadTree, NvFromDX(m_matView), NvFromDX(m_matProj), m_pQuadTreeShaderInputMappings_Wireframe, hSavestate);
		GFSDK_WaveWorks_Savestate_RestoreD3D9(hSavestate);
		m_pOceanFX->EndPass();
	}
	m_pOceanFX->End();
}

void OceanSurface::renderShaded(const D3DXMATRIX& matView, const D3DXMATRIX& matProj, GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate, BOOL freeze_cam)
{
	m_pOceanFX->SetTechnique("RenderOceanSurfTech");

	m_pOceanFX->Begin(0, 0);
	{
		m_pOceanFX->SetTexture("g_texColorMap", m_pBicolorMap);
		m_pOceanFX->SetTexture("g_texCubeMap", m_pCubeMap);
		m_pOceanFX->SetTexture("g_texFoamIntensityMap", m_pFoamIntensityMap);
		m_pOceanFX->SetTexture("g_texFoamDiffuseMap", m_pFoamDiffuseMap);

		// Colors
		m_pOceanFX->SetVector("g_SkyColor", &m_params.sky_color);
		m_pOceanFX->SetVector("g_DeepColor",  &m_params.waterbody_color);

		// Matrices
		D3DXMATRIX matVP =  matView * matProj;
		m_pOceanFX->SetMatrix("g_matViewProj", &matVP);

		D3DVIEWPORT9 vp;
		m_pd3dDevice->GetViewport(&vp);

		if(!freeze_cam) {
			m_matView = matView;
			m_matProj = matProj;
		}

		m_pOceanFX->BeginPass(Pass_PatchVS_WavePS);
		GFSDK_WaveWorks_Simulation_SetRenderStateD3D9(hSim, NvFromDX(m_matView), m_pSimulationShaderInputMappings_Shaded, hSavestate);
		GFSDK_WaveWorks_Quadtree_DrawD3D9(m_hOceanQuadTree, NvFromDX(m_matView), NvFromDX(m_matProj), m_pQuadTreeShaderInputMappings_Shaded, hSavestate);
		GFSDK_WaveWorks_Savestate_RestoreD3D9(hSavestate);
		m_pOceanFX->EndPass();
	}
	m_pOceanFX->End();
}

void OceanSurface::getQuadTreeStats(GFSDK_WaveWorks_Quadtree_Stats& stats)
{
	GFSDK_WaveWorks_Quadtree_GetStats(m_hOceanQuadTree, stats);
}
