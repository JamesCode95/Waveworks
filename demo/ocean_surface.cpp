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
#include "ocean_hull_profile.h"
#include "ocean_env.h"
#include "ocean_sky_map.h"
#include "ocean_shader_common.h"

#include "GFSDK_WaveWorks_D3D_Util.h"

#pragma warning(disable:4127)

extern HRESULT LoadFile(LPCTSTR FileName, ID3DXBuffer** ppBuffer);

OceanSurface::OceanSurface(OceanSurfaceState* pSurfaceState) :
	m_pSurfaceState(pSurfaceState)
{
	m_pBicolorMap	= NULL;
	m_pFoamDiffuseMap = NULL;
	m_pFoamIntensityMap = NULL;
	m_pWakeMap = NULL;
	m_pShipFoamMap = NULL;
	m_pGustMap = NULL;
	m_pQuadLayout		= NULL;
	m_pOceanFX		= NULL;
	m_pRenderSurfaceTechnique = NULL;
	m_pRenderSurfaceShadedPass = NULL;
	m_pRenderSurfaceWireframePass = NULL;
	m_hOceanQuadTree	= NULL;

	m_pShiftFadeBlurLocalFoamTechnique = NULL;
	m_pShiftFadeBlurLocalFoamShadedPass = NULL;
	m_pShiftFadeBlurLocalFoamUVOffsetBlurVariable = NULL;
	m_pShiftFadeBlurLocalFoamFadeAmountVariable = NULL;
	m_pShiftFadeBlurLocalFoamTextureVariable = NULL;
	m_pLocalFoamMapReceiver = NULL;
	m_pLocalFoamMapReceiverSRV = NULL;
	m_pLocalFoamMapReceiverRTV = NULL;
	m_pLocalFoamMapFader = NULL;
	m_pLocalFoamMapFaderSRV = NULL;
	m_pLocalFoamMapFaderRTV = NULL;
	m_pRenderSurfaceLocalFoamMapVariable = NULL;


	UINT NumQuadtreeShaderInputs = GFSDK_WaveWorks_Quadtree_GetShaderInputCountD3D11();
	UINT NumSimulationShaderInputs = GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D11();
	m_pQuadTreeShaderInputMappings_Shaded = new UINT [NumQuadtreeShaderInputs];
	m_pQuadTreeShaderInputMappings_Wireframe = new UINT [NumQuadtreeShaderInputs];
	m_pSimulationShaderInputMappings_Shaded = new UINT [NumSimulationShaderInputs];
	m_pSimulationShaderInputMappings_Wireframe = new UINT [NumSimulationShaderInputs];

	m_pRenderSurfaceMatViewProjVariable = NULL;
	m_pRenderSurfaceMatViewVariable = NULL;
	m_pRenderSurfaceSkyColorVariable = NULL;
	m_pRenderSurfaceLightDirectionVariable = NULL;
	m_pRenderSurfaceLightColorVariable = NULL;
	m_pRenderSurfaceWaterColorVariable = NULL;
	m_pRenderSurfacePatchColorVariable = NULL;
	m_pRenderSurfaceColorMapVariable = NULL;
	m_pRenderSurfaceCubeMap0Variable = NULL;
	m_pRenderSurfaceCubeMap1Variable = NULL;
	m_pRenderSurfaceCubeBlendVariable = NULL;
	m_pRenderSurfaceCube0RotateSinCosVariable = NULL;
	m_pRenderSurfaceCube1RotateSinCosVariable = NULL;
	m_pRenderSurfaceCubeMultVariable = NULL;

	m_pSpotlightNumVariable = NULL;
	m_pRenderSurfaceSpotlightPositionVariable = NULL;
	m_pRenderSurfaceSpotLightAxisAndCosAngleVariable = NULL;
	m_pRenderSurfaceSpotlightColorVariable = NULL;
	m_pSpotlightShadowMatrixVar = NULL;
	m_pSpotlightShadowResourceVar = NULL;

	m_pRenderSurfaceSpotlightColorVariable = NULL;

	m_pRenderSurfaceHullProfileCoordOffsetAndScaleVariable = NULL;
	m_pRenderSurfaceHullProfileHeightOffsetAndHeightScaleAndTexelSizeVariable = NULL;
	m_pRenderSurfaceHullProfileMapVariable = NULL;

	m_pRenderSurfaceWakeMapVariable = NULL;
	m_pRenderSurfaceShipFoamMapVariable = NULL;
	m_pRenderSurfaceWorldToShipVariable = NULL;

	m_pd3dDevice = DXUTGetD3D11Device();

	memset(&m_params, 0, sizeof(m_params));

    for (UINT i=0; i<2; ++i)
    {
        m_pParticlesBuffer[i] = NULL;
        m_pParticlesBufferUAV[i] = NULL;
        m_pParticlesBufferSRV[i] = NULL;
    }

    m_ParticleWriteBuffer = 0;

    m_pDrawParticlesCB = NULL;
    m_pDrawParticlesBuffer = NULL;
    m_pDrawParticlesBufferUAV = NULL;
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
	SAFE_RELEASE(m_pFoamDiffuseMap);
	SAFE_RELEASE(m_pFoamIntensityMap);
	SAFE_RELEASE(m_pWakeMap);
	SAFE_RELEASE(m_pShipFoamMap);
	SAFE_RELEASE(m_pGustMap);

	SAFE_RELEASE(m_pLocalFoamMapReceiver);
	SAFE_RELEASE(m_pLocalFoamMapReceiverSRV);
	SAFE_RELEASE(m_pLocalFoamMapReceiverRTV);
	SAFE_RELEASE(m_pLocalFoamMapFader);
	SAFE_RELEASE(m_pLocalFoamMapFaderSRV);
	SAFE_RELEASE(m_pLocalFoamMapFaderRTV);

	SAFE_DELETE_ARRAY(m_pQuadTreeShaderInputMappings_Shaded);
	SAFE_DELETE_ARRAY(m_pQuadTreeShaderInputMappings_Wireframe);
	SAFE_DELETE_ARRAY(m_pSimulationShaderInputMappings_Shaded);
	SAFE_DELETE_ARRAY(m_pSimulationShaderInputMappings_Wireframe);

    for (UINT i=0; i<2; ++i)
    {
        SAFE_RELEASE(m_pParticlesBuffer[i]);
        SAFE_RELEASE(m_pParticlesBufferUAV[i]);
        SAFE_RELEASE(m_pParticlesBufferSRV[i]);
    }

    SAFE_RELEASE(m_pDrawParticlesCB);
    SAFE_RELEASE(m_pDrawParticlesBuffer);
    SAFE_RELEASE(m_pDrawParticlesBufferUAV);
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
	D3DX11_EFFECT_SHADER_DESC shaderDesc;
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
        ID3DXBuffer* pEffectBuffer = NULL;
        V_RETURN(LoadFile(TEXT(".\\Media\\ocean_surface_d3d11.fxo"), &pEffectBuffer));
        V_RETURN(D3DX11CreateEffectFromMemory(pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, m_pd3dDevice, &m_pOceanFX));
        pEffectBuffer->Release();

		// Hook up the shader mappings
		m_pRenderSurfaceTechnique = m_pOceanFX->GetTechniqueByName("RenderOceanSurfTech");
		m_pRenderSurfaceShadedPass = m_pRenderSurfaceTechnique->GetPassByName("Pass_PatchSolid");
		m_pRenderSurfaceWireframePass = m_pRenderSurfaceTechnique->GetPassByName("Pass_PatchWireframe");

		m_pRenderSurfaceMatViewProjVariable = m_pOceanFX->GetVariableByName("g_matViewProj")->AsMatrix();
		m_pRenderSurfaceMatViewVariable = m_pOceanFX->GetVariableByName("g_matView")->AsMatrix();
		m_pRenderSurfaceSkyColorVariable = m_pOceanFX->GetVariableByName("g_SkyColor")->AsVector();
		m_pRenderSurfaceLightDirectionVariable = m_pOceanFX->GetVariableByName("g_LightDir")->AsVector();
		m_pRenderSurfaceLightColorVariable = m_pOceanFX->GetVariableByName("g_LightColor")->AsVector();
		m_pRenderSurfaceWaterColorVariable = m_pOceanFX->GetVariableByName("g_DeepColor")->AsVector();
		m_pRenderSurfacePatchColorVariable = m_pOceanFX->GetVariableByName("g_PatchColor")->AsVector();
		m_pRenderSurfaceColorMapVariable = m_pOceanFX->GetVariableByName("g_texColorMap")->AsShaderResource();
		m_pRenderSurfaceCubeMap0Variable = m_pOceanFX->GetVariableByName("g_texCubeMap0")->AsShaderResource();
		m_pRenderSurfaceCubeMap1Variable = m_pOceanFX->GetVariableByName("g_texCubeMap1")->AsShaderResource();
		m_pRenderSurfaceCubeBlendVariable = m_pOceanFX->GetVariableByName("g_CubeBlend")->AsScalar();
		m_pRenderSurfaceCube0RotateSinCosVariable = m_pOceanFX->GetVariableByName("g_SkyCube0RotateSinCos")->AsVector();
		m_pRenderSurfaceCube1RotateSinCosVariable = m_pOceanFX->GetVariableByName("g_SkyCube1RotateSinCos")->AsVector();
		m_pRenderSurfaceCubeMultVariable = m_pOceanFX->GetVariableByName("g_SkyCubeMult")->AsVector();

		m_pSpotlightNumVariable = m_pOceanFX->GetVariableByName("g_LightsNum")->AsScalar();
		m_pRenderSurfaceSpotlightPositionVariable = m_pOceanFX->GetVariableByName("g_SpotlightPosition")->AsVector();
		m_pRenderSurfaceSpotLightAxisAndCosAngleVariable = m_pOceanFX->GetVariableByName("g_SpotLightAxisAndCosAngle")->AsVector();
		m_pRenderSurfaceSpotlightColorVariable = m_pOceanFX->GetVariableByName("g_SpotlightColor")->AsVector();
		m_pSpotlightShadowMatrixVar = m_pOceanFX->GetVariableByName("g_SpotlightMatrix")->AsMatrix();
		m_pSpotlightShadowResourceVar = m_pOceanFX->GetVariableByName("g_SpotlightResource")->AsShaderResource();

		m_pRenderSurfaceFogExponentVariable = m_pOceanFX->GetVariableByName("g_FogExponent")->AsScalar();

		m_pRenderSurfaceFoamIntensityMapVariable = m_pOceanFX->GetVariableByName("g_texFoamIntensityMap")->AsShaderResource();
		m_pRenderSurfaceFoamDiffuseMapVariable = m_pOceanFX->GetVariableByName("g_texFoamDiffuseMap")->AsShaderResource();
		m_pGlobalFoamFadeVariable = m_pOceanFX->GetVariableByName("g_GlobalFoamFade")->AsScalar();

		m_pRenderSurfaceHullProfileCoordOffsetAndScaleVariable = m_pOceanFX->GetVariableByName("g_HullProfileCoordOffsetAndScale")->AsVector();
		m_pRenderSurfaceHullProfileHeightOffsetAndHeightScaleAndTexelSizeVariable = m_pOceanFX->GetVariableByName("g_HullProfileHeightOffsetAndHeightScaleAndTexelSize")->AsVector();
		m_pRenderSurfaceHullProfileMapVariable = m_pOceanFX->GetVariableByName("g_texHullProfileMap")->AsShaderResource();

		m_pRenderSurfaceGustMapVariable = m_pOceanFX->GetVariableByName("g_texGustMap")->AsShaderResource();
		m_pRenderSurfaceGustUVVariable = m_pOceanFX->GetVariableByName("g_GustUV")->AsVector();

		m_pRenderSurfaceWakeMapVariable = m_pOceanFX->GetVariableByName("g_texWakeMap")->AsShaderResource();
		m_pRenderSurfaceShipFoamMapVariable = m_pOceanFX->GetVariableByName("g_texShipFoamMap")->AsShaderResource();
		m_pRenderSurfaceWorldToShipVariable = m_pOceanFX->GetVariableByName("g_matWorldToShip")->AsMatrix();

		m_pRenderSurfaceReflectionTextureVariable = m_pOceanFX->GetVariableByName("g_texReflection")->AsShaderResource();
        m_pRenderSurfaceReflectionPosTextureVariable = m_pOceanFX->GetVariableByName("g_texReflectionPos")->AsShaderResource();
		m_pRenderSurfaceScreenSizeInvVariable = m_pOceanFX->GetVariableByName("g_ScreenSizeInv")->AsVector();

		m_pRenderSurfaceScreenSizeInvVariable = m_pOceanFX->GetVariableByName("g_ScreenSizeInv")->AsVector();
		
		m_pRenderSurfaceSceneToShadowMapVariable = m_pOceanFX->GetVariableByName("g_matSceneToShadowMap")->AsMatrix();
		m_pRenderSurfaceLightningPositionVariable = m_pOceanFX->GetVariableByName("g_LightningPosition")->AsVector();
		m_pRenderSurfaceLightningColorVariable = m_pOceanFX->GetVariableByName("g_LightningColor")->AsVector();
		m_pRenderSurfaceCloudFactorVariable = m_pOceanFX->GetVariableByName("g_CloudFactor")->AsScalar();
		m_pRenderSurfaceLocalFoamMapVariable = m_pOceanFX->GetVariableByName("g_texLocalFoamMap")->AsShaderResource();
	
		m_pShiftFadeBlurLocalFoamTechnique = m_pOceanFX->GetTechniqueByName("LocalFoamMapTech");
		m_pShiftFadeBlurLocalFoamShadedPass = m_pShiftFadeBlurLocalFoamTechnique->GetPassByName("Pass_Solid");
		m_pShiftFadeBlurLocalFoamTextureVariable = m_pOceanFX->GetVariableByName("g_texLocalFoamSource")->AsShaderResource();
		m_pShiftFadeBlurLocalFoamUVOffsetBlurVariable = m_pOceanFX->GetVariableByName("g_UVOffsetBlur")->AsVector();
		m_pShiftFadeBlurLocalFoamFadeAmountVariable = m_pOceanFX->GetVariableByName("g_FadeAmount")->AsScalar();

		m_pRenderSurfaceShowSpraySimVariable = m_pOceanFX->GetVariableByName("g_ShowSpraySim")->AsScalar();
		m_pRenderSurfaceShowFoamSimVariable = m_pOceanFX->GetVariableByName("g_ShowFoamSim")->AsScalar();

		m_pRenderSurfaceGustsEnabledVariable = m_pOceanFX->GetVariableByName("g_bGustsEnabled")->AsScalar();
		m_pRenderSurfaceWakeEnabledVariable = m_pOceanFX->GetVariableByName("g_bWakeEnabled")->AsScalar();

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
		pShadedReflectionHS->Release();
		pWireframeReflectionHS->Release();
		pShadedReflectionDS->Release();
		pWireframeReflectionDS->Release();
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
	if(NULL == m_pFoamIntensityMap)
	{
		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(D3DX11CreateTextureFromFile(m_pd3dDevice, TEXT(".\\media\\foam_intensity_perlin2.dds"), NULL, NULL, &pD3D11Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pFoamIntensityMap));
		SAFE_RELEASE(pD3D11Resource);
	}

	if(NULL == m_pFoamDiffuseMap)
	{
		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(D3DX11CreateTextureFromFile(m_pd3dDevice, TEXT(".\\media\\foam.dds"), NULL, NULL, &pD3D11Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pFoamDiffuseMap));
		SAFE_RELEASE(pD3D11Resource);
	}

	if(NULL == m_pWakeMap)
	{
		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(D3DX11CreateTextureFromFile(m_pd3dDevice, TEXT(".\\media\\wake_map.dds"), NULL, NULL, &pD3D11Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pWakeMap));
		SAFE_RELEASE(pD3D11Resource);
	}

	if(NULL == m_pShipFoamMap)
	{
		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(D3DX11CreateTextureFromFile(m_pd3dDevice, TEXT(".\\media\\ship_foam_map.dds"), NULL, NULL, &pD3D11Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pShipFoamMap));
		SAFE_RELEASE(pD3D11Resource);
	}


	if(NULL == m_pGustMap)
	{
		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(D3DX11CreateTextureFromFile(m_pd3dDevice, TEXT(".\\media\\gust_ripples_map.dds"), NULL, NULL, &pD3D11Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pGustMap));
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

	createLocalFoamMaps();

#if ENABLE_SPRAY_PARTICLES
    UINT elementSize = (sizeof(float) * 4) * 2;

    for (UINT i=0; i<2; ++i)
    {
        CD3D11_BUFFER_DESC bufDesc(SPRAY_PARTICLE_COUNT * elementSize, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, elementSize);
        V_RETURN(m_pd3dDevice->CreateBuffer(&bufDesc, NULL, &m_pParticlesBuffer[i]));

        CD3D11_UNORDERED_ACCESS_VIEW_DESC descUAV(m_pParticlesBuffer[i], DXGI_FORMAT_UNKNOWN, 0, SPRAY_PARTICLE_COUNT);
        descUAV.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
        V_RETURN(m_pd3dDevice->CreateUnorderedAccessView(m_pParticlesBuffer[i], &descUAV, &m_pParticlesBufferUAV[i]));

        CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(m_pParticlesBuffer[i], DXGI_FORMAT_UNKNOWN, 0, SPRAY_PARTICLE_COUNT);
        V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pParticlesBuffer[i], &descSRV, &m_pParticlesBufferSRV[i]));
    }

    {
        CD3D11_BUFFER_DESC bufDesc(sizeof(int) * 4, D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS);
        D3D11_SUBRESOURCE_DATA subresourceData;
        int drawArguments[4] = {0, 1, 1, 0};
        subresourceData.pSysMem = drawArguments;
        V_RETURN(m_pd3dDevice->CreateBuffer(&bufDesc, &subresourceData, &m_pDrawParticlesBuffer));

        CD3D11_UNORDERED_ACCESS_VIEW_DESC descUAV(m_pDrawParticlesBuffer, DXGI_FORMAT_R32G32B32A32_UINT, 0, 1);
        V_RETURN(m_pd3dDevice->CreateUnorderedAccessView(m_pDrawParticlesBuffer, &descUAV, &m_pDrawParticlesBufferUAV));

        CD3D11_BUFFER_DESC descCB(sizeof(int) * 4, D3D11_BIND_CONSTANT_BUFFER);
        V_RETURN(m_pd3dDevice->CreateBuffer(&descCB, NULL, &m_pDrawParticlesCB));
    }

    m_pRenderSprayParticlesTechnique = m_pOceanFX->GetTechniqueByName("RenderSprayParticles");
    m_pSimulateSprayParticlesTechnique = m_pOceanFX->GetTechniqueByName("SimulateSprayParticles");
    m_pDispatchArgumentsTechnique = m_pOceanFX->GetTechniqueByName("PrepareDispatchArguments");

    m_pViewRightVariable = m_pOceanFX->GetVariableByName("g_ViewRight")->AsVector();
    m_pViewUpVariable = m_pOceanFX->GetVariableByName("g_ViewUp")->AsVector();
    m_pViewForwardVariable = m_pOceanFX->GetVariableByName("g_ViewForward")->AsVector();
    m_pTimeStep = m_pOceanFX->GetVariableByName("g_TimeStep")->AsScalar();
    m_pParticlesBufferVariable = m_pOceanFX->GetVariableByName("g_SprayParticleDataSRV")->AsShaderResource();
#endif

    m_pViewForwardVariable = m_pOceanFX->GetVariableByName("g_ViewForward")->AsVector();



    return S_OK;
}

void OceanSurface::beginRenderToLocalFoamMap(ID3D11DeviceContext* pDC, D3DXMATRIX& matWorldToFoam)
{
	matWorldToFoam = m_matWorldToShip;	// Same mapping as wakes

	pDC->OMSetRenderTargets(1, &m_pLocalFoamMapReceiverRTV, NULL);

	CD3D11_VIEWPORT viewport(0.0f, 0.0f, (float)LOCAL_FOAMMAP_TEX_SIZE, (float)LOCAL_FOAMMAP_TEX_SIZE);
    pDC->RSSetViewports(1, &viewport);
}

void OceanSurface::endRenderToLocalFoamMap(ID3D11DeviceContext* pDC, D3DXVECTOR2 shift_amount, float blur_amount, float fade_amount)
{
	static bool first_time = true;
	D3DXVECTOR4 ShiftBlurUV;
	ID3D11RenderTargetView* pRTVs[1];

	D3D11_VIEWPORT original_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	UINT num_original_viewports = sizeof(original_viewports)/sizeof(original_viewports[0]);
	pDC->RSGetViewports( &num_original_viewports, original_viewports);

	CD3D11_VIEWPORT viewport(0.0f, 0.0f, (float)LOCAL_FOAMMAP_TEX_SIZE, (float)LOCAL_FOAMMAP_TEX_SIZE);
    pDC->RSSetViewports(1, &viewport);

	// Blurring horizontally/shifting/fading from local foam map receiver to local foam map fader
	pRTVs[0] = m_pLocalFoamMapFaderRTV;
	pDC->OMSetRenderTargets(1, pRTVs, NULL);
	ShiftBlurUV.x = shift_amount.x; // xy must be set according to vessel speed and elapsed time
	ShiftBlurUV.y = shift_amount.y;
	ShiftBlurUV.z = 0;
	ShiftBlurUV.w = blur_amount; // must be set according to elapsed time
	m_pShiftFadeBlurLocalFoamUVOffsetBlurVariable->SetFloatVector((FLOAT*)&ShiftBlurUV);
	m_pShiftFadeBlurLocalFoamFadeAmountVariable->SetFloat(fade_amount); // must be set according to elapsed time
	// REMOVE ME, IT'S JUST A CHECK!
	/*
	if(first_time)
	{
		m_pShiftFadeBlurLocalFoamTextureVariable->SetResource(m_pGustMap);
		first_time=0;
	}
	else
	*/
	{
		m_pShiftFadeBlurLocalFoamTextureVariable->SetResource(m_pLocalFoamMapReceiverSRV);
	}
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_pShiftFadeBlurLocalFoamShadedPass->Apply(0,pDC);
	pDC->Draw(4, 0);
	m_pShiftFadeBlurLocalFoamTextureVariable->SetResource(NULL);
	m_pShiftFadeBlurLocalFoamShadedPass->Apply(0,pDC);

	// Blurring vertically from local foam map fader to local foam map receiver
	pRTVs[0] = m_pLocalFoamMapReceiverRTV;
	pDC->OMSetRenderTargets(1, pRTVs, NULL);
	
	ShiftBlurUV.x = 0; 
	ShiftBlurUV.y = 0;
	ShiftBlurUV.z = blur_amount; // must be set according to elapsed time
	ShiftBlurUV.w = 0;
	m_pShiftFadeBlurLocalFoamUVOffsetBlurVariable->SetFloatVector((FLOAT*)&ShiftBlurUV);
	m_pShiftFadeBlurLocalFoamFadeAmountVariable->SetFloat(1.0f);
	m_pShiftFadeBlurLocalFoamTextureVariable->SetResource(m_pLocalFoamMapFaderSRV);
	m_pShiftFadeBlurLocalFoamShadedPass->Apply(0,pDC);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pDC->Draw(4, 0);
	m_pShiftFadeBlurLocalFoamTextureVariable->SetResource(NULL);

	pDC->RSSetViewports(num_original_viewports, original_viewports);
    pDC->OMSetRenderTargets(0, NULL, NULL);
}

void OceanSurface::createLocalFoamMaps()
{
	SAFE_RELEASE(m_pLocalFoamMapReceiver);
	SAFE_RELEASE(m_pLocalFoamMapReceiverSRV);
	SAFE_RELEASE(m_pLocalFoamMapReceiverRTV);
	SAFE_RELEASE(m_pLocalFoamMapFader);
	SAFE_RELEASE(m_pLocalFoamMapFaderSRV);
	SAFE_RELEASE(m_pLocalFoamMapFaderRTV);

	D3D11_TEXTURE2D_DESC tex_desc;
	D3D11_SHADER_RESOURCE_VIEW_DESC textureSRV_desc;

	ZeroMemory(&textureSRV_desc,sizeof(textureSRV_desc));
	ZeroMemory(&tex_desc,sizeof(tex_desc));

	tex_desc.Width              = (UINT)(LOCAL_FOAMMAP_TEX_SIZE);
	tex_desc.Height             = (UINT)(LOCAL_FOAMMAP_TEX_SIZE);
	tex_desc.MipLevels          = (UINT)max(1,log(max((float)tex_desc.Width,(float)tex_desc.Height))/(float)log(2.0f));
	tex_desc.ArraySize          = 1;
	tex_desc.Format             = DXGI_FORMAT_R16_FLOAT;
	tex_desc.SampleDesc.Count   = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage              = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags     = 0;
	tex_desc.MiscFlags          = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	textureSRV_desc.Format                    = DXGI_FORMAT_R16_FLOAT;
	textureSRV_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	textureSRV_desc.Texture2D.MostDetailedMip = 0;

	m_pd3dDevice->CreateTexture2D(&tex_desc, NULL, &m_pLocalFoamMapReceiver);
	m_pd3dDevice->CreateShaderResourceView(m_pLocalFoamMapReceiver, &textureSRV_desc, &m_pLocalFoamMapReceiverSRV);
	m_pd3dDevice->CreateRenderTargetView(m_pLocalFoamMapReceiver, NULL, &m_pLocalFoamMapReceiverRTV);
	m_pd3dDevice->CreateTexture2D(&tex_desc, NULL, &m_pLocalFoamMapFader);
	m_pd3dDevice->CreateShaderResourceView(m_pLocalFoamMapFader, &textureSRV_desc, &m_pLocalFoamMapFaderSRV);
	m_pd3dDevice->CreateRenderTargetView(m_pLocalFoamMapFader, NULL, &m_pLocalFoamMapFaderRTV);
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

	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = BICOLOR_TEX_SIZE;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

void OceanSurface::setHullProfiles(const OceanHullProfile* hullProfiles, UINT NumHullProfiles)
{
	assert(NumHullProfiles <= MaxNumVessels);

	D3DXVECTOR4 vHullProfileCoordOffsetAndScales[MaxNumVessels];
	D3DXVECTOR4 vHullProfileHeightOffsetAndHeightScaleAndTexelSizes[MaxNumVessels];
	ID3D11ShaderResourceView* pHullProfileSRVs[MaxNumVessels];
	for(UINT i = 0; i != NumHullProfiles; ++i) {
		vHullProfileCoordOffsetAndScales[i] = D3DXVECTOR4(	hullProfiles[i].m_WorldToProfileCoordsOffset.x,
															hullProfiles[i].m_WorldToProfileCoordsOffset.y,
															hullProfiles[i].m_WorldToProfileCoordsScale.x,
															hullProfiles[i].m_WorldToProfileCoordsScale.y);
		vHullProfileHeightOffsetAndHeightScaleAndTexelSizes[i] = D3DXVECTOR4(hullProfiles[i].m_ProfileToWorldHeightOffset, hullProfiles[i].m_ProfileToWorldHeightScale, hullProfiles[i].m_TexelSizeInWorldSpace, 0.f);
		pHullProfileSRVs[i] = hullProfiles[i].GetSRV();
	}

	m_pRenderSurfaceHullProfileMapVariable->SetResourceArray(pHullProfileSRVs,0,NumHullProfiles);
	m_pRenderSurfaceHullProfileCoordOffsetAndScaleVariable->SetFloatVectorArray((FLOAT*)&vHullProfileCoordOffsetAndScales,0,NumHullProfiles);
	m_pRenderSurfaceHullProfileHeightOffsetAndHeightScaleAndTexelSizeVariable->SetFloatVectorArray((FLOAT*)&vHullProfileHeightOffsetAndHeightScaleAndTexelSizes,0,NumHullProfiles);
}

void OceanSurface::renderWireframe(	ID3D11DeviceContext* pDC,
									const D3DXMATRIX& matView, const D3DXMATRIX& matProj,
									GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate,
									BOOL freeze_cam
									)
{
	// Matrices
	D3DXMATRIX matVP =  matView * matProj;
	m_pRenderSurfaceMatViewProjVariable->SetMatrix((FLOAT*)&matVP);
	m_pRenderSurfaceMatViewVariable->SetMatrix((FLOAT*)&matView);
	m_pRenderSurfaceWorldToShipVariable->SetMatrix((FLOAT*)&m_matWorldToShip);

	// Wireframe color
	D3DXVECTOR4 patch_color(0.2f, 0.2f, 0.2f, 1.f);
	m_pRenderSurfacePatchColorVariable->SetFloatVector((FLOAT*)&patch_color);

	D3D11_VIEWPORT vp;
	UINT NumViewports = 1;
	pDC->RSGetViewports(&NumViewports,&vp);

	if(!freeze_cam) {
		m_pSurfaceState->m_matView = matView;
		m_pSurfaceState->m_matProj = matProj;
	}

	m_pRenderSurfaceWireframePass->Apply(0,pDC);
	GFSDK_WaveWorks_Simulation_SetRenderStateD3D11(hSim, pDC, NvFromDX(m_pSurfaceState->m_matView), m_pSimulationShaderInputMappings_Wireframe, hSavestate);
	GFSDK_WaveWorks_Quadtree_DrawD3D11(m_hOceanQuadTree, pDC, NvFromDX(m_pSurfaceState->m_matView), NvFromDX(m_pSurfaceState->m_matProj), m_pQuadTreeShaderInputMappings_Wireframe, hSavestate);
	GFSDK_WaveWorks_Savestate_RestoreD3D11(hSavestate, pDC);

	// Release ref to hull profile
	m_pRenderSurfaceHullProfileMapVariable->SetResource(0);
	m_pRenderSurfaceShadedPass->Apply(0,pDC);
}

void OceanSurface::renderShaded(ID3D11DeviceContext* pDC,
								const D3DXMATRIX& matCameraView, const D3DXMATRIX& matView, const D3DXMATRIX& matProj,
								GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate,
								const OceanEnvironment& ocean_env,
								BOOL freeze_cam,
								float foam_fade,
								bool show_spray_sim,
								bool show_foam_sim,
								bool gusts_enabled,
								bool wake_enabled
								)
{
	m_pRenderSurfaceColorMapVariable->SetResource(m_pBicolorMap);
	m_pRenderSurfaceCubeMap0Variable->SetResource(ocean_env.pSky0->m_pReflectionSRV);
	m_pRenderSurfaceCubeMap1Variable->SetResource(ocean_env.pSky1->m_pReflectionSRV);
	m_pRenderSurfaceCubeBlendVariable->SetFloat(ocean_env.sky_interp);
	m_pRenderSurfaceFoamIntensityMapVariable->SetResource(m_pFoamIntensityMap);
	m_pRenderSurfaceFoamDiffuseMapVariable->SetResource(m_pFoamDiffuseMap);
	m_pRenderSurfaceWakeMapVariable->SetResource(m_pWakeMap);
	m_pRenderSurfaceShipFoamMapVariable->SetResource(m_pShipFoamMap);
	m_pRenderSurfaceGustMapVariable->SetResource(m_pGustMap);
	m_pRenderSurfaceReflectionTextureVariable->SetResource(ocean_env.pPlanarReflectionSRV);
    m_pRenderSurfaceReflectionPosTextureVariable->SetResource(ocean_env.pPlanarReflectionPosSRV);
	m_pRenderSurfaceLocalFoamMapVariable->SetResource(m_pLocalFoamMapReceiverSRV);

	m_pRenderSurfaceGustUVVariable->SetFloatVector((FLOAT*)&ocean_env.gust_UV);
	m_pRenderSurfaceLightningColorVariable->SetFloatVector((FLOAT*)&ocean_env.lightning_light_intensity);
	m_pRenderSurfaceLightningPositionVariable->SetFloatVector((FLOAT*)&ocean_env.lightning_light_position);

	m_pGlobalFoamFadeVariable->SetFloat(foam_fade);

	D3D11_VIEWPORT viewport;
	UINT numviewports = 1;
	pDC->RSGetViewports(&numviewports,&viewport);
	D3DXVECTOR4 ScreenSizeInv = D3DXVECTOR4(1.0f/(float)viewport.Width,1.0f/(float)viewport.Height,0,0);
	m_pRenderSurfaceScreenSizeInvVariable->SetFloatVector((FLOAT*)&ScreenSizeInv);

	D3DXVECTOR4 sincos0;
	sincos0.x = sinf(ocean_env.pSky0->m_Orientation);
	sincos0.y = cosf(ocean_env.pSky0->m_Orientation);
	m_pRenderSurfaceCube0RotateSinCosVariable->SetFloatVector((FLOAT*)&sincos0);

	D3DXVECTOR4 sincos1;
	sincos1.x = sinf(ocean_env.pSky1->m_Orientation);
	sincos1.y = cosf(ocean_env.pSky1->m_Orientation);
	m_pRenderSurfaceCube1RotateSinCosVariable->SetFloatVector((FLOAT*)&sincos1);

	m_pRenderSurfaceCubeMultVariable->SetFloatVector((FLOAT*)&ocean_env.sky_map_color_mult);

	// Colors
	m_pRenderSurfaceSkyColorVariable->SetFloatVector((FLOAT*)&ocean_env.sky_color);
	m_pRenderSurfaceWaterColorVariable->SetFloatVector((FLOAT*)&m_params.waterbody_color);

	// Lighting
	m_pRenderSurfaceLightDirectionVariable->SetFloatVector((FLOAT*)&ocean_env.main_light_direction);
	m_pRenderSurfaceLightColorVariable->SetFloatVector((FLOAT*)&ocean_env.main_light_color);

	// Spot lights - transform to view space
	D3DXMATRIX matSpotlightsToView = ocean_env.spotlights_to_world_matrix * matCameraView;
	D3DXMATRIX matViewToSpotlights;
	D3DXMatrixInverse(&matViewToSpotlights,NULL,&matSpotlightsToView);

	D3DXVECTOR4 spotlight_position[MaxNumSpotlights];
	D3DXVECTOR4 spotlight_axis_and_cos_angle[MaxNumSpotlights];

	D3DXVec4TransformArray(spotlight_position,sizeof(spotlight_position[0]),ocean_env.spotlight_position,sizeof(ocean_env.spotlight_position[0]),&matSpotlightsToView,MaxNumSpotlights);
	D3DXVec3TransformNormalArray((D3DXVECTOR3*)spotlight_axis_and_cos_angle,sizeof(spotlight_axis_and_cos_angle[0]),(D3DXVECTOR3*)ocean_env.spotlight_axis_and_cos_angle,sizeof(ocean_env.spotlight_axis_and_cos_angle[0]),&matSpotlightsToView,MaxNumSpotlights);

	for(int i=0; i!=MaxNumSpotlights; ++i) {

		spotlight_axis_and_cos_angle[i].w = ocean_env.spotlight_axis_and_cos_angle[i].w;

#if ENABLE_SHADOWS
		D3DXMATRIX spotlight_shadow_matrix = matViewToSpotlights * ocean_env.spotlight_shadow_matrix[i];
		m_pSpotlightShadowMatrixVar->SetMatrixArray((float*)&spotlight_shadow_matrix, i, 1);
		m_pSpotlightShadowResourceVar->SetResourceArray((ID3D11ShaderResourceView**)&ocean_env.spotlight_shadow_resource[i], i, 1);
#endif
	}

	// Spot lights
	m_pSpotlightNumVariable->SetInt(ocean_env.activeLightsNum);
	m_pRenderSurfaceSpotlightPositionVariable->SetFloatVectorArray((FLOAT*)spotlight_position,0,MaxNumSpotlights);
	m_pRenderSurfaceSpotLightAxisAndCosAngleVariable->SetFloatVectorArray((FLOAT*)spotlight_axis_and_cos_angle,0,MaxNumSpotlights);
	m_pRenderSurfaceSpotlightColorVariable->SetFloatVectorArray((FLOAT*)ocean_env.spotlight_color,0,MaxNumSpotlights);

	// Fog
	m_pRenderSurfaceFogExponentVariable->SetFloat(ocean_env.fog_exponent);

	// Cloud Factor
	m_pRenderSurfaceCloudFactorVariable->SetFloat(ocean_env.cloud_factor);

	// Show spray and foam rendering variables
	m_pRenderSurfaceShowSpraySimVariable->SetFloat(show_spray_sim?1.0f:0.0f);
	m_pRenderSurfaceShowFoamSimVariable->SetFloat(show_foam_sim?1.0f:0.0f);

	// Gusts/wakes
	m_pRenderSurfaceGustsEnabledVariable->SetBool(gusts_enabled);
	m_pRenderSurfaceWakeEnabledVariable->SetBool(wake_enabled);

	// Matrices
	D3DXMATRIX matVP =  matView * matProj;
	m_pRenderSurfaceMatViewProjVariable->SetMatrix((FLOAT*)&matVP);
	m_pRenderSurfaceMatViewVariable->SetMatrix((FLOAT*)&matView);
	m_pRenderSurfaceWorldToShipVariable->SetMatrix((FLOAT*)&m_matWorldToShip);

    D3DXMATRIX matViewInv;
    D3DXMatrixInverse(&matViewInv, NULL, &matView);
    m_pViewForwardVariable->SetFloatVector((float*)&matViewInv._31);

	D3D11_VIEWPORT vp;
	UINT NumViewports = 1;
	pDC->RSGetViewports(&NumViewports,&vp);

	if(!freeze_cam) {
		m_pSurfaceState->m_matView = matView;
		m_pSurfaceState->m_matProj = matProj;
	}

	m_pRenderSurfaceShadedPass->Apply(0,pDC);

#if ENABLE_SPRAY_PARTICLES
    UINT count = (UINT)-1;
    pDC->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, 1, 1, &m_pParticlesBufferUAV[m_ParticleWriteBuffer], &count);
#endif

	GFSDK_WaveWorks_Simulation_SetRenderStateD3D11(hSim, pDC, NvFromDX(m_pSurfaceState->m_matView), m_pSimulationShaderInputMappings_Shaded, hSavestate);
	GFSDK_WaveWorks_Quadtree_DrawD3D11(m_hOceanQuadTree, pDC, NvFromDX(m_pSurfaceState->m_matView), NvFromDX(m_pSurfaceState->m_matProj), m_pQuadTreeShaderInputMappings_Shaded, hSavestate);
	GFSDK_WaveWorks_Savestate_RestoreD3D11(hSavestate, pDC);

	// Release refs to inputs
	ID3D11ShaderResourceView* pNullSRVs[max(MaxNumVessels,MaxNumSpotlights)];
	memset(pNullSRVs,0,sizeof(pNullSRVs));
	m_pRenderSurfaceHullProfileMapVariable->SetResourceArray(pNullSRVs,0,MaxNumVessels);
	m_pRenderSurfaceReflectionTextureVariable->SetResource(NULL);
    m_pRenderSurfaceReflectionPosTextureVariable->SetResource(NULL);
	m_pSpotlightShadowResourceVar->SetResourceArray(pNullSRVs,0,MaxNumSpotlights);
	m_pRenderSurfaceLocalFoamMapVariable->SetResource(NULL);
	m_pRenderSurfaceShadedPass->Apply(0,pDC);

#if ENABLE_SPRAY_PARTICLES
    ID3D11UnorderedAccessView* pNullUAV = NULL;
    pDC->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, 1, 1, &pNullUAV, NULL);

    pDC->CopyStructureCount(m_pDrawParticlesCB, 0, m_pParticlesBufferUAV[m_ParticleWriteBuffer]);
#endif
}
void OceanSurface::getQuadTreeStats(GFSDK_WaveWorks_Quadtree_Stats& stats)
{
	GFSDK_WaveWorks_Quadtree_GetStats(m_hOceanQuadTree, stats);
}

void OceanSurface::setWorldToShipMatrix(const D3DXMATRIX& matShipToWorld)
{
	D3DXMatrixInverse(&m_matWorldToShip,NULL,&matShipToWorld);
}

