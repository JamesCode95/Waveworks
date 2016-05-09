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
#include "ocean_vessel.h"
#include "ocean_hull_profile.h"
#include "ocean_hull_sensors.h"
#include "ocean_env.h"
#include "ocean_smoke.h"
#include "ocean_psm.h"
#include "ocean_surface_heights.h"

#include "GFSDK_WaveWorks.h"
#include "GFSDK_WaveWorks_D3D_Util.h"
#include "DXUTcamera.h"

#include <tchar.h>

#pragma warning(disable:4127)

const float kAccelerationDueToGravity = 9.81f;
const float kDensityOfWater = 1000.f;
const float kMaximumFractionalCornerError = 0.0001f; // 100ppm
const float kMaxSimulationTimeStep = 0.02f;

const float kSpotlightClipNear = 0.1f;
const float kSpotlightClipFar = 200.0f;

extern HRESULT LoadFile(LPCTSTR FileName, ID3DXBuffer** ppBuffer);
extern HRESULT CreateTextureFromFileSRGB(
        ID3D11Device*               pDevice,
        LPCTSTR                     pSrcFile,
        ID3D11Resource**            ppTexture);

inline bool operator!=(const D3DVERTEXELEMENT9& lhs, const D3DVERTEXELEMENT9& rhs) {
	return lhs.Stream != rhs.Stream ||
		lhs.Offset != rhs.Offset ||
		lhs.Type != rhs.Type ||
		lhs.Method != rhs.Method ||
		lhs.Usage != rhs.Usage ||
		lhs.UsageIndex != rhs.UsageIndex;
}

namespace {

	float sqr(float x) { return x * x; }

	D3DXVECTOR3 D3DXVec3Min(const D3DXVECTOR3& lhs, const D3DXVECTOR3& rhs) {
		return D3DXVECTOR3(min(lhs.x,rhs.x), min(lhs.y,rhs.y), min(lhs.z,rhs.z));
	}

	D3DXVECTOR3 D3DXVec3Max(const D3DXVECTOR3& lhs, const D3DXVECTOR3& rhs) {
		return D3DXVECTOR3(max(lhs.x,rhs.x), max(lhs.y,rhs.y), max(lhs.z,rhs.z));
	}
}

HRESULT BoatMesh::CreateInputLayout(ID3D11Device* pd3dDevice,
									UINT iMesh, UINT iVB,
									const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength,
									ID3D11InputLayout** pIL
									)
{
	// Translate from D3D9 decl...
	SDKMESH_VERTEX_BUFFER_HEADER* pVBHeader = m_pVertexBufferArray + m_pMeshArray[ iMesh ].VertexBuffers[iVB];

	D3D11_INPUT_ELEMENT_DESC vertex_layout[MAX_VERTEX_ELEMENTS];
	UINT num_layout_elements = 0;
	const D3DVERTEXELEMENT9 d3d9_decl_end = D3DDECL_END();
	while(pVBHeader->Decl[num_layout_elements] != d3d9_decl_end) {

		const D3DVERTEXELEMENT9& d3d9_decl_element = pVBHeader->Decl[num_layout_elements];
		D3D11_INPUT_ELEMENT_DESC& d3d11_layout_element = vertex_layout[num_layout_elements];

		// Translate usage
		switch(d3d9_decl_element.Usage) {
			case D3DDECLUSAGE_POSITION: d3d11_layout_element.SemanticName = "POSITION"; break;
			case D3DDECLUSAGE_NORMAL: d3d11_layout_element.SemanticName = "NORMAL"; break;
			case D3DDECLUSAGE_TEXCOORD: d3d11_layout_element.SemanticName = "TEXCOORD"; break;
			case D3DDECLUSAGE_COLOR: d3d11_layout_element.SemanticName = "COLOR"; break;
			default:
				return E_FAIL;	// Whoops, this usage not handled yet!
		}

		// Translate usage index
		d3d11_layout_element.SemanticIndex = d3d9_decl_element.UsageIndex;

		// Translate type
		switch(d3d9_decl_element.Type) {
			case D3DDECLTYPE_FLOAT1: d3d11_layout_element.Format = DXGI_FORMAT_R32_FLOAT; break;
			case D3DDECLTYPE_FLOAT2: d3d11_layout_element.Format = DXGI_FORMAT_R32G32_FLOAT; break;
			case D3DDECLTYPE_FLOAT3: d3d11_layout_element.Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
			case D3DDECLTYPE_FLOAT4: d3d11_layout_element.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			case D3DDECLTYPE_D3DCOLOR: d3d11_layout_element.Format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			default:
				return E_FAIL;	// Whoops, this format not handled yet!
		}

		// Translate stream
		d3d11_layout_element.InputSlot = d3d9_decl_element.Stream;

		// Translate offset
		d3d11_layout_element.AlignedByteOffset = d3d9_decl_element.Offset;

		// No instancing
		d3d11_layout_element.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		d3d11_layout_element.InstanceDataStepRate = 0;

		++num_layout_elements;
	}

	return pd3dDevice->CreateInputLayout(vertex_layout, num_layout_elements, pShaderBytecodeWithInputSignature, BytecodeLength, pIL);
}

OceanVessel::OceanVessel(OceanVesselDynamicState* pDynamicState) :
	m_pDynamicState(pDynamicState)
{
	m_MeshFileName = NULL;
	m_pMesh = NULL;
	m_pLayout = NULL;

	m_pFX = NULL;
	m_pRenderToSceneTechnique = NULL;
    m_pRenderToShadowMapTechnique = NULL;
	m_pRenderToHullProfileTechnique = NULL;
	m_pRenderQuadToUITechnique = NULL;
	m_pRenderQuadToCrackFixTechnique = NULL;
	m_pWireframeOverrideTechnique = NULL;

	m_pMatWorldViewProjVariable = NULL;
	m_pMatWorldVariable = NULL;
	m_pMatWorldViewVariable = NULL;
	m_pTexDiffuseVariable = NULL;
	m_pTexRustMapVariable = NULL;
	m_pTexRustVariable = NULL;
	m_pTexBumpVariable = NULL;
	m_pDiffuseColorVariable = NULL;
	m_pLightDirectionVariable = NULL;
	m_pLightColorVariable = NULL;
	m_pAmbientColorVariable = NULL;

	m_pSpotlightNumVariable = NULL;
	m_pSpotlightPositionVariable = NULL;
	m_pSpotLightAxisAndCosAngleVariable = NULL;
	m_pSpotlightColorVariable = NULL;
	m_pSpotlightShadowMatrixVar = NULL;
	m_pSpotlightShadowResourceVar = NULL;

	m_pLightningPositionVariable = NULL;
	m_pLightningColorVariable = NULL;

	m_pFogExponentVariable = NULL;

	m_pWhiteTextureSRV = NULL;
	m_pRustMapSRV = NULL;
	m_pRustSRV = NULL;
	m_pBumpSRV = NULL;

	m_pHullProfileSRV[0] = NULL;
	m_pHullProfileSRV[1] = NULL;
	m_pHullProfileRTV[0] = NULL;
	m_pHullProfileRTV[1] = NULL;
	m_pHullProfileDSV = NULL;

	m_pd3dDevice = DXUTGetD3D11Device();

	m_HeightDrag = 1.f;
	m_PitchDrag = 2.f;
	m_YawDrag = 1.f;
	m_YawCoefficient = 1.f;
	m_RollDrag = 1.f;

	m_Length = 30.f;
	m_CameraHeightAboveWater = 6.f;
	m_CameraLongitudinalOffset = 5.f;
	m_MetacentricHeight = 1.f;
	m_LongitudinalCOM = -7.f;
	m_MassMult = 0.5f;
	m_PitchInertiaMult = 0.2f;
	m_RollInertiaMult = 0.3f;

	m_InitialPitch = 0.0438f;
	m_InitialHeight = -0.75f;

	m_HullProfileTextureWH = 512;

	m_DiffuseGamma = 3.f;

	m_FunnelLongitudinalOffset = 0.f;
	m_FunnelHeightAboveWater = 6.f;
	m_FunnelMouthSize = D3DXVECTOR2(1.f,1.f);

	m_NumSmokeParticles = 4096;
	m_SmokeParticleEmitRate = FLOAT(m_NumSmokeParticles)/10.f;
	m_SmokeParticleEmitMinVelocity = 1.f;
	m_SmokeParticleEmitMaxVelocity = 1.f;
	m_SmokeParticleMinBuoyancy = 0.f;
	m_SmokeParticleMaxBuoyancy = 1.f;
	m_SmokeParticleCoolingRate = 0.f;
	m_SmokeParticleEmitSpread = 0.f;
	m_SmokeParticleBeginSize = 1.f;
	m_SmokeParticleEndSize = 1.f;
	m_SmokeWindDrag = 1.f;
	m_SmokePSMBoundsFadeMargin = 0.1f;
	m_SmokeWindNoiseLevel = 2.f;
	m_SmokeWindNoiseSpatialScale = 1.f;
	m_SmokeWindNoiseTimeScale = 1.f;
	m_SmokeTint = D3DXVECTOR3( 1.f, 1.f, 1.f);
	m_SmokeShadowOpacity = 1.f;
	m_SmokeTextureFileName = NULL;
	m_pSmoke = NULL;

	m_PSMRes = 512;
	m_PSMMinCorner = D3DXVECTOR3(-1.f,-1.f,-1.f);
	m_PSMMaxCorner = D3DXVECTOR3( 1.f, 1.f, 1.f);
	m_pPSM = NULL;

	m_NumSurfaceHeightSamples = 1000;
	m_pSurfaceHeights = NULL;

	m_pHullSensors = NULL;
}

OceanVessel::~OceanVessel()
{
	SAFE_DELETE(m_pHullSensors);
	SAFE_DELETE(m_pSurfaceHeights);
	SAFE_DELETE_ARRAY(m_SmokeTextureFileName);
	SAFE_DELETE(m_pSmoke);
	SAFE_DELETE(m_pPSM);
	SAFE_DELETE_ARRAY(m_MeshFileName);
	SAFE_RELEASE(m_pHullProfileSRV[0]);
	SAFE_RELEASE(m_pHullProfileSRV[1]);
	SAFE_RELEASE(m_pHullProfileRTV[0]);
	SAFE_RELEASE(m_pHullProfileRTV[1]);
	SAFE_RELEASE(m_pHullProfileDSV);
	SAFE_RELEASE(m_pRustMapSRV);
	SAFE_RELEASE(m_pRustSRV);
	SAFE_RELEASE(m_pBumpSRV);
	SAFE_RELEASE(m_pWhiteTextureSRV);

	SAFE_DELETE(m_pMesh);
	SAFE_RELEASE(m_pLayout);
	SAFE_RELEASE(m_pFX);

    m_SpotlightsShadows.clear();
}

HRESULT OceanVessel::init(LPCTSTR cfg_string, bool allow_smoke)
{
	HRESULT hr = S_OK;
	
	// Parse the cfg file
	V_RETURN(parseConfig(cfg_string));

	// Load the mesh
	SAFE_DELETE(m_pMesh);
	m_pMesh = new BoatMesh();
	V_RETURN(m_pMesh->Create(m_pd3dDevice, m_MeshFileName));

	// Get the bounding box and figure out the scale needed to achieve the desired length,
	// then figure out the other dims
	UINT num_meshes = m_pMesh->GetNumMeshes();
	if(0 == num_meshes)
		return E_FAIL;

	m_bbExtents = m_pMesh->GetMeshBBoxExtents(0);
	m_bbCentre = m_pMesh->GetMeshBBoxCenter(0);
	FLOAT meshRenderScale = 0.5f * m_Length/m_bbExtents.z;

	D3DXMATRIX matMeshScale;
	D3DXMatrixScaling(&matMeshScale, meshRenderScale, meshRenderScale, meshRenderScale);

	D3DXMATRIX matMeshOrient;
	D3DXMatrixRotationY(&matMeshOrient, D3DX_PI);

	D3DXMATRIX matMeshOffset;
	D3DXMatrixTranslation(&matMeshOffset, -m_bbCentre.x, 0.f, -m_bbCentre.z);

	m_MeshToLocal = matMeshOffset * matMeshScale * matMeshOrient;

	D3DXMatrixIdentity(&m_CameraToLocal);
	m_CameraToLocal._42 = m_CameraHeightAboveWater;
	m_CameraToLocal._43 = m_CameraLongitudinalOffset;

	D3DXMatrixRotationX(&m_FunnelMouthToLocal,-3.14f*0.3f);
	//D3DXMatrixIdentity(&m_FunnelMouthToLocal);
	m_FunnelMouthToLocal._42 = m_FunnelHeightAboveWater;
	m_FunnelMouthToLocal._43 = m_FunnelLongitudinalOffset;

	m_Draft = (m_bbExtents.y-m_bbCentre.y) * meshRenderScale;								// Assumes mesh was modelled with the MWL at y = 0
	m_Beam = 2.f * m_bbExtents.x * meshRenderScale;
	m_MeshHeight = 2.f * m_bbExtents.y * meshRenderScale;
	m_BuoyantArea = m_Length * m_Beam;
	m_Mass = m_BuoyantArea * m_Draft * kDensityOfWater;									// At equilibrium, the displaced water is equal to the mass of the ship
	m_Mass *= 0.25f*D3DX_PI;															// We approximate the hull profile with an ellipse, it is important to
																						// match this in the mass calc so that the ship sits at the right height
																						// in the water at rest
	m_Mass *= m_MassMult;
	m_PitchInertia = m_Mass * (m_Draft * m_Draft + m_Length * m_Length)/12.f;	// Use the inertia of the displaced water
	m_RollInertia = m_Mass * (m_Draft * m_Draft + m_Beam * m_Beam)/12.f;

	m_PitchInertia *= m_PitchInertiaMult;
	m_RollInertia *= m_RollInertiaMult;

	SAFE_RELEASE(m_pFX);
    ID3DXBuffer* pEffectBuffer = NULL;
    V_RETURN(LoadFile(TEXT(".\\Media\\ocean_vessel_d3d11.fxo"), &pEffectBuffer));
    V_RETURN(D3DX11CreateEffectFromMemory(pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, m_pd3dDevice, &m_pFX));
    pEffectBuffer->Release();

	m_pRenderToSceneTechnique = m_pFX->GetTechniqueByName("RenderVesselToSceneTech");
	m_pRenderToShadowMapTechnique = m_pFX->GetTechniqueByName("RenderVesselToShadowMapTech");
	m_pRenderToHullProfileTechnique = m_pFX->GetTechniqueByName("RenderVesselToHullProfileTech");
	m_pRenderQuadToUITechnique = m_pFX->GetTechniqueByName("RenderQuadToUITech");
	m_pRenderQuadToCrackFixTechnique = m_pFX->GetTechniqueByName("RenderQuadToCrackFixTech");
	m_pWireframeOverrideTechnique = m_pFX->GetTechniqueByName("WireframeOverrideTech");
	m_pMatWorldViewProjVariable = m_pFX->GetVariableByName("g_matWorldViewProj")->AsMatrix();
	m_pMatWorldVariable = m_pFX->GetVariableByName("g_matWorld")->AsMatrix();
	m_pMatWorldViewVariable = m_pFX->GetVariableByName("g_matWorldView")->AsMatrix();
	m_pTexDiffuseVariable = m_pFX->GetVariableByName("g_texDiffuse")->AsShaderResource();
	m_pTexRustMapVariable = m_pFX->GetVariableByName("g_texRustMap")->AsShaderResource();
	m_pTexRustVariable = m_pFX->GetVariableByName("g_texRust")->AsShaderResource();
	m_pTexBumpVariable = m_pFX->GetVariableByName("g_texBump")->AsShaderResource();
	m_pDiffuseColorVariable = m_pFX->GetVariableByName("g_DiffuseColor")->AsVector();
	m_pLightDirectionVariable = m_pFX->GetVariableByName("g_LightDirection")->AsVector();
	m_pLightColorVariable = m_pFX->GetVariableByName("g_LightColor")->AsVector();
	m_pAmbientColorVariable = m_pFX->GetVariableByName("g_AmbientColor")->AsVector();

	m_pSpotlightNumVariable = m_pFX->GetVariableByName("g_LightsNum")->AsScalar();
	m_pSpotlightPositionVariable = m_pFX->GetVariableByName("g_SpotlightPosition")->AsVector();
	m_pSpotLightAxisAndCosAngleVariable = m_pFX->GetVariableByName("g_SpotLightAxisAndCosAngle")->AsVector();
	m_pSpotlightColorVariable = m_pFX->GetVariableByName("g_SpotlightColor")->AsVector();
	m_pSpotlightShadowMatrixVar = m_pFX->GetVariableByName("g_SpotlightMatrix")->AsMatrix();
	m_pSpotlightShadowResourceVar = m_pFX->GetVariableByName("g_SpotlightResource")->AsShaderResource();

	m_pLightningPositionVariable = m_pFX->GetVariableByName("g_LightningPosition")->AsVector();
	m_pLightningColorVariable = m_pFX->GetVariableByName("g_LightningColor")->AsVector();

	m_pFogExponentVariable = m_pFX->GetVariableByName("g_FogExponent")->AsScalar();

	D3DX11_PASS_DESC PassDesc;
	V_RETURN(m_pRenderToSceneTechnique->GetPassByIndex(0)->GetDesc(&PassDesc));

	SAFE_RELEASE(m_pLayout);
	V_RETURN(m_pMesh->CreateInputLayout(m_pd3dDevice, 0, 0, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &m_pLayout));

	// Set up an all-white texture SRV to use when a mesh subset has no associated texture
	{
		SAFE_RELEASE(m_pWhiteTextureSRV);

		enum { WhiteTextureWH = 4 };
		D3D11_TEXTURE2D_DESC tex_desc;
		tex_desc.Width = WhiteTextureWH;
		tex_desc.Height = WhiteTextureWH;
		tex_desc.ArraySize = 1;
		tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex_desc.CPUAccessFlags = 0;
		tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		tex_desc.MipLevels = 1;
		tex_desc.MiscFlags = 0;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.SampleDesc.Quality = 0;
		tex_desc.Usage = D3D11_USAGE_IMMUTABLE;

		DWORD tex_data[WhiteTextureWH * WhiteTextureWH];
		for(int i = 0; i != sizeof(tex_data)/sizeof(tex_data[0]); ++i) {
			tex_data[i] = 0xFFFFFFFF;
		}

		D3D11_SUBRESOURCE_DATA tex_srd;
		tex_srd.pSysMem = tex_data;
		tex_srd.SysMemPitch = WhiteTextureWH * sizeof(tex_data[0]);
		tex_srd.SysMemSlicePitch = sizeof(tex_data);

		ID3D11Texture2D* pWhiteTetxure = NULL;
		V_RETURN(m_pd3dDevice->CreateTexture2D(&tex_desc,&tex_srd,&pWhiteTetxure));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pWhiteTetxure,NULL,&m_pWhiteTextureSRV));
		SAFE_RELEASE(pWhiteTetxure);
	}

	{
		SAFE_RELEASE(m_pHullProfileSRV[0]);
		SAFE_RELEASE(m_pHullProfileSRV[1]);
		SAFE_RELEASE(m_pHullProfileRTV[0]);
		SAFE_RELEASE(m_pHullProfileRTV[1]);
		SAFE_RELEASE(m_pHullProfileDSV);
		ID3D11Texture2D* pTexture = NULL;

		// Set up textures for rendering hull profile
		D3D11_TEXTURE2D_DESC tex_desc;
		tex_desc.Width = m_HullProfileTextureWH;
		tex_desc.Height = m_HullProfileTextureWH;
		tex_desc.ArraySize = 1;
		tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		tex_desc.CPUAccessFlags = 0;
		tex_desc.Format = DXGI_FORMAT_R16G16_UNORM;
		tex_desc.MipLevels = 1;
		tex_desc.MiscFlags = 0;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.SampleDesc.Quality = 0;
		tex_desc.Usage = D3D11_USAGE_DEFAULT;

		V_RETURN(m_pd3dDevice->CreateTexture2D(&tex_desc,NULL,&pTexture));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pTexture,NULL,&m_pHullProfileSRV[0]));
		V_RETURN(m_pd3dDevice->CreateRenderTargetView(pTexture,NULL,&m_pHullProfileRTV[0]));
		SAFE_RELEASE(pTexture);

		tex_desc.MipLevels = 0;
		tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		V_RETURN(m_pd3dDevice->CreateTexture2D(&tex_desc,NULL,&pTexture));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pTexture,NULL,&m_pHullProfileSRV[1]));
		V_RETURN(m_pd3dDevice->CreateRenderTargetView(pTexture,NULL,&m_pHullProfileRTV[1]));
		SAFE_RELEASE(pTexture);

		D3D11_TEXTURE2D_DESC depth_tex_desc;
		depth_tex_desc.Width = m_HullProfileTextureWH;
		depth_tex_desc.Height = m_HullProfileTextureWH;
		depth_tex_desc.ArraySize = 1;
		depth_tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depth_tex_desc.CPUAccessFlags = 0;
		depth_tex_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_tex_desc.MipLevels = 1;
		depth_tex_desc.MiscFlags = 0;
		depth_tex_desc.SampleDesc.Count = 1;
		depth_tex_desc.SampleDesc.Quality = 0;
		depth_tex_desc.Usage = D3D11_USAGE_DEFAULT;

		V_RETURN(m_pd3dDevice->CreateTexture2D(&depth_tex_desc,NULL,&pTexture));
		V_RETURN(m_pd3dDevice->CreateDepthStencilView(pTexture,NULL,&m_pHullProfileDSV));
		SAFE_RELEASE(pTexture);
	}

#if ENABLE_SHADOWS
    if (!m_Spotlights.empty())
    {
        size_t lightsNum = m_Spotlights.size();
        m_SpotlightsShadows.resize(lightsNum);

        for (size_t i=0; i<lightsNum; ++i)
        {
            SpotlightShadow& shadow = m_SpotlightsShadows[i];

            CD3D11_TEXTURE2D_DESC desc(DXGI_FORMAT_R24G8_TYPELESS, (UINT)kSpotlightShadowResolution, (UINT)kSpotlightShadowResolution, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL);
            V_RETURN(m_pd3dDevice->CreateTexture2D(&desc, NULL, &shadow.m_pResource));

            CD3D11_DEPTH_STENCIL_VIEW_DESC descDSV(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D24_UNORM_S8_UINT);
            V_RETURN(m_pd3dDevice->CreateDepthStencilView(shadow.m_pResource, &descDSV, &m_SpotlightsShadows[i].m_pDSV));

            CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
            V_RETURN(m_pd3dDevice->CreateShaderResourceView(shadow.m_pResource, &descSRV, &m_SpotlightsShadows[i].m_pSRV));
        }
    }
#endif

	if(m_SmokeTextureFileName && allow_smoke) {

		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(CreateTextureFromFileSRGB(m_pd3dDevice, m_SmokeTextureFileName, &pD3D11Resource));
		ID3D11ShaderResourceView* pSmokeTextureSRV;
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &pSmokeTextureSRV));
		SAFE_RELEASE(pD3D11Resource);

		m_pSmoke = new OceanSmoke();
		V_RETURN(m_pSmoke->init(	pSmokeTextureSRV,
									m_NumSmokeParticles,
									m_SmokeParticleEmitRate,
									m_SmokeParticleBeginSize,
									m_SmokeParticleEndSize,
									m_SmokeParticleEmitMinVelocity,
									m_SmokeParticleEmitMaxVelocity,
									m_SmokeParticleEmitSpread,
									m_SmokeWindDrag,
									m_SmokeParticleMinBuoyancy,
									m_SmokeParticleMaxBuoyancy,
									m_SmokeParticleCoolingRate,
									m_FunnelMouthSize,
									m_SmokePSMBoundsFadeMargin,
									m_SmokeShadowOpacity,
									m_SmokeTint,
									m_SmokeWindNoiseSpatialScale,
									m_SmokeWindNoiseTimeScale
									));
		SAFE_RELEASE(pSmokeTextureSRV);
	}

	m_pPSM = new OceanPSM();
	V_RETURN(m_pPSM->init(m_PSMMinCorner,m_PSMMaxCorner,m_PSMRes));

	if(NULL == m_pRustMapSRV)
	{
		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(D3DX11CreateTextureFromFile(m_pd3dDevice, TEXT(".\\media\\rustmap.dds"), NULL, NULL, &pD3D11Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pRustMapSRV));
		SAFE_RELEASE(pD3D11Resource);
	}

	if(NULL == m_pRustSRV)
	{
		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(D3DX11CreateTextureFromFile(m_pd3dDevice, TEXT(".\\media\\rust.dds"), NULL, NULL, &pD3D11Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pRustSRV));
		SAFE_RELEASE(pD3D11Resource);
	}

	if(NULL == m_pBumpSRV)
	{
		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(D3DX11CreateTextureFromFile(m_pd3dDevice, TEXT(".\\media\\foam_intensity_perlin2.dds"), NULL, NULL, &pD3D11Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pBumpSRV));
		SAFE_RELEASE(pD3D11Resource);
	}

	gfsdk_float2 UVToWorldScale;
	UVToWorldScale.x = 2.f * sqrtf(m_Beam*m_Beam+m_MeshHeight*m_MeshHeight);	// Use double-diagonal, to be conservative
	UVToWorldScale.x +=	m_Length;												// Then add another vessel length, to make sure we catch big bow sprays
	UVToWorldScale.y = m_Length + 2.f * m_MeshHeight;							// Add height, to be conservative

	m_pSurfaceHeights = new OceanSurfaceHeights(m_NumSurfaceHeightSamples,UVToWorldScale);
	V_RETURN(m_pSurfaceHeights->init());

	m_pHullSensors = new OceanHullSensors();
	V_RETURN(m_pHullSensors->init(m_pMesh, *getMeshToLocalXform()));
	m_bFirstSensorUpdate = true;

	return S_OK;
}

void OceanVessel::renderVessel(ID3D11DeviceContext* pDC, ID3DX11EffectTechnique* pTechnique, const OceanVesselSubset* pSubsetOverride, bool wireframe, bool depthOnly)
{
	if(NULL == m_pMesh)
		return;

	// Iterate over mesh subsets to draw
	BoatMesh& mesh = *m_pMesh;
	UINT Strides[1];
	UINT Offsets[1];
	ID3D11Buffer* pVB[1];
	pVB[0] = mesh.GetVB11(0,0);
	Strides[0] = (UINT)mesh.GetVertexStride(0,0);
	Offsets[0] = 0;
    pDC->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
	pDC->IASetInputLayout(m_pLayout);

	m_pTexRustMapVariable->SetResource(m_pRustMapSRV);
	m_pTexRustVariable->SetResource(m_pRustSRV);
	m_pTexBumpVariable->SetResource(m_pBumpSRV);

	if(pSubsetOverride) {
		pDC->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
		pDC->IASetIndexBuffer( pSubsetOverride->pIB, pSubsetOverride->ib_format, 0);
		m_pTexDiffuseVariable->SetResource(m_pWhiteTextureSRV);
		D3DXVECTOR4 diffuse = D3DXVECTOR4(100,100,100,100);
		m_pDiffuseColorVariable->SetFloatVector((FLOAT*)&diffuse);
		pTechnique->GetPassByIndex(0)->Apply(0, pDC);
		if(wireframe)
			m_pWireframeOverrideTechnique->GetPassByIndex(0)->Apply(0,pDC);
		pDC->DrawIndexed(pSubsetOverride->index_count, 0, 0);
	} else {
		pDC->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		pDC->IASetIndexBuffer( mesh.GetIB11(0), mesh.GetIBFormat11(0), 0 );
		pTechnique->GetPassByIndex(0)->Apply(0, pDC);
		if(wireframe)
			m_pWireframeOverrideTechnique->GetPassByIndex(0)->Apply(0,pDC);
		for (UINT subset = 0; subset < mesh.GetNumSubsets(0); ++subset)
		{
			SDKMESH_SUBSET* pSubset = mesh.GetSubset( 0, subset ); 

			if (depthOnly == false) // TODO: Currently don't support alpha-tested materials
			{
				SDKMESH_MATERIAL* pMat = mesh.GetMaterial(pSubset->MaterialID);

				m_pTexDiffuseVariable->SetResource(pMat->pDiffuseRV11);// ? pMat->pDiffuseRV11 : m_pWhiteTextureSRV);

				D3DXVECTOR4 diffuse = pMat->Diffuse;
				diffuse.x = powf(diffuse.x,m_DiffuseGamma);//*1.9f;
				diffuse.y = powf(diffuse.y,m_DiffuseGamma);//*1.9f;
				diffuse.z = powf(diffuse.z,m_DiffuseGamma);//*1.9f;
				// de-saturating the diffuse color a bit
				D3DXVECTOR4  LuminanceWeights = D3DXVECTOR4(0.299f,0.587f,0.114f, 0.0f);
				float Luminance = D3DXVec4Dot(&diffuse,&LuminanceWeights);
				D3DXVECTOR4 LuminanceVec = D3DXVECTOR4(Luminance,Luminance,Luminance,1.0f);
				D3DXVec4Lerp(&diffuse,&diffuse,&LuminanceVec,0.7f);

				m_pDiffuseColorVariable->SetFloatVector((FLOAT*)&diffuse);
	            
				// HACK to render the hull with no backface culling but the rest with backface
				if(pSubset->MaterialID != 0)
					pTechnique->GetPassByIndex(0)->Apply(0, pDC);
				else
					pTechnique->GetPassByIndex(1)->Apply(0, pDC);

				if(wireframe)
					m_pWireframeOverrideTechnique->GetPassByIndex(0)->Apply(0,pDC);
			}

			pDC->DrawIndexed( (UINT)pSubset->IndexCount, (UINT)pSubset->IndexStart, (UINT)pSubset->VertexStart );
		}
	}
}

void OceanVessel::renderVesselToScene(	ID3D11DeviceContext* pDC,
										const D3DXMATRIX& matView,
										const D3DXMATRIX& matProj,
										const OceanEnvironment& ocean_env,
										const OceanVesselSubset* pSubsetOverride,
										bool wireframe
										)
{
	D3DXMATRIX matLocalToView = m_pDynamicState->m_LocalToWorld * matView;

	// View-proj
	D3DXMATRIX matW = m_MeshToLocal * m_pDynamicState->m_LocalToWorld;
	D3DXMATRIX matWV = m_MeshToLocal * matLocalToView;
	D3DXMATRIX matWVP = matWV * matProj;
	m_pMatWorldViewProjVariable->SetMatrix((FLOAT*)&matWVP);
	m_pMatWorldVariable->SetMatrix((FLOAT*)&matW);
	m_pMatWorldViewVariable->SetMatrix((FLOAT*)&matWV);

	// Global lighting
	m_pLightDirectionVariable->SetFloatVector((FLOAT*)&ocean_env.main_light_direction);
	m_pLightColorVariable->SetFloatVector((FLOAT*)&ocean_env.main_light_color);
	m_pAmbientColorVariable->SetFloatVector((FLOAT*)&ocean_env.sky_color);

	// Spot lights - transform to view space
	D3DXMATRIX matSpotlightsToView = ocean_env.spotlights_to_world_matrix * matView;
	D3DXMATRIX matViewToSpotlights;
	D3DXMatrixInverse(&matViewToSpotlights,NULL,&matSpotlightsToView);
	D3DXVECTOR4 spotlight_position[MaxNumSpotlights];
	D3DXVECTOR4 spotlight_axis_and_cos_angle[MaxNumSpotlights];
	D3DXVECTOR4 spotlight_color[MaxNumSpotlights];
	int lightsNum = 0;

	D3DXVec4TransformArray(spotlight_position,sizeof(spotlight_position[0]),ocean_env.spotlight_position,sizeof(ocean_env.spotlight_position[0]),&matSpotlightsToView,MaxNumSpotlights);
	D3DXVec3TransformNormalArray((D3DXVECTOR3*)spotlight_axis_and_cos_angle,sizeof(spotlight_axis_and_cos_angle[0]),(D3DXVECTOR3*)ocean_env.spotlight_axis_and_cos_angle,sizeof(ocean_env.spotlight_axis_and_cos_angle[0]),&matSpotlightsToView,MaxNumSpotlights);

	for(int i=0; i!=ocean_env.activeLightsNum; ++i) {

		if (ocean_env.lightFilter != -1 && ocean_env.objectID[i] != ocean_env.lightFilter) continue;

		spotlight_position[lightsNum] = spotlight_position[i];
		spotlight_axis_and_cos_angle[lightsNum] = spotlight_axis_and_cos_angle[i];
		spotlight_color[lightsNum] = ocean_env.spotlight_color[i];
		spotlight_axis_and_cos_angle[lightsNum].w = ocean_env.spotlight_axis_and_cos_angle[i].w;

#if ENABLE_SHADOWS
		D3DXMATRIX spotlight_shadow_matrix = matViewToSpotlights * ocean_env.spotlight_shadow_matrix[i];
		m_pSpotlightShadowMatrixVar->SetMatrixArray((float*)&spotlight_shadow_matrix, lightsNum, 1);
		m_pSpotlightShadowResourceVar->SetResourceArray((ID3D11ShaderResourceView**)&ocean_env.spotlight_shadow_resource[i], lightsNum, 1);
#endif

		++lightsNum;
	}

	m_pSpotlightNumVariable->SetInt(lightsNum);
	m_pSpotlightPositionVariable->SetFloatVectorArray((FLOAT*)spotlight_position,0,lightsNum);
	m_pSpotLightAxisAndCosAngleVariable->SetFloatVectorArray((FLOAT*)spotlight_axis_and_cos_angle,0,lightsNum);
	m_pSpotlightColorVariable->SetFloatVectorArray((FLOAT*)spotlight_color,0,lightsNum);

	// Lightnings
	m_pLightningColorVariable->SetFloatVector((FLOAT*)&ocean_env.lightning_light_intensity);
	m_pLightningPositionVariable->SetFloatVector((FLOAT*)&ocean_env.lightning_light_position);

	// Fog
	m_pFogExponentVariable->SetFloat(ocean_env.fog_exponent*ocean_env.cloud_factor);

	renderVessel(pDC, m_pRenderToSceneTechnique, pSubsetOverride, wireframe, false);

	// Release input refs
	ID3D11ShaderResourceView* pNullSRVs[MaxNumSpotlights];
	memset(pNullSRVs,0,sizeof(pNullSRVs));
	m_pSpotlightShadowResourceVar->SetResourceArray(pNullSRVs,0,MaxNumSpotlights);
	m_pRenderToSceneTechnique->GetPassByIndex(0)->Apply(0,pDC);
}


void OceanVessel::renderReflectedVesselToScene(	ID3D11DeviceContext* pDC,
												const CBaseCamera& camera,
												const D3DXPLANE& world_reflection_plane,
												const OceanEnvironment& ocean_env
												)
{
    D3DXMATRIX matView = *camera.GetViewMatrix();
	D3DXMATRIX matProj = *camera.GetProjMatrix();

	D3DXMATRIX matReflection;
	D3DXMatrixReflect(&matReflection, &world_reflection_plane);

	matView = matReflection*matView;

	renderVesselToScene(pDC, matView, matProj, ocean_env, NULL, false);
}

void OceanVessel::updateVesselShadows(	ID3D11DeviceContext* pDC
										)
{
#if ENABLE_SHADOWS
	D3D11_VIEWPORT original_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	UINT num_original_viewports = sizeof(original_viewports)/sizeof(original_viewports[0]);
	pDC->RSGetViewports( &num_original_viewports, original_viewports);

    CD3D11_VIEWPORT viewport(0.0f, 0.0f, (float)kSpotlightShadowResolution, (float)kSpotlightShadowResolution);
    pDC->RSSetViewports(1, &viewport);

    size_t lightsNum = m_Spotlights.size();
    for (size_t i=0; i<lightsNum; ++i)
    {
        if (m_SpotlightsShadows[i].m_Dirty == false)
        {
            continue;
        }

        const Spotlight& sl = m_Spotlights[i];

        D3DXMATRIX matView;
	    D3DXMATRIX matProj;

		D3DXVECTOR3 lightPosWorldSpace;
		D3DXVec3TransformCoord(&lightPosWorldSpace, (D3DXVECTOR3*)&sl.position, &m_pDynamicState->m_LocalToWorld);

		D3DXVECTOR3 lightAxisWorldSpace;
		D3DXVec3TransformNormal(&lightAxisWorldSpace, (D3DXVECTOR3*)&sl.axis, &m_pDynamicState->m_LocalToWorld);

        D3DXVECTOR3 lookAt = (D3DXVECTOR3&)sl.position + (D3DXVECTOR3&)sl.axis;
        D3DXVECTOR3 up(1.0f, 0.0, 1.0f);
        D3DXMatrixLookAtLH(&matView, (D3DXVECTOR3*)&sl.position, &lookAt, &up);

        D3DXMatrixPerspectiveFovLH(&matProj, m_Spotlights[i].beam_angle, 1.0f, kSpotlightClipNear, kSpotlightClipFar);
	
        D3DXMATRIX matW = m_MeshToLocal;
	    D3DXMATRIX matWV = matW * matView;
	    D3DXMATRIX matWVP = matWV * matProj;
	    m_pMatWorldViewProjVariable->SetMatrix((FLOAT*)&matWVP);

        m_SpotlightsShadows[i].m_ViewProjMatrix = matView * matProj;

        pDC->ClearDepthStencilView(m_SpotlightsShadows[i].m_pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
        pDC->OMSetRenderTargets(0, NULL, m_SpotlightsShadows[i].m_pDSV);

	    renderVessel(pDC, m_pRenderToShadowMapTechnique, NULL, false, true);

        m_SpotlightsShadows[i].m_Dirty = false;
    }

    pDC->RSSetViewports(num_original_viewports, original_viewports);

    pDC->OMSetRenderTargets(0, NULL, NULL);
#endif
}

void OceanVesselDynamicState::setPosition(D3DXVECTOR2 pos)
{
	m_Position = pos;
	ResetDynamicState();
}

OceanVesselDynamicState::OceanVesselDynamicState()
{
	m_bFirstUpdate = true;
}

void OceanVesselDynamicState::setHeading(D3DXVECTOR2 heading, FLOAT speed)
{
	m_NominalHeading = heading;
	m_Speed = speed;
	D3DXVec2Normalize(&m_NominalHeading, &m_NominalHeading);
	ResetDynamicState();
}

void OceanVessel::updateVesselMotion(ID3D11DeviceContext* pDC, GFSDK_WaveWorks_SimulationHandle hSim, FLOAT sea_level, FLOAT time_delta, FLOAT water_scale)
{
	m_pDynamicState->m_Position += time_delta * m_pDynamicState->m_Speed * m_pDynamicState->m_NominalHeading;

	const FLOAT actual_heading_angle = atan2f(m_pDynamicState->m_NominalHeading.x, m_pDynamicState->m_NominalHeading.y) + m_pDynamicState->m_Yaw;
	D3DXVECTOR2 actual_heading;
	actual_heading.x = sinf(actual_heading_angle);
	actual_heading.y = cosf(actual_heading_angle);
	const D3DXVECTOR2 heading_perp = D3DXVECTOR2(actual_heading.y, -actual_heading.x);

	// Use the displacement of our current position for establishing a footprint
	gfsdk_float4 nominal_displacement;
	GFSDK_WaveWorks_Simulation_GetDisplacements(hSim, (gfsdk_float2*)&m_pDynamicState->m_Position, &nominal_displacement, 1);
	gfsdk_float2 UVToWorldRotation;
	UVToWorldRotation.x = actual_heading.y;
	UVToWorldRotation.y = actual_heading.x;
	gfsdk_float2 worldCentroid;
	worldCentroid.x = m_pDynamicState->m_Position.x + nominal_displacement.x;
	worldCentroid.y = m_pDynamicState->m_Position.y + nominal_displacement.y;
	m_pSurfaceHeights->updateHeights(pDC,hSim,UVToWorldRotation,worldCentroid);

	// Force a sensor update on first update (for subsequent updates, we will re-use the trailing update next frame)
	if(m_bFirstSensorUpdate) {
		if(m_pDynamicState->m_bFirstUpdate) {

			// m_pDynamicState->m_LocalToWorld has yet to be updated so lookup the displacement of the origin and use a reasonable estimate

			gfsdk_float2 lookup_coord;
			lookup_coord.x = m_pDynamicState->m_Position.x;
			lookup_coord.y = m_pDynamicState->m_Position.y;

			gfsdk_float4 nominal_displacement;
			m_pSurfaceHeights->getDisplacements(&lookup_coord,&nominal_displacement,1);

			D3DXMATRIX mat_roll;
			D3DXMatrixRotationZ(&mat_roll, m_pDynamicState->m_Roll);

			D3DXMATRIX mat_pitch;
			D3DXMatrixRotationX(&mat_pitch, -m_pDynamicState->m_Pitch);

			D3DXMATRIX mat_heading;
			const FLOAT heading = atan2f(m_pDynamicState->m_NominalHeading.x, m_pDynamicState->m_NominalHeading.y);
			D3DXMatrixRotationY(&mat_heading, heading + m_pDynamicState->m_Yaw);

			m_pDynamicState->m_LocalToWorld = mat_roll * mat_pitch * mat_heading;

			m_pDynamicState->m_LocalToWorld._41 = m_pDynamicState->m_Position.x + nominal_displacement.x;
			m_pDynamicState->m_LocalToWorld._42 = sea_level + m_pDynamicState->m_Height;
			m_pDynamicState->m_LocalToWorld._43 = m_pDynamicState->m_Position.y + nominal_displacement.y;

			m_pHullSensors->update(m_pSurfaceHeights,m_pDynamicState->m_LocalToWorld);
			m_pDynamicState->m_bFirstUpdate = false;

		} else {

			// m_pDynamicState->m_LocalToWorld is valid
			m_pHullSensors->update(m_pSurfaceHeights,m_pDynamicState->m_LocalToWorld);
		}
		m_bFirstSensorUpdate = false;
	}

	// Caculate the means
	D3DXVECTOR4 mean_displacement(0.f,0.f,0.f,0.f);
	float mean_displaced_head = 0.f;
	float mean_pitch_moment = 0.f;
	float mean_roll_moment = 0.f;
	const int num_samples = m_pHullSensors->getNumSensors();
	float immersed_ratio = 0.f;
	{
		const D3DXVECTOR4* pDisplacements = m_pHullSensors->getDisplacements();
		const D3DXVECTOR3* pWorldSensorPositions = m_pHullSensors->getWorldSensorPositions();
		const D3DXVECTOR3* pWorldSensorNormals = m_pHullSensors->getWorldSensorNormals();
		const D3DXVECTOR3* pSensorPositions = m_pHullSensors->getSensorPositions();
		for(int sample_ix = 0; sample_ix != num_samples; ++sample_ix)
		{
			mean_displacement += pDisplacements[sample_ix];

			// Only immersed areas of the hull contribute to physics
			float water_depth_at_sample = pDisplacements[sample_ix].z - pWorldSensorPositions[sample_ix].z;
			if(water_depth_at_sample > 0.f)
			{
				float upward_head = water_depth_at_sample * -pWorldSensorNormals[sample_ix].z;	// Assume that non-upward pressure cancel out
				mean_displaced_head += upward_head;
				mean_pitch_moment += upward_head * pSensorPositions[sample_ix].z;
				mean_roll_moment += upward_head * pSensorPositions[sample_ix].x;
				immersed_ratio += 1.f;
			}
		}
	}
	mean_displacement *= water_scale/float(num_samples);
	mean_displaced_head *= water_scale/float(num_samples);
	mean_pitch_moment *= (water_scale*water_scale)/float(num_samples);
	mean_roll_moment *= (water_scale*water_scale)/float(num_samples);
	immersed_ratio /= float(num_samples);

	// Directly sample displacement at bow and stern
	D3DXVECTOR3 bowPos;
	bowPos.x = 0.f;
	bowPos.y = 0.f;
	bowPos.z = 0.5f * m_Length;
	D3DXVECTOR3 sternPos = -bowPos;
	D3DXVec3TransformCoord(&bowPos,&bowPos,&m_pDynamicState->m_LocalToWorld);
	D3DXVec3TransformCoord(&sternPos,&sternPos,&m_pDynamicState->m_LocalToWorld);

	gfsdk_float2 lookup_coords[2];
	lookup_coords[0].x = bowPos.x;
	lookup_coords[0].y = bowPos.z;
	lookup_coords[1].x = sternPos.x;
	lookup_coords[1].y = sternPos.z;

	gfsdk_float2 projected_stern_to_bow;
	projected_stern_to_bow.x = bowPos.x - sternPos.x;
	projected_stern_to_bow.y = bowPos.y - sternPos.y;
	const float projected_length = sqrtf(projected_stern_to_bow.x*projected_stern_to_bow.x + projected_stern_to_bow.y*projected_stern_to_bow.y);

	gfsdk_float4 bow_stern_disps[2];
	m_pSurfaceHeights->getDisplacements(lookup_coords,bow_stern_disps,sizeof(bow_stern_disps)/sizeof(bow_stern_disps[0]));

	D3DXVECTOR2 sea_yaw_vector = NvToDX(bow_stern_disps[0]) - NvToDX(bow_stern_disps[1]);
	FLOAT sea_yaw_angle = D3DXVec2Dot(&heading_perp,&sea_yaw_vector)/projected_length;
	FLOAT sea_yaw_rate = (sea_yaw_angle - m_pDynamicState->m_PrevSeaYaw)/time_delta;
	m_pDynamicState->m_PrevSeaYaw = sea_yaw_angle;

	if(m_pDynamicState->m_DynamicsCountdown) {
		// Snap to surface on first few updates
		m_pDynamicState->m_Height = m_InitialHeight;
		m_pDynamicState->m_Pitch = m_InitialPitch;
		m_pDynamicState->m_Roll = 0.f;
		--m_pDynamicState->m_DynamicsCountdown;
	} else {
		// Run pseudo-dynamics
		const int num_steps = int(ceilf(time_delta/kMaxSimulationTimeStep));
		const FLOAT time_step = time_delta/float(num_steps);
		for(int i = 0; i != num_steps; ++i)
		{
			// Height
			const FLOAT buoyancy_accel = (m_BuoyantArea * mean_displaced_head * kDensityOfWater * kAccelerationDueToGravity)/m_Mass;
			const FLOAT drag_accel = (m_HeightDrag * m_pDynamicState->m_HeightRate * immersed_ratio);
			const FLOAT height_accel = (buoyancy_accel - kAccelerationDueToGravity - drag_accel);
			m_pDynamicState->m_HeightRate += height_accel * time_step;
			m_pDynamicState->m_Height += m_pDynamicState->m_HeightRate * time_step;

			// Pitch
			const FLOAT pitch_COM_accel = m_LongitudinalCOM * cosf(m_pDynamicState->m_Pitch) * m_Mass * kAccelerationDueToGravity/m_PitchInertia;
			const FLOAT pitch_buoyancy_accel = (m_BuoyantArea * mean_pitch_moment * kDensityOfWater * kAccelerationDueToGravity)/m_PitchInertia;
			const FLOAT pitch_drag_accel = (m_PitchDrag * m_pDynamicState->m_PitchRate * immersed_ratio);
			const FLOAT pitch_accel = (pitch_buoyancy_accel - pitch_drag_accel - pitch_COM_accel);
			m_pDynamicState->m_PitchRate += pitch_accel * time_step;
			m_pDynamicState->m_Pitch += m_pDynamicState->m_PitchRate * time_step;

			// Roll
			const FLOAT roll_buoyancy_accel = (m_BuoyantArea * mean_roll_moment * kDensityOfWater * kAccelerationDueToGravity)/m_RollInertia;
			const FLOAT roll_righting_accel = 2.f * sinf(m_pDynamicState->m_Roll) * m_MetacentricHeight * m_Mass * kAccelerationDueToGravity/m_RollInertia;
			const FLOAT roll_drag_accel = (m_RollDrag * m_pDynamicState->m_RollRate * immersed_ratio);
			const FLOAT roll_accel = (roll_buoyancy_accel - roll_drag_accel - roll_righting_accel);
			m_pDynamicState->m_RollRate += roll_accel * time_step;
			m_pDynamicState->m_Roll += m_pDynamicState->m_RollRate * time_step;

			// Yaw
			const FLOAT yaw_accel = immersed_ratio * (m_YawDrag * (sea_yaw_rate - m_pDynamicState->m_YawRate) + m_YawCoefficient * (sea_yaw_angle - m_pDynamicState->m_Yaw));
			m_pDynamicState->m_YawRate += yaw_accel * time_step;
			m_pDynamicState->m_Yaw += m_pDynamicState->m_YawRate * time_step;

			// Clamp pitch to {-pi/2,pi/2}
			if(m_pDynamicState->m_Pitch > 0.5f * D3DX_PI)
				m_pDynamicState->m_Pitch = 0.5f * D3DX_PI;
			if(m_pDynamicState->m_Pitch < -0.5f * D3DX_PI)
				m_pDynamicState->m_Pitch = -0.5f * D3DX_PI;
		}
	}

	D3DXMATRIX mat_roll;
	D3DXMatrixRotationZ(&mat_roll, m_pDynamicState->m_Roll);

	D3DXMATRIX mat_pitch;
	D3DXMatrixRotationX(&mat_pitch, -m_pDynamicState->m_Pitch);

	D3DXMATRIX mat_heading;
	const FLOAT heading = atan2f(m_pDynamicState->m_NominalHeading.x, m_pDynamicState->m_NominalHeading.y);
	D3DXMatrixRotationY(&mat_heading, heading + m_pDynamicState->m_Yaw);

	m_pDynamicState->m_LocalToWorld = mat_roll * mat_pitch * mat_heading;

	m_pDynamicState->m_LocalToWorld._41 = m_pDynamicState->m_Position.x + mean_displacement.x;
	m_pDynamicState->m_LocalToWorld._42 = sea_level + m_pDynamicState->m_Height;
	m_pDynamicState->m_LocalToWorld._43 = m_pDynamicState->m_Position.y + mean_displacement.y;

	// We want damped movement for camera 
	D3DXMATRIX mat_local_to_world_damped;
	D3DXMatrixRotationZ(&mat_roll, m_pDynamicState->m_Roll*0.5f);
	D3DXMatrixRotationX(&mat_pitch, -m_pDynamicState->m_Pitch*0.5f);
	D3DXMatrixRotationY(&mat_heading, heading + m_pDynamicState->m_Yaw*0.5f);
	mat_local_to_world_damped = mat_roll * mat_pitch * mat_heading;
	mat_local_to_world_damped._41 = m_pDynamicState->m_Position.x + mean_displacement.x;
	mat_local_to_world_damped._42 = sea_level + m_pDynamicState->m_Height;
	mat_local_to_world_damped._43 = m_pDynamicState->m_Position.y + mean_displacement.y;
	m_pDynamicState->m_CameraToWorld = m_CameraToLocal*mat_local_to_world_damped;

	// We do not want the wake to yaw around, hence
	D3DXMatrixRotationY(&m_pDynamicState->m_WakeToWorld, heading + 3.141592f*1.5f);
	m_pDynamicState->m_WakeToWorld._41 = m_pDynamicState->m_Position.x + mean_displacement.x;
	m_pDynamicState->m_WakeToWorld._42 = sea_level + m_pDynamicState->m_Height;
	m_pDynamicState->m_WakeToWorld._43 = m_pDynamicState->m_Position.y + mean_displacement.y;

	// Ensure sensors are bang-up-to-date
	m_pHullSensors->update(m_pSurfaceHeights,m_pDynamicState->m_LocalToWorld);
}

void OceanVesselDynamicState::ResetDynamicState()
{
	m_Pitch = 0.f;
	m_PitchRate = 0.f;
	m_Roll = 0.f;
	m_RollRate = 0.f;
	m_Yaw = 0.f;
	m_YawRate = 0.f;
	m_PrevSeaYaw = 0.f;
	m_Height = 0.f;
	m_HeightRate = 0.f;
	m_DynamicsCountdown = 3;
	m_bFirstUpdate = true;
}

void OceanVessel::renderVesselToHullProfile(ID3D11DeviceContext* pDC, OceanHullProfile& profile)
{
	OceanHullProfile result(m_pHullProfileSRV[1]);

	// Calculate transforms etc. - we render from below, world-aligned

	// Transform calc - 1/ transform the bounds based on current mesh->world
	D3DXVECTOR3 xEdge(2.f*m_bbExtents.x,0.f,0.f);
	D3DXVECTOR3 yEdge(0.f,2.f*m_bbExtents.y,0.f);
	D3DXVECTOR3 zEdge(0.f,0.f,2.f*m_bbExtents.z);
	D3DXVECTOR3 bbCorners[8];
	bbCorners[0] = m_bbCentre - m_bbExtents;
	bbCorners[1] = bbCorners[0] + xEdge;
	bbCorners[2] = bbCorners[0] + yEdge;
	bbCorners[3] = bbCorners[1] + yEdge;
	bbCorners[4] = bbCorners[0] + zEdge;
	bbCorners[5] = bbCorners[1] + zEdge;
	bbCorners[6] = bbCorners[2] + zEdge;
	bbCorners[7] = bbCorners[3] + zEdge;

	D3DXMATRIX matW = m_MeshToLocal * m_pDynamicState->m_LocalToWorld;
	D3DXVec3TransformCoordArray(bbCorners, sizeof(bbCorners[0]), bbCorners, sizeof(bbCorners[0]), &matW, sizeof(bbCorners)/sizeof(bbCorners[0]));

	// Transform calc - 2/ calculate the world bounds
	D3DXVECTOR3 minCorner = bbCorners[0];
	D3DXVECTOR3 maxCorner = minCorner;
	for(int i = 1; i != sizeof(bbCorners)/sizeof(bbCorners[0]); ++i) {
		minCorner = D3DXVec3Min(minCorner,bbCorners[i]);
		maxCorner = D3DXVec3Max(maxCorner,bbCorners[i]);
	}

	// Transform calc - 3/inflate the world bounds so that the x and y footprints are equal
	FLOAT w = maxCorner.x - minCorner.x;
	FLOAT l = maxCorner.z - minCorner.z;
	if(w > l) {
		minCorner.z -= 0.5f*(w-l);
		maxCorner.z += 0.5f*(w-l);
		l = w;
	} else {
		minCorner.x -= 0.5f*(l-w);
		maxCorner.x += 0.5f*(l-w);
		w = l;
	}

	// Transform calc - 4/ calculate hull profile transforms
	result.m_ProfileToWorldHeightScale = maxCorner.y - minCorner.y;
	result.m_ProfileToWorldHeightOffset = minCorner.y;
	result.m_WorldToProfileCoordsScale = D3DXVECTOR2(1.f/(maxCorner.x-minCorner.x),1.f/(maxCorner.z-minCorner.z));
	result.m_WorldToProfileCoordsOffset = D3DXVECTOR2(result.m_WorldToProfileCoordsScale.x * -minCorner.x, result.m_WorldToProfileCoordsScale.y * -minCorner.z);
	result.m_TexelSizeInWorldSpace = w/float(m_HullProfileTextureWH);

	// Transform calc - 5/ set up view-proj for rendering into the hull profile
	D3DXMATRIX matVP;
	memset(matVP,0,sizeof(matVP));
	matVP._11 = 2.f/(maxCorner.x-minCorner.x);	// x out from x
	matVP._32 = 2.f/(minCorner.z-maxCorner.z);	// y out from z
	matVP._23 = 1.f/(maxCorner.y-minCorner.y);	// z out from y
	matVP._44 = 1.f;
	matVP._41 =  1.f - matVP._11 * maxCorner.x;
	matVP._42 = -1.f - matVP._32 * maxCorner.z;
	matVP._43 =  1.f - matVP._23 * maxCorner.y;

	// Set up matrices for rendering
	D3DXMATRIX matWVP = matW* matVP;
	m_pMatWorldViewProjVariable->SetMatrix((FLOAT*)&matWVP);
	m_pMatWorldVariable->SetMatrix((FLOAT*)&matW);

	// Save rt setup to restore shortly...
	D3D11_VIEWPORT original_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	UINT num_original_viewports = sizeof(original_viewports)/sizeof(original_viewports[0]);
	pDC->RSGetViewports( &num_original_viewports, original_viewports);
	ID3D11RenderTargetView* original_rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	ID3D11DepthStencilView* original_dsv = NULL;
	UINT num_original_rtvs = sizeof(original_rtvs)/sizeof(original_rtvs[0]);
    pDC->OMGetRenderTargets( num_original_rtvs, original_rtvs, &original_dsv );

	// Do the rendering
	pDC->ClearDepthStencilView(m_pHullProfileDSV, D3D11_CLEAR_DEPTH, 1.f, 0);
	const FLOAT rtvClearColor[4] = { 1.f, 0.f, 0.f, 0.f };
	pDC->ClearRenderTargetView(m_pHullProfileRTV[0], rtvClearColor);
	pDC->OMSetRenderTargets( 1, &m_pHullProfileRTV[0], m_pHullProfileDSV);

	D3D11_VIEWPORT vp;
	vp.TopLeftX = vp.TopLeftY = 0.f;
	vp.Height = vp.Width = FLOAT(m_HullProfileTextureWH);
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.f;
	pDC->RSSetViewports(1, &vp);

	renderVessel(pDC, m_pRenderToHullProfileTechnique, NULL, false, false);

	// Fix up cracks
	pDC->OMSetRenderTargets( 1, &m_pHullProfileRTV[1], NULL);
	m_pTexDiffuseVariable->SetResource(m_pHullProfileSRV[0]);
	m_pRenderQuadToCrackFixTechnique->GetPassByIndex(0)->Apply(0, pDC);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pDC->IASetInputLayout(NULL);
	pDC->Draw(4,0);
	m_pTexDiffuseVariable->SetResource(NULL);
	m_pRenderQuadToCrackFixTechnique->GetPassByIndex(0)->Apply(0, pDC);

	// Restore original state
	pDC->OMSetRenderTargets(num_original_rtvs, original_rtvs, original_dsv);
	pDC->RSSetViewports(num_original_viewports, original_viewports);
	SAFE_RELEASE(original_dsv);
	for(UINT i = 0; i != num_original_rtvs; ++i) {
		SAFE_RELEASE(original_rtvs[i]);
	}

	// Generate mips
	pDC->GenerateMips(m_pHullProfileSRV[1]);

	// Set result
	profile = result;
}

void OceanVessel::renderTextureToUI(ID3D11DeviceContext* pDC, ID3D11ShaderResourceView* pSRV)
{
	m_pTexDiffuseVariable->SetResource(pSRV);
	m_pRenderQuadToUITechnique->GetPassByIndex(0)->Apply(0, pDC);

	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pDC->IASetInputLayout(NULL);
	pDC->Draw(4,0);

	m_pTexDiffuseVariable->SetResource(NULL);
	m_pRenderQuadToUITechnique->GetPassByIndex(0)->Apply(0, pDC);
}

void OceanVessel::renderHullProfileInUI(ID3D11DeviceContext* pDC)
{
	renderTextureToUI(pDC, m_pHullProfileSRV[1]);
}

void OceanVessel::updateVesselLightsInEnv(OceanEnvironment& env, const D3DXMATRIX& matView, float lighting_mult, int objectID)
{
	const int num_lights_to_update = min(int(m_Spotlights.size()), MaxNumSpotlights - env.activeLightsNum);
	for(int i = 0; i != num_lights_to_update; ++i) {
		const Spotlight& sl = m_Spotlights[i];
		env.spotlight_position[env.activeLightsNum] = D3DXVECTOR4(sl.position,1.f);

		const float cosHalfBeamAngle = cosf(0.5f * sl.beam_angle);
		env.spotlight_axis_and_cos_angle[env.activeLightsNum] = D3DXVECTOR4(sl.axis,cosHalfBeamAngle);

		env.spotlight_color[env.activeLightsNum] = D3DXVECTOR4(D3DXVECTOR3(sl.color) * lighting_mult,1.f);
#if ENABLE_SHADOWS
		env.spotlight_shadow_matrix[env.activeLightsNum] = m_SpotlightsShadows[i].m_ViewProjMatrix;
		env.spotlight_shadow_resource[env.activeLightsNum] = m_SpotlightsShadows[i].m_pSRV;
#endif

		env.objectID[env.activeLightsNum] = objectID;
		++env.activeLightsNum;
	}

	env.spotlights_to_world_matrix = m_pDynamicState->m_LocalToWorld;
}

namespace {

	bool IsKwd(const char* str, const char* kwd) {
		const int kwd_len = strlen(kwd);
		return 0 == strncmp(kwd,str,kwd_len);
	}

	const char* GetValString(const char* str, const char* kwd) {
		const int kwd_len = strlen(kwd);

		if(strncmp(kwd,str,kwd_len))
			return NULL;

		const char* curr = str + kwd_len;

		// Next char *must* be space
		if(!isspace(*curr))
			return NULL;

		// Skip intervening wspc
		while(*curr && isspace(*curr))
			++curr;

		if(!*curr)
			return NULL;

		return curr;
	}

	bool ReadFloat(const char* str, const char* kwd, float& value) {

		const char* val_string = GetValString(str,kwd);
		if(NULL == val_string)
			return false;

		value = (float)atof(val_string);

		return true;
	}

	bool ReadInt(const char* str, const char* kwd, int& value) {

		const char* val_string = GetValString(str,kwd);
		if(NULL == val_string)
			return false;

		value = atoi(val_string);

		return true;
	}

	bool ReadString(const char* str, const char* kwd, LPTSTR& value) {

		const char* val_string = GetValString(str,kwd);
		if(NULL == val_string)
			return false;

		int l = MultiByteToWideChar(CP_ACP, 0, val_string, -1, NULL, 0);
		delete [] value;
		value = new TCHAR[l];
		MultiByteToWideChar(CP_ACP, 0, val_string, -1, value, l);
		
		return true;
	}
}

OceanVessel::Spotlight* OceanVessel::processSpotlightConfigLine(const char* line, Spotlight* pSpot)
{
	const char* curr = line;

	// Skip leading wspc
	while(*curr && isspace(*curr))
		++curr;

	// Skip empty lines
	if(!*curr)
		return pSpot;

	if(ReadFloat(curr,"x",pSpot->position.x))
		return pSpot;
	if(ReadFloat(curr,"y",pSpot->position.y))
		return pSpot;
	if(ReadFloat(curr,"z",pSpot->position.z))
		return pSpot;

	if(ReadFloat(curr,"axis-x",pSpot->axis.x))
		return pSpot;
	if(ReadFloat(curr,"axis-y",pSpot->axis.y))
		return pSpot;
	if(ReadFloat(curr,"axis-z",pSpot->axis.z))
		return pSpot;

	if(ReadFloat(curr,"r",pSpot->color.x))
		return pSpot;
	if(ReadFloat(curr,"g",pSpot->color.y))
		return pSpot;
	if(ReadFloat(curr,"b",pSpot->color.z))
		return pSpot;

	if(ReadFloat(curr,"beam_angle",pSpot->beam_angle)) {
		pSpot->beam_angle *= D3DX_PI/180.f;	// Convert deg to rad
		return pSpot;
	}

	if(IsKwd(curr,"EndSpotlight")) {
		D3DXVec3Normalize((D3DXVECTOR3*)&pSpot->axis,(D3DXVECTOR3*)&pSpot->axis);
		return NULL;
	}

	return pSpot;
}

OceanVessel::Spotlight* OceanVessel::processGlobalConfigLine(const char* line)
{
	const char* curr = line;

	// Skip leading wspc
	while(*curr && isspace(*curr))
		++curr;

	// Skip empty lines
	if(!*curr)
		return NULL;

	if(ReadFloat(curr,"Length",m_Length))
		return NULL;

	if(ReadFloat(curr,"CameraHeightAboveWater",m_CameraHeightAboveWater))
		return NULL;

	if(ReadFloat(curr,"CameraLongitudinalOffset",m_CameraLongitudinalOffset))
		return NULL;

	if(ReadFloat(curr,"HeightDrag",m_HeightDrag))
		return NULL;

	if(ReadFloat(curr,"PitchDrag",m_PitchDrag))
		return NULL;

	if(ReadFloat(curr,"YawDrag",m_YawDrag))
		return NULL;

	if(ReadFloat(curr,"YawCoefficient",m_YawCoefficient))  
		return NULL;

	if(ReadFloat(curr,"RollDrag",m_RollDrag))
		return NULL;

	if(ReadFloat(curr,"MetacentricHeight",m_MetacentricHeight))
		return NULL;

	if(ReadFloat(curr,"MassMult",m_MassMult))
		return NULL;

	if(ReadFloat(curr,"PitchInertiaMult",m_PitchInertiaMult))
		return NULL;

	if(ReadFloat(curr,"RollInertiaMult",m_RollInertiaMult))
		return NULL;

	if(ReadFloat(curr,"InitialPitch",m_InitialPitch))
		return NULL;

	if(ReadFloat(curr,"InitialHeight",m_InitialHeight))
		return NULL;

	if(ReadFloat(curr,"LongitudinalCOM",m_LongitudinalCOM))
		return NULL;

	if(ReadFloat(curr,"DiffuseGamma",m_DiffuseGamma))
		return NULL;

	if(ReadFloat(curr,"SmokeEmitRate",m_SmokeParticleEmitRate))
		return NULL;

	if(ReadFloat(curr,"SmokeEmitMinVelocity",m_SmokeParticleEmitMinVelocity))
		return NULL;

	if(ReadFloat(curr,"SmokeEmitMaxVelocity",m_SmokeParticleEmitMaxVelocity))
		return NULL;

	if(ReadFloat(curr,"SmokeMinBuoyancy",m_SmokeParticleMinBuoyancy))
		return NULL;

	if(ReadFloat(curr,"SmokeMaxBuoyancy",m_SmokeParticleMaxBuoyancy))
		return NULL;

	if(ReadFloat(curr,"SmokeCoolingRate",m_SmokeParticleCoolingRate))
		return NULL;

	if(ReadFloat(curr,"SmokeEmitSpread",m_SmokeParticleEmitSpread))
		return NULL;

	if(ReadFloat(curr,"SmokeParticleBeginSize",m_SmokeParticleBeginSize))
		return NULL;

	if(ReadFloat(curr,"SmokeParticleEndSize",m_SmokeParticleEndSize))
		return NULL;

	if(ReadFloat(curr,"SmokeWindDrag",m_SmokeWindDrag))
		return NULL;

	if(ReadFloat(curr,"PSMMinCornerX",m_PSMMinCorner.x))
		return NULL;

	if(ReadFloat(curr,"PSMMinCornerY",m_PSMMinCorner.y))
		return NULL;

	if(ReadFloat(curr,"PSMMinCornerZ",m_PSMMinCorner.z))
		return NULL;

	if(ReadFloat(curr,"PSMMaxCornerX",m_PSMMaxCorner.x))
		return NULL;

	if(ReadFloat(curr,"PSMMaxCornerY",m_PSMMaxCorner.y))
		return NULL;

	if(ReadFloat(curr,"PSMMaxCornerZ",m_PSMMaxCorner.z))
		return NULL;

	if(ReadFloat(curr,"SmokePSMBoundsFadeMargin",m_SmokePSMBoundsFadeMargin))
		return NULL;

	if(ReadFloat(curr,"SmokeWindNoiseLevel",m_SmokeWindNoiseLevel))
		return NULL;

	if(ReadFloat(curr,"SmokeWindNoiseSpatialScale",m_SmokeWindNoiseSpatialScale))
		return NULL;

	if(ReadFloat(curr,"SmokeWindNoiseTimeScale",m_SmokeWindNoiseTimeScale))
		return NULL;

	if(ReadFloat(curr,"SmokeShadowOpacity",m_SmokeShadowOpacity))
		return NULL;

	if(ReadFloat(curr,"SmokeTintR",m_SmokeTint.x))
		return NULL;

	if(ReadFloat(curr,"SmokeTintG",m_SmokeTint.y))
		return NULL;

	if(ReadFloat(curr,"SmokeTintB",m_SmokeTint.z))
		return NULL;

	if(ReadInt(curr,"PSMRes",m_PSMRes))
		return NULL;

	if(ReadFloat(curr,"FunnelLongitudinalOffset",m_FunnelLongitudinalOffset))
		return NULL;

	if(ReadFloat(curr,"FunnelHeightAboveWater",m_FunnelHeightAboveWater))
		return NULL;

	if(ReadFloat(curr,"FunnelXSize",m_FunnelMouthSize.x))
		return NULL;

	if(ReadFloat(curr,"FunnelYSize",m_FunnelMouthSize.y))
		return NULL;

	if(ReadInt(curr,"NumSmokeParticles",m_NumSmokeParticles))
		return NULL;

	if(ReadInt(curr,"NumSurfaceHeightSamples",m_NumSurfaceHeightSamples))
		return NULL;

	if(ReadInt(curr,"HullProfileTextureWH",m_HullProfileTextureWH))
		return NULL;

	if(ReadString(curr,"Mesh",m_MeshFileName))
		return NULL;

	if(ReadString(curr,"SmokeTexture",m_SmokeTextureFileName))
		return NULL;

	if(IsKwd(curr,"BeginSpotlight")) {
		Spotlight new_spotlight;
		memset(&new_spotlight,0,sizeof(new_spotlight));
		m_Spotlights.push_back(new_spotlight);
		return &m_Spotlights.back();
	}

	return NULL;
}

HRESULT OceanVessel::parseConfig(LPCTSTR cfg_string)
{
	int buffer_size = 256;
	char* buffer = new char[buffer_size];
	Spotlight* pCurrSpotlight = NULL;
	const LPCTSTR cfg_end = cfg_string + _tcslen(cfg_string);
	for(LPCTSTR cfg_cur = cfg_string; cfg_cur != cfg_end;) {
		bool eol = false;
		int line_length = 0;
		while(!eol) {
			int ch = *cfg_cur;
			++cfg_cur;
			if(cfg_cur == cfg_end) {
				eol = true;
			} else {
				if(line_length == buffer_size) {
					// Need to realloc
					buffer_size *= 2;
					char* new_buff = new char[buffer_size];
					memcpy(new_buff, buffer, line_length * sizeof(buffer[0]));
					delete [] buffer;
					buffer = new_buff;
				}
				
				if('\n' == ch) {
					eol = true;
					ch = '\0';
				}

				buffer[line_length] = (char)ch;
				++line_length;
			}
		}

		if(NULL == pCurrSpotlight) {
			pCurrSpotlight = processGlobalConfigLine(buffer);
		} else {
			pCurrSpotlight = processSpotlightConfigLine(buffer, pCurrSpotlight);
		}
	}

	delete [] buffer;

	// Mesh name is mandatory
	if(NULL == m_MeshFileName)
		return E_FAIL;

	return S_OK;
}

void OceanVessel::updateSmokeSimulation(ID3D11DeviceContext* pDC, const CBaseCamera& camera, FLOAT time_delta, const D3DXVECTOR2& wind_dir, FLOAT wind_speed, FLOAT emit_rate_scale)
{
	if(m_pSmoke)
	{
		D3DXVECTOR3 vWindVector;
		vWindVector.x = wind_dir.x * wind_speed;
		vWindVector.y = 0.f;
		vWindVector.z = wind_dir.y * wind_speed;
		D3DXMATRIX matEmitter = m_FunnelMouthToLocal * m_pDynamicState->m_LocalToWorld;
		m_pSmoke->updateSimulation(pDC, camera, time_delta, matEmitter, vWindVector, m_SmokeWindNoiseLevel * wind_speed, emit_rate_scale);
	}
}

void OceanVessel::renderSmokeToScene(	ID3D11DeviceContext* pDC,
										const CBaseCamera& camera,
										const OceanEnvironment& ocean_env
										)
{
	if(m_pSmoke)
	{
		m_pSmoke->renderToScene(pDC, camera, m_pPSM, ocean_env);
	}
}

void OceanVessel::renderSmokeToPSM(	ID3D11DeviceContext* pDC,
									const OceanEnvironment& ocean_env
									)
{
	if(m_pSmoke)
	{
		m_pSmoke->renderToPSM(pDC, m_pPSM, ocean_env);
	}
}

BoatMesh* OceanVessel::getMesh() const
{
	return m_pMesh;
}

void OceanVessel::beginRenderToPSM(ID3D11DeviceContext* pDC, const D3DXVECTOR2& wind_dir)
{
	const D3DXMATRIX matEmitter = m_FunnelMouthToLocal * m_pDynamicState->m_LocalToWorld;

	// Get the emitter position from the emitter matrix and set up a wind-aligned local
	// space with the emitter at the origin
	D3DXVECTOR3 emitter_pos = D3DXVECTOR3(0,0,0);
	D3DXVec3TransformCoord(&emitter_pos, &emitter_pos, &matEmitter);

	// Local y is wind-aligned in the plane
	D3DXVECTOR3 local_y = -D3DXVECTOR3(wind_dir.x,0,wind_dir.y);
	local_y.y = 0.f;
	D3DXVec3Normalize(&local_y, &local_y);

	// Local z is world down (effective light dir - to put shadows on bottom of smoke plume)
	D3DXVECTOR3 local_z = D3DXVECTOR3(0.f,-1.f,0.f);

	// Local x is implied by y and z
	D3DXVECTOR3 local_x;
	D3DXVec3Cross(&local_x,&local_y,&local_z);

	D3DXMATRIX matPSM;
	D3DXMatrixTranslation(&matPSM,emitter_pos.x,emitter_pos.y,emitter_pos.z);

	matPSM._11 = local_x.x;
	matPSM._12 = local_x.y;
	matPSM._13 = local_x.z;

	matPSM._21 = local_y.x;
	matPSM._22 = local_y.y;
	matPSM._23 = local_y.z;

	matPSM._31 = local_z.x;
	matPSM._32 = local_z.y;
	matPSM._33 = local_z.z;


	m_pPSM->beginRenderToPSM(matPSM,pDC);
}

void OceanVessel::endRenderToPSM(ID3D11DeviceContext* pDC)
{
	m_pPSM->endRenderToPSM(pDC);
}
