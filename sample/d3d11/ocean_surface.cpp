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
#include "SDKMisc.h"
#include "ocean_surface.h"

#include "GFSDK_WaveWorks_D3D_Util.h"
#include "GFSDK_Logger.h"
#include "../common/LoggerImpl.h"

#pragma warning(disable:4127)

OceanSurface::OceanSurface()
{
	m_pOceanFX = NULL;
	m_hOceanQuadTree = NULL;
	pDistanceFieldModule = NULL;
	m_pQuadLayout		= NULL;
	m_pRayContactLayout = NULL;
	m_pRenderRayContactTechnique = NULL;
	m_pContactVB = NULL;
	m_pContactIB = NULL;

	UINT NumQuadtreeShaderInputs = GFSDK_WaveWorks_Quadtree_GetShaderInputCountD3D11();
	UINT NumSimulationShaderInputs = GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D11();
	m_pQuadTreeShaderInputMappings_Shore = new UINT [NumQuadtreeShaderInputs];
	m_pSimulationShaderInputMappings_Shore = new UINT [NumSimulationShaderInputs];

	m_pd3dDevice = DXUTGetD3D11Device();
}

OceanSurface::~OceanSurface()
{
	if(m_hOceanQuadTree)
	{
		GFSDK_WaveWorks_Quadtree_Destroy(m_hOceanQuadTree);
		m_hOceanQuadTree = NULL;
	}
	SAFE_DELETE_ARRAY(m_pQuadTreeShaderInputMappings_Shore);
	SAFE_DELETE_ARRAY(m_pSimulationShaderInputMappings_Shore);
	SAFE_RELEASE(m_pOceanFX);
	SAFE_RELEASE(m_pQuadLayout);
	SAFE_RELEASE(m_pRayContactLayout);
	SAFE_RELEASE(m_pContactVB);
	SAFE_RELEASE(m_pContactIB);
}

HRESULT OceanSurface::initQuadTree(const GFSDK_WaveWorks_Quadtree_Params& params)
{
	NV_LOG(L"Initing the QuadTree");

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

HRESULT OceanSurface::init()
{
	HRESULT hr = S_OK;

	if(NULL == m_pOceanFX)
	{
		TCHAR path[MAX_PATH];

		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("ocean_surface_d3d11.fxo")));
		V_RETURN(D3DX11CreateEffectFromFile(path, 0, m_pd3dDevice, &m_pOceanFX));


		// Hook up the shader mappings
		m_pRenderSurfaceTechnique = m_pOceanFX->GetTechniqueByName("RenderOceanSurfTech");
		m_pRenderSurfaceShadedWithShorelinePass = m_pRenderSurfaceTechnique->GetPassByName("Pass_Solid_WithShoreline");

		D3DX11_PASS_SHADER_DESC passShaderDesc;

		V_RETURN(m_pRenderSurfaceShadedWithShorelinePass->GetVertexShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pShadedShoreReflectionVS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceShadedWithShorelinePass->GetHullShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pShadedShoreReflectionHS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceShadedWithShorelinePass->GetDomainShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pShadedShoreReflectionDS = GetReflection(passShaderDesc);

		V_RETURN(m_pRenderSurfaceShadedWithShorelinePass->GetPixelShaderDesc(&passShaderDesc));
		ID3D11ShaderReflection* pShadedShoreReflectionPS = GetReflection(passShaderDesc);

		UINT NumQuadtreeShaderInputs = GFSDK_WaveWorks_Quadtree_GetShaderInputCountD3D11();
		UINT NumSimulationShaderInputs = GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D11();

		for(UINT i = 0; i != NumQuadtreeShaderInputs; ++i)
		{
			GFSDK_WaveWorks_ShaderInput_Desc inputDesc;
			GFSDK_WaveWorks_Quadtree_GetShaderInputDescD3D11(i, &inputDesc);
			m_pQuadTreeShaderInputMappings_Shore[i] = GetShaderInputRegisterMapping(pShadedShoreReflectionVS, pShadedShoreReflectionHS, pShadedShoreReflectionDS, pShadedShoreReflectionPS, inputDesc);
		}

		for(UINT i = 0; i != NumSimulationShaderInputs; ++i)
		{
			GFSDK_WaveWorks_ShaderInput_Desc inputDesc;
			GFSDK_WaveWorks_Simulation_GetShaderInputDescD3D11(i, &inputDesc);
			m_pSimulationShaderInputMappings_Shore[i] = GetShaderInputRegisterMapping(pShadedShoreReflectionVS, pShadedShoreReflectionHS, pShadedShoreReflectionDS, pShadedShoreReflectionPS, inputDesc);
		}

		pShadedShoreReflectionVS->Release();
		pShadedShoreReflectionPS->Release();
		pShadedShoreReflectionHS->Release();
		pShadedShoreReflectionDS->Release();

		//m_pRenderSurfaceWireframeWithShorelinePass = m_pRenderSurfaceTechnique->GetPassByName("Pass_Wireframe_WithShoreline");
	}
		
	if(NULL == m_pQuadLayout)
	{
		const D3D11_INPUT_ELEMENT_DESC quad_layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		const UINT num_layout_elements = sizeof(quad_layout)/sizeof(quad_layout[0]);

		ID3DX11EffectTechnique* pDisplayLogoTech = m_pOceanFX->GetTechniqueByName("DisplayLogoTech");

		D3DX11_PASS_DESC PassDesc;
		V_RETURN(pDisplayLogoTech->GetPassByIndex(0)->GetDesc(&PassDesc));

		V_RETURN(m_pd3dDevice->CreateInputLayout(	quad_layout, num_layout_elements,
													PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize,
													&m_pQuadLayout
													));
	}

	// Creating ray & contact related D3D objects
	m_pRenderRayContactTechnique = m_pOceanFX->GetTechniqueByName("RenderRayContactTech");
	if(NULL == m_pRayContactLayout)
	{
		const D3D11_INPUT_ELEMENT_DESC ray_contact_layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		const UINT num_layout_elements = sizeof(ray_contact_layout)/sizeof(ray_contact_layout[0]);

		D3DX11_PASS_DESC PassDesc;
		V_RETURN(m_pRenderRayContactTechnique->GetPassByIndex(0)->GetDesc(&PassDesc));

		V_RETURN(m_pd3dDevice->CreateInputLayout(	ray_contact_layout, num_layout_elements,
													PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize,
													&m_pRayContactLayout
													));
	}

	{

		float vertex_data[5*4] = 
								{0, 0, 0, 1,
								 1, 1, 0, 0,
								 0, 1, 1, 0,
								-1, 1, 0, 0,
								 0, 1,-1, 0};
		D3D11_BUFFER_DESC vBufferDesc;
		vBufferDesc.ByteWidth = 5 * sizeof(XMFLOAT4);
		vBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vBufferDesc.CPUAccessFlags = 0;
		vBufferDesc.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vBufferData;
		vBufferData.pSysMem = vertex_data;
		vBufferData.SysMemPitch = 0;
		vBufferData.SysMemSlicePitch = 0;
		V_RETURN(m_pd3dDevice->CreateBuffer(&vBufferDesc, &vBufferData, &m_pContactVB));

		static const WORD indices[] = {0,1,2, 0,2,3, 0,3,4, 0,4,1};
		D3D11_BUFFER_DESC iBufferDesc;
		iBufferDesc.ByteWidth = sizeof(indices);
		iBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		iBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		iBufferDesc.CPUAccessFlags = 0;
		iBufferDesc.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iBufferData;
		iBufferData.pSysMem = indices;
		iBufferData.SysMemPitch = 0;
		iBufferData.SysMemSlicePitch = 0;
		V_RETURN(m_pd3dDevice->CreateBuffer(&iBufferDesc, &iBufferData, &m_pContactIB));
	}

	return S_OK;
}


void OceanSurface::renderShaded(	ID3D11DeviceContext* pDC, 
									const XMMATRIX matView, 
									const XMMATRIX matProj,
									GFSDK_WaveWorks_SimulationHandle hSim, 
									GFSDK_WaveWorks_SavestateHandle hSavestate, 
									const XMFLOAT2 windDir, 
									const float steepness, 
									const float amplitude, 
									const float wavelength, 
									const float speed, 
									const float parallelness,
                                    const float totalTime
									)
{
	D3D11_VIEWPORT vp;
	UINT NumViewports = 1;
	pDC->RSGetViewports( &NumViewports, &vp );

	if( pDistanceFieldModule != NULL)
	{
		// Apply data tex SRV
		XMMATRIX topDownMatrix;
		pDistanceFieldModule->GetWorldToTopDownTextureMatrix( topDownMatrix );

		XMFLOAT4X4 tdmStore;
		XMStoreFloat4x4(&tdmStore, topDownMatrix);

		m_pOceanFX->GetVariableByName("g_WorldToTopDownTextureMatrix")->AsMatrix()->SetMatrix( (FLOAT*)&tdmStore );
			
		m_pOceanFX->GetVariableByName("g_GerstnerSteepness")->AsScalar()->SetFloat( steepness );
		m_pOceanFX->GetVariableByName("g_BaseGerstnerAmplitude")->AsScalar()->SetFloat( amplitude );
		m_pOceanFX->GetVariableByName("g_BaseGerstnerWavelength")->AsScalar()->SetFloat( wavelength );
		m_pOceanFX->GetVariableByName("g_BaseGerstnerSpeed")->AsScalar()->SetFloat( speed );
		m_pOceanFX->GetVariableByName("g_BaseGerstnerParallelness")->AsScalar()->SetFloat( parallelness );
		m_pOceanFX->GetVariableByName("g_WindDirection")->AsVector()->SetFloatVector( (FLOAT*) &windDir );
		m_pOceanFX->GetVariableByName("g_DataTexture")->AsShaderResource()->SetResource( pDistanceFieldModule->GetDataTextureSRV() );
		m_pOceanFX->GetVariableByName("g_Time")->AsScalar()->SetFloat( totalTime );

		m_pRenderSurfaceShadedWithShorelinePass->Apply( 0, pDC );
		GFSDK_WaveWorks_Simulation_SetRenderStateD3D11(hSim, pDC, NvFromDX(matView), m_pSimulationShaderInputMappings_Shore, hSavestate);
		GFSDK_WaveWorks_Quadtree_DrawD3D11(m_hOceanQuadTree, pDC, NvFromDX(matView), NvFromDX(matProj), m_pQuadTreeShaderInputMappings_Shore, hSavestate);
		
		m_pOceanFX->GetVariableByName("g_DataTexture")->AsShaderResource()->SetResource( NULL );
	}
	GFSDK_WaveWorks_Savestate_RestoreD3D11(hSavestate, pDC);
}

void OceanSurface::getQuadTreeStats(GFSDK_WaveWorks_Quadtree_Stats& stats)
{
	GFSDK_WaveWorks_Quadtree_GetStats(m_hOceanQuadTree, stats);
}
