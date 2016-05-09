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
#include "DXUTcamera.h"
#include "ocean_env.h"
#include "ocean_spray.h"
#include "ocean_spray_common.h"
#include "ocean_psm.h"
#include "ocean_surface_heights.h"

#include "GFSDK_WaveWorks.h"
#include "GFSDK_WaveWorks_D3D_Util.h"

#include "SDKmesh.h"

#pragma warning(disable:4127)

extern HRESULT LoadFile(LPCTSTR FileName, ID3DXBuffer** ppBuffer);

namespace {

	const float kParticleKillDepth = 3.f;
	const float kParticleInitialScale = 0.75f;
	const float kParticleJitter = 0.5f;
	const float kBowGenThreshold = 0.5f;
}

namespace {

	inline int PoissonOutcomeKnuth(float lambda) {

		// Adapted to log space to avoid exp() overflow for large lambda
		int k = 0;
		float p = 0.f;
		do {
			k = k+1;
			p = p - logf((float)rand()/(float)(RAND_MAX+1));
		}
		while (p < lambda);

		return k-1;
	}

	inline int PoissonOutcome(float lambda) {
		return PoissonOutcomeKnuth(lambda);
	}
}

namespace {
	struct VisualizationVertex {
		D3DXVECTOR4 position_and_intensity;
		D3DXVECTOR2 uv;
	};
}

OceanSpray::OceanSpray()
{
	m_pd3dDevice = DXUTGetD3D11Device();

	m_pSplashTextureSRV = NULL;
	m_pRenderInstanceDataBuffer = NULL;
	m_pRenderInstanceDataSRV = NULL;
	m_pRenderVelocityDataBuffer = NULL;
	m_pRenderVelocityDataSRV = NULL;
	m_pRenderOrientationsAndDecimationsBuffer = NULL;
	m_pRenderOrientationsAndDecimationsSRV = NULL;

	m_pDrawParticlesCB = NULL;
	m_pDrawParticlesBuffer = NULL;
	m_pDrawParticlesBufferUAV = NULL;
	m_pDrawParticlesBufferStaging = NULL;

	m_pFX = NULL;
	m_pRenderSprayToSceneTechTechnique = NULL;
	m_pRenderSprayToPSMTechnique = NULL;
	m_pRenderSprayToFoamTechnique = NULL;
	m_pInitSortTechnique = NULL;
	m_pBitonicSortTechnique = NULL;
	m_pMatrixTransposeTechnique = NULL;
	m_pSensorVisualizationTechnique = NULL;
	m_pAudioVisualizationTechnique = NULL;

	m_pTexSplashVariable = NULL;
	m_pTexDepthVariable = NULL;
	m_pRenderInstanceDataVariable = NULL;
	m_pRenderOrientationAndDecimationDataVariable = NULL;
	m_pMatProjVariable = NULL;
	m_pMatProjInvVariable = NULL;
	m_pMatViewVariable = NULL;
	m_pLightDirectionVariable = NULL;
	m_pLightColorVariable = NULL;
	m_pAmbientColorVariable = NULL;
	m_pFogExponentVariable = NULL;
	m_pInvParticleLifeTimeVariable = NULL;
	m_pSimpleParticlesVariable = NULL;

	m_pLightningPositionVariable = NULL;
	m_pLightningColorVariable = NULL;

	m_pSpotlightNumVariable = NULL;
	m_pRenderSurfaceSpotlightPositionVariable = NULL;
	m_pRenderSurfaceSpotLightAxisAndCosAngleVariable = NULL;
	m_pRenderSurfaceSpotlightColorVariable = NULL;
	m_pSpotlightShadowMatrixVar = NULL;
	m_pSpotlightShadowResourceVar = NULL;

	m_pPSMParams = NULL;
	m_pMatViewToPSMVariable = NULL;

	m_piDepthSortLevelVariable = NULL;
	m_piDepthSortLevelMaskVariable = NULL;
	m_piDepthSortWidthVariable = NULL;
	m_piDepthSortHeightVariable = NULL;
	m_pParticleDepthSortUAVVariable = NULL;
	m_pParticleDepthSortSRVVariable = NULL;

	m_pWorldToVesselVariable = NULL;
	m_pVesselToWorldVariable = NULL;

	m_worldToHeightLookupScaleVariable = NULL;
	m_worldToHeightLookupRotVariable = NULL;
	m_worldToHeightLookupOffsetVariable = NULL;
	m_texHeightLookupVariable = NULL;

	m_pMatWorldToFoamVariable = NULL;

	m_pAudioVisualizationRectVariable = NULL;
	m_pAudioVisualizationMarginVariable = NULL;
	m_pAudioVisualizationLevelVariable = NULL;

	m_pDepthSort1SRV = NULL;
	m_pDepthSort1UAV = NULL;
	m_pDepthSort2SRV = NULL;
	m_pDepthSort2UAV = NULL;

	m_numParticlesAlive = 0;

	m_pHullSensors = NULL;

	m_pSensorsVisualizationVertexBuffer = NULL;
	m_pSensorsVisualizationIndexBuffer = NULL;
	m_pSensorVisualizationLayout = NULL;

	m_firstUpdate = true;

	for (UINT i=0; i<2; ++i)
	{
		m_pParticlesBuffer[i] = NULL;
		m_pParticlesBufferUAV[i] = NULL;
		m_pParticlesBufferSRV[i] = NULL;
	}

	memset(m_sprayVizualisationFade, 0, sizeof(m_sprayVizualisationFade));
	memset(m_rimSprayVizualisationFade, 0, sizeof(m_rimSprayVizualisationFade));

	m_ParticleWriteBuffer = 0;

	m_SplashPowerFromLastUpdate = 0.f;
}

OceanSpray::~OceanSpray()
{
	SAFE_RELEASE(m_pSplashTextureSRV);
	SAFE_RELEASE(m_pRenderInstanceDataBuffer);
	SAFE_RELEASE(m_pRenderInstanceDataSRV);
	SAFE_RELEASE(m_pRenderVelocityDataBuffer);
	SAFE_RELEASE(m_pRenderVelocityDataSRV);
	SAFE_RELEASE(m_pRenderOrientationsAndDecimationsBuffer);
	SAFE_RELEASE(m_pRenderOrientationsAndDecimationsSRV);

	SAFE_RELEASE(m_pFX);
	SAFE_DELETE(m_pPSMParams);

	for (UINT i=0; i<2; ++i)
	{
		SAFE_RELEASE(m_pParticlesBuffer[i]);
		SAFE_RELEASE(m_pParticlesBufferUAV[i]);
		SAFE_RELEASE(m_pParticlesBufferSRV[i]);
	}

	SAFE_RELEASE(m_pDrawParticlesCB);
	SAFE_RELEASE(m_pDrawParticlesBuffer);
	SAFE_RELEASE(m_pDrawParticlesBufferUAV);
	SAFE_RELEASE(m_pDrawParticlesBufferStaging);

	SAFE_RELEASE(m_pDepthSort1SRV);
	SAFE_RELEASE(m_pDepthSort1UAV);
	SAFE_RELEASE(m_pDepthSort2SRV);
	SAFE_RELEASE(m_pDepthSort2UAV);

	SAFE_RELEASE(m_pSensorsVisualizationVertexBuffer);
	SAFE_RELEASE(m_pSensorsVisualizationIndexBuffer);
	SAFE_RELEASE(m_pSensorVisualizationLayout);
}

HRESULT OceanSpray::init(OceanHullSensors* pHullSensors)
{
	HRESULT hr = S_OK;

    ID3DXBuffer* pEffectBuffer = NULL;
    V_RETURN(LoadFile(TEXT(".\\Media\\ocean_spray_d3d11.fxo"), &pEffectBuffer));
    V_RETURN(D3DX11CreateEffectFromMemory(pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, m_pd3dDevice, &m_pFX));
    pEffectBuffer->Release();

	m_pRenderSprayToSceneTechTechnique = m_pFX->GetTechniqueByName("RenderSprayToSceneTech");
	m_pRenderSprayToPSMTechnique = m_pFX->GetTechniqueByName("RenderSprayToPSMTech");
	m_pRenderSprayToFoamTechnique = m_pFX->GetTechniqueByName("RenderSprayToFoamTech");
	m_pInitSortTechnique = m_pFX->GetTechniqueByName("InitSortTech");
	m_pBitonicSortTechnique = m_pFX->GetTechniqueByName("BitonicSortTech");
	m_pMatrixTransposeTechnique = m_pFX->GetTechniqueByName("MatrixTransposeTech");
	m_pAudioVisualizationTechnique = m_pFX->GetTechniqueByName("AudioVisualizationTech");

	m_pTexSplashVariable = m_pFX->GetVariableByName("g_texSplash")->AsShaderResource();
	m_pTexDepthVariable = m_pFX->GetVariableByName("g_texDepth")->AsShaderResource();
	m_pRenderInstanceDataVariable = m_pFX->GetVariableByName("g_RenderInstanceData")->AsShaderResource();
	m_pRenderVelocityAndTimeDataVariable = m_pFX->GetVariableByName("g_RenderVelocityAndTimeData")->AsShaderResource();
	m_pRenderOrientationAndDecimationDataVariable = m_pFX->GetVariableByName("g_RenderOrientationAndDecimationData")->AsShaderResource();
	m_pMatProjVariable = m_pFX->GetVariableByName("g_matProj")->AsMatrix();
	m_pMatProjInvVariable = m_pFX->GetVariableByName("g_matProjInv")->AsMatrix();
	m_pMatViewVariable = m_pFX->GetVariableByName("g_matView")->AsMatrix();
	m_pLightDirectionVariable = m_pFX->GetVariableByName("g_LightDirection")->AsVector();
	m_pLightColorVariable = m_pFX->GetVariableByName("g_LightColor")->AsVector();
	m_pAmbientColorVariable = m_pFX->GetVariableByName("g_AmbientColor")->AsVector();
	m_pFogExponentVariable = m_pFX->GetVariableByName("g_FogExponent")->AsScalar();
	m_pInvParticleLifeTimeVariable = m_pFX->GetVariableByName("g_InvParticleLifeTime")->AsScalar();
	m_pSimpleParticlesVariable = m_pFX->GetVariableByName("g_SimpleParticles")->AsScalar();

	m_pLightningPositionVariable = m_pFX->GetVariableByName("g_LightningPosition")->AsVector();
	m_pLightningColorVariable = m_pFX->GetVariableByName("g_LightningColor")->AsVector();

	m_pSpotlightNumVariable = m_pFX->GetVariableByName("g_LightsNum")->AsScalar();
	m_pRenderSurfaceSpotlightPositionVariable = m_pFX->GetVariableByName("g_SpotlightPosition")->AsVector();
	m_pRenderSurfaceSpotLightAxisAndCosAngleVariable = m_pFX->GetVariableByName("g_SpotLightAxisAndCosAngle")->AsVector();
	m_pRenderSurfaceSpotlightColorVariable = m_pFX->GetVariableByName("g_SpotlightColor")->AsVector();
	m_pSpotlightShadowMatrixVar = m_pFX->GetVariableByName("g_SpotlightMatrix")->AsMatrix();
	m_pSpotlightShadowResourceVar = m_pFX->GetVariableByName("g_SpotlightResource")->AsShaderResource();

	m_pMatViewToPSMVariable = m_pFX->GetVariableByName("g_matViewToPSM")->AsMatrix();
	m_pPSMParams = new OceanPSMParams(m_pFX);

	// Fire-and-forgets
	m_pFX->GetVariableByName("g_PSMOpacityMultiplier")->AsScalar()->SetFloat(kPSMOpacityMultiplier);

	m_piDepthSortLevelVariable = m_pFX->GetVariableByName("g_iDepthSortLevel")->AsScalar();
	m_piDepthSortLevelMaskVariable = m_pFX->GetVariableByName("g_iDepthSortLevelMask")->AsScalar();
	m_piDepthSortWidthVariable = m_pFX->GetVariableByName("g_iDepthSortWidth")->AsScalar();
	m_piDepthSortHeightVariable = m_pFX->GetVariableByName("g_iDepthSortHeight")->AsScalar();
	m_pParticleDepthSortUAVVariable = m_pFX->GetVariableByName("g_ParticleDepthSortUAV")->AsUnorderedAccessView();
	m_pParticleDepthSortSRVVariable = m_pFX->GetVariableByName("g_ParticleDepthSortSRV")->AsShaderResource();

	m_pAudioVisualizationRectVariable = m_pFX->GetVariableByName("g_AudioVisualizationRect")->AsVector();
	m_pAudioVisualizationMarginVariable = m_pFX->GetVariableByName("g_AudioVisualizationMargin")->AsVector();
	m_pAudioVisualizationLevelVariable = m_pFX->GetVariableByName("g_AudioVisualizationLevel")->AsScalar();

	if(NULL == m_pSplashTextureSRV)
	{
		ID3D11Resource* pD3D11Resource = NULL;
		V_RETURN(D3DX11CreateTextureFromFile(m_pd3dDevice, TEXT(".\\media\\splash.dds"), NULL, NULL, &pD3D11Resource, NULL));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &m_pSplashTextureSRV));
		SAFE_RELEASE(pD3D11Resource);
	}

	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = NUMSPRAYPARTICLES * sizeof(D3DXVECTOR4);
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC; /*D3D11_USAGE_DEFAULT*/
		buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE /*| D3D11_BIND_UNORDERED_ACCESS*/;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; /*0*/
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.FirstElement = 0;
		srv_desc.Buffer.NumElements = NUMSPRAYPARTICLES;

		/*
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
		uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
		uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uav_desc.Buffer.FirstElement = 0;
		uav_desc.Buffer.NumElements = NUMSPRAYPARTICLES * 4;
		uav_desc.Buffer.Flags = 0;
		*/

		V_RETURN(m_pd3dDevice->CreateBuffer(&buffer_desc, NULL, &m_pRenderInstanceDataBuffer));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pRenderInstanceDataBuffer, &srv_desc, &m_pRenderInstanceDataSRV));

		V_RETURN(m_pd3dDevice->CreateBuffer(&buffer_desc, NULL, &m_pRenderVelocityDataBuffer));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pRenderVelocityDataBuffer, &srv_desc, &m_pRenderVelocityDataSRV));
		// V_RETURN(m_pd3dDevice->CreateUnorderedAccessView(pBuffer, &uav_desc, &m_pSimulationInstanceDataUAV));
	}

	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = 4 * NUMSPRAYPARTICLES * sizeof(WORD);
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC; /*D3D11_USAGE_DEFAULT*/
		buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE /*| D3D11_BIND_UNORDERED_ACCESS*/;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; /*0*/
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		srv_desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.FirstElement = 0;
		srv_desc.Buffer.NumElements = NUMSPRAYPARTICLES;

		/*
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
		uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
		uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uav_desc.Buffer.FirstElement = 0;
		uav_desc.Buffer.NumElements = NUMSPRAYPARTICLES * 4;
		uav_desc.Buffer.Flags = 0;
		*/

		V_RETURN(m_pd3dDevice->CreateBuffer(&buffer_desc, NULL, &m_pRenderOrientationsAndDecimationsBuffer));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pRenderOrientationsAndDecimationsBuffer, &srv_desc, &m_pRenderOrientationsAndDecimationsSRV));
		// V_RETURN(m_pd3dDevice->CreateUnorderedAccessView(pBuffer, &uav_desc, &m_pSimulationInstanceDataUAV));
	}

	initializeGenerators(pHullSensors);

#if ENABLE_GPU_SIMULATION
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

	{
		CD3D11_BUFFER_DESC bufDesc(sizeof(int) * 4, 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ);
		V_RETURN(m_pd3dDevice->CreateBuffer(&bufDesc, NULL, &m_pDrawParticlesBufferStaging));
	}

	m_pInitSprayParticlesTechnique = m_pFX->GetTechniqueByName("InitSprayParticles");
	m_pSimulateSprayParticlesTechnique = m_pFX->GetTechniqueByName("SimulateSprayParticles");
	m_pDispatchArgumentsTechnique = m_pFX->GetTechniqueByName("PrepareDispatchArguments");

	m_pParticlesNum = m_pFX->GetVariableByName("g_ParticlesNum")->AsScalar();
	m_pSimulationTime = m_pFX->GetVariableByName("g_SimulationTime")->AsScalar();
	m_pWindSpeed = m_pFX->GetVariableByName("g_WindSpeed")->AsVector();
	m_pParticlesBufferVariable = m_pFX->GetVariableByName("g_SprayParticleDataSRV")->AsShaderResource();

	m_pWorldToVesselVariable = m_pFX->GetVariableByName("g_worldToVessel")->AsMatrix();
	m_pVesselToWorldVariable = m_pFX->GetVariableByName("g_vesselToWorld")->AsMatrix();

	m_worldToHeightLookupScaleVariable = m_pFX->GetVariableByName("g_worldToHeightLookupScale")->AsVector();
	m_worldToHeightLookupRotVariable = m_pFX->GetVariableByName("g_worldToHeightLookupRot")->AsVector();
	m_worldToHeightLookupOffsetVariable = m_pFX->GetVariableByName("g_worldToHeightLookupOffset")->AsVector();
	m_texHeightLookupVariable = m_pFX->GetVariableByName("g_texHeightLookup")->AsShaderResource();

	m_pMatWorldToFoamVariable = m_pFX->GetVariableByName("g_matWorldToFoam")->AsMatrix();
#endif

#if SPRAY_PARTICLE_SORTING
	// Create depth sort buffers
	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = 2 * SPRAY_PARTICLE_COUNT * sizeof(float);
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		buffer_desc.StructureByteStride = 2 * sizeof(float);

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		srv_desc.Format = DXGI_FORMAT_UNKNOWN;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.FirstElement = 0;
		srv_desc.Buffer.NumElements = SPRAY_PARTICLE_COUNT;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
		uav_desc.Format = DXGI_FORMAT_UNKNOWN;
		uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uav_desc.Buffer.FirstElement = 0;
		uav_desc.Buffer.NumElements = SPRAY_PARTICLE_COUNT;
		uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;

		ID3D11Buffer* pBuffer = NULL;
		V_RETURN(m_pd3dDevice->CreateBuffer(&buffer_desc, NULL, &pBuffer));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pBuffer, &srv_desc, &m_pDepthSort1SRV));
		V_RETURN(m_pd3dDevice->CreateUnorderedAccessView(pBuffer, &uav_desc, &m_pDepthSort1UAV));
		SAFE_RELEASE(pBuffer);

		V_RETURN(m_pd3dDevice->CreateBuffer(&buffer_desc, NULL, &pBuffer));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pBuffer, &srv_desc, &m_pDepthSort2SRV));
		V_RETURN(m_pd3dDevice->CreateUnorderedAccessView(pBuffer, &uav_desc, &m_pDepthSort2UAV));
		SAFE_RELEASE(pBuffer);
	}
#endif

	// sensor visualization D3D objects
	{
		const D3D11_INPUT_ELEMENT_DESC svis_layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "INTENSITY", 0, DXGI_FORMAT_R32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }};

		const UINT num_layout_elements = sizeof(svis_layout)/sizeof(svis_layout[0]);

		m_pSensorVisualizationTechnique = m_pFX->GetTechniqueByName("SensorVisualizationTech");

		D3DX11_PASS_DESC PassDesc;
		V_RETURN(m_pSensorVisualizationTechnique->GetPassByIndex(0)->GetDesc(&PassDesc));

		V_RETURN(m_pd3dDevice->CreateInputLayout(	svis_layout, num_layout_elements,
													PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize,
													&m_pSensorVisualizationLayout
													));
		
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = 4 * (NUMSPRAYGENERATORS + NUMRIMSPRAYGENERATORS) * sizeof(VisualizationVertex);
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC; 
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; 
		buffer_desc.MiscFlags = 0;
		V_RETURN(m_pd3dDevice->CreateBuffer(&buffer_desc, NULL, &m_pSensorsVisualizationVertexBuffer));
	}

	{
		const int max_num_generators = NUMSPRAYGENERATORS + NUMRIMSPRAYGENERATORS;
		DWORD indices[max_num_generators*6];
		int vertex_ix = 0;
		DWORD* pCurrIx = indices;
		for(int i = 0; i != max_num_generators; ++i)
		{
			pCurrIx[0] = vertex_ix + 0;
			pCurrIx[1] = vertex_ix + 2;
			pCurrIx[2] = vertex_ix + 1;
			pCurrIx[3] = vertex_ix + 0;
			pCurrIx[4] = vertex_ix + 3;
			pCurrIx[5] = vertex_ix + 2;
			pCurrIx += 6;
			vertex_ix += 4;
		}

		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = 6 * max_num_generators * sizeof(DWORD);
		buffer_desc.Usage = D3D11_USAGE_IMMUTABLE; 
		buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0; 
		buffer_desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA srd;
		srd.pSysMem = indices;
		srd.SysMemPitch = sizeof(indices);
		srd.SysMemSlicePitch = sizeof(indices);

		V_RETURN(m_pd3dDevice->CreateBuffer(&buffer_desc, &srd, &m_pSensorsVisualizationIndexBuffer));
	}

	return hr;
}

UINT OceanSpray::GetParticleBudget() const
{
#if ENABLE_GPU_SIMULATION
	return SPRAY_PARTICLE_COUNT;
#else
	return NUMSPRAYPARTICLES;
#endif
}

UINT OceanSpray::GetParticleCount(ID3D11DeviceContext* pDC)
{
	UINT particleCount = m_numParticlesAlive;

#if ENABLE_GPU_SIMULATION
	pDC->CopyResource(m_pDrawParticlesBufferStaging, m_pDrawParticlesBuffer);

	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	pDC->Map(m_pDrawParticlesBufferStaging, 0, D3D11_MAP_READ, 0, &mappedSubresource);
	particleCount = *((UINT*)mappedSubresource.pData);
	pDC->Unmap(m_pDrawParticlesBufferStaging, 0);
#endif

	return particleCount;
}

void OceanSpray::renderParticles(ID3D11DeviceContext* pDC, ID3DX11EffectTechnique* pTech, ID3D11ShaderResourceView* pDepthSRV, D3D11_PRIMITIVE_TOPOLOGY topology)
{
	// Particle
	m_pTexSplashVariable->SetResource(m_pSplashTextureSRV);
	m_pTexDepthVariable->SetResource(pDepthSRV);
	m_pInvParticleLifeTimeVariable->SetFloat(1.f/kParticleTTL);

	pDC->IASetInputLayout(NULL);
	pDC->IASetPrimitiveTopology(topology);

#if ENABLE_GPU_SIMULATION
	m_pParticleDepthSortSRVVariable->SetResource(m_pDepthSort1SRV);
	m_pParticlesBufferVariable->SetResource(m_pParticlesBufferSRV[m_ParticleWriteBuffer]);
	pTech->GetPassByIndex(0)->Apply(0, pDC);

	pDC->DrawInstancedIndirect(m_pDrawParticlesBuffer, 0);

	// Clear resource usage
	m_pParticlesBufferVariable->SetResource(NULL);
	m_pParticleDepthSortSRVVariable->SetResource(NULL);
#else
	m_pRenderInstanceDataVariable->SetResource(m_pRenderInstanceDataSRV);
	m_pRenderOrientationAndDecimationDataVariable->SetResource(m_pRenderOrientationsAndDecimationsSRV);
	pTech->GetPassByIndex(0)->Apply(0, pDC);

	pDC->Draw(m_numParticlesAlive,0);

	// Clear resource usage
	m_pRenderInstanceDataVariable->SetResource(NULL);
	m_pRenderOrientationAndDecimationDataVariable->SetResource(NULL);
#endif

	m_pTexDepthVariable->SetResource(NULL);
	pTech->GetPassByIndex(0)->Apply(0, pDC);
}

void OceanSpray::renderToScene(	ID3D11DeviceContext* pDC,
								OceanVessel* pOceanVessel,
								const CBaseCamera& camera,
								const OceanEnvironment& ocean_env,
								ID3D11ShaderResourceView* pDepthSRV,
								bool simple
								)
{
	// Matrices
	D3DXMATRIX matView = *camera.GetViewMatrix();
	D3DXMATRIX matProj = *camera.GetProjMatrix();

	D3DXMATRIX matInvView;
	D3DXMatrixInverse(&matInvView, NULL, &matView);

	D3DXMATRIX matInvProj;
	D3DXMatrixInverse(&matInvProj, NULL, &matProj);

	// View-proj
	m_pMatProjVariable->SetMatrix((FLOAT*)&matProj);
	m_pMatProjInvVariable->SetMatrix((FLOAT*)&matInvProj);
	m_pMatViewVariable->SetMatrix((FLOAT*)&matView);

#if SPRAY_PARTICLE_SORTING
	depthSortParticles(pDC);
#endif

	// Simple particles
	m_pSimpleParticlesVariable->SetFloat(simple?1.0f:0.0f);

	// Global lighting
	m_pLightDirectionVariable->SetFloatVector((FLOAT*)&ocean_env.main_light_direction);
	m_pLightColorVariable->SetFloatVector((FLOAT*)&ocean_env.main_light_color);
	m_pAmbientColorVariable->SetFloatVector((FLOAT*)&ocean_env.sky_color);

	// Lightnings
	m_pLightningColorVariable->SetFloatVector((FLOAT*)&ocean_env.lightning_light_intensity);
	m_pLightningPositionVariable->SetFloatVector((FLOAT*)&ocean_env.lightning_light_position);

	// Fog
	m_pFogExponentVariable->SetFloat(ocean_env.fog_exponent);

	D3DXMATRIX matWorldToVessel;
	D3DXMatrixInverse(&matWorldToVessel,NULL,pOceanVessel->getWorldXform());
	D3DXMATRIX matViewToVessel = matInvView * matWorldToVessel;
	m_pWorldToVesselVariable->SetMatrix((float*)&matViewToVessel);

	int lightsNum = 0;

	// Spot lights - transform to view space
	D3DXMATRIX matSpotlightsToView = ocean_env.spotlights_to_world_matrix * matView;
	D3DXMATRIX matViewToSpotlights;
	D3DXMatrixInverse(&matViewToSpotlights,NULL,&matSpotlightsToView);

	D3DXVECTOR4 spotlight_position[MaxNumSpotlights];
	D3DXVECTOR4 spotlight_axis_and_cos_angle[MaxNumSpotlights];
	D3DXVECTOR4 spotlight_color[MaxNumSpotlights];

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
	m_pRenderSurfaceSpotlightPositionVariable->SetFloatVectorArray((FLOAT*)spotlight_position,0,lightsNum);
	m_pRenderSurfaceSpotLightAxisAndCosAngleVariable->SetFloatVectorArray((FLOAT*)spotlight_axis_and_cos_angle,0,lightsNum);
	m_pRenderSurfaceSpotlightColorVariable->SetFloatVectorArray((FLOAT*)spotlight_color,0,lightsNum);

	// PSM
	D3DXVECTOR3 no_tint(1,1,1); // For now
	pOceanVessel->getPSM()->setReadParams(*m_pPSMParams,no_tint);

	D3DXMATRIX matViewToPSM = matInvView * *pOceanVessel->getPSM()->getWorldToPSMUV();
	m_pMatViewToPSMVariable->SetMatrix((FLOAT*)&matViewToPSM);

	renderParticles(pDC,m_pRenderSprayToSceneTechTechnique, pDepthSRV, D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);

	pOceanVessel->getPSM()->clearReadParams(*m_pPSMParams);
	m_pRenderSprayToSceneTechTechnique->GetPassByIndex(0)->Apply(0,pDC);
}

void OceanSpray::renderToPSM(	ID3D11DeviceContext* pDC,
								OceanPSM* pPSM,
								const OceanEnvironment& ocean_env
								)
{
	m_pSimpleParticlesVariable->SetFloat(0.0f);
	m_pMatViewVariable->SetMatrix((FLOAT*)pPSM->getPSMView());
	m_pMatProjVariable->SetMatrix((FLOAT*)pPSM->getPSMProj());
	pPSM->setWriteParams(*m_pPSMParams);

	renderParticles(pDC,m_pRenderSprayToPSMTechnique, NULL, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
}

namespace {

	const float normal_speed_min = 0.5f;
	const float normal_speed_max = 10.f;
	const float min_speed_mult = 2.0f;
	const float max_speed_mult = 2.5f;
	const float max_speed = 12.f;
	const float rim_up_speed_min = 4.f;
	const float rim_up_speed_max = 5.f;
	const float rim_out_speed_min = -5.f;
	const float rim_out_speed_max = 8.f;

	// This is the time density we would apply if we wanted to run all our generators at maximum
	// rate without ever running out of particle slots
	const float nominal_time_density = float(OceanSpray::NUMSPRAYPARTICLES)/(float(OceanSpray::NUMSPRAYGENERATORS+OceanSpray::NUMRIMSPRAYGENERATORS)*(kParticleTTL));

	// But in reality, not every generator is emitting all of the time, so we can run at a much higher
	// rate
	const float time_density = 7000.f * nominal_time_density;
	const float rim_time_density = 150.f * nominal_time_density;

	float CalcMeanSprayGen(float normal_speed, float water_line_proximity, float time_interval, float waterline_detection_radius)
	{
		const float water_line_max =  waterline_detection_radius;
		const float water_line_min = -waterline_detection_radius;
		const float water_line_mid = 0.5f * (water_line_min + water_line_max);
		const float normal_speed_sqr = normal_speed * fabs(normal_speed);

		float result = 0.f;
		if(water_line_proximity > water_line_min && water_line_proximity < water_line_max) {
			if(normal_speed_sqr > normal_speed_min) {
				// Triangular CDF for water line
				if(water_line_proximity < water_line_mid) {
					result = (water_line_proximity - water_line_min)/(water_line_mid - water_line_min);
				} else {
					result = (water_line_proximity - water_line_max)/(water_line_mid - water_line_max);
				}

				if(normal_speed_sqr < normal_speed_max) {
					float velocity_multiplier = sqrtf(normal_speed_sqr-normal_speed_min)/(normal_speed_max-normal_speed_min);
					result *= velocity_multiplier;
				}
			}
		}

		result *= time_density * time_interval;

		return result;
	}

	float CalcMeanRimSprayGen(float water_height, float time_interval)
	{
		return rim_time_density * time_interval * water_height;
	}
}

void OceanSpray::updateSprayGenerators(GFSDK_WaveWorks_SimulationHandle hOceanSimulation, OceanVessel* pOceanVessel, float deltaTime, float kWaterScale, gfsdk_float2 wind_speed, float spray_mult)
{
	D3DXVECTOR3 wind_speed_3d = D3DXVECTOR3(wind_speed.x,wind_speed.y,0.f);

	// Re-zero splash power
	m_SplashPowerFromLastUpdate = 0.f;

	// get sensor data
	const OceanHullSensors* pSensors = pOceanVessel->getHullSensors();
	const D3DXVECTOR3* pLocalSensorPositions = pSensors->getSensorPositions();
	const D3DXVECTOR3* pWorldSensorPositions = pSensors->getWorldSensorPositions();
	const D3DXVECTOR3* pWorldSensorNormals = pSensors->getWorldSensorNormals();
	const D3DXVECTOR4* pSensorDisplacements = pSensors->getDisplacements();
	const gfsdk_float2* pSensorReadbackCoords = pSensors->getReadbackCoords();

	const D3DXVECTOR3* pWorldRimSensorPositions = pSensors->getWorldRimSensorPositions();
	const D3DXVECTOR3* pWorldRimSensorNormals = pSensors->getWorldRimSensorNormals();
	const D3DXVECTOR4* pRimSensorDisplacements = pSensors->getRimDisplacements();

	// we load the dice in favour of bow events for hull spray
	const float sensors_min_z = pSensors->getBoundsMinCorner().z;
	const float sensors_max_z = pSensors->getBoundsMaxCorner().z;

	// local variables
	D3DXVECTOR3 displacementSpeedInitial;
	D3DXVECTOR3 displacementSpeedAtHull;
	D3DXVECTOR3 rv;

	// getting hull speed
	for(int i = 0; i < m_pHullSensors->getNumSensors(); i++)
	{
		m_hullSpeed[i].x = (pWorldSensorPositions[i].x - m_WorldSensorPositionsOld[i].x)/deltaTime;
		m_hullSpeed[i].y = (pWorldSensorPositions[i].y - m_WorldSensorPositionsOld[i].y)/deltaTime;
		m_hullSpeed[i].z = (pWorldSensorPositions[i].z - m_WorldSensorPositionsOld[i].z)/deltaTime;
	}

	// getting wave speed
	for(int i = 0; i < m_pHullSensors->getNumSensors(); i++)
	{
		m_displacementsSpeed[i].x = (pSensorDisplacements[i].x - m_displacementsOld[i].x)/deltaTime;
		m_displacementsSpeed[i].y = (pSensorDisplacements[i].y - m_displacementsOld[i].y)/deltaTime;
		m_displacementsSpeed[i].z = (pSensorDisplacements[i].z - m_displacementsOld[i].z)/deltaTime;
	}

	if(m_firstUpdate) {
		m_firstUpdate = false;
	} else {

		// Emit hull particles when -
		//	- the waterline is nearby
		//	- the relative velocity between hull and boat has a strong component toward the hull
		// We do this stochastically, and we are careful to use a poisson CDF in order to get the probabilities right
		// for a given time interval

		const float power_nrm = 1.f/(float(m_pHullSensors->getNumSensors())*deltaTime);
		for(int i = 0; i < m_pHullSensors->getNumSensors(); i++)
		{
			float length_param = (pLocalSensorPositions[i].z-sensors_min_z)/(sensors_max_z-sensors_min_z);
			if(length_param < kBowGenThreshold) {
				m_sprayAmount[i] = 0;
				continue;
			}

			float bow_mult = (length_param - kBowGenThreshold)/(1.f-kBowGenThreshold);

			float water_line_proximity = pSensorDisplacements[i].z - pWorldSensorPositions[i].z;
			D3DXVECTOR3 relative_velocity_of_wave = m_displacementsSpeed[i] - m_hullSpeed[i];
			D3DXVECTOR3 hull_normal = D3DXVECTOR3(pWorldSensorNormals[i].x,pWorldSensorNormals[i].y,pWorldSensorNormals[i].z);
			const float hull_perp_water_speed = -D3DXVec3Dot(&relative_velocity_of_wave, &hull_normal);

			float mean_spray_gen = spray_mult * CalcMeanSprayGen(hull_perp_water_speed,water_line_proximity,deltaTime,m_pHullSensors->getMeanSensorRadius());

			// Load the dice towards bow-spray
			mean_spray_gen *= bow_mult;

			m_sprayAmount[i] = PoissonOutcome(mean_spray_gen);

			if(m_sprayAmount[i]) {

				// We will need a water normal
				gfsdk_float2 water_gradient;
				pOceanVessel->getSurfaceHeights()->getGradients(&pSensorReadbackCoords[i], &water_gradient, 1);
				D3DXVECTOR3 water_normal = D3DXVECTOR3(water_gradient.x,water_gradient.y,1.f);
				D3DXVec3Normalize(&water_normal,&water_normal);

				// We use the contact direction to figure out how 'squirty' this spray event is
				D3DXVECTOR3 contact_line_dir;
				D3DXVec3Cross(&contact_line_dir, &water_normal, &hull_normal);
				D3DXVec3Normalize(&contact_line_dir,&contact_line_dir);

				// Calculate a hypothetical spray direction based on the water line
				D3DXVECTOR3 hypothetical_spray_dir;
				D3DXVec3Cross(&hypothetical_spray_dir, &hull_normal, &contact_line_dir);
				// No need to normalize, already perp
				
				D3DXVec3TransformNormal(&rv,&m_SprayGeneratorShipSpaceSprayDir[i], pOceanVessel->getWorldXform());
				m_SprayGeneratorWorldSpaceSprayDir[i].x = rv.x;
				m_SprayGeneratorWorldSpaceSprayDir[i].y = rv.z;
				m_SprayGeneratorWorldSpaceSprayDir[i].z = rv.y;

				// Squirtiness is how well actual and hypothetical line up
				const float squirtiness = max(0,D3DXVec3Dot(&hypothetical_spray_dir,&m_SprayGeneratorWorldSpaceSprayDir[i]));

				const float normals_dp = max(0.f,-D3DXVec3Dot(&water_normal, &hull_normal));
				const float water_perp_water_speed = D3DXVec3Dot(&relative_velocity_of_wave, &water_normal);
				const float speed_of_contact_line_relative_to_hull = max(0.f,water_perp_water_speed/sqrtf(1.f-normals_dp*normals_dp));

				float hull_perp_water_speed_mult = min_speed_mult + (max_speed_mult-min_speed_mult) * ((float)rand()/(float)RAND_MAX);
				float speed = squirtiness * speed_of_contact_line_relative_to_hull + hull_perp_water_speed_mult * hull_perp_water_speed;
				if(speed > max_speed)
					speed = max_speed;

				// Emit along a random direction between the hull spray direction and the water surface
				D3DXVECTOR3 hull_spray_direction = m_SprayGeneratorWorldSpaceSprayDir[i];
				D3DXVECTOR3 water_spray_direction = hull_spray_direction + D3DXVec3Dot(&hull_spray_direction,&water_normal) * water_normal;
				D3DXVec3Normalize(&water_spray_direction,&water_spray_direction);

				float spray_rand = ((float)rand()/(float)RAND_MAX);
				D3DXVECTOR3 spray_direction = (1.f-spray_rand) * hull_spray_direction + spray_rand * water_spray_direction;
				D3DXVec3Normalize(&spray_direction,&spray_direction);

				m_spraySpeed[i] = speed * spray_direction;

				// Add the hull speed back in for front-facing surfaces
				D3DXVECTOR3 hull_speed_dir = m_hullSpeed[i];
				D3DXVec3Normalize(&hull_speed_dir,&hull_speed_dir);
				float hull_perp_hull_speed_dir = -D3DXVec3Dot(&hull_speed_dir,&hull_normal);
				if(hull_perp_hull_speed_dir > 0.f) {
					m_spraySpeed[i] += hull_perp_hull_speed_dir * m_hullSpeed[i];
				}

				// Offset so that particles emerge from water
				D3DXVECTOR3 offset_vec = hull_normal + normals_dp * water_normal;
				D3DXVec3Normalize(&offset_vec,&offset_vec);
				offset_vec -= water_normal;
				m_sprayOffset[i] = 0.5f * offset_vec * kParticleInitialScale;

				// Add the KE to the tracker
				m_SplashPowerFromLastUpdate += m_sprayAmount[i]*D3DXVec3Dot(&m_spraySpeed[i],&m_spraySpeed[i])*power_nrm;
			}
		}

		// Emit rim particles when the rim is under-water
		for(int i = 0; i < m_pHullSensors->getNumRimSensors() && m_numParticlesAlive < NUMSPRAYPARTICLES; i++)
		{
			float water_height = pRimSensorDisplacements[i].z - pWorldRimSensorPositions[i].z;
			m_rimSprayAmount[i] = 0;
			if(water_height > 0.f) 
			{
				float mean_rim_spray_gen = spray_mult * CalcMeanRimSprayGen(water_height, deltaTime);
				int spray_amount = PoissonOutcome(mean_rim_spray_gen);
				m_rimSprayAmount[i] = spray_amount;
				if((m_numParticlesAlive + spray_amount) > NUMSPRAYPARTICLES)
					spray_amount = NUMSPRAYPARTICLES - m_numParticlesAlive;
				for(int emit_ix = 0; emit_ix != spray_amount; ++emit_ix)
				{
					float lerp = (float)rand()/(float)RAND_MAX;
					D3DXVECTOR3 lerped_gen_position = (1.f-lerp) * pWorldRimSensorPositions[i] + lerp * m_WorldRimSensorPositionsOld[i];

					D3DXVec3TransformNormal(&rv,&m_RimSprayGeneratorShipSpaceSprayDir[i], pOceanVessel->getWorldXform());
					D3DXVECTOR3 deck_up;
					deck_up.x = rv.x;
					deck_up.y = rv.z;
					deck_up.z = rv.y;

					D3DXVECTOR3 water_out = -pWorldRimSensorNormals[i];

					// Spawn at random location along water height
					lerped_gen_position += water_height * (float)rand()/(float)RAND_MAX * deck_up;

					// Spawn in the water
					lerped_gen_position -= 0.5f * water_out * kParticleInitialScale;

					m_particlePosition[m_numParticlesAlive].x = lerped_gen_position.x;
					m_particlePosition[m_numParticlesAlive].y = lerped_gen_position.y;
					m_particlePosition[m_numParticlesAlive].z = lerped_gen_position.z;

					// Ensure particles follow movements of hull
					float random_up_speed = rim_up_speed_min + (rim_up_speed_max-rim_up_speed_min)*(float)rand()/(float)RAND_MAX;
					float random_out_speed = rim_out_speed_min + (rim_out_speed_max-rim_out_speed_min)*(float)rand()/(float)RAND_MAX;
					D3DXVECTOR3 hull_speed = (pWorldRimSensorPositions[i] - m_WorldRimSensorPositionsOld[i])/deltaTime;
					D3DXVECTOR3 emit_speed = random_up_speed * deck_up + random_out_speed * water_out;

					m_particleSpeed[m_numParticlesAlive] = hull_speed + emit_speed;
					m_particleTime[m_numParticlesAlive]		= 0.f;

					float speedVariation = (float)rand() / (float)RAND_MAX;
					m_particleScale[m_numParticlesAlive]	= kParticleInitialScale / (speedVariation * 2.0f + 0.25f);

					// Randomize orientation
					const float orientation = 2.f * D3DX_PI * (float)rand()/(float)RAND_MAX;
					const float c = cosf(orientation);
					const float s = sinf(orientation);
					m_particleOrientations[2*m_numParticlesAlive+0] = (WORD)floorf(0.5f + c * 32767.f);
					m_particleOrientations[2*m_numParticlesAlive+1] = (WORD)floorf(0.5f + s * 32767.f);

					++m_numParticlesAlive;
				}
			}
		}

		//generating particles
		for(int i = 0; i < m_pHullSensors->getNumSensors() && m_numParticlesAlive < NUMSPRAYPARTICLES; i++)
		{
			for(int emit_ix = 0; emit_ix != m_sprayAmount[i] && m_numParticlesAlive < NUMSPRAYPARTICLES; ++emit_ix)
			{
				// Interpolate back to previous location to avoid burstiness
				float lerp = (float)rand()/(float)RAND_MAX;
				D3DXVECTOR3 lerped_gen_position = (1.f-lerp) * pWorldSensorPositions[i] + lerp * m_WorldSensorPositionsOld[i];
				lerped_gen_position += m_sprayOffset[i];	// Keep particles from intersecting hull
				m_particlePosition[m_numParticlesAlive].x = lerped_gen_position.x;
				m_particlePosition[m_numParticlesAlive].y = lerped_gen_position.y;
				m_particlePosition[m_numParticlesAlive].z = lerped_gen_position.z;

				// Jitter the emit position - note that this is intentionally biased towards the centre
				D3DXVECTOR3 spray_tangent;
				D3DXVec3Cross(&spray_tangent,&m_SprayGeneratorWorldSpaceSprayDir[i],&pWorldSensorNormals[i]);
				float r = kParticleJitter * (float)rand()/(float)RAND_MAX;
				float theta = D3DX_PI * (float)rand()/(float)RAND_MAX;
				m_particlePosition[m_numParticlesAlive] += r * cosf(theta) * spray_tangent;
				m_particlePosition[m_numParticlesAlive] += r * sinf(theta) * m_SprayGeneratorWorldSpaceSprayDir[i];

				float speedVariation = (float)rand() / (float)RAND_MAX;

				m_particleSpeed[m_numParticlesAlive].x	= m_spraySpeed[i].x;
				m_particleSpeed[m_numParticlesAlive].y	= m_spraySpeed[i].y;
				m_particleSpeed[m_numParticlesAlive].z	= m_spraySpeed[i].z;
				m_particleTime[m_numParticlesAlive]		= 0;
				m_particleScale[m_numParticlesAlive]	= kParticleInitialScale / (speedVariation * 2.0f + 0.25f);

				// Randomize orientation
				const float orientation = 2.f * D3DX_PI * (float)rand()/(float)RAND_MAX;
				const float c = cosf(orientation);
				const float s = sinf(orientation);
				m_particleOrientations[2*m_numParticlesAlive+0] = (WORD)floorf(0.5f + c * 32767.f);
				m_particleOrientations[2*m_numParticlesAlive+1] = (WORD)floorf(0.5f + s * 32767.f);

#if !ENABLE_GPU_SIMULATION
				// Pre-roll sim to avoid bunching
				float preRollDeltaTime = lerp * deltaTime;
				simulateParticles(m_numParticlesAlive,m_numParticlesAlive+1,preRollDeltaTime,wind_speed_3d);
#endif
				++m_numParticlesAlive;
			}
		}
	}

	// passing displacements to old global variables
	for(int i = 0; i < m_pHullSensors->getNumSensors(); i++)
	{
		m_displacementsOld[i].x = pSensorDisplacements[i].x;
		m_displacementsOld[i].y = pSensorDisplacements[i].y;
		m_displacementsOld[i].z = pSensorDisplacements[i].z;
	}

	// passing hull points to old global variables
	for(int i = 0; i < m_pHullSensors->getNumSensors(); i++)
	{
		m_WorldSensorPositionsOld[i].x = pWorldSensorPositions[i].x;
		m_WorldSensorPositionsOld[i].y = pWorldSensorPositions[i].y;
		m_WorldSensorPositionsOld[i].z = pWorldSensorPositions[i].z;
	}

	// same for rim points
	for(int i = 0; i < m_pHullSensors->getNumRimSensors(); i++)
	{
		m_WorldRimSensorPositionsOld[i].x = pWorldRimSensorPositions[i].x;
		m_WorldRimSensorPositionsOld[i].y = pWorldRimSensorPositions[i].y;
		m_WorldRimSensorPositionsOld[i].z = pWorldRimSensorPositions[i].z;
	}
}

void OceanSpray::initializeGenerators(OceanHullSensors* pHullSensors)
{
	m_pHullSensors = pHullSensors;

	const D3DXVECTOR3* pSensorNormals = pHullSensors->getSensorNormals();
	for(int i = 0; i != pHullSensors->getNumSensors(); ++i) {

		D3DXVECTOR3 up = D3DXVECTOR3(0,1,0);
		D3DXVECTOR3 ideal_contact_dir;
		D3DXVec3Cross(&ideal_contact_dir,&pSensorNormals[i],&up);
		D3DXVec3Normalize(&ideal_contact_dir,&ideal_contact_dir);

		D3DXVec3Cross(&m_SprayGeneratorShipSpaceSprayDir[i],&ideal_contact_dir,&pSensorNormals[i]);
	}

	const D3DXVECTOR3* pRimSensorNormals = pHullSensors->getRimSensorNormals();
	for(int i = 0; i != pHullSensors->getNumRimSensors(); ++i) {

		D3DXVECTOR3 up = D3DXVECTOR3(0,1,0);
		D3DXVECTOR3 ideal_contact_dir;
		D3DXVec3Cross(&ideal_contact_dir,&pRimSensorNormals[i],&up);
		D3DXVec3Normalize(&ideal_contact_dir,&ideal_contact_dir);

		D3DXVec3Cross(&m_RimSprayGeneratorShipSpaceSprayDir[i],&ideal_contact_dir,&pRimSensorNormals[i]);
	}

	m_firstUpdate = true;
}

void OceanSpray::simulateParticles(int begin_ix, int end_ix, float deltaTime, const D3DXVECTOR3& wind_speed_3d)
{
	int num_steps = int(ceilf(deltaTime/kMaxSimulationTimeStep));
	FLOAT time_step = deltaTime/float(num_steps);

	for(int step = 0; step != num_steps; ++step) {

		// updating spray particles positions
		for(int i=begin_ix;i<end_ix; i++)
		{
			float x_delta = m_particleSpeed[i].x*time_step;
			float y_delta = m_particleSpeed[i].y*time_step;
			float z_delta = m_particleSpeed[i].z*time_step;

			m_particlePosition[i].x += x_delta;
			m_particlePosition[i].y += y_delta;
			m_particlePosition[i].z += z_delta;

			float distance_moved = sqrtf(x_delta*x_delta + y_delta*y_delta + z_delta*z_delta);
			m_particleScale[i] += distance_moved * kParticleScaleRate;
		}

		// updating spray particles speeds
		for(int i=begin_ix;i<end_ix; i++)
		{
			D3DXVECTOR3 accel = -kParticleDrag * (m_particleSpeed[i] - wind_speed_3d);
			accel.z -= kGravity;
			m_particleSpeed[i].x += accel.x*time_step;
			m_particleSpeed[i].y += accel.y*time_step;
			m_particleSpeed[i].z += accel.z*time_step;
		}

		// updating particle times
		for(int i=begin_ix;i<end_ix; i++)
		{
			m_particleTime[i]+=time_step;
		}
	}
}

void OceanSpray::updateSprayParticles(ID3D11DeviceContext* pDC, OceanVessel* pOceanVessel, float deltaTime, float kWaterScale, gfsdk_float2 wind_speed)
{
#if ENABLE_GPU_SIMULATION
	D3DXVECTOR3 wind_speed_3d = D3DXVECTOR3(wind_speed.y,0.0f,wind_speed.y);

	if (m_numParticlesAlive) // New particles to simulate
	{
		D3D11_MAPPED_SUBRESOURCE msr;
		pDC->Map(m_pRenderInstanceDataBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		float* pCurrRenderData = (float*)msr.pData;

		pDC->Map(m_pRenderVelocityDataBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		float* pCurrVelocityData = (float*)msr.pData;

		for(int src_ix=0;src_ix<m_numParticlesAlive; src_ix++)
		{
			pCurrRenderData[0] = m_particlePosition[src_ix].x;
			pCurrRenderData[1] = m_particlePosition[src_ix].z;
			pCurrRenderData[2] = m_particlePosition[src_ix].y;
			pCurrRenderData[3] = m_particleScale[src_ix];
			pCurrRenderData += 4;

			pCurrVelocityData[0] = m_particleSpeed[src_ix].x;
			pCurrVelocityData[1] = m_particleSpeed[src_ix].z;
			pCurrVelocityData[2] = m_particleSpeed[src_ix].y;
			pCurrVelocityData += 4;
		}
		pDC->Unmap(m_pRenderInstanceDataBuffer, 0);
		pDC->Unmap(m_pRenderVelocityDataBuffer, 0);

		m_pParticlesNum->SetInt(m_numParticlesAlive);
		m_pRenderInstanceDataVariable->SetResource(m_pRenderInstanceDataSRV);
		m_pRenderVelocityAndTimeDataVariable->SetResource(m_pRenderVelocityDataSRV);
		m_pInitSprayParticlesTechnique->GetPassByIndex(0)->Apply(0, pDC);
		
		UINT count = (UINT)-1;
		pDC->CSSetUnorderedAccessViews(1, 1, &m_pParticlesBufferUAV[m_ParticleWriteBuffer], &count);

		UINT blocksNum = (m_numParticlesAlive + SimulateSprayParticlesCSBlocksSize - 1) / SimulateSprayParticlesCSBlocksSize;
		pDC->Dispatch(blocksNum, 1, 1);

		m_numParticlesAlive = 0;
	}

	{ // Prepare DispatchIndirect arguments for simulation
		pDC->CopyStructureCount(m_pDrawParticlesCB, 0, m_pParticlesBufferUAV[m_ParticleWriteBuffer]);

		m_pDispatchArgumentsTechnique->GetPassByIndex(0)->Apply(0, pDC);

		pDC->CSSetConstantBuffers(2, 1, &m_pDrawParticlesCB);
		pDC->CSSetUnorderedAccessViews(0, 1, &m_pDrawParticlesBufferUAV, NULL);

		pDC->Dispatch(1, 1, 1);

		ID3D11UnorderedAccessView* pNullUAV = NULL;
		pDC->CSSetUnorderedAccessViews(0, 1, &pNullUAV, NULL);
	}

	{ // Simulate particles

		m_ParticleWriteBuffer = 1 - m_ParticleWriteBuffer;

		m_pSimulationTime->SetFloat(deltaTime);

		m_pWindSpeed->SetFloatVector((FLOAT*)&wind_speed_3d);

		m_pVesselToWorldVariable->SetMatrix((float*)pOceanVessel->getWorldXform());

		D3DXMATRIX matWorldToVessel;
		D3DXMatrixInverse(&matWorldToVessel,NULL,pOceanVessel->getWorldXform());
		m_pWorldToVesselVariable->SetMatrix((float*)&matWorldToVessel);

		gfsdk_float2 worldToUVScale;
		gfsdk_float2 worldToUVOffset;
		gfsdk_float2 worldToUVRot;
		pOceanVessel->getSurfaceHeights()->getGPUWorldToUVTransform(worldToUVOffset, worldToUVRot, worldToUVScale);

		m_worldToHeightLookupScaleVariable->SetFloatVector((float*)&worldToUVScale);
		m_worldToHeightLookupRotVariable->SetFloatVector((float*)&worldToUVRot);
		m_worldToHeightLookupOffsetVariable->SetFloatVector((float*)&worldToUVOffset);

		m_texHeightLookupVariable->SetResource(pOceanVessel->getSurfaceHeights()->getGPULookupSRV());

		m_pParticlesBufferVariable->SetResource(m_pParticlesBufferSRV[1-m_ParticleWriteBuffer]);
		m_pSimulateSprayParticlesTechnique->GetPassByIndex(0)->Apply(0, pDC);

		pDC->CSSetConstantBuffers(2, 1, &m_pDrawParticlesCB);

		UINT count = 0;
		pDC->CSSetUnorderedAccessViews(1, 1, &m_pParticlesBufferUAV[m_ParticleWriteBuffer], &count);

		pDC->DispatchIndirect(m_pDrawParticlesBuffer, 0);

		ID3D11UnorderedAccessView* pNullUAV = NULL;
		pDC->CSSetUnorderedAccessViews(1, 1, &pNullUAV, NULL);

		pDC->CopyStructureCount(m_pDrawParticlesBuffer, 0, m_pParticlesBufferUAV[m_ParticleWriteBuffer]);
		pDC->CopyStructureCount(m_pDrawParticlesCB, 0, m_pParticlesBufferUAV[m_ParticleWriteBuffer]);

		// Release refs
		m_texHeightLookupVariable->SetResource(NULL);
		m_pSimulateSprayParticlesTechnique->GetPassByIndex(0)->Apply(0, pDC);
	}
#else
	D3DXVECTOR3 wind_speed_3d = D3DXVECTOR3(wind_speed.y,wind_speed.y,0.f);
	simulateParticles(0,m_numParticlesAlive,deltaTime,wind_speed_3d);

	D3DXMATRIX worldToVessel;
	D3DXMatrixInverse(&worldToVessel,NULL,pOceanVessel->getWorldXform());

	D3D11_MAPPED_SUBRESOURCE msr;
	pDC->Map(m_pRenderInstanceDataBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	float* pCurrRenderData = (float*)msr.pData;

	pDC->Map(m_pRenderOrientationsAndDecimationsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	WORD* pCurrOrientationAndDecimationData = (WORD*)msr.pData;

	// compact out particles that are below water surface, and update our render buffer at the same time
	int dst_ix = 0;
	for(int src_ix=0;src_ix<m_numParticlesAlive; src_ix++)
	{
		D3DXVECTOR3 world_pos;
		world_pos.x = m_particlePosition[src_ix].x;
		world_pos.y = m_particlePosition[src_ix].z;
		world_pos.z = m_particlePosition[src_ix].y;

		bool kill_this_particle = false;
		if(world_pos.y < -kParticleKillDepth)
		{
			kill_this_particle=true;
		}
		if(m_particleTime[src_ix] > kParticleTTL)
		{
			kill_this_particle=true;
		}

		if(!kill_this_particle) {
			if(src_ix != dst_ix) {
				// Keep this particle
				m_particlePosition[dst_ix] = m_particlePosition[src_ix];
				m_particleSpeed[dst_ix] = m_particleSpeed[src_ix];
				m_particleTime[dst_ix] = m_particleTime[src_ix];
				m_particleScale[dst_ix] = m_particleScale[src_ix];
				m_particleOrientations[2*dst_ix+0] = m_particleOrientations[2*src_ix+0];
				m_particleOrientations[2*dst_ix+1] = m_particleOrientations[2*src_ix+1];
			}

			pCurrRenderData[0] = m_particlePosition[dst_ix].x;
			pCurrRenderData[1] = m_particlePosition[dst_ix].z;
			pCurrRenderData[2] = m_particlePosition[dst_ix].y;
			pCurrRenderData[3] = m_particleScale[dst_ix];
			pCurrRenderData += 4;

			pCurrOrientationAndDecimationData[0] = m_particleOrientations[2*dst_ix+0];
			pCurrOrientationAndDecimationData[1] = m_particleOrientations[2*dst_ix+1];
			pCurrOrientationAndDecimationData += 4;

			const float decimation = min(kFadeInTime,m_particleTime[dst_ix])/kFadeInTime;
			pCurrOrientationAndDecimationData[2] = (WORD)floorf(0.5f + decimation * 32767.f);

			++dst_ix;
		}
	}

	pDC->Unmap(m_pRenderInstanceDataBuffer,0);
	pDC->Unmap(m_pRenderOrientationsAndDecimationsBuffer,0);
	m_numParticlesAlive = dst_ix;
#endif
}

void OceanSpray::renderToFoam(	ID3D11DeviceContext* pDC,
								const D3DXMATRIX& matWorldToFoam,
								OceanVessel* pOceanVessel,
								float deltaTime
								)
{
	// Just swap y and z in view matrix
	D3DXMATRIX matView = D3DXMATRIX(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1);
	m_pMatViewVariable->SetMatrix((float*)&matView);

	m_pMatWorldToFoamVariable->SetMatrix((float*)&matWorldToFoam);

	gfsdk_float2 worldToUVScale;
	gfsdk_float2 worldToUVOffset;
	gfsdk_float2 worldToUVRot;
	pOceanVessel->getSurfaceHeights()->getGPUWorldToUVTransform(worldToUVOffset, worldToUVRot, worldToUVScale);

	m_worldToHeightLookupScaleVariable->SetFloatVector((float*)&worldToUVScale);
	m_worldToHeightLookupRotVariable->SetFloatVector((float*)&worldToUVRot);
	m_worldToHeightLookupOffsetVariable->SetFloatVector((float*)&worldToUVOffset);

	m_texHeightLookupVariable->SetResource(pOceanVessel->getSurfaceHeights()->getGPULookupSRV());

	m_pSimulationTime->SetFloat(deltaTime);
	m_pSimpleParticlesVariable->SetFloat(0.0f);

	renderParticles(pDC,m_pRenderSprayToFoamTechnique, NULL, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// Release refs
	m_texHeightLookupVariable->SetResource(NULL);
	m_pRenderSprayToFoamTechnique->GetPassByIndex(0)->Apply(0,pDC);
}

void OceanSpray::depthSortParticles( ID3D11DeviceContext* pDC )
{
#if ENABLE_GPU_SIMULATION
	int num_elements = SPRAY_PARTICLE_COUNT;

	{ // Init sorting data
		m_pParticlesBufferVariable->SetResource(m_pParticlesBufferSRV[m_ParticleWriteBuffer]);
		m_pInitSortTechnique->GetPassByIndex(0)->Apply(0, pDC);

		pDC->CSSetConstantBuffers(2, 1, &m_pDrawParticlesCB);

		UINT count = 0;
		pDC->CSSetUnorderedAccessViews(0, 1, &m_pDepthSort1UAV, &count);

		pDC->Dispatch(num_elements/SprayParticlesCSBlocksSize, 1, 1);
	}

	const UINT matrix_width = BitonicSortCSBlockSize;
	const UINT matrix_height = num_elements / BitonicSortCSBlockSize;

	// First sort the rows for the levels <= to the block size
	for( UINT level = 2 ; level <= BitonicSortCSBlockSize ; level = level * 2 )
	{
		setDepthSortConstants( level, level, matrix_height, matrix_width );

		// Sort the row data
		m_pParticleDepthSortUAVVariable->SetUnorderedAccessView(m_pDepthSort1UAV);
		m_pBitonicSortTechnique->GetPassByIndex(0)->Apply(0, pDC);
		pDC->Dispatch( num_elements / BitonicSortCSBlockSize, 1, 1 );
	}

	// Then sort the rows and columns for the levels > than the block size
	// Transpose. Sort the Columns. Transpose. Sort the Rows.
	for( int level = (BitonicSortCSBlockSize * 2) ; level <= num_elements ; level = level * 2 )
	{
		setDepthSortConstants( (level / BitonicSortCSBlockSize), (level & ~num_elements) / BitonicSortCSBlockSize, matrix_width, matrix_height );

		// Transpose the data from buffer 1 into buffer 2
		m_pParticleDepthSortUAVVariable->SetUnorderedAccessView(m_pDepthSort2UAV);
		m_pParticleDepthSortSRVVariable->SetResource(m_pDepthSort1SRV);
		m_pMatrixTransposeTechnique->GetPassByIndex(0)->Apply(0, pDC);
		pDC->Dispatch( matrix_width / TransposeCSBlockSize, matrix_height / TransposeCSBlockSize, 1 );
		m_pParticleDepthSortSRVVariable->SetResource(NULL);
		m_pMatrixTransposeTechnique->GetPassByIndex(0)->Apply(0, pDC);

		// Sort the transposed column data
		m_pBitonicSortTechnique->GetPassByIndex(0)->Apply(0, pDC);
		pDC->Dispatch( num_elements / BitonicSortCSBlockSize, 1, 1 );

		setDepthSortConstants( BitonicSortCSBlockSize, level, matrix_height, matrix_width );

		// Transpose the data from buffer 2 back into buffer 1
		m_pParticleDepthSortUAVVariable->SetUnorderedAccessView(m_pDepthSort1UAV);
		m_pParticleDepthSortSRVVariable->SetResource(m_pDepthSort2SRV);
		m_pMatrixTransposeTechnique->GetPassByIndex(0)->Apply(0, pDC);
		pDC->Dispatch( matrix_height / TransposeCSBlockSize, matrix_width / TransposeCSBlockSize, 1 );
		m_pParticleDepthSortSRVVariable->SetResource(NULL);
		m_pMatrixTransposeTechnique->GetPassByIndex(0)->Apply(0, pDC);

		// Sort the row data
		m_pBitonicSortTechnique->GetPassByIndex(0)->Apply(0, pDC);
		pDC->Dispatch( num_elements / BitonicSortCSBlockSize, 1, 1 );
	}

	// Release outputs
	m_pParticleDepthSortUAVVariable->SetUnorderedAccessView(NULL);
	m_pMatrixTransposeTechnique->GetPassByIndex(0)->Apply(0, pDC);
	m_pBitonicSortTechnique->GetPassByIndex(0)->Apply(0, pDC);
#endif
}

void OceanSpray::setDepthSortConstants( UINT iLevel, UINT iLevelMask, UINT iWidth, UINT iHeight )
{
	m_piDepthSortLevelVariable->SetInt(iLevel);
	m_piDepthSortLevelMaskVariable->SetInt(iLevelMask);
	m_piDepthSortWidthVariable->SetInt(iWidth);
	m_piDepthSortHeightVariable->SetInt(iHeight);
}

void CalcTangentAndBinormal(D3DXVECTOR3& tangent, D3DXVECTOR3& binormal, const D3DXVECTOR3& normal)
{
	D3DXVECTOR3 perp;
	if(fabs(normal.x) < fabs(normal.y) && fabs(normal.x) < fabs(normal.z)) {
		perp = D3DXVECTOR3(1.f,0.f,0.f);
	} 
	else if(fabs(normal.y) < fabs(normal.z)) {
		perp = D3DXVECTOR3(0.f,1.f,0.f);
	}
	else {
		perp = D3DXVECTOR3(0.f,0.f,1.f);
	}

	D3DXVec3Cross(&tangent,&perp,&normal);
	D3DXVec3Normalize(&tangent,&tangent);
	D3DXVec3Cross(&binormal,&tangent,&normal);
}

void OceanSpray::UpdateSensorsVisualizationVertexBuffer(ID3D11DeviceContext* pDC, OceanVessel* pOceanVessel,float deltaTime)
{
	const OceanHullSensors* pSensors = pOceanVessel->getHullSensors();
	const D3DXVECTOR3* pSensorPositions = pSensors->getSensorPositions();
	const D3DXVECTOR3* pSensorNormals = pSensors->getSensorNormals();
	const D3DXVECTOR3* pRimSensorPositions = pSensors->getRimSensorPositions();
	const D3DXVECTOR3* pRimSensorNormals = pSensors->getRimSensorNormals();

	D3D11_MAPPED_SUBRESOURCE subresource;
	pDC->Map(m_pSensorsVisualizationVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
	VisualizationVertex* sensorsVisualizationVertexData = (VisualizationVertex*)subresource.pData;

	const float sensor_min_size = 0.075f;
	const float sensor_max_size = 0.4f;
	const float spray_amount_fade_rate = 1.f;

	for(int i=0;i<pSensors->getNumSensors(); i++)
	{
		if(m_sprayAmount[i] > 0)
			m_sprayVizualisationFade[i] = 1.f;
		const float intensity = (float)m_sprayVizualisationFade[i] + 0.001f;
		const float sensor_size = m_sprayVizualisationFade[i] * sensor_max_size + (1.f-m_sprayVizualisationFade[i]) * sensor_min_size;
		if(m_sprayVizualisationFade[i] > 0.f)
			m_sprayVizualisationFade[i] -= spray_amount_fade_rate * deltaTime;
		if(m_sprayVizualisationFade[i] < 0.f)
			m_sprayVizualisationFade[i] = 0.f;

		D3DXVECTOR3 tangent, binormal;
		CalcTangentAndBinormal(tangent, binormal, pSensorNormals[i]);
		tangent *= sensor_size;
		binormal *= sensor_size;

		sensorsVisualizationVertexData[0].position_and_intensity = D3DXVECTOR4(pSensorPositions[i] + tangent + binormal, intensity);
		sensorsVisualizationVertexData[1].position_and_intensity = D3DXVECTOR4(pSensorPositions[i] + tangent - binormal, intensity);
		sensorsVisualizationVertexData[2].position_and_intensity = D3DXVECTOR4(pSensorPositions[i] - tangent - binormal, intensity);
		sensorsVisualizationVertexData[3].position_and_intensity = D3DXVECTOR4(pSensorPositions[i] - tangent + binormal, intensity);

		sensorsVisualizationVertexData[0].uv = D3DXVECTOR2(0.f,0.f);
		sensorsVisualizationVertexData[1].uv = D3DXVECTOR2(1.f,0.f);
		sensorsVisualizationVertexData[2].uv = D3DXVECTOR2(1.f,1.f);
		sensorsVisualizationVertexData[3].uv = D3DXVECTOR2(0.f,1.f);

		sensorsVisualizationVertexData += 4;
	}
	for(int i=0;i<pSensors->getNumRimSensors(); i++)
	{
		if(m_rimSprayAmount[i] > 0)
			m_rimSprayVizualisationFade[i] = 1.f;
		const float intensity = -(float)m_rimSprayVizualisationFade[i] - 0.001f; // making it negative to differentiate rim sensors from regular sensors
		const float sensor_size = m_rimSprayVizualisationFade[i] * sensor_max_size + (1.f-m_rimSprayVizualisationFade[i]) * sensor_min_size;
		if(m_rimSprayVizualisationFade[i] > 0.f)
			m_rimSprayVizualisationFade[i] -= spray_amount_fade_rate * deltaTime;
		if(m_rimSprayVizualisationFade[i] < 0.f)
			m_rimSprayVizualisationFade[i] = 0.f;

		D3DXVECTOR3 tangent, binormal;
		CalcTangentAndBinormal(tangent, binormal, pRimSensorNormals[i]);
		tangent *= sensor_size;
		binormal *= sensor_size;

		sensorsVisualizationVertexData[0].position_and_intensity = D3DXVECTOR4(pRimSensorPositions[i] + tangent + binormal, intensity);
		sensorsVisualizationVertexData[1].position_and_intensity = D3DXVECTOR4(pRimSensorPositions[i] + tangent - binormal, intensity);
		sensorsVisualizationVertexData[2].position_and_intensity = D3DXVECTOR4(pRimSensorPositions[i] - tangent - binormal, intensity);
		sensorsVisualizationVertexData[3].position_and_intensity = D3DXVECTOR4(pRimSensorPositions[i] - tangent + binormal, intensity);

		sensorsVisualizationVertexData[0].uv = D3DXVECTOR2(0.f,0.f);
		sensorsVisualizationVertexData[1].uv = D3DXVECTOR2(1.f,0.f);
		sensorsVisualizationVertexData[2].uv = D3DXVECTOR2(1.f,1.f);
		sensorsVisualizationVertexData[3].uv = D3DXVECTOR2(0.f,1.f);

		sensorsVisualizationVertexData += 4;
	}

	pDC->Unmap(m_pSensorsVisualizationVertexBuffer, 0);
}

void OceanSpray::RenderSensors(ID3D11DeviceContext* pDC, const CBaseCamera& camera, OceanVessel* pOceanVessel)
{
	const OceanHullSensors* pSensors = pOceanVessel->getHullSensors();
	const int num_sensors = pSensors->getNumSensors() + pSensors->getNumRimSensors();

	D3DXMATRIX matWorldView = *pOceanVessel->getWorldXform() * *camera.GetViewMatrix();

	m_pMatViewVariable->SetMatrix(matWorldView);
	m_pMatProjVariable->SetMatrix((float*)camera.GetProjMatrix());
	m_pSensorVisualizationTechnique->GetPassByIndex(0)->Apply(0, pDC);

	const UINT vertexStride = sizeof(VisualizationVertex);
	const UINT vbOffset = 0;

	pDC->IASetInputLayout(m_pSensorVisualizationLayout);
	pDC->IASetVertexBuffers(0, 1, &m_pSensorsVisualizationVertexBuffer, &vertexStride, &vbOffset);
	pDC->IASetIndexBuffer(m_pSensorsVisualizationIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pDC->DrawIndexed(6*num_sensors,0,0);
}

void OceanSpray::renderAudioVisualization(ID3D11DeviceContext* pDC, float l, float t, float r, float b, float h_margin, float v_margin, float level)
{
	FLOAT litterbug[4] = {l,t,r,b};
	m_pAudioVisualizationRectVariable->SetFloatVector(litterbug);

	FLOAT margin[4] = {h_margin, v_margin, 0.f, 0.f};
	m_pAudioVisualizationMarginVariable->SetFloatVector(margin);

	m_pAudioVisualizationLevelVariable->SetFloat(level);

	m_pAudioVisualizationTechnique->GetPassByIndex(0)->Apply(0, pDC);

	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pDC->IASetInputLayout(NULL);
	pDC->Draw(12,0);
}