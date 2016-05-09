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

#ifndef _OCEAN_VESSEL_H
#define _OCEAN_VESSEL_H

#include <D3DX11Effect.h>

#include "GFSDK_WaveWorks_Types.h"
#include "SDKmesh.h"

#include <vector>

class OceanHullProfile;
struct OceanEnvironment;
class OceanSmoke;
class CBaseCamera;
class OceanSurfaceHeights;
class OceanHullSensors;
class OceanPSM;

class BoatMesh : public CDXUTSDKMesh 
{
public:

	// Hook to create input layout from mesh data
	HRESULT CreateInputLayout(	ID3D11Device* pd3dDevice,
								UINT iMesh, UINT iVB,
								const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength,
								ID3D11InputLayout** pIL
								);

	const SDKMESH_VERTEX_BUFFER_HEADER* getVBHeader(int i) const { return m_pVertexBufferArray + i; }
	const SDKMESH_INDEX_BUFFER_HEADER* getIBHeader(int i) const { return m_pIndexBufferArray + i; }
};

struct OceanVesselSubset {
	ID3D11Buffer* pIB;
	DXGI_FORMAT ib_format;
	int index_count;
};

struct OceanVesselDynamicState
{
	OceanVesselDynamicState();

	void setPosition(D3DXVECTOR2 pos);
	void setHeading(D3DXVECTOR2 heading, FLOAT speed);

	D3DXVECTOR2 m_Position;
	D3DXVECTOR2 m_NominalHeading;
	FLOAT m_Speed;

	FLOAT m_Pitch;
	FLOAT m_PitchRate;
	FLOAT m_Roll;
	FLOAT m_RollRate;
	FLOAT m_Yaw;
	FLOAT m_YawRate;
	FLOAT m_PrevSeaYaw;
	FLOAT m_Height;
	FLOAT m_HeightRate;

	UINT m_DynamicsCountdown;

	bool m_bFirstUpdate;

	D3DXMATRIX m_CameraToWorld;
	D3DXMATRIX m_LocalToWorld;
	D3DXMATRIX m_WakeToWorld;

private:

	void ResetDynamicState();
};

class OceanVessel
{
public:
	OceanVessel(OceanVesselDynamicState* pDynamicState);
	~OceanVessel();

	HRESULT init(LPCTSTR cfg_string, bool allow_smoke);
	void updateVesselMotion(ID3D11DeviceContext* pDC, GFSDK_WaveWorks_SimulationHandle hSim, FLOAT sea_level, FLOAT time_delta, FLOAT water_scale);

	void renderVesselToScene(	ID3D11DeviceContext* pDC,
								const D3DXMATRIX& matView,
								const D3DXMATRIX& matProj,
								const OceanEnvironment& ocean_env,
								const OceanVesselSubset* pSubsetOverride,
								bool wireframe
								);

	void renderReflectedVesselToScene(	ID3D11DeviceContext* pDC,
										const CBaseCamera& camera,
										const D3DXPLANE& world_reflection_plane,
										const OceanEnvironment& ocean_env
										);

	void updateVesselShadows(ID3D11DeviceContext* pDC);

	void updateVesselLightsInEnv(OceanEnvironment& env, const D3DXMATRIX& matView, float lighting_mult, int objectID = 0);

	void renderVesselToHullProfile(ID3D11DeviceContext* pDC, OceanHullProfile& profile);

	void renderHullProfileInUI(ID3D11DeviceContext* pDC);

	void updateSmokeSimulation(ID3D11DeviceContext* pDC, const CBaseCamera& camera, FLOAT time_delta, const D3DXVECTOR2& wind_dir, FLOAT wind_speed, FLOAT emit_rate_scale);

	void renderSmokeToScene(	ID3D11DeviceContext* pDC,
								const CBaseCamera& camera,
								const OceanEnvironment& ocean_env
								);

	void beginRenderToPSM(ID3D11DeviceContext* pDC, const D3DXVECTOR2& wind_dir);
	void endRenderToPSM(ID3D11DeviceContext* pDC);
	void renderSmokeToPSM(	ID3D11DeviceContext* pDC,
							const OceanEnvironment& ocean_env
							);

	void renderTextureToUI(ID3D11DeviceContext* pDC, ID3D11ShaderResourceView* pSRV);

	const D3DXMATRIX* getCameraXform() const { return &m_pDynamicState->m_CameraToWorld; }
	const D3DXMATRIX* getWorldXform() const { return &m_pDynamicState->m_LocalToWorld; }
	const D3DXMATRIX* getWakeToWorldXform() const { return &m_pDynamicState->m_WakeToWorld; }
	const D3DXMATRIX* getMeshToLocalXform() const { return &m_MeshToLocal; }

	FLOAT getLength() const { return m_Length; }

	BoatMesh* getMesh() const;

	OceanSurfaceHeights* getSurfaceHeights() const { return m_pSurfaceHeights; }

	OceanPSM* getPSM() const { return m_pPSM; }

	OceanHullSensors* getHullSensors() const { return m_pHullSensors; }

	float getYaw() const { return m_pDynamicState->m_Yaw; }
	float getYawRate() const { return m_pDynamicState->m_YawRate; }

private:

	// ---------------------------------- GPU shading data ------------------------------------

	// D3D objects
	ID3D11Device* m_pd3dDevice;

	BoatMesh* m_pMesh;
	FLOAT m_meshRenderScale;
	FLOAT m_DiffuseGamma;
	ID3D11InputLayout* m_pLayout;

	ID3DX11Effect* m_pFX;
	ID3DX11EffectTechnique* m_pRenderToSceneTechnique;
    ID3DX11EffectTechnique* m_pRenderToShadowMapTechnique;
	ID3DX11EffectTechnique* m_pRenderToHullProfileTechnique;
	ID3DX11EffectTechnique* m_pRenderQuadToUITechnique;
	ID3DX11EffectTechnique* m_pRenderQuadToCrackFixTechnique;
	ID3DX11EffectTechnique* m_pWireframeOverrideTechnique;
	ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable;
	ID3DX11EffectMatrixVariable* m_pMatWorldViewVariable;
	ID3DX11EffectMatrixVariable* m_pMatWorldVariable;
	ID3DX11EffectShaderResourceVariable *m_pTexDiffuseVariable;
	ID3DX11EffectShaderResourceVariable *m_pTexRustMapVariable;
	ID3DX11EffectShaderResourceVariable *m_pTexRustVariable;
	ID3DX11EffectShaderResourceVariable *m_pTexBumpVariable;
	ID3DX11EffectVectorVariable* m_pDiffuseColorVariable;
	ID3DX11EffectVectorVariable* m_pLightDirectionVariable;
	ID3DX11EffectVectorVariable* m_pLightColorVariable;
	ID3DX11EffectVectorVariable* m_pAmbientColorVariable;

	ID3DX11EffectScalarVariable* m_pSpotlightNumVariable;
	ID3DX11EffectVectorVariable* m_pSpotlightPositionVariable;
	ID3DX11EffectVectorVariable* m_pSpotLightAxisAndCosAngleVariable;
	ID3DX11EffectVectorVariable* m_pSpotlightColorVariable;
	ID3DX11EffectMatrixVariable* m_pSpotlightShadowMatrixVar;
	ID3DX11EffectShaderResourceVariable* m_pSpotlightShadowResourceVar;

	ID3DX11EffectVectorVariable* m_pLightningPositionVariable;
	ID3DX11EffectVectorVariable* m_pLightningColorVariable;

	ID3DX11EffectScalarVariable* m_pFogExponentVariable;

	ID3D11ShaderResourceView* m_pWhiteTextureSRV;
	ID3D11ShaderResourceView* m_pRustMapSRV;
	ID3D11ShaderResourceView* m_pRustSRV;
	ID3D11ShaderResourceView* m_pBumpSRV;

	ID3D11ShaderResourceView* m_pHullProfileSRV[2];
	ID3D11RenderTargetView* m_pHullProfileRTV[2];
	ID3D11DepthStencilView* m_pHullProfileDSV;

	void renderVessel(	ID3D11DeviceContext* pDC,
						ID3DX11EffectTechnique* pTechnique,
						const OceanVesselSubset* pSubsetOverride,
						bool wireframe,
						bool depthOnly);

	FLOAT m_Draft;
	FLOAT m_Length;
	FLOAT m_Beam;
	FLOAT m_MeshHeight;
	FLOAT m_BuoyantArea;
	FLOAT m_Mass;
	FLOAT m_PitchInertia;
	FLOAT m_RollInertia;
	FLOAT m_MetacentricHeight;
	FLOAT m_LongitudinalCOM;
	FLOAT m_MassMult;
	FLOAT m_PitchInertiaMult;
	FLOAT m_RollInertiaMult;

	FLOAT m_InitialPitch;
	FLOAT m_InitialHeight;

	D3DXVECTOR3 m_bbCentre;
	D3DXVECTOR3 m_bbExtents;

	OceanVesselDynamicState* m_pDynamicState;

	FLOAT m_HeightDrag;
	FLOAT m_PitchDrag;
	FLOAT m_YawDrag;
	FLOAT m_YawCoefficient;
	FLOAT m_RollDrag;

	FLOAT m_CameraHeightAboveWater;
	FLOAT m_CameraLongitudinalOffset;

	INT m_HullProfileTextureWH;

	LPTSTR m_MeshFileName;

	D3DXMATRIX m_MeshToLocal;
	D3DXMATRIX m_CameraToLocal;

	struct Spotlight{
		D3DVECTOR position;
		D3DVECTOR axis;
		float beam_angle;
		D3DVECTOR color;
	};

	std::vector<Spotlight> m_Spotlights;

    struct SpotlightShadow
    {
        SpotlightShadow()
            : m_pResource(NULL)
            , m_pDSV(NULL)
            , m_pSRV(NULL)
            , m_Dirty(true)
        {}

        ~SpotlightShadow()
        {
            SAFE_RELEASE(m_pResource);
            SAFE_RELEASE(m_pDSV);
            SAFE_RELEASE(m_pSRV);
        }

        D3DMATRIX                   m_ViewProjMatrix;
        bool                        m_Dirty;

        ID3D11Texture2D*            m_pResource;
        ID3D11DepthStencilView*     m_pDSV;
        ID3D11ShaderResourceView*   m_pSRV;
    };

    std::vector<SpotlightShadow> m_SpotlightsShadows;

	HRESULT parseConfig(LPCTSTR cfg_string);
	Spotlight* processGlobalConfigLine(const char* line);
	Spotlight* processSpotlightConfigLine(const char* line, Spotlight* pSpot);

	// Smoke sim
	FLOAT m_FunnelHeightAboveWater;
	FLOAT m_FunnelLongitudinalOffset;
	D3DXVECTOR2 m_FunnelMouthSize;
	D3DXMATRIX m_FunnelMouthToLocal;

	LPTSTR m_SmokeTextureFileName;
	INT m_NumSmokeParticles;
	FLOAT m_SmokeParticleEmitRate;
	FLOAT m_SmokeParticleBeginSize;
	FLOAT m_SmokeParticleEndSize;
	FLOAT m_SmokeParticleEmitMinVelocity;
	FLOAT m_SmokeParticleEmitMaxVelocity;
	FLOAT m_SmokeParticleEmitSpread;
	FLOAT m_SmokeParticleMinBuoyancy;
	FLOAT m_SmokeParticleMaxBuoyancy;
	FLOAT m_SmokeParticleCoolingRate;
	FLOAT m_SmokeWindDrag;
	FLOAT m_SmokePSMBoundsFadeMargin;
	FLOAT m_SmokeWindNoiseLevel;
	FLOAT m_SmokeWindNoiseSpatialScale;
	FLOAT m_SmokeWindNoiseTimeScale;
	D3DXVECTOR3 m_SmokeTint;
	FLOAT m_SmokeShadowOpacity;
	OceanSmoke* m_pSmoke;

	int m_PSMRes;
	D3DXVECTOR3 m_PSMMinCorner;
	D3DXVECTOR3 m_PSMMaxCorner;
	OceanPSM* m_pPSM;

	INT m_NumSurfaceHeightSamples;
	OceanSurfaceHeights* m_pSurfaceHeights;

	OceanHullSensors* m_pHullSensors;
	bool m_bFirstSensorUpdate;
};

#endif	// _OCEAN_VESSEL_H
