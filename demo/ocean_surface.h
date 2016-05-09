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

#ifndef _OCEAN_WAVE_H
#define _OCEAN_WAVE_H

#include <D3DX11Effect.h>

#include <GFSDK_WaveWorks.h>

class OceanHullProfile;
struct OceanEnvironment;
struct OceanSkyMapInfo;

//#define DEBUG_VS
//#define DEBUG_PS

// Ocean grid setting
#define BICOLOR_TEX_SIZE			256

#define LOCAL_FOAMMAP_TEX_SIZE			1024


struct OceanSurfaceParameters
{
	// Shading properties
    D3DXVECTOR4 waterbody_color;
    float sky_blending;
};

struct OceanSurfaceState
{
	D3DXMATRIX m_matView;
	D3DXMATRIX m_matProj;
};

class OceanSurface
{
public:
	OceanSurface(OceanSurfaceState* pSurfaceState);
	~OceanSurface();

	OceanSurfaceParameters m_params;

	HRESULT init(const OceanSurfaceParameters& params);

	// create color/fresnel lookup table.
	void createFresnelMap();
	
	// create local foam maps
	void OceanSurface::createLocalFoamMaps();

	// ---------------------------------- GPU shading data ------------------------------------

	// D3D objects
	ID3D11Device* m_pd3dDevice;
	ID3D11InputLayout* m_pQuadLayout;
	
	// Color look up 1D texture
	ID3D11ShaderResourceView* m_pBicolorMap;			// (RGBA8)
	ID3D11ShaderResourceView* m_pFoamIntensityMap;
	ID3D11ShaderResourceView* m_pFoamDiffuseMap;
	ID3D11ShaderResourceView* m_pWakeMap;
	ID3D11ShaderResourceView* m_pShipFoamMap;
	ID3D11ShaderResourceView* m_pGustMap;


	ID3DX11EffectTechnique*	  m_pShiftFadeBlurLocalFoamTechnique;
	ID3DX11EffectPass*		  m_pShiftFadeBlurLocalFoamShadedPass;
	ID3DX11EffectVectorVariable* m_pShiftFadeBlurLocalFoamUVOffsetBlurVariable;
	ID3DX11EffectScalarVariable* m_pShiftFadeBlurLocalFoamFadeAmountVariable;
	ID3DX11EffectShaderResourceVariable* m_pShiftFadeBlurLocalFoamTextureVariable;
	ID3D11Texture2D*		  m_pLocalFoamMapReceiver;
	ID3D11ShaderResourceView* m_pLocalFoamMapReceiverSRV;
	ID3D11RenderTargetView*   m_pLocalFoamMapReceiverRTV;
	ID3D11Texture2D*		  m_pLocalFoamMapFader;
	ID3D11ShaderResourceView* m_pLocalFoamMapFaderSRV;
	ID3D11RenderTargetView*   m_pLocalFoamMapFaderRTV;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceLocalFoamMapVariable;


	ID3DX11Effect* m_pOceanFX;
	ID3DX11EffectTechnique* m_pRenderSurfaceTechnique;
	ID3DX11EffectPass* m_pRenderSurfaceShadedPass;
	ID3DX11EffectPass* m_pRenderSurfaceWireframePass;
	ID3DX11EffectMatrixVariable* m_pRenderSurfaceMatViewProjVariable;
	ID3DX11EffectMatrixVariable* m_pRenderSurfaceMatViewVariable;
    ID3DX11EffectVectorVariable* m_pRenderSurfaceSkyColorVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceLightDirectionVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceLightColorVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceWaterColorVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfacePatchColorVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceColorMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceCubeMap0Variable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceCubeMap1Variable;
	ID3DX11EffectScalarVariable* m_pRenderSurfaceCubeBlendVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceCube0RotateSinCosVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceCube1RotateSinCosVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceCubeMultVariable;

	ID3DX11EffectScalarVariable* m_pSpotlightNumVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceSpotlightPositionVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceSpotLightAxisAndCosAngleVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceSpotlightColorVariable;
	ID3DX11EffectMatrixVariable* m_pSpotlightShadowMatrixVar;
	ID3DX11EffectShaderResourceVariable* m_pSpotlightShadowResourceVar;

	ID3DX11EffectScalarVariable* m_pRenderSurfaceFogExponentVariable;

	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceFoamIntensityMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceFoamDiffuseMapVariable;
	ID3DX11EffectScalarVariable* m_pGlobalFoamFadeVariable;

	ID3DX11EffectVectorVariable* m_pRenderSurfaceHullProfileCoordOffsetAndScaleVariable;
	ID3DX11EffectVectorVariable* m_pRenderSurfaceHullProfileHeightOffsetAndHeightScaleAndTexelSizeVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceHullProfileMapVariable;

	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceWakeMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceShipFoamMapVariable;
	ID3DX11EffectMatrixVariable*		 m_pRenderSurfaceWorldToShipVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceGustMapVariable;
	ID3DX11EffectVectorVariable*		 m_pRenderSurfaceGustUVVariable;

	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceReflectionTextureVariable;
	ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceReflectionDepthTextureVariable;
    ID3DX11EffectShaderResourceVariable* m_pRenderSurfaceReflectionPosTextureVariable;
	ID3DX11EffectVectorVariable*		 m_pRenderSurfaceScreenSizeInvVariable;

	ID3DX11EffectMatrixVariable*		 m_pRenderSurfaceSceneToShadowMapVariable;
	ID3DX11EffectVectorVariable*		 m_pRenderSurfaceLightningPositionVariable;
	ID3DX11EffectVectorVariable*		 m_pRenderSurfaceLightningColorVariable;

	ID3DX11EffectScalarVariable*		 m_pRenderSurfaceCloudFactorVariable;
	ID3DX11EffectScalarVariable*		 m_pRenderSurfaceShowSpraySimVariable;
	ID3DX11EffectScalarVariable*		 m_pRenderSurfaceShowFoamSimVariable;

	ID3DX11EffectScalarVariable*		 m_pRenderSurfaceGustsEnabledVariable;
	ID3DX11EffectScalarVariable*		 m_pRenderSurfaceWakeEnabledVariable;


	// --------------------------------- Rendering routines -----------------------------------

	// Rendering
	void renderShaded(	ID3D11DeviceContext* pDC,
						const D3DXMATRIX& matCameraView, const D3DXMATRIX& matView, const D3DXMATRIX& matProj,
						GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate,
						const OceanEnvironment& ocean_env,
						BOOL freeze_cam,
						float foam_fade,
						bool show_spray_sim,
						bool show_foam_sim,
						bool gusts_enabled,
						bool wake_enabled
						);
	void renderWireframe(	ID3D11DeviceContext* pDC,
							const D3DXMATRIX& matView, const D3DXMATRIX& matProj,
							GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_SavestateHandle hSavestate,
							BOOL freeze_cam
							);

    void simulateSprayParticles( ID3D11DeviceContext* pDC, float timeStep );

    void renderSprayParticles(  ID3D11DeviceContext* pDC,
						        const D3DXMATRIX& matView, const D3DXMATRIX& matProj,
						        const OceanEnvironment& ocean_env,
						        BOOL freeze_cam
						        );
	void getQuadTreeStats(GFSDK_WaveWorks_Quadtree_Stats& stats);
	void setHullProfiles(const OceanHullProfile* hullProfiles, UINT NumHullProfiles);
	void setWorldToShipMatrix(const D3DXMATRIX& matShip);

	void beginRenderToLocalFoamMap(ID3D11DeviceContext* pDC, D3DXMATRIX& matWorldToFoam);
	void endRenderToLocalFoamMap(ID3D11DeviceContext* pDC, D3DXVECTOR2 shift_amount, float blur_amount, float fade_amount);

	// --------------------------------- Surface geometry -----------------------------------
	GFSDK_WaveWorks_QuadtreeHandle m_hOceanQuadTree;
	HRESULT initQuadTree(const GFSDK_WaveWorks_Quadtree_Params& params);

	UINT* m_pQuadTreeShaderInputMappings_Shaded;
	UINT* m_pQuadTreeShaderInputMappings_Wireframe;

	UINT* m_pSimulationShaderInputMappings_Shaded;
	UINT* m_pSimulationShaderInputMappings_Wireframe;

	OceanSurfaceState* m_pSurfaceState;

	D3DXMATRIX m_matWorldToShip;

    ID3D11Buffer*               m_pParticlesBuffer[2];
    ID3D11UnorderedAccessView*  m_pParticlesBufferUAV[2];
    ID3D11ShaderResourceView*   m_pParticlesBufferSRV[2];

    UINT                        m_ParticleWriteBuffer;

    ID3D11Buffer*               m_pDrawParticlesCB;
    ID3D11Buffer*               m_pDrawParticlesBuffer;
    ID3D11UnorderedAccessView*  m_pDrawParticlesBufferUAV;
    
    ID3DX11EffectTechnique*     m_pRenderSprayParticlesTechnique;
    ID3DX11EffectTechnique*     m_pSimulateSprayParticlesTechnique;
    ID3DX11EffectTechnique*     m_pDispatchArgumentsTechnique;

    ID3DX11EffectVectorVariable* m_pViewRightVariable;
    ID3DX11EffectVectorVariable* m_pViewUpVariable;
    ID3DX11EffectVectorVariable* m_pViewForwardVariable;
    ID3DX11EffectScalarVariable* m_pTimeStep;

    ID3DX11EffectShaderResourceVariable* m_pParticlesBufferVariable;
};

#endif	// _OCEAN_WAVE_H
