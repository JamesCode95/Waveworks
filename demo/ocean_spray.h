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

#ifndef _OCEAN_SPRAY_H
#define _OCEAN_SPRAY_H

#include <D3DX10.h>

#include "GFSDK_WaveWorks_Types.h"
#include "ocean_vessel.h"
#include "ocean_hull_sensors.h"

class OceanPSMParams;
class OceanPSM;

class OceanSpray
{
public:
	OceanSpray();
	~OceanSpray();

	enum { NUMSPRAYPARTICLES = 50000 };
	enum { NUMSPRAYGENERATORS = OceanHullSensors::MaxNumSensors };
	enum { NUMRIMSPRAYGENERATORS = OceanHullSensors::MaxNumRimSensors };

	HRESULT init(OceanHullSensors* pHullSensors);
	void updateSprayGenerators(GFSDK_WaveWorks_SimulationHandle hOceanSimulation, OceanVessel* pOceanVessel, float deltaTime, float kWaterScale, gfsdk_float2 wind_speed, float spray_mult);
	void updateSprayParticles(ID3D11DeviceContext* pDC, OceanVessel* pOceanVessel, float deltaTime, float kWaterScale, gfsdk_float2 wind_speed);

	float getSplashPowerFromLastUpdate() const { return m_SplashPowerFromLastUpdate; }

	void renderToScene(	ID3D11DeviceContext* pDC,
						OceanVessel* pOceanVessel,
						const CBaseCamera& camera,
						const OceanEnvironment& ocean_env,
						ID3D11ShaderResourceView* pDepthSRV = NULL,
						bool simple = false
						);

	void renderToPSM(	ID3D11DeviceContext* pDC,
						OceanPSM* pPSM,
						const OceanEnvironment& ocean_env
						);

	void renderToFoam(	ID3D11DeviceContext* pDC,
						const D3DXMATRIX& matWorldToFoam,
						OceanVessel* pOceanVessel,
						float deltaTime
						);

	UINT GetParticleBudget() const;

	UINT GetParticleCount(ID3D11DeviceContext* pDC);

	void UpdateSensorsVisualizationVertexBuffer(ID3D11DeviceContext* pDC, OceanVessel* pOceanVessel,float deltaTime);
	void RenderSensors(ID3D11DeviceContext* pDC, const CBaseCamera& camera, OceanVessel* pOceanVessel);

	void renderAudioVisualization(ID3D11DeviceContext* pDC, float l, float t, float r, float b, float h_margin, float v_margin, float level);

private:

	void initializeGenerators(OceanHullSensors* pHullSensors);

	void renderParticles(ID3D11DeviceContext* pDC, ID3DX11EffectTechnique* pTech, ID3D11ShaderResourceView* pSRV, D3D11_PRIMITIVE_TOPOLOGY topology);
	void simulateParticles(int begin_ix, int end_ix, float deltaTime, const D3DXVECTOR3& wind_speed_3d);

	void depthSortParticles(ID3D11DeviceContext* pDC);
	void setDepthSortConstants( UINT iLevel, UINT iLevelMask, UINT iWidth, UINT iHeight );

	ID3DX11Effect* m_pFX;
	ID3DX11EffectTechnique* m_pRenderSprayToSceneTechTechnique;
	ID3DX11EffectTechnique* m_pRenderSprayToPSMTechnique;
	ID3DX11EffectTechnique* m_pRenderSprayToFoamTechnique;
	ID3DX11EffectTechnique* m_pInitSortTechnique;
	ID3DX11EffectTechnique* m_pBitonicSortTechnique;
	ID3DX11EffectTechnique* m_pMatrixTransposeTechnique;
	ID3DX11EffectTechnique* m_pSensorVisualizationTechnique;
	ID3DX11EffectTechnique* m_pAudioVisualizationTechnique;

	ID3DX11EffectShaderResourceVariable* m_pTexSplashVariable;
	ID3DX11EffectShaderResourceVariable* m_pTexDepthVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderInstanceDataVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderVelocityAndTimeDataVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderOrientationAndDecimationDataVariable;
	ID3DX11EffectMatrixVariable* m_pMatProjVariable;
	ID3DX11EffectMatrixVariable* m_pMatViewVariable;
	ID3DX11EffectMatrixVariable* m_pMatProjInvVariable;
	ID3DX11EffectVectorVariable* m_pLightDirectionVariable;
	ID3DX11EffectVectorVariable* m_pLightColorVariable;
	ID3DX11EffectVectorVariable* m_pAmbientColorVariable;
	ID3DX11EffectScalarVariable* m_pFogExponentVariable;
	ID3DX11EffectScalarVariable* m_pInvParticleLifeTimeVariable;
	ID3DX11EffectScalarVariable* m_pSimpleParticlesVariable;

	ID3DX11EffectVectorVariable* m_pLightningPositionVariable;
	ID3DX11EffectVectorVariable* m_pLightningColorVariable;

	ID3DX11EffectScalarVariable* m_pSpotlightNumVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceSpotlightPositionVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceSpotLightAxisAndCosAngleVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceSpotlightColorVariable;
	ID3DX11EffectMatrixVariable* m_pSpotlightShadowMatrixVar;
	ID3DX11EffectShaderResourceVariable* m_pSpotlightShadowResourceVar;

	// Depth sort params
	ID3DX11EffectScalarVariable* m_piDepthSortLevelVariable;
	ID3DX11EffectScalarVariable* m_piDepthSortLevelMaskVariable;
	ID3DX11EffectScalarVariable* m_piDepthSortWidthVariable;
	ID3DX11EffectScalarVariable* m_piDepthSortHeightVariable;
	ID3DX11EffectUnorderedAccessViewVariable* m_pParticleDepthSortUAVVariable;
	ID3DX11EffectShaderResourceVariable* m_pParticleDepthSortSRVVariable;

	// PSM params
	ID3DX11EffectMatrixVariable* m_pMatViewToPSMVariable;
	OceanPSMParams* m_pPSMParams;

	// D3D objects
	ID3D11Device* m_pd3dDevice;
	ID3D11ShaderResourceView*  m_pSplashTextureSRV;
	ID3D11Buffer*              m_pRenderInstanceDataBuffer;
	ID3D11ShaderResourceView*  m_pRenderInstanceDataSRV;
	ID3D11Buffer*              m_pRenderVelocityDataBuffer;
	ID3D11ShaderResourceView*  m_pRenderVelocityDataSRV;
	ID3D11Buffer*              m_pRenderOrientationsAndDecimationsBuffer;
	ID3D11ShaderResourceView*  m_pRenderOrientationsAndDecimationsSRV;

	ID3D11ShaderResourceView* m_pDepthSort1SRV;
	ID3D11UnorderedAccessView* m_pDepthSort1UAV;
	ID3D11ShaderResourceView* m_pDepthSort2SRV;
	ID3D11UnorderedAccessView* m_pDepthSort2UAV;

	OceanHullSensors* m_pHullSensors;
	D3DXVECTOR3 	m_SprayGeneratorShipSpaceSprayDir[NUMSPRAYGENERATORS];
	D3DXVECTOR3		m_SprayGeneratorWorldSpaceSprayDir[NUMSPRAYGENERATORS];
	D3DXVECTOR4 	m_displacementsOld[NUMSPRAYGENERATORS];
	D3DXVECTOR3 	m_displacementsSpeed[NUMSPRAYGENERATORS];
	D3DXVECTOR3		m_WorldSensorPositionsOld[NUMSPRAYGENERATORS];

	ID3D11Buffer*	m_pSensorsVisualizationVertexBuffer;
	ID3D11Buffer*	m_pSensorsVisualizationIndexBuffer;
	ID3D11InputLayout* m_pSensorVisualizationLayout;

	int				m_sprayAmount[NUMSPRAYGENERATORS];
	float			m_sprayVizualisationFade[NUMSPRAYGENERATORS];
	D3DXVECTOR3		m_spraySpeed[NUMSPRAYGENERATORS];
	D3DXVECTOR3		m_sprayOffset[NUMSPRAYGENERATORS];

	D3DXVECTOR3 	m_hullSpeed[NUMSPRAYGENERATORS];

	D3DXVECTOR3		m_WorldRimSensorPositionsOld[NUMRIMSPRAYGENERATORS];
	D3DXVECTOR3 	m_RimSprayGeneratorShipSpaceSprayDir[NUMRIMSPRAYGENERATORS];
	D3DXVECTOR3		m_RimSprayGeneratorWorldSpaceSprayDir[NUMRIMSPRAYGENERATORS];
	int				m_rimSprayAmount[NUMRIMSPRAYGENERATORS];
	float			m_rimSprayVizualisationFade[NUMRIMSPRAYGENERATORS];

	D3DXVECTOR3		m_particlePosition[NUMSPRAYPARTICLES];
	D3DXVECTOR3		m_particleSpeed[NUMSPRAYPARTICLES];
	float			m_particleTime[NUMSPRAYPARTICLES];
	float			m_particleScale[NUMSPRAYPARTICLES];
	WORD			m_particleOrientations[2*NUMSPRAYPARTICLES];
	int				m_numParticlesAlive;

	bool			m_firstUpdate;

	// GPU simulation variables
	ID3D11Buffer*				m_pParticlesBuffer[2];
	ID3D11UnorderedAccessView*	m_pParticlesBufferUAV[2];
	ID3D11ShaderResourceView*	m_pParticlesBufferSRV[2];

	UINT						m_ParticleWriteBuffer;

	ID3D11Buffer*				m_pDrawParticlesCB;
	ID3D11Buffer*				m_pDrawParticlesBuffer;
	ID3D11UnorderedAccessView*	m_pDrawParticlesBufferUAV;
	ID3D11Buffer*				m_pDrawParticlesBufferStaging;

	ID3DX11EffectTechnique*		m_pInitSprayParticlesTechnique;
	ID3DX11EffectTechnique*		m_pSimulateSprayParticlesTechnique;
	ID3DX11EffectTechnique*		m_pDispatchArgumentsTechnique;

	ID3DX11EffectScalarVariable* m_pParticlesNum;
	ID3DX11EffectScalarVariable* m_pSimulationTime;
	ID3DX11EffectVectorVariable* m_pWindSpeed;
	ID3DX11EffectShaderResourceVariable* m_pParticlesBufferVariable;

	ID3DX11EffectMatrixVariable* m_pWorldToVesselVariable;
	ID3DX11EffectMatrixVariable* m_pVesselToWorldVariable;

	ID3DX11EffectVectorVariable* m_worldToHeightLookupScaleVariable;
	ID3DX11EffectVectorVariable* m_worldToHeightLookupRotVariable;
	ID3DX11EffectVectorVariable* m_worldToHeightLookupOffsetVariable;
	ID3DX11EffectShaderResourceVariable* m_texHeightLookupVariable;

	ID3DX11EffectMatrixVariable* m_pMatWorldToFoamVariable;

	ID3DX11EffectVectorVariable* m_pAudioVisualizationRectVariable;
	ID3DX11EffectVectorVariable* m_pAudioVisualizationMarginVariable;
	ID3DX11EffectScalarVariable* m_pAudioVisualizationLevelVariable;

	float m_SplashPowerFromLastUpdate;
};

#endif	// _OCEAN_SPRAY_H
