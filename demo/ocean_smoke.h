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

#ifndef _OCEAN_SMOKE_H
#define _OCEAN_SMOKE_H

#include <D3DX11Effect.h>

struct OceanEnvironment;
class OceanPSM;
class CBaseCamera;
class OceanPSMParams;

class OceanSmoke
{
public:
	OceanSmoke();
	~OceanSmoke();

	HRESULT init(	ID3D11ShaderResourceView* pSmokeTexture,
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
					);

	// Particles are emitted over a unit x-y disk centred on the origin
	void updateSimulation(	ID3D11DeviceContext* pDC,
							const CBaseCamera& camera,
							FLOAT time_delta,
							const D3DXMATRIX& emitterMatrix,
							const D3DXVECTOR3& wind_vector,
							const FLOAT wind_noise,
							FLOAT emit_rate_scale
							);

	void renderToScene(	ID3D11DeviceContext* pDC,
						const CBaseCamera& camera,
						OceanPSM* pPSM,
						const OceanEnvironment& ocean_env
						);

	void renderToPSM(	ID3D11DeviceContext* pDC,
						OceanPSM* pPSM,
						const OceanEnvironment& ocean_env
						);

private:

	void renderParticles(ID3D11DeviceContext* pDC, ID3DX11EffectTechnique* pTech);

	void depthSortParticles(ID3D11DeviceContext* pDC);
	void setDepthSortConstants( UINT iLevel, UINT iLevelMask, UINT iWidth, UINT iHeight );

	ID3DX11Effect* m_pFX;
	ID3DX11EffectTechnique* m_pRenderToSceneTechnique;
	ID3DX11EffectTechnique* m_pRenderToPSMTechnique;
	ID3DX11EffectTechnique* m_pEmitParticlesTechnique;
	ID3DX11EffectTechnique* m_pSimulateParticlesTechnique;
	ID3DX11EffectTechnique* m_pBitonicSortTechnique;
	ID3DX11EffectTechnique* m_pMatrixTransposeTechnique;

	// Generic simulation/emit/sort params
	ID3DX11EffectScalarVariable* m_pParticleIndexOffsetVariable;
	ID3DX11EffectScalarVariable* m_pParticleCountVariable;
	ID3DX11EffectScalarVariable* m_pTimeStepVariable;
	ID3DX11EffectScalarVariable* m_pPreRollEndTimeVariable;
	ID3DX11EffectVectorVariable* m_pBuoyancyParamsVariable;
	ID3DX11EffectScalarVariable* m_pNoiseTimeVariable;
	ID3DX11EffectUnorderedAccessViewVariable* m_pParticleDepthSortUAVVariable;
	ID3DX11EffectShaderResourceVariable* m_pParticleDepthSortSRVVariable;

	// Emit params
	ID3DX11EffectUnorderedAccessViewVariable* m_pSimulationInstanceDataVariable;
	ID3DX11EffectUnorderedAccessViewVariable* m_pSimulationVelocitiesVariable;
	ID3DX11EffectShaderResourceVariable* m_pRandomUVVariable;
	ID3DX11EffectScalarVariable* m_pRandomOffsetVariable;
	ID3DX11EffectMatrixVariable* m_pCurrEmitterMatrixVariable;
	ID3DX11EffectMatrixVariable* m_pPrevEmitterMatrixVariable;
	ID3DX11EffectVectorVariable* m_pEmitInterpScaleAndOffsetVariable;
	ID3DX11EffectVectorVariable* m_pEmitMinMaxVelocityAndSpreadVariable;
	ID3DX11EffectVectorVariable* m_pEmitAreaScaleVariable;

	// Sim params
	ID3DX11EffectScalarVariable* m_pWindDragVariable;
	ID3DX11EffectVectorVariable* m_pWindVectorAndNoiseMultVariable;

	// Rendering params
	ID3DX11EffectShaderResourceVariable* m_pTexDiffuseVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderInstanceDataVariable;
	ID3DX11EffectMatrixVariable* m_pMatProjVariable;
	ID3DX11EffectMatrixVariable* m_pMatViewVariable;
	ID3DX11EffectVectorVariable* m_pLightDirectionVariable;
	ID3DX11EffectVectorVariable* m_pLightColorVariable;
	ID3DX11EffectVectorVariable* m_pAmbientColorVariable;
	ID3DX11EffectScalarVariable* m_pFogExponentVariable;
	ID3DX11EffectVectorVariable* m_pParticleBeginEndScaleVariable;
	ID3DX11EffectScalarVariable* m_pInvParticleLifeTimeVariable;

	ID3DX11EffectVectorVariable* m_pLightningPositionVariable;
	ID3DX11EffectVectorVariable* m_pLightningColorVariable;

	// Noise params
	ID3DX11EffectShaderResourceVariable* m_ppermTextureVariable;
	ID3DX11EffectShaderResourceVariable* m_pgradTexture4dVariable;

	// PSM params
	ID3DX11EffectMatrixVariable* m_pMatViewToPSMVariable;
	OceanPSMParams* m_pPSMParams;

	// Depth sort params
	ID3DX11EffectScalarVariable* m_piDepthSortLevelVariable;
	ID3DX11EffectScalarVariable* m_piDepthSortLevelMaskVariable;
	ID3DX11EffectScalarVariable* m_piDepthSortWidthVariable;
	ID3DX11EffectScalarVariable* m_piDepthSortHeightVariable;

	// D3D resources
	ID3D11Device* m_pd3dDevice;
	ID3D11ShaderResourceView* m_pSmokeTextureSRV;
	ID3D11ShaderResourceView* m_pRandomUVSRV;
	ID3D11ShaderResourceView* m_pRenderInstanceDataSRV;
	ID3D11UnorderedAccessView* m_pSimulationInstanceDataUAV;
	ID3D11UnorderedAccessView* m_pSimulationVelocitiesUAV;
	ID3D11ShaderResourceView* m_ppermTextureSRV;
	ID3D11ShaderResourceView* m_pgradTexture4dSRV;
	ID3D11ShaderResourceView* m_pDepthSort1SRV;
	ID3D11UnorderedAccessView* m_pDepthSort1UAV;
	ID3D11ShaderResourceView* m_pDepthSort2SRV;
	ID3D11UnorderedAccessView* m_pDepthSort2UAV;

	int m_MaxNumParticles;
	int m_ActiveNumParticles;
	int m_NumDepthSortParticles;
	int m_EmitParticleIndex;
	int m_EmitParticleRandomOffset;

	float m_EmitMinVelocity;
	float m_EmitMaxVelocity;
	float m_EmitSpread;
	float m_ParticleBeginSize;
	float m_ParticleEndSize;
	float m_NominalEmitRate;
	float m_ActualEmitRate;
	float m_TimeToNextEmit;
	float m_WindDrag;
	float m_MinBuoyancy;
	float m_MaxBuoyancy;
	float m_CoolingRate;
	float m_NoiseTime;
	D3DXVECTOR2 m_EmitAreaScale;

	D3DXVECTOR3 m_TintColor;

	D3DXMATRIX m_EmitterMatrix;
	bool m_EmitterMatrixIsValid;

	D3DXVECTOR3 m_WindVector;
	bool m_WindVectorIsValid;
};

#endif	// _OCEAN_SMOKE_H
