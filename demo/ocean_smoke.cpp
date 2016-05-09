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
// Copyright � 2008- 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
 
#include "DXUT.h"
#include "ocean_smoke.h"

#include "DXUTcamera.h"
#include "ocean_psm.h"
#include "ocean_shader_common.h"
#include "ocean_env.h"

extern HRESULT LoadFile(LPCTSTR FileName, ID3DXBuffer** ppBuffer);

namespace {

	enum { NumRandomUV = (RAND_MAX/2) };

	const float kMaxSimulationTimeStep = 0.06f;

	// noise permutation table
	const unsigned char kNoisePerms[] = {	151,160,137,91,90,15,
											131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
											190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
											88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
											77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
											102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
											135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
											5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
											223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
											129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
											251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
											49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
											138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
											};

	enum { NumNoisePerms = sizeof(kNoisePerms)/sizeof(kNoisePerms[0]) };

	const int kNoiseG4[] = {
		0, -1, -1, -1,
		0, -1, -1, 1,
		0, -1, 1, -1,
		0, -1, 1, 1,
		0, 1, -1, -1,
		0, 1, -1, 1,
		0, 1, 1, -1,
		0, 1, 1, 1,
		-1, -1, 0, -1,
		-1, 1, 0, -1,
		1, -1, 0, -1,
		1, 1, 0, -1,
		-1, -1, 0, 1,
		-1, 1, 0, 1,
		1, -1, 0, 1,
		1, 1, 0, 1,
		
		-1, 0, -1, -1,
		1, 0, -1, -1,
		-1, 0, -1, 1,
		1, 0, -1, 1,
		-1, 0, 1, -1,
		1, 0, 1, -1,
		-1, 0, 1, 1,
		1, 0, 1, 1,
		0, -1, -1, 0,
		0, -1, -1, 0,
		0, -1, 1, 0,
		0, -1, 1, 0,
		0, 1, -1, 0,
		0, 1, -1, 0,
		0, 1, 1, 0,
		0, 1, 1, 0,
	};

	unsigned char encode_noiseg4(int val) {
		switch(val)
		{
			case -1: return 0x80;
			case 1: return 0x7F;
			default: return 0;
		}
	}

	enum { NumNoiseG4s = sizeof(kNoiseG4)/(4*sizeof(kNoiseG4[0])) };

	enum { kPSMLayersPerSlice = 2 };
}

OceanSmoke::OceanSmoke()
{
	m_pFX = NULL;

	m_pRenderToSceneTechnique = NULL;
	m_pRenderToPSMTechnique = NULL;
	m_pEmitParticlesTechnique = NULL;
	m_pSimulateParticlesTechnique = NULL;
	m_pBitonicSortTechnique = NULL;
	m_pMatrixTransposeTechnique = NULL;

	m_pParticleIndexOffsetVariable = NULL;
	m_pParticleCountVariable = NULL;
	m_pTimeStepVariable = NULL;
	m_pPreRollEndTimeVariable = NULL;
	m_pBuoyancyParamsVariable = NULL;
	m_pNoiseTimeVariable = NULL;
	m_pParticleDepthSortUAVVariable = NULL;
	m_pParticleDepthSortSRVVariable = NULL;

	m_pSimulationInstanceDataVariable = NULL;
	m_pSimulationVelocitiesVariable = NULL;
	m_pRandomUVVariable = NULL;
	m_pRandomOffsetVariable = NULL;
	m_pCurrEmitterMatrixVariable = NULL;
	m_pPrevEmitterMatrixVariable = NULL;
	m_pEmitInterpScaleAndOffsetVariable = NULL;
	m_pEmitMinMaxVelocityAndSpreadVariable = NULL;
	m_pEmitAreaScaleVariable = NULL;

	m_pWindDragVariable = NULL;
	m_pWindVectorAndNoiseMultVariable = NULL;

	m_pTexDiffuseVariable = NULL;
	m_pRenderInstanceDataVariable = NULL;
	m_pMatProjVariable = NULL;
	m_pMatViewVariable = NULL;
	m_pLightDirectionVariable = NULL;
	m_pLightColorVariable = NULL;
	m_pAmbientColorVariable = NULL;
	m_pFogExponentVariable = NULL;
	m_pParticleBeginEndScaleVariable = NULL;
	m_pInvParticleLifeTimeVariable = NULL;

	m_pLightningPositionVariable = NULL;
	m_pLightningColorVariable = NULL;

	m_ppermTextureVariable = NULL;
	m_pgradTexture4dVariable = NULL;

	m_pPSMParams = NULL;
	m_pMatViewToPSMVariable = NULL;

	m_piDepthSortLevelVariable = NULL;
	m_piDepthSortLevelMaskVariable = NULL;
	m_piDepthSortWidthVariable = NULL;
	m_piDepthSortHeightVariable = NULL;

	m_pd3dDevice = DXUTGetD3D11Device();
	m_pSmokeTextureSRV = NULL;
	m_pRandomUVSRV = NULL;
	m_pRenderInstanceDataSRV = NULL;
	m_pSimulationInstanceDataUAV = NULL;
	m_pSimulationVelocitiesUAV = NULL;
	m_ppermTextureSRV = NULL;
	m_pgradTexture4dSRV = NULL;

	m_pDepthSort1SRV = NULL;
	m_pDepthSort1UAV = NULL;
	m_pDepthSort2SRV = NULL;
	m_pDepthSort2UAV = NULL;

	m_MaxNumParticles = 0;
	m_ActiveNumParticles = 0;
	m_NumDepthSortParticles = 0;
	m_EmitParticleIndex = 0;
	m_EmitParticleRandomOffset = 0;

	m_ParticleBeginSize = 1.f;
	m_ParticleEndSize = 1.f;
	m_EmitMinVelocity = 0.f;
	m_EmitMaxVelocity = 0.f;
	m_EmitSpread = 1.f;
	m_ActualEmitRate = 0.f;
	m_NominalEmitRate = 0.f;
	m_TimeToNextEmit = 0.f;
	m_WindDrag = 1.f;
	m_MinBuoyancy = 0.f;
	m_MaxBuoyancy = 1.f;
	m_CoolingRate = 0.5f;
	m_NoiseTime = 0.f;
	m_EmitAreaScale = D3DXVECTOR2(1.f,1.f);

	D3DXMatrixIdentity(&m_EmitterMatrix);
	m_EmitterMatrixIsValid = false;
	m_WindVector = D3DXVECTOR3(0.f,0.f,0.f);
	m_WindVectorIsValid = false;
}

OceanSmoke::~OceanSmoke()
{
	SAFE_RELEASE(m_pFX);
	SAFE_DELETE(m_pPSMParams);

	SAFE_RELEASE(m_pDepthSort1SRV);
	SAFE_RELEASE(m_pDepthSort1UAV);
	SAFE_RELEASE(m_pDepthSort2SRV);
	SAFE_RELEASE(m_pDepthSort2UAV);

	SAFE_RELEASE(m_pSmokeTextureSRV);
	SAFE_RELEASE(m_pRandomUVSRV);
	SAFE_RELEASE(m_pRenderInstanceDataSRV);
	SAFE_RELEASE(m_pSimulationInstanceDataUAV);
	SAFE_RELEASE(m_pSimulationVelocitiesUAV);
	SAFE_RELEASE(m_ppermTextureSRV);
	SAFE_RELEASE(m_pgradTexture4dSRV);
}

HRESULT OceanSmoke::init(	ID3D11ShaderResourceView* pSmokeTexture,
							int num_particles,
							float emit_rate,
							float particle_begin_size,
							float particle_end_size,
							float emit_min_velocity,
							float emit_max_velocity,
							float emit_spread,
							float wind_drag,
							float min_buoyancy,
							float max_buoyancy,
							float cooling_rate,
							const D3DXVECTOR2& emitAreaScale,
							float psm_bounds_fade_margin,
							float shadow_opacity_multiplier,
							const D3DXVECTOR3& tint_color,
							float wind_noise_spatial_scale,
							float wind_noise_time_scale
							)
{
	HRESULT hr;

	m_MaxNumParticles = num_particles;
	m_ActiveNumParticles = 0;
	m_EmitParticleIndex = 0;
	m_EmitParticleRandomOffset = 0;
	m_ParticleBeginSize = particle_begin_size;
	m_ParticleEndSize = particle_end_size;
	m_EmitMinVelocity = emit_min_velocity;
	m_EmitMaxVelocity = emit_max_velocity;
	m_EmitSpread = emit_spread;
	m_NominalEmitRate = emit_rate;
	m_ActualEmitRate = 0.f;
	m_TimeToNextEmit = 0.f;
	m_EmitterMatrixIsValid = false;
	m_WindVectorIsValid = false;
	m_WindDrag = wind_drag;
	m_MinBuoyancy = min_buoyancy;
	m_MaxBuoyancy = max_buoyancy;
	m_CoolingRate = cooling_rate;
	m_NoiseTime = 0.f;
	m_EmitAreaScale = emitAreaScale;
	m_TintColor = tint_color;

	// Depth sort works to power of two, so...
	m_NumDepthSortParticles = 1024;
	while(m_NumDepthSortParticles < m_MaxNumParticles)
		m_NumDepthSortParticles *= 2;

	SAFE_RELEASE(m_pSmokeTextureSRV);
	m_pSmokeTextureSRV = pSmokeTexture;
	m_pSmokeTextureSRV->AddRef();

	SAFE_RELEASE(m_pFX);
    ID3DXBuffer* pEffectBuffer = NULL;
    V_RETURN(LoadFile(TEXT(".\\Media\\ocean_smoke_d3d11.fxo"), &pEffectBuffer));
    V_RETURN(D3DX11CreateEffectFromMemory(pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, m_pd3dDevice, &m_pFX));
    pEffectBuffer->Release();

	m_pRenderToSceneTechnique = m_pFX->GetTechniqueByName("RenderSmokeToSceneTech");
	m_pRenderToPSMTechnique = m_pFX->GetTechniqueByName("RenderSmokeToPSMTech");
	m_pEmitParticlesTechnique = m_pFX->GetTechniqueByName("EmitParticlesTech");
	m_pSimulateParticlesTechnique = m_pFX->GetTechniqueByName("SimulateParticlesTech");
	m_pBitonicSortTechnique = m_pFX->GetTechniqueByName("BitonicSortTech");
	m_pMatrixTransposeTechnique = m_pFX->GetTechniqueByName("MatrixTransposeTech");

	m_pParticleIndexOffsetVariable = m_pFX->GetVariableByName("g_ParticleIndexOffset")->AsScalar();
	m_pParticleCountVariable = m_pFX->GetVariableByName("g_ParticleCount")->AsScalar();
	m_pTimeStepVariable = m_pFX->GetVariableByName("g_TimeStep")->AsScalar();
	m_pPreRollEndTimeVariable = m_pFX->GetVariableByName("g_PreRollEndTime")->AsScalar();
	m_pBuoyancyParamsVariable = m_pFX->GetVariableByName("g_BuoyancyParams")->AsVector();
	m_pNoiseTimeVariable = m_pFX->GetVariableByName("g_NoiseTime")->AsScalar();
	m_pParticleDepthSortUAVVariable = m_pFX->GetVariableByName("g_ParticleDepthSortUAV")->AsUnorderedAccessView();
	m_pParticleDepthSortSRVVariable = m_pFX->GetVariableByName("g_ParticleDepthSortSRV")->AsShaderResource();

	m_pSimulationInstanceDataVariable = m_pFX->GetVariableByName("g_SimulationInstanceData")->AsUnorderedAccessView();
	m_pSimulationVelocitiesVariable = m_pFX->GetVariableByName("g_SimulationVelocities")->AsUnorderedAccessView();
	m_pRandomUVVariable = m_pFX->GetVariableByName("g_RandomUV")->AsShaderResource();
	m_pRandomOffsetVariable = m_pFX->GetVariableByName("g_RandomOffset")->AsScalar();
	m_pCurrEmitterMatrixVariable = m_pFX->GetVariableByName("g_CurrEmitterMatrix")->AsMatrix();
	m_pPrevEmitterMatrixVariable = m_pFX->GetVariableByName("g_PrevEmitterMatrix")->AsMatrix();
	m_pEmitInterpScaleAndOffsetVariable = m_pFX->GetVariableByName("g_EmitInterpScaleAndOffset")->AsVector();
	m_pEmitMinMaxVelocityAndSpreadVariable = m_pFX->GetVariableByName("g_EmitMinMaxVelocityAndSpread")->AsVector();
	m_pEmitAreaScaleVariable = m_pFX->GetVariableByName("g_EmitAreaScale")->AsVector();

	m_pWindDragVariable =  m_pFX->GetVariableByName("g_WindDrag")->AsScalar();
	m_pWindVectorAndNoiseMultVariable =  m_pFX->GetVariableByName("g_WindVectorAndNoiseMult")->AsVector();

	m_pTexDiffuseVariable = m_pFX->GetVariableByName("g_texDiffuse")->AsShaderResource();
	m_pRenderInstanceDataVariable = m_pFX->GetVariableByName("g_RenderInstanceData")->AsShaderResource();
	m_pMatProjVariable = m_pFX->GetVariableByName("g_matProj")->AsMatrix();
	m_pMatViewVariable = m_pFX->GetVariableByName("g_matView")->AsMatrix();
	m_pLightDirectionVariable = m_pFX->GetVariableByName("g_LightDirection")->AsVector();
	m_pLightColorVariable = m_pFX->GetVariableByName("g_LightColor")->AsVector();
	m_pAmbientColorVariable = m_pFX->GetVariableByName("g_AmbientColor")->AsVector();
	m_pFogExponentVariable = m_pFX->GetVariableByName("g_FogExponent")->AsScalar();
	m_pParticleBeginEndScaleVariable = m_pFX->GetVariableByName("g_ParticleBeginEndScale")->AsVector();
	m_pInvParticleLifeTimeVariable = m_pFX->GetVariableByName("g_InvParticleLifeTime")->AsScalar();

	m_pLightningPositionVariable = m_pFX->GetVariableByName("g_LightningPosition")->AsVector();
	m_pLightningColorVariable = m_pFX->GetVariableByName("g_LightningColor")->AsVector();

	m_ppermTextureVariable = m_pFX->GetVariableByName("permTexture")->AsShaderResource();
	m_pgradTexture4dVariable = m_pFX->GetVariableByName("gradTexture4d")->AsShaderResource();

	m_pMatViewToPSMVariable = m_pFX->GetVariableByName("g_matViewToPSM")->AsMatrix();
	m_pPSMParams = new OceanPSMParams(m_pFX);

	m_piDepthSortLevelVariable = m_pFX->GetVariableByName("g_iDepthSortLevel")->AsScalar();
	m_piDepthSortLevelMaskVariable = m_pFX->GetVariableByName("g_iDepthSortLevelMask")->AsScalar();
	m_piDepthSortWidthVariable = m_pFX->GetVariableByName("g_iDepthSortWidth")->AsScalar();
	m_piDepthSortHeightVariable = m_pFX->GetVariableByName("g_iDepthSortHeight")->AsScalar();

	// Fire-and-forgets
	m_pFX->GetVariableByName("g_NoiseSpatialScale")->AsScalar()->SetFloat(wind_noise_spatial_scale);
	m_pFX->GetVariableByName("g_NoiseTimeScale")->AsScalar()->SetFloat(wind_noise_time_scale);
	m_pFX->GetVariableByName("g_PSMOpacityMultiplier")->AsScalar()->SetFloat(shadow_opacity_multiplier);
	m_pFX->GetVariableByName("g_PSMFadeMargin")->AsScalar()->SetFloat(psm_bounds_fade_margin);

	// Create buffer for random unit circle
	{
		srand(0);
		FLOAT randomUV[NumRandomUV*2];
		for(int i = 0; i != NumRandomUV; ++i) {
			randomUV[2*i+0] = (FLOAT(rand())/RAND_MAX);
			randomUV[2*i+1] = (FLOAT(rand())/RAND_MAX);
		}

		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = sizeof(randomUV);
		buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
		buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA srd;
		srd.pSysMem = randomUV;
		srd.SysMemPitch = 0;
		srd.SysMemSlicePitch = 0;

		ID3D11Buffer* pBuffer = NULL;
		V_RETURN(m_pd3dDevice->CreateBuffer(&buffer_desc, &srd, &pBuffer));

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		srv_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.FirstElement = 0;
		srv_desc.Buffer.NumElements = NumRandomUV;
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pBuffer, &srv_desc, &m_pRandomUVSRV));

		SAFE_RELEASE(pBuffer);
	}

	// Create noise buffers
	{
		D3D11_TEXTURE1D_DESC tex_desc;
		tex_desc.Width = NumNoisePerms;
		tex_desc.MipLevels = 1;
		tex_desc.ArraySize = 1;
		tex_desc.Format = DXGI_FORMAT_R8_UNORM;
		tex_desc.Usage = D3D11_USAGE_IMMUTABLE;
		tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex_desc.CPUAccessFlags = 0;
		tex_desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA srd;
		srd.pSysMem = kNoisePerms;
		srd.SysMemPitch = sizeof(kNoisePerms);
		srd.SysMemSlicePitch = 0;

		ID3D11Texture1D* pTex = NULL;
		V_RETURN(m_pd3dDevice->CreateTexture1D(&tex_desc, &srd, &pTex));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pTex, NULL, &m_ppermTextureSRV));

		SAFE_RELEASE(pTex);
	}

	{
		D3D11_TEXTURE1D_DESC tex_desc;
		tex_desc.Width = NumNoiseG4s;
		tex_desc.MipLevels = 1;
		tex_desc.ArraySize = 1;
		tex_desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
		tex_desc.Usage = D3D11_USAGE_IMMUTABLE;
		tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex_desc.CPUAccessFlags = 0;
		tex_desc.MiscFlags = 0;

		DWORD tex_data[NumNoiseG4s];
		for(int i = 0; i != NumNoiseG4s; ++i)
		{
			unsigned char r = encode_noiseg4(kNoiseG4[4*i+0]);
			unsigned char g = encode_noiseg4(kNoiseG4[4*i+1]);
			unsigned char b = encode_noiseg4(kNoiseG4[4*i+2]);
			unsigned char a = encode_noiseg4(kNoiseG4[4*i+3]);
			tex_data[i] = (r << 24) | (g << 16) | (b << 8) | a;
		}

		D3D11_SUBRESOURCE_DATA srd;
		srd.pSysMem = tex_data;
		srd.SysMemPitch = sizeof(tex_data);
		srd.SysMemSlicePitch = 0;

		ID3D11Texture1D* pTex = NULL;
		V_RETURN(m_pd3dDevice->CreateTexture1D(&tex_desc, &srd, &pTex));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pTex, NULL, &m_pgradTexture4dSRV));

		SAFE_RELEASE(pTex);
	}

	// Create simulation buffers
	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = m_MaxNumParticles * sizeof(D3DXVECTOR4);
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.FirstElement = 0;
		srv_desc.Buffer.NumElements = m_MaxNumParticles;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
		uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
		uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uav_desc.Buffer.FirstElement = 0;
		uav_desc.Buffer.NumElements = m_MaxNumParticles * 4;
		uav_desc.Buffer.Flags = 0;

		ID3D11Buffer* pBuffer = NULL;
		V_RETURN(m_pd3dDevice->CreateBuffer(&buffer_desc, NULL, &pBuffer));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pBuffer, &srv_desc, &m_pRenderInstanceDataSRV));
		V_RETURN(m_pd3dDevice->CreateUnorderedAccessView(pBuffer, &uav_desc, &m_pSimulationInstanceDataUAV));
		SAFE_RELEASE(pBuffer);

		V_RETURN(m_pd3dDevice->CreateBuffer(&buffer_desc, NULL, &pBuffer));
		V_RETURN(m_pd3dDevice->CreateUnorderedAccessView(pBuffer, &uav_desc, &m_pSimulationVelocitiesUAV));
		SAFE_RELEASE(pBuffer);
	}

	// Create depth sort buffers
	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.ByteWidth = 2 * m_NumDepthSortParticles * sizeof(float);
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		buffer_desc.StructureByteStride = 2 * sizeof(float);

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		srv_desc.Format = DXGI_FORMAT_UNKNOWN;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.FirstElement = 0;
		srv_desc.Buffer.NumElements = m_NumDepthSortParticles;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
		uav_desc.Format = DXGI_FORMAT_UNKNOWN;
		uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uav_desc.Buffer.FirstElement = 0;
		uav_desc.Buffer.NumElements = m_NumDepthSortParticles;
		uav_desc.Buffer.Flags = 0;

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

	return S_OK;
}

void OceanSmoke::updateSimulation(ID3D11DeviceContext* pDC, const CBaseCamera& camera, FLOAT time_delta, const D3DXMATRIX& emitterMatrix, const D3DXVECTOR3& wind_vector, const FLOAT wind_noise, FLOAT emit_rate_scale)
{
	const D3DXMATRIX prevEmitterMatrix = m_EmitterMatrix;
	const bool prevEmitterMatrixIsValid = m_EmitterMatrixIsValid;

	m_ActualEmitRate = m_NominalEmitRate * emit_rate_scale;

	// Clear the depth sort (this ensures that pow-2 pad entries are set to -1.f view-space,
	// which will force them to sort to end)
	const FLOAT depthSortClearValues[] = {-1.f, -1.f, -1.f, -1.f};
	pDC->ClearUnorderedAccessViewUint(m_pDepthSort1UAV,(UINT*)depthSortClearValues);

	// Set common params
	D3DXVECTOR4 vWindVector(wind_vector.x,wind_vector.y,wind_vector.z,wind_noise);
	m_pWindVectorAndNoiseMultVariable->SetFloatVector((FLOAT*)&vWindVector);
	m_pWindDragVariable->SetFloat(m_WindDrag);

	D3DXVECTOR4 vBuoyancyParams(m_MinBuoyancy, m_MaxBuoyancy, -m_CoolingRate, 0.f);
	m_pBuoyancyParamsVariable->SetFloatVector((FLOAT*)&vBuoyancyParams);

	m_ppermTextureVariable->SetResource(m_ppermTextureSRV);
	m_pgradTexture4dVariable->SetResource(m_pgradTexture4dSRV);

    D3DXMATRIX matView = *camera.GetViewMatrix();
	m_pMatViewVariable->SetMatrix((FLOAT*)&matView);

	int num_steps = int(ceilf(time_delta/kMaxSimulationTimeStep));
	FLOAT time_step = time_delta/float(num_steps);
	FLOAT step_interp_scale = 1.f/float(num_steps);
	if(0 == num_steps) {
		// Force through at least one sim step, in order to update depth sort
		num_steps = 1;
		time_step = 0.f;
		step_interp_scale = 1.f;
	}
	for(int step = 0; step != num_steps; ++step)
	{
		const FLOAT step_interp_offset = float(step) * step_interp_scale;
		m_pNoiseTimeVariable->SetFloat(m_NoiseTime);

		// Can/should emit?
		int num_particles_emitted = 0;
		if(prevEmitterMatrixIsValid) {

			if(time_step < m_TimeToNextEmit) {

				// Time slice too short to emit
				m_TimeToNextEmit -= time_step;

			} else {

				// Emit particles
				const float emit_interval = 1.f/m_ActualEmitRate;
				const int num_particles_to_emit = (int)ceilf((time_step - m_TimeToNextEmit)/emit_interval);
				if(num_particles_to_emit) {

					m_pTimeStepVariable->SetFloat(time_delta);	// NB: We set the entire time slice when emitting, so that we can
																// interpolate emit positions etc. in a consistent way

					// But this means that we need to subtract the remainder of the timeslice when calculating pre-roll
					const float pre_roll_end_time = time_step * float(num_steps - step - 1);
					m_pPreRollEndTimeVariable->SetFloat(pre_roll_end_time);

					m_pParticleIndexOffsetVariable->SetInt(m_EmitParticleIndex);
					m_pParticleCountVariable->SetInt(num_particles_to_emit);

					m_pSimulationInstanceDataVariable->SetUnorderedAccessView(m_pSimulationInstanceDataUAV);
					m_pSimulationVelocitiesVariable->SetUnorderedAccessView(m_pSimulationVelocitiesUAV);
					m_pParticleDepthSortUAVVariable->SetUnorderedAccessView(m_pDepthSort1UAV);
					m_pRandomUVVariable->SetResource(m_pRandomUVSRV);

					m_pRandomOffsetVariable->SetInt(m_EmitParticleRandomOffset);
					m_pCurrEmitterMatrixVariable->SetMatrix((FLOAT*)&emitterMatrix);
					m_pPrevEmitterMatrixVariable->SetMatrix((FLOAT*)&prevEmitterMatrix);

					D3DXVECTOR4 vEmitInterpScaleAndOffset;
					vEmitInterpScaleAndOffset.y = step_interp_offset + step_interp_scale * m_TimeToNextEmit/time_step;
					vEmitInterpScaleAndOffset.x = step_interp_scale * emit_interval/time_step;
					vEmitInterpScaleAndOffset.z = vEmitInterpScaleAndOffset.w = 0.f;
					m_pEmitInterpScaleAndOffsetVariable->SetFloatVector((FLOAT*)&vEmitInterpScaleAndOffset);

					D3DXVECTOR4 vEmitVelocityMinMaxAndSpread;
					vEmitVelocityMinMaxAndSpread.x = m_EmitMinVelocity;
					vEmitVelocityMinMaxAndSpread.y = m_EmitMaxVelocity;
					vEmitVelocityMinMaxAndSpread.z = m_EmitSpread;
					vEmitVelocityMinMaxAndSpread.w = 0.f;
					m_pEmitMinMaxVelocityAndSpreadVariable->SetFloatVector((FLOAT*)&vEmitVelocityMinMaxAndSpread);

					D3DXVECTOR4 vEmitAreaScale;
					vEmitAreaScale.x = m_EmitAreaScale.x;
					vEmitAreaScale.y = m_EmitAreaScale.y;
					vEmitAreaScale.z = 0.f;
					vEmitAreaScale.w = 0.f;
					m_pEmitAreaScaleVariable->SetFloatVector((FLOAT*)&vEmitAreaScale);

					const int num_groups = 1+ (num_particles_to_emit-1)/EmitParticlesCSBlocksSize;
					m_pEmitParticlesTechnique->GetPassByIndex(0)->Apply(0, pDC);
					pDC->Dispatch(num_groups,1,1);

					// Clear resource usage
					m_pSimulationInstanceDataVariable->SetUnorderedAccessView(NULL);
					m_pParticleDepthSortUAVVariable->SetUnorderedAccessView(NULL);
					m_pEmitParticlesTechnique->GetPassByIndex(0)->Apply(0, pDC);

					// Update particle counts/indices
					num_particles_emitted = num_particles_to_emit;
					m_EmitParticleIndex = (m_EmitParticleIndex + num_particles_to_emit) % m_MaxNumParticles;
					m_ActiveNumParticles = min(m_MaxNumParticles,m_ActiveNumParticles+num_particles_to_emit);
					m_EmitParticleRandomOffset = (m_EmitParticleRandomOffset + 2 * num_particles_to_emit) % NumRandomUV;
					m_TimeToNextEmit = m_TimeToNextEmit + float(num_particles_to_emit) * emit_interval - time_step;
				}
			}
		}
		
		// Do simulation (exclude just-emitted particles, which will have had pre-roll)
		const int num_particles_to_simulate = m_ActiveNumParticles - num_particles_emitted;
		if(num_particles_to_simulate) {

			int simulate_start_index = 0;
			if(m_ActiveNumParticles == m_MaxNumParticles) {
				simulate_start_index = m_EmitParticleIndex;
			}

			m_pTimeStepVariable->SetFloat(time_step);
			m_pParticleIndexOffsetVariable->SetInt(simulate_start_index);
			m_pParticleCountVariable->SetInt(num_particles_to_simulate);

			m_pSimulationInstanceDataVariable->SetUnorderedAccessView(m_pSimulationInstanceDataUAV);
			m_pSimulationVelocitiesVariable->SetUnorderedAccessView(m_pSimulationVelocitiesUAV);
			m_pParticleDepthSortUAVVariable->SetUnorderedAccessView(m_pDepthSort1UAV);

			const int num_groups = 1+ (num_particles_to_simulate-1)/SimulateParticlesCSBlocksSize;
			m_pSimulateParticlesTechnique->GetPassByIndex(0)->Apply(0, pDC);
			pDC->Dispatch(num_groups,1,1);

			// Clear resource usage
			m_pSimulationInstanceDataVariable->SetUnorderedAccessView(NULL);
			m_pParticleDepthSortUAVVariable->SetUnorderedAccessView(NULL);
			m_pSimulateParticlesTechnique->GetPassByIndex(0)->Apply(0, pDC);
		}

		// Evolve procedural noise
		m_NoiseTime += time_step;
	}

	// Update emitter matrix
	m_EmitterMatrix = emitterMatrix;
	m_EmitterMatrixIsValid = true;

	// Update wind
	m_WindVector = wind_vector;
	m_WindVectorIsValid = true;

	// Depth-sort ready for rendering
	if(m_ActiveNumParticles) {
		depthSortParticles(pDC);
	}
}

void OceanSmoke::renderToScene(	ID3D11DeviceContext* pDC,
								const CBaseCamera& camera,
								OceanPSM* pPSM,
								const OceanEnvironment& ocean_env
								)
{
	if(0==m_ActiveNumParticles)
		return;

	// Matrices
    D3DXMATRIX matView = *camera.GetViewMatrix();
	D3DXMATRIX matProj = *camera.GetProjMatrix();
	
	// View-proj
	m_pMatProjVariable->SetMatrix((FLOAT*)&matProj);
	m_pMatViewVariable->SetMatrix((FLOAT*)&matView);

	// Global lighting
	m_pLightDirectionVariable->SetFloatVector((FLOAT*)&ocean_env.main_light_direction);
	m_pLightColorVariable->SetFloatVector((FLOAT*)&ocean_env.main_light_color);
	m_pAmbientColorVariable->SetFloatVector((FLOAT*)&ocean_env.sky_color);

	// Lightnings
	m_pLightningColorVariable->SetFloatVector((FLOAT*)&ocean_env.lightning_light_intensity);
	m_pLightningPositionVariable->SetFloatVector((FLOAT*)&ocean_env.lightning_light_position);

	// Fog
	m_pFogExponentVariable->SetFloat(ocean_env.fog_exponent);

	// PSM
	pPSM->setReadParams(*m_pPSMParams,m_TintColor);

	D3DXMATRIX matInvView;
	D3DXMatrixInverse(&matInvView, NULL, &matView);
	D3DXMATRIX matViewToPSM = matInvView * *pPSM->getWorldToPSMUV();
	m_pMatViewToPSMVariable->SetMatrix((FLOAT*)&matViewToPSM);

	renderParticles(pDC,m_pRenderToSceneTechnique);

	pPSM->clearReadParams(*m_pPSMParams);
	m_pRenderToSceneTechnique->GetPassByIndex(0)->Apply(0,pDC);
}

void OceanSmoke::renderToPSM(	ID3D11DeviceContext* pDC,
								OceanPSM* pPSM,
								const OceanEnvironment& ocean_env
								)
{
	if(0==m_ActiveNumParticles)
		return;

	m_pMatViewVariable->SetMatrix((FLOAT*)pPSM->getPSMView());
	m_pMatProjVariable->SetMatrix((FLOAT*)pPSM->getPSMProj());
	pPSM->setWriteParams(*m_pPSMParams);

	renderParticles(pDC,m_pRenderToPSMTechnique);
}

void OceanSmoke::renderParticles(ID3D11DeviceContext* pDC, ID3DX11EffectTechnique* pTech)
{
	// Particle
	D3DXVECTOR4 vParticleBeginEndScale(m_ParticleBeginSize, m_ParticleEndSize, 0.f, 0.f);
	m_pParticleBeginEndScaleVariable->SetFloatVector((FLOAT*)&vParticleBeginEndScale);
	m_pTexDiffuseVariable->SetResource(m_pSmokeTextureSRV);
	m_pRenderInstanceDataVariable->SetResource(m_pRenderInstanceDataSRV);
	m_pParticleDepthSortSRVVariable->SetResource(m_pDepthSort1SRV);
	m_pInvParticleLifeTimeVariable->SetFloat(m_ActualEmitRate/float(m_MaxNumParticles));

	const int num_particles_to_render = m_ActiveNumParticles;
	pTech->GetPassByIndex(0)->Apply(0, pDC);
	pDC->IASetInputLayout(NULL);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	pDC->Draw(num_particles_to_render,0);

	// Clear resource usage
	m_pRenderInstanceDataVariable->SetResource(NULL);
	m_pParticleDepthSortSRVVariable->SetResource(NULL);
	pTech->GetPassByIndex(0)->Apply(0, pDC);
}

void OceanSmoke::depthSortParticles(ID3D11DeviceContext* pDC)
{
	int num_elements = 1;
	while(num_elements < m_ActiveNumParticles)
	{
		num_elements *= 2;
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
}

void OceanSmoke::setDepthSortConstants( UINT iLevel, UINT iLevelMask, UINT iWidth, UINT iHeight )
{
	m_piDepthSortLevelVariable->SetInt(iLevel);
	m_piDepthSortLevelMaskVariable->SetInt(iLevelMask);
	m_piDepthSortWidthVariable->SetInt(iWidth);
	m_piDepthSortHeightVariable->SetInt(iHeight);
}
