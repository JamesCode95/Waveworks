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
// documentation without an express license agreemeRent from NVIDIA Corporation is
// strictly prohibited.
//

#include "DXUT.h"
#include "resource.h"

#include <direct.h>

#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"

#include "ocean_surface.h"
#include "ocean_vessel.h"
#include "ocean_vessel_config.h"
#include "ocean_hull_profile.h"
#include "ocean_sky_map.h"
#include "ocean_env.h"
#include "ocean_surface_heights.h"
#include "atmospheric.h"
#include "ocean_PSM.h"
#include "ocean_spray.h"
#include "ocean_hull_sensors.h"
#include "GFSDK_WaveWorks.h"
#include "GFSDK_WaveWorks_D3D_Util.h"
#include "NVAudio.h"
#include "ocean_audio.h"

#define ENABLE_SSAO 1
#define ENABLE_TXAA 0
#define DOWNSAMPLE_SCALE 1

#define SCENE_SHADOWMAP_SIZE 1024 

enum { FMOD_logo_w = 100 };
enum { FMOD_logo_h = 45 };

#if ENABLE_SSAO
#include "GFSDK_SSAO.h"

GFSDK_SSAO_Status g_SSAO_Status = GFSDK_SSAO_OK;
GFSDK_SSAO_Context* g_pSSAO_Context = NULL;
GFSDK_SSAO_Parameters g_SSAO_Params;
#endif

bool g_TXAAisActive = false;
unsigned int g_TXAA_Status = 1;

#if ENABLE_TXAA
#include "Txaa.h"
TxaaCtxDX g_TXAA_Ctx;
#endif

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 

// Disable warning "conditional expression is constant"
#pragma warning(disable:4127)

extern HRESULT LoadFile(LPCTSTR FileName, ID3DXBuffer** ppBuffer);
extern HRESULT CreateTextureFromFileSRGB(
        ID3D11Device*               pDevice,
        LPCTSTR                     pSrcFile,
        ID3D11Resource**            ppTexture);

extern bool DXUTIsNextArg(  __inout WCHAR*& strCmdLine, 
							__inout WCHAR* strArg );

class CVesselBorneCamera : public CFirstPersonCamera
{
public:
	CVesselBorneCamera()
	{
		m_DoApplyVesselBorneXform = FALSE;
	}

	virtual void FrameMove( FLOAT fElapsedTime )
	{
		CFirstPersonCamera::FrameMove(fElapsedTime);
		if(m_DoApplyVesselBorneXform) {

			m_mCameraWorld._41 = 0.f;
			m_mCameraWorld._42 = 0.f;
			m_mCameraWorld._43 = 0.f;
			m_mCameraWorld = m_mCameraWorld * m_VesselBorneXform;


			//m_vEye.x = m_mCameraWorld._41;
			//m_vEye.y = m_mCameraWorld._42;
			//m_vEye.z = m_mCameraWorld._43;

			//m_vLookAt.x = m_vEye.x + m_mCameraWorld._31;
			//m_vLookAt.y = m_vEye.y + m_mCameraWorld._32;
			//m_vLookAt.z = m_vEye.z + m_mCameraWorld._33;


			//m_mCameraWorld = m_VesselBorneXform;
			D3DXMatrixInverse( &m_mView, NULL, &m_mCameraWorld );

			m_DoApplyVesselBorneXform = FALSE; // Until next time
		}
	}

	void SetVesselBorneXform(const D3DXMATRIX* pxform)
	{
		m_DoApplyVesselBorneXform = TRUE;
		m_VesselBorneXform = *pxform;
	}

	void AssimilateVesselBorneXform()
	{
		D3DXVECTOR3 eyePt = *GetEyePt();
		D3DXVECTOR3 lookAtPt = eyePt + *GetWorldAhead();

		SetViewParams(&eyePt,&lookAtPt);

		m_DoApplyVesselBorneXform = FALSE; // Until next time
	}

private:
	BOOL m_DoApplyVesselBorneXform;
	D3DXMATRIX m_VesselBorneXform;
};

//--------------------------------------------------------------------------------------
// Presets and preset state
//--------------------------------------------------------------------------------------

enum SkyMap { 
	SkyMap_Cloudy = 0,
	SkyMap_Grey,
	SkyMap_Storm,
	NumSkyMaps
};

OceanSkyMapInfo g_SkyMaps[NumSkyMaps];

struct Preset {
	WCHAR* preset_name;
	float wind_speed;				// In Beaufort
	float smoke_speed;				// In m/s
	float smoke_emit_rate_scale;
	float ship_lighting;
	float visibility_distance;
	SkyMap sky_map;
	D3DVECTOR sky_color_mult;
	D3DVECTOR dir_light_color;
	bool  lightnings_enabled;
	float lightning_avg_time_to_next_strike;
	float lightning_average_number_of_cstrikes;
	float cloud_factor;			// 0.0 - clear sky, 1.0 - overcast
	float time_of_day;			// in hours
	float upward_transition_time;
	float foam_spray_fade;
};

Preset kPresets[] = {
   //Name				  Wind speed     Smoke speed     Smoke rate mult    Ship lights    Vis       Sky map        Sky color mult     Dir light color      Lightning... -interval -number    Cloud   TOD    Trans time    Foam/spray fade
	{L"Light Air",		  1.5,           2.5f,           1.f,               1.f,           20000.f,  SkyMap_Cloudy, { 1.f, 1.f, 1.f},  {0.49f,0.49f,0.49f}, false,       0.0f,     0.0f,      0.0f,   5.5f,  3.f,          1.f             },
	{L"Light Breeze",	  2.0,           3.f,            1.f,               1.f,           10000.f,  SkyMap_Cloudy, { 1.f, 1.f, 1.f},  {0.49f,0.49f,0.49f}, false,       0.0f,     0.0f,      0.0f,   6.0f,  3.f,          1.f             },
	{L"Gentle Breeze",	  3.0,           4.f,            2.f,               1.f,           10000.f,  SkyMap_Grey,   {.81f,.81f,.81f},  {0.36f,0.36f,0.36f}, false,       0.0f,     0.0f,      0.0f,   8.0f,  3.f,          1.f             },
	{L"Fresh Breeze",     5.0,           6.f,            2.f,               0.f,           10000.f,  SkyMap_Grey,   {.81f,.81f,.81f},  {0.25f,0.25f,0.25f}, false,       0.0f,     0.0f,      1.0f,   8.0f,  5.f,          1.f             },
	{L"Moderate Gale",    7.0,           8.f,            2.f,               0.f,           2000.f,   SkyMap_Grey,   {.49f,.49f,.49f},  {0.16f,0.16f,0.16f}, false,       0.0f,     0.0f,      1.0f,   8.0f,  5.f,          1.f             },
	{L"Strong Gale",      9.0,           10.f,           3.f,               1.f,           800.f,    SkyMap_Storm,  {.25f,.30f,.42f},  {0.09f,0.09f,0.09f}, false,       0.0f,     0.0f,      1.0f,   9.0f,  5.f,          1.f             },
	{L"Storm",            10.0,          11.f,           4.f,               1.f,           400.f,    SkyMap_Storm,  {.12f,.16f,.25f},  {0.04f,0.04f,0.04f}, false,       0.0f,     0.0f,      1.0f,   10.0f, 5.f,          1.f             },
	{L"Violent Storm",    11.0,          12.f,           4.f,               1.f,           400.f,    SkyMap_Storm,  {.12f,.16f,.25f},  {0.04f,0.04f,0.04f}, true,        4.0f,     2.0f,      1.0f,   10.0f, 5.f,          1.f             },
	{L"Hurricane",	      12.0,          13.f,           4.f,               1.f,           400.f,    SkyMap_Storm,  {.12f,.16f,.25f},  {0.04f,0.04f,0.04f}, true,        2.0f,     5.0f,      1.0f,   10.0f, 5.f,          1.f             },
};


enum { NumPresets = sizeof(kPresets)/sizeof(kPresets[0]) };

float g_CurrentPreset = 0.f;
int g_TargetPreset = 0;

float FoamSprayFadeOfPreset(float preset)
{
	if(preset < 0.f) return kPresets[0].foam_spray_fade;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].foam_spray_fade;
	else {
		int lower = (int)floorf(preset);
		float blend = preset - float(lower);
		return (1.f-blend) * kPresets[lower].foam_spray_fade + blend * kPresets[lower+1].foam_spray_fade;
	}
}

float TransitionTimeOfPreset(float preset)
{
	if(preset < 0.f) return kPresets[0].upward_transition_time;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].upward_transition_time;
	else {
		int lower = (int)floorf(preset);
		return kPresets[lower].upward_transition_time;
	}
}

float LightningsEnabledOfPreset(float preset) {

	if(preset < 0.f) return kPresets[0].lightnings_enabled;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].lightnings_enabled;
	else {
		int lower = (int)floorf(preset);
		return kPresets[lower].lightnings_enabled;
	}
}

float AverageTimeToNextLightningStrikeOfPreset(float preset) {
	if(preset < 0.f) return kPresets[0].lightning_avg_time_to_next_strike;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].lightning_avg_time_to_next_strike;
	else {
		int lower = (int)floorf(preset);
		float blend = preset - float(lower);
		return (1.f-blend) * kPresets[lower].lightning_avg_time_to_next_strike + blend * kPresets[lower+1].lightning_avg_time_to_next_strike;
	}
}

float AverageNumberOfConsecutiveLightningStrikesOfPreset(float preset) {
	if(preset < 0.f) return kPresets[0].lightning_average_number_of_cstrikes;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].lightning_average_number_of_cstrikes;
	else {
		int lower = (int)floorf(preset);
		float blend = preset - float(lower);
		return (1.f-blend) * kPresets[lower].lightning_average_number_of_cstrikes + blend * kPresets[lower+1].lightning_average_number_of_cstrikes;
	}
}

float WindSpeedOfPreset(float preset) {

	if(preset < 0.f) return kPresets[0].wind_speed;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].wind_speed;
	else {
		int lower = (int)floorf(preset);
		float blend = preset - float(lower);
		return (1.f-blend) * kPresets[lower].wind_speed + blend * kPresets[lower+1].wind_speed;
	}
}

float SmokeSpeedOfPreset(float preset) {

	if(preset < 0.f) return kPresets[0].smoke_speed;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].smoke_speed;
	else {
		int lower = (int)floorf(preset);
		float blend = preset - float(lower);
		return (1.f-blend) * kPresets[lower].smoke_speed + blend * kPresets[lower+1].smoke_speed;
	}
}

float SmokeEmitRateScaleOfPreset(float preset) {

	if(preset < 0.f) return kPresets[0].smoke_emit_rate_scale;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].smoke_emit_rate_scale;
	else {
		int lower = (int)floorf(preset);
		float blend = preset - float(lower);
		return (1.f-blend) * kPresets[lower].smoke_emit_rate_scale + blend * kPresets[lower+1].smoke_emit_rate_scale;
	}
}

float ShipLightingOfPreset(float preset) {

	if(preset < 0.f) return kPresets[0].ship_lighting;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].ship_lighting;
	else {
		int lower = (int)floorf(preset);
		float blend = preset - float(lower);
		return (1.f-blend) * kPresets[lower].ship_lighting + blend * kPresets[lower+1].ship_lighting;
	}
}

float FogExponentOfVisibilityDistance(float vd) {
	return vd > 0.f ? logf(0.01f)/(vd*vd) : 0.f;
}

float FogExponentOfPreset(float preset) {

	if(preset < 0.f) return FogExponentOfVisibilityDistance(kPresets[0].visibility_distance);
	else if(preset >= float(NumPresets-1)) return FogExponentOfVisibilityDistance(kPresets[NumPresets-1].visibility_distance);
	else {
		int lower = (int)floorf(preset);
		float lower_vd = kPresets[lower].visibility_distance;
		float upper_vd = kPresets[lower+1].visibility_distance;
		float blend = preset - float(lower);
		if(lower_vd == 0.f || upper_vd == 0.f) {
			return (1.f-blend) * FogExponentOfVisibilityDistance(kPresets[lower].visibility_distance) + blend * FogExponentOfVisibilityDistance(kPresets[lower+1].visibility_distance);
		} else {
			float vd = expf((1.f-blend) * logf(lower_vd) + blend * logf(upper_vd));
			return FogExponentOfVisibilityDistance(vd);
		}
	}
}

D3DXVECTOR3 SkyColorMultOfPreset(float preset) {

	if(preset < 0.f) return kPresets[0].sky_color_mult;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].sky_color_mult;
	else {
		int lower = (int)floorf(preset);
		float blend = preset - float(lower);
		return (1.f-blend) * kPresets[lower].sky_color_mult + blend * kPresets[lower+1].sky_color_mult;
	}
}

D3DXVECTOR3 DirLightColorOfPreset(float preset) {

	if(preset < 0.f) return kPresets[0].dir_light_color;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].dir_light_color;
	else {
		int lower = (int)floorf(preset);
		float blend = preset - float(lower);
		return (1.f-blend) * kPresets[lower].dir_light_color + blend * kPresets[lower+1].dir_light_color;
	}
}

void SkyMapsOfPreset(float preset, SkyMap& lower_map, SkyMap& upper_map, float& map_blend) {
	if(preset < 0.f) {
		lower_map = kPresets[0].sky_map;
		upper_map = lower_map;
		map_blend = 0.f;
	}
	else if(preset >= float(NumPresets-1)) {
		lower_map = kPresets[NumPresets-1].sky_map;
		upper_map = lower_map;
		map_blend = 0.f;
	}
	else {
		int lower = (int)floorf(preset);
		lower_map = kPresets[lower].sky_map;
		upper_map = kPresets[lower+1].sky_map;
		map_blend = preset - float(lower);
	}
}

D3DXVECTOR3 SkyColorOfPreset(float preset) {

	SkyMap lower_map;
	SkyMap upper_map;
	float map_blend;
	SkyMapsOfPreset(preset, lower_map, upper_map, map_blend);

	D3DXVECTOR3 gross_color = (1.f-map_blend) * g_SkyMaps[lower_map].m_GrossColor + map_blend * g_SkyMaps[upper_map].m_GrossColor;
	D3DXVECTOR3 mult = SkyColorMultOfPreset(preset);

	return D3DXVECTOR3(mult.x * gross_color.x, mult.y * gross_color.y, mult.z * gross_color.z);
}

float TimeOfDayOfPreset(float preset) 
{
	if(preset < 0.f) return kPresets[0].time_of_day;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].time_of_day;
	else {
		int lower = (int)floorf(preset);
		float blend = preset - float(lower);
		return (1.f-blend) * kPresets[lower].time_of_day + blend * kPresets[lower+1].time_of_day;
	}
}

float CloudFactorOfPreset(float preset) 
{
	if(preset < 0.f) return kPresets[0].cloud_factor;
	else if(preset >= float(NumPresets-1)) return kPresets[NumPresets-1].cloud_factor;
	else {
		int lower = (int)floorf(preset);
		float blend = preset - float(lower);
		return (1.f-blend) * kPresets[lower].cloud_factor + blend * kPresets[lower+1].cloud_factor;
	}
}

D3DXVECTOR3 SunPositionOfTimeOfDay(float time) 
{
	float rtime = 3.14f*(time/12.0f) - 3.14f*0.5f;
	return D3DXVECTOR3(cos(rtime), 0,sin(rtime));
}

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
bool                    g_bShowHelp = false;    // If true, it renders the UI control text
bool					g_bShowCredits = false;
bool                    g_bShowStats = false;
CVesselBorneCamera      g_Camera;  
CDXUTDialogResourceManager g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg         g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*        g_pTxtHelper = NULL;    // Text helper
CDXUTDialog             g_HUD;                  // manages the 3D UI
CDXUTDialog             g_SampleUI;             // dialog for sample specific controls

DXGI_SWAP_CHAIN_DESC	g_SwapChainDesc;

ID3D11Texture2D*        g_pColorBuffer      = NULL;
ID3D11RenderTargetView* g_pColorBufferRTV   = NULL;
ID3D11ShaderResourceView* g_pColorBufferSRV = NULL;

ID3D11Texture2D*        g_pDepthBuffer      = NULL;
ID3D11DepthStencilView* g_pDepthBufferDSV   = NULL;
ID3D11DepthStencilView* g_pDepthBufferReadDSV   = NULL;
ID3D11ShaderResourceView* g_pDepthBufferSRV = NULL;

ID3D11Texture2D*        g_pDepthBufferResolved = NULL;
ID3D11DepthStencilView* g_pDepthBufferResolvedDSV = NULL;
ID3D11DepthStencilView* g_pDepthBufferResolvedReadDSV = NULL;
ID3D11ShaderResourceView* g_pDepthBufferResolvedSRV = NULL;

ID3D11Texture2D*        g_pColorBufferResolved = NULL;
ID3D11RenderTargetView* g_pColorBufferResolvedRTV = NULL;
ID3D11ShaderResourceView* g_pColorBufferResolvedSRV = NULL;

GFSDK_WaveWorks_SavestateHandle	g_hOceanSavestate = NULL;
GFSDK_WaveWorks_SimulationHandle g_hOceanSimulation = NULL;
OceanSurfaceState       g_OceanSurfState;
OceanSurface*			g_pOceanSurf = NULL;
OceanVesselDynamicState g_HeroVesselState;
OceanVessel*			g_pHeroVessel = NULL;
OceanVesselDynamicState g_CameraVesselState;
OceanVessel*			g_pCameraVessel = NULL;
OceanSpray*				g_pOceanSpray = NULL;
GFSDK_WaveWorks_Simulation_Params	g_ocean_simulation_param;
GFSDK_WaveWorks_Simulation_Settings	g_ocean_simulation_settings;
OceanSurfaceParameters		g_ocean_surface_param;
OceanEnvironment			g_ocean_env;
GFSDK_WaveWorks_Quadtree_Params		g_ocean_param_quadtree;
GFSDK_WaveWorks_Quadtree_Stats		g_ocean_stats_quadtree;
GFSDK_WaveWorks_Simulation_Stats    g_ocean_stats_simulation;
GFSDK_WaveWorks_Simulation_Stats    g_ocean_stats_simulation_filtered;
int   g_max_detail_level;

bool g_RenderSSAO = true;
bool g_bSimulateWater = true;
bool g_ForceKick = false;
bool g_ShowReadbackMarkers = false;
bool g_DebugCam = false;
bool g_bShowSpraySimulation = false;
bool g_bShowFoamSimulation = false;
bool g_bEnableFoamAndSpray = true;
bool g_bEnableSmoke = true;
bool g_ShowHUD = true;
bool g_bUseCustomTimeOfDay = false;
bool g_bWakeEnabled = true;
bool g_bGustsEnabled = true;
bool g_bRenderShip = true;
bool g_bShowSprayAudioLevel = false;
float g_recentMaxSplashPower = 0.f;

enum VesselMode {
	VesselMode_External = 0,
	VesselMode_OnBoard,
	Num_VesselModes
};
VesselMode g_VesselMode = VesselMode_External;

enum CloudMode {
	CloudMode_Auto = 0,
	CloudMode_Enable,
	CloudMode_Disable,
	Num_CloudModes
};
const WCHAR* kCloudModeNames[Num_CloudModes] = {
	L"Auto",
	L"Enabled",
	L"Disabled"
};
CloudMode g_CloudMode = CloudMode_Auto;

enum WaterRenderMode {
	WaterRenderMode_Off = 0,
	WaterRenderMode_Wireframe,
	WaterRenderMode_Solid,
	Num_WaterRenderModes
};
const WCHAR* kWaterRenderModeNames[Num_WaterRenderModes] = {
	L"Off",
	L"Wireframe",
	L"Solid"
};
WaterRenderMode g_WaterRenderMode = WaterRenderMode_Solid;

float g_TimeMultiplier = 1.f;   // For controlling bullet time during demo

ID3D11ShaderResourceView* g_pLogoTex = NULL;
ID3D11ShaderResourceView* g_pFMODLogoTex = NULL;
ID3DX11Effect* g_pSkyboxFX = NULL;

ID3DX11EffectTechnique* g_pDownsampleDepthTechnique = NULL;
ID3DX11EffectTechnique* g_pUpsampleParticlesTechnique = NULL;
ID3DX11EffectShaderResourceVariable* g_pColorMapVariable = NULL;
ID3DX11EffectShaderResourceVariable* g_pDepthMapVariable = NULL;
ID3DX11EffectShaderResourceVariable* g_pDepthMapMSVariable = NULL;
ID3DX11EffectMatrixVariable* g_pMatProjInvVariable = NULL;

ID3DX11EffectTechnique* g_pSkyBoxTechnique = NULL;
ID3DX11EffectShaderResourceVariable* g_pSkyBoxSkyCubeMap0Variable = NULL;
ID3DX11EffectShaderResourceVariable* g_pSkyBoxSkyCubeMap1Variable = NULL;
ID3DX11EffectScalarVariable* g_pSkyCubeBlendVariable = NULL;
ID3DX11EffectVectorVariable* g_pSkyCube0RotateSinCosVariable = NULL;
ID3DX11EffectVectorVariable* g_pSkyCube1RotateSinCosVariable = NULL;
ID3DX11EffectVectorVariable* g_pSkyCubeMultVariable = NULL;
ID3DX11EffectVectorVariable* g_pSkyBoxFogColorVariable = NULL;
ID3DX11EffectScalarVariable* g_pSkyBoxFogExponentVariable = NULL;
ID3DX11EffectMatrixVariable* g_pSkyBoxMatViewProjVariable = NULL;
ID3DX11EffectVectorVariable* g_pSkyBoxLightningColorVariable = NULL;
ID3DX11EffectVectorVariable* g_pSkyBoxSunPositionVariable = NULL;
ID3DX11EffectScalarVariable* g_pSkyBoxCloudFactorVariable = NULL;
ID3D11Buffer* g_pSkyBoxVB = NULL;
ID3D11Buffer* g_pLogoVB = NULL;
ID3D11Buffer* g_pFMODLogoVB = NULL;

// Reflection related global variables
ID3D11Texture2D			 *g_reflectionColorResource = NULL;
ID3D11ShaderResourceView *g_reflectionColorResourceSRV = NULL;
ID3D11RenderTargetView   *g_reflectionColorResourceRTV = NULL;

ID3D11Texture2D			 *g_reflectionDepthResource = NULL;
ID3D11DepthStencilView   *g_reflectionDepthResourceDSV = NULL;
ID3D11ShaderResourceView *g_reflectionDepthResourceSRV = NULL;

ID3D11Texture2D			 *g_pReflectionViewDepth = NULL;
ID3D11RenderTargetView   *g_pReflectionViewDepthRTV = NULL;
ID3D11ShaderResourceView *g_pReflectionViewDepthSRV = NULL;

// Scene shadowmap related global variables
ID3D11Texture2D			 *g_sceneShadowMapResource = NULL;
ID3D11ShaderResourceView *g_sceneShadowMapResourceSRV = NULL;
ID3D11DepthStencilView   *g_sceneShadowMapResourceDSV = NULL;


// These are borrowed from the effect in g_pOceanSurf
ID3DX11EffectTechnique* g_pLogoTechnique = NULL;
ID3DX11EffectShaderResourceVariable* g_pLogoTextureVariable = NULL;

ID3D11InputLayout* g_pSkyboxLayout = NULL;

ID3D11InputLayout* g_pMarkerLayout = NULL;
ID3DX11Effect* g_pMarkerFX = NULL;
ID3DX11EffectTechnique* g_pMarkerTechnique = NULL;
ID3DX11EffectMatrixVariable* g_pMarkerMatViewProjVariable = NULL;
ID3D11Buffer* g_pMarkerVB = NULL;
ID3D11Buffer* g_pMarkerIB = NULL;

float g_NearPlane = 1.0f;
float g_FarPlane = 20000.0f;
double g_TotalSimulatedTime = 100000000.0f;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3
#define IDC_WATERMODE           4
#define IDC_TOGGLEPAUSE			5
#define IDC_TOGGLESHOWSPRAYSIM  6
#define IDC_TOGGLESHOWFOAMSIM   7
#define IDC_WIND_SPEED_READOUT	8
#define IDC_TOGGLETIMEOFDAY     9
#define IDC_TIMEOFDAY_SLIDER	10
#define IDC_CLOUDSMODE			11
#define IDC_TOGGLESMOKE			12
#define IDC_TOGGLEFOAMSPRAY		13
#define IDC_TOGGLEWAKE  		14
#define IDC_TOGGLEGUSTS  		15
#define IDC_TOGGLESHIP  		16
#define IDC_TOGGLESHOWAUDIOLEVEL 17
#define IDC_TOGGLESTATS 		18
#define IDC_WEATHERPRESETS 		19
#define IDC_TIMEOFDAY_READOUT	20

const FLOAT kMinTimeOfDay = 5.5f;	// 5:30am
const FLOAT kMaxTimeOfDay = 10.f;	// 10:00am
const FLOAT kTimeOfDaySliderScaleFactor = 100.0f/(kMaxTimeOfDay-kMinTimeOfDay);

const FLOAT kVesselSpeed = 2.0f;

const FLOAT kWaterScale = 1.f;

float g_CustomTimeOfDay = kMinTimeOfDay;

float GetTimeOfDay()
{
	return g_bUseCustomTimeOfDay ? g_CustomTimeOfDay : TimeOfDayOfPreset(g_CurrentPreset);
}

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool    CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
bool    CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
bool    CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void    CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
void    CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void    CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void    CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void    CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void    CALLBACK OnD3D11DestroyDevice( void* pUserContext );

void    InitApp();
void    RenderText( ID3D11DeviceContext* pDC, double fTime );
void	AddGUISet();
void	DownsampleDepth(ID3D11DeviceContext* pDC);
void	InterpolateDepth(ID3D11DeviceContext* pDC);
void	RenderSkybox(ID3D11DeviceContext* pDC);
void	RenderLogo(ID3D11DeviceContext* pDC, ID3D11ShaderResourceView* pSRV, ID3D11Buffer* pVB);
void	RenderMarkers(ID3D11DeviceContext* pDC);
void	CycleVesselMode();
void	SetVesselMode_OnBoard();
void	SetVesselMode_External();
D3DXVECTOR3 GetOceanSurfaceLookAtPoint();

// Cursors are bizarrely problematic because:
// 1) The definition of ShowCursor makes it slightly difficult to use (you can't track the state of the internal 
//    counter directly).
// 2) The code in DXUT seems broken.  You cannot enable a cursor in fullscreen by calling DXUTSetCursorSettings.
// So just call the windoze API directly here and ignore DXUT's support.
class Cursor
{
public:
	Cursor(): m_Visible(true) {}
	void SetVisibility(bool v)
	{
		if (v != m_Visible)
		{
			m_Visible = v;
			ShowCursor(m_Visible);
		}
	}
	void ToggleVisibility()
	{
		SetVisibility(!m_Visible);
	}
private:
	bool m_Visible;
};

Cursor g_CursorVisibility;

//--------------------------------------------------------------------------------------
// Little util function to shave the end off a file system path
//--------------------------------------------------------------------------------------
bool upalevel(TCHAR* path)
{
	TCHAR* curr = path + _tcslen(path);
	while(curr > path)
	{
		if(*curr == '\\')
		{
			*curr = '\0';
			return true;
		}
		--curr;
	}

	return false;
}

//--------------------------------------------------------------------------------------
// Little util function to detect directory existence
//--------------------------------------------------------------------------------------
bool DirectoryExists(LPCTSTR szPath)
{
  DWORD dwAttrib = GetFileAttributes(szPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
         (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR cmdline, int )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	// App assumes later that working dir is run-dir. This will be true by default for an
	// exe launched from a packaged build of the demo, but during development the working dir
	// could be anywhere, so we take a couple of reasonable attempts to crawl up to our run-dir
	// from some assumed sub-dir exe location...
	TCHAR szExePath[MAX_PATH];
	GetModuleFileName(NULL,szExePath,MAX_PATH);
	if(upalevel(szExePath)) {
		_tchdir(szExePath);
		int num_attempts = 0;
		// Test for existence of ./media to figure out when we're int the right place
		while(!DirectoryExists(L".\\Media") && num_attempts < 2)
		{
			if(!upalevel(szExePath))
				break;
			_tchdir(szExePath);
			++num_attempts;
		}
	}

    // Set the callback functions. These functions allow DXUT to notify
    // the application about device changes, user input, and windows messages.  The 
    // callbacks are optional so you need only set callbacks for events you're interested 
    // in. However, if you don't handle the device reset/lost callbacks then the sample 
    // framework won't be able to reset your device since the application must first 
    // release all device resources before resetting.  Likewise, if you don't handle the 
    // device created/destroyed callbacks then DXUT won't be able to 
    // recreate your device resources.
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( KeyboardProc );
    DXUTSetCallbackFrameMove( OnFrameMove );

	DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

	DXUTSetIsInGammaCorrectMode(true);

    // Show the cursor and clip it when in full screen
    DXUTSetCursorSettings( true, true );

	DXUTSetHotkeyHandling(true,true,false);	// 'Pause' has dedicated checkbox in UI

    InitApp();

    // Initialize DXUT and create the desired Win32 window and Direct3D 
    // device for the application. Calling each of these functions is optional, but they
    // allow you to set several options which control the behavior of the framework.
    DXUTInit( true, true, NULL ); // Parse the command line, and show msgboxes
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"WaveWorks" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1280, 720 );

    // Pass control to DXUT for handling the message pump and 
    // dispatching render calls. DXUT will call your FrameMove 
    // and FrameRender callback when there is idle time between handling window messages.
    DXUTMainLoop();

    // Perform any application-level cleanup here. Direct3D device resources are released within the
    // appropriate callback functions and therefore don't require any cleanup code here.
	NVAudioTerminate();

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
	g_ocean_param_quadtree.min_patch_length		= 40.0f;
	g_ocean_param_quadtree.upper_grid_coverage	= 1024.f;
	g_ocean_param_quadtree.mesh_dim				= 128;
	g_ocean_param_quadtree.sea_level			= 0.0f; 
	g_ocean_param_quadtree.auto_root_lod		= 10;
	g_ocean_param_quadtree.tessellation_lod		= 60.0f;
	g_ocean_param_quadtree.geomorphing_degree	= 1.f;
	g_ocean_param_quadtree.use_tessellation		= true;
	g_ocean_param_quadtree.enable_CPU_timers	= true;

	g_ocean_simulation_param.time_scale				= 0.7f;//0.5f;
	g_ocean_simulation_param.wave_amplitude			= 0.7f;//1.0f;
	g_ocean_simulation_param.wind_dir					= NvFromDX(D3DXVECTOR2(0.8f, 0.6f));
	g_ocean_simulation_param.wind_speed				= WindSpeedOfPreset(g_CurrentPreset);
	g_ocean_simulation_param.wind_dependency			= 0.98f;
	g_ocean_simulation_param.choppy_scale				= 1.f;
	g_ocean_simulation_param.small_wave_fraction		= 0.f;
	g_ocean_simulation_param.foam_dissipation_speed = 0.6f;
	g_ocean_simulation_param.foam_falloff_speed = 0.985f;
	g_ocean_simulation_param.foam_generation_amount = 0.12f;
	g_ocean_simulation_param.foam_generation_threshold = 0.25f;

	g_ocean_simulation_settings.fft_period						= 1000.0f;
	g_ocean_simulation_settings.detail_level					= GFSDK_WaveWorks_Simulation_DetailLevel_Normal;
	g_ocean_simulation_settings.readback_displacements			= true;
	g_ocean_simulation_settings.num_readback_FIFO_entries		= 0;
	g_ocean_simulation_settings.aniso_level						= 4;
	g_ocean_simulation_settings.CPU_simulation_threading_model  = GFSDK_WaveWorks_Simulation_CPU_Threading_Model_Automatic;
	g_ocean_simulation_settings.use_Beaufort_scale				= true;
	g_ocean_simulation_settings.num_GPUs						= 1;
	g_ocean_simulation_settings.enable_CUDA_timers				= true;
	g_ocean_simulation_settings.enable_gfx_timers				= true;
	g_ocean_simulation_settings.enable_CPU_timers				= true;

	g_ocean_surface_param.waterbody_color		= D3DXVECTOR4(0.07f, 0.15f, 0.2f, 0);
	g_ocean_surface_param.sky_blending		= 100.0f;

	g_ocean_env.main_light_direction		= D3DXVECTOR3(0.6f,0.53f,0.6f);

	memset(&g_ocean_stats_simulation_filtered, 0, sizeof(g_ocean_stats_simulation_filtered));

    // Initialize dialogs
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

	g_Camera.SetRotateButtons( true, false, false );
	g_Camera.SetScalers(0.003f, 40.0f);

	const D3DCOLOR kHUDBackgroundColor = 0x7F000000;
	g_HUD.SetBackgroundColors(kHUDBackgroundColor);

	g_HUD.SetCallback( OnGUIEvent );
    g_SampleUI.SetCallback( OnGUIEvent );

	// Get on board
	SetVesselMode_OnBoard();

	// Init sky maps
	memset(g_SkyMaps, 0, sizeof(g_SkyMaps));
	g_SkyMaps[SkyMap_Cloudy].m_SkyDomeFileName = TEXT(".\\media\\sky_blue_cube.dds");
	g_SkyMaps[SkyMap_Cloudy].m_ReflectFileName = TEXT(".\\media\\reflect_sky_blue_cube.dds");
	g_SkyMaps[SkyMap_Cloudy].m_Orientation = 0.35f * D3DX_PI;
	g_SkyMaps[SkyMap_Cloudy].m_GrossColor = D3DXVECTOR3(0.42f,0.48f,0.55f);
	g_SkyMaps[SkyMap_Grey].m_SkyDomeFileName = TEXT(".\\media\\sky_grey_cube.dds");
	g_SkyMaps[SkyMap_Grey].m_ReflectFileName = TEXT(".\\media\\reflect_sky_grey_cube.dds");
	g_SkyMaps[SkyMap_Grey].m_Orientation = 0.5f * D3DX_PI;
	g_SkyMaps[SkyMap_Grey].m_GrossColor = D3DXVECTOR3(0.31f,0.35f,0.40f);
	g_SkyMaps[SkyMap_Storm].m_SkyDomeFileName = TEXT(".\\media\\sky_storm_cube.dds");
	g_SkyMaps[SkyMap_Storm].m_ReflectFileName = TEXT(".\\media\\reflect_sky_storm_cube.dds");
	g_SkyMaps[SkyMap_Storm].m_Orientation = 1.f * D3DX_PI;
	g_SkyMaps[SkyMap_Storm].m_GrossColor = D3DXVECTOR3(0.10f,0.12f,0.11f);

	InitOceanAudio();

	AddGUISet();

	// Turn vessels into the wind, like a good sailor :)
	g_HeroVesselState.setHeading(NvToDX(g_ocean_simulation_param.wind_dir), kVesselSpeed);			
	g_CameraVesselState.setHeading(NvToDX(g_ocean_simulation_param.wind_dir), kVesselSpeed);
}

void AddGUISet()
{
	int iY = 10;

	CDXUTComboBox* pComboBox;
	CDXUTSlider* pSlider;

	g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle Fullscreen", 10, iY, 150, 24);
    g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change Device", 10, iY += 30, 150, 24, VK_F2);
	g_HUD.AddStatic(IDC_WIND_SPEED_READOUT, L"Wind speed", 35, iY += 45, 130, 24);

	g_HUD.AddStatic(0, L"Weather", 10, iY += 45, 130, 24);
	g_HUD.AddComboBox(IDC_WEATHERPRESETS, 10, iY += 20, 150, 24, 0, false, &pComboBox);
	pComboBox->SetDropHeight(150);
	pComboBox->SetTextColor(D3DCOLOR_RGBA(0,0,0,255));
	for(int i=0;i<NumPresets;i++)
	{
		pComboBox->AddItem(kPresets[i].preset_name,(LPVOID) (i));
	}

	g_HUD.AddStatic(0, L"Weather effects", 10, iY += 30, 130, 24);
	g_HUD.AddComboBox(IDC_CLOUDSMODE, 10, iY += 20, 150, 24, 0, false, &pComboBox);
	pComboBox->SetDropHeight(50);
	pComboBox->SetTextColor(D3DCOLOR_RGBA(0,0,0,255));
	for(int i=0;i<Num_CloudModes;i++)
	{
		pComboBox->AddItem(kCloudModeNames[i],(LPVOID) (i));
	}
	pComboBox->SetSelectedByData((void*)g_CloudMode);

	g_HUD.AddStatic(IDC_TIMEOFDAY_READOUT, L"Time of Day", 10, iY += 30, 130, 24);
	g_HUD.AddSlider(IDC_TIMEOFDAY_SLIDER, 20, iY += 20, 60, 24, 0, 100, int((GetTimeOfDay()-kMinTimeOfDay) * kTimeOfDaySliderScaleFactor), false, &pSlider);
	pSlider->SetEnabled(g_bUseCustomTimeOfDay);
	g_HUD.AddCheckBox(IDC_TOGGLETIMEOFDAY, L"Auto", 100, iY, 40, 24, !g_bUseCustomTimeOfDay);

	g_HUD.AddStatic(0, L"Water", 10, iY += 50, 130, 24);
	g_HUD.AddComboBox(IDC_WATERMODE, 10, iY += 20, 150, 24, 0, false, &pComboBox);
	pComboBox->SetDropHeight(50);
	pComboBox->SetTextColor(D3DCOLOR_RGBA(0,0,0,255));
	for(int i=0;i<Num_WaterRenderModes;i++)
	{
		pComboBox->AddItem(kWaterRenderModeNames[i],(LPVOID) (i));
	}
	pComboBox->SetSelectedByData((void*)g_WaterRenderMode);

	// No stats in shipping version: g_HUD.AddCheckBox(IDC_TOGGLESTATS, L"Show Stats", 10, iY += 30, 130, 24, g_bShowStats);

	g_HUD.AddCheckBox(IDC_TOGGLESHIP, L"Ship", 10, iY += 60, 65, 24, g_bRenderShip);
	g_HUD.AddCheckBox(IDC_TOGGLESMOKE, L"Smoke", 85, iY, 65, 24, g_bEnableSmoke);
	g_HUD.AddCheckBox(IDC_TOGGLEWAKE, L"Wake", 10, iY += 27, 65, 24, g_bWakeEnabled);
	g_HUD.AddCheckBox(IDC_TOGGLEGUSTS, L"Gusts", 85, iY, 65, 24, g_bGustsEnabled);
	g_HUD.AddCheckBox(IDC_TOGGLEFOAMSPRAY, L"Foam/Spray", 10, iY += 27, 65, 24, g_bEnableFoamAndSpray);

	g_HUD.AddStatic(0, L"Visualizations", 10, iY += 50, 130, 24);
	g_HUD.AddCheckBox(IDC_TOGGLESHOWSPRAYSIM, L"Spray", 20, iY += 27, 65, 24, g_bShowSpraySimulation);
	g_HUD.AddCheckBox(IDC_TOGGLESHOWFOAMSIM, L"Foam", 95, iY, 65, 24, g_bShowFoamSimulation);
	g_HUD.AddCheckBox(IDC_TOGGLESHOWAUDIOLEVEL, L"Spray power", 20, iY += 27, 130, 24, g_bShowSprayAudioLevel);

	g_HUD.AddCheckBox(IDC_TOGGLEPAUSE, L"Pause", 10, iY += 45, 130, 24, DXUTIsTimePaused());
	g_DebugCam = DXUTIsTimePaused();	// We gang debug-cam and pause on the same UI
}

void UpdatePresetTransition(float fElapsedTime)
{
	const float fTargetPreset = (float)g_TargetPreset;
	if(g_CurrentPreset != fTargetPreset) 
	{
		const float trans_time = TransitionTimeOfPreset(g_CurrentPreset);
		const float trans_rate = 1.f/trans_time;
		const float max_delta = fElapsedTime * trans_rate;

		if(fTargetPreset > g_CurrentPreset) {
			float delta = fTargetPreset - g_CurrentPreset;
			if(delta < max_delta) g_CurrentPreset = fTargetPreset;
			else g_CurrentPreset += max_delta;
		} else {
			float delta = g_CurrentPreset - fTargetPreset;
			if(delta < max_delta) g_CurrentPreset = fTargetPreset;
			else g_CurrentPreset -= max_delta;
		}
		g_ocean_simulation_param.wind_speed = WindSpeedOfPreset(g_CurrentPreset);
		GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
		GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));

		if(!g_bUseCustomTimeOfDay) {
			g_CustomTimeOfDay = TimeOfDayOfPreset(g_CurrentPreset);
			g_HUD.GetSlider(IDC_TIMEOFDAY_SLIDER)->SetValue((int)(kTimeOfDaySliderScaleFactor*(g_CustomTimeOfDay-kMinTimeOfDay)));
		}
	}
}

//--------------------------------------------------------------------------------------
// Called during device initialization, this code checks the device for some 
// minimum set of capabilities, and rejects those that don't pass by returning E_FAIL.
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable(	D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, 
										D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
	return false;
}

bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
	// It seems that AdapterInfo->m_pAdapter cannot be trusted (DXUT bug?), so enumerate our own
	IDXGIFactory1* pFactory = DXUTGetDXGIFactory();
	IDXGIAdapter* pAdapter = NULL;
	HRESULT hr = pFactory->EnumAdapters(AdapterInfo->AdapterOrdinal,&pAdapter);
	if(FAILED(hr) || NULL == pAdapter)
		return false;

	// Check ocean capability
	bool result = GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_D3D11(pAdapter, g_ocean_simulation_settings.detail_level);
	SAFE_RELEASE(pAdapter);
    return result;
}


//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// DXUT will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise IDirect3D9::CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	IDXGIAdapter* adapter = NULL;
	DXGI_ADAPTER_DESC ad;
	DXUTGetDXGIFactory()->EnumAdapters(pDeviceSettings->d3d11.AdapterOrdinal,&adapter);
	adapter->GetDesc(&ad);

	pDeviceSettings->d3d11.AutoCreateDepthStencil = false;

	static bool s_bFirstTime = true;
	if( s_bFirstTime )
	{
		// 4xAA first time
		pDeviceSettings->d3d11.sd.SampleDesc.Count = 4;
		pDeviceSettings->d3d11.sd.SampleDesc.Quality = 0;
		pDeviceSettings->d3d11.SyncInterval = 1;
		s_bFirstTime = false;
	}

	g_SwapChainDesc = pDeviceSettings->d3d11.sd;

	//pDeviceSettings->d3d11.sd.SampleDesc.Count = 1;
	//pDeviceSettings->d3d11.sd.SampleDesc.Quality = 0;

	// Check detail level support
	int detail_level = 0;
	for(; detail_level != Num_GFSDK_WaveWorks_Simulation_DetailLevels; ++detail_level) {
		if(!GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_D3D11(adapter, (GFSDK_WaveWorks_Simulation_DetailLevel)detail_level))
			break;
	}
	if(0 == detail_level)
		return false;

	g_max_detail_level = (GFSDK_WaveWorks_Simulation_DetailLevel)(detail_level - 1);
	g_ocean_simulation_settings.detail_level = (GFSDK_WaveWorks_Simulation_DetailLevel)g_max_detail_level;

	SAFE_RELEASE(adapter);

	return true;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_SettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

	GFSDK_WaveWorks_InitD3D11(pd3dDevice,NULL,GFSDK_WAVEWORKS_API_GUID);

	// Ocean sim
	GFSDK_WaveWorks_Simulation_CreateD3D11(g_ocean_simulation_settings, g_ocean_simulation_param, pd3dDevice, &g_hOceanSimulation);
	GFSDK_WaveWorks_Savestate_CreateD3D11(GFSDK_WaveWorks_StatePreserve_All, pd3dDevice, &g_hOceanSavestate);
	g_ForceKick = true;

	// Ocean object
	g_pOceanSurf = new OceanSurface(&g_OceanSurfState);
	g_pOceanSurf->init(g_ocean_surface_param);
	g_pOceanSurf->initQuadTree(g_ocean_param_quadtree);
	GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));

	// Effect hooks borrowed from ocean object
	g_pLogoTechnique = g_pOceanSurf->m_pOceanFX->GetTechniqueByName("DisplayBufferTech");
	g_pLogoTextureVariable = g_pOceanSurf->m_pOceanFX->GetVariableByName("g_texBufferMap")->AsShaderResource();

	// Vessels
	g_pHeroVessel = new OceanVessel(&g_HeroVesselState);
	g_pHeroVessel->init(kVesselConfigString, true);
	g_pCameraVessel = new OceanVessel(&g_CameraVesselState);
	g_pCameraVessel->init(kVesselConfigString, false);

	// Set camera vessel down where the camera is currently looking
	static bool bFirstVesselInit = true;
	if(bFirstVesselInit) {

		const D3DXVECTOR2 vessel_position_2d = D3DXVECTOR2(0,0);
		g_CameraVesselState.setPosition(vessel_position_2d);

		// And hero to the side
		D3DXVECTOR2 vec_to_other(-g_ocean_simulation_param.wind_dir.y, g_ocean_simulation_param.wind_dir.x);
		vec_to_other += NvToDX(g_ocean_simulation_param.wind_dir);
		g_HeroVesselState.setPosition(vessel_position_2d + g_pCameraVessel->getLength() * vec_to_other);

		// Subsequent inits can use the existing dynamic state
		bFirstVesselInit = false;
	}

	// Skybox
    {
        ID3DXBuffer* pEffectBuffer = NULL;
        V_RETURN(LoadFile(TEXT(".\\Media\\skybox_d3d11.fxo"), &pEffectBuffer));
        V_RETURN(D3DX11CreateEffectFromMemory(pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, pd3dDevice, &g_pSkyboxFX));
        pEffectBuffer->Release();
    }

	g_pDownsampleDepthTechnique = g_pSkyboxFX->GetTechniqueByName("DownsampleDepthTech");
	g_pUpsampleParticlesTechnique = g_pSkyboxFX->GetTechniqueByName("UpsampleParticlesTech");
	g_pColorMapVariable = g_pSkyboxFX->GetVariableByName("g_texColor")->AsShaderResource();
	g_pDepthMapVariable = g_pSkyboxFX->GetVariableByName("g_texDepth")->AsShaderResource();
	g_pDepthMapMSVariable = g_pSkyboxFX->GetVariableByName("g_texDepthMS")->AsShaderResource();
	g_pMatProjInvVariable = g_pSkyboxFX->GetVariableByName("g_matProjInv")->AsMatrix();

	g_pSkyBoxTechnique = g_pSkyboxFX->GetTechniqueByName("SkyboxTech");
	g_pSkyBoxMatViewProjVariable = g_pSkyboxFX->GetVariableByName("g_matViewProj")->AsMatrix();
	g_pSkyBoxFogColorVariable = g_pSkyboxFX->GetVariableByName("g_FogColor")->AsVector();
	g_pSkyBoxFogExponentVariable = g_pSkyboxFX->GetVariableByName("g_FogExponent")->AsScalar();
	g_pSkyBoxSkyCubeMap0Variable = g_pSkyboxFX->GetVariableByName("g_texSkyCube0")->AsShaderResource();
	g_pSkyBoxSkyCubeMap1Variable = g_pSkyboxFX->GetVariableByName("g_texSkyCube1")->AsShaderResource();
	g_pSkyCubeBlendVariable = g_pSkyboxFX->GetVariableByName("g_SkyCubeBlend")->AsScalar();
	g_pSkyCube0RotateSinCosVariable = g_pSkyboxFX->GetVariableByName("g_SkyCube0RotateSinCos")->AsVector();
	g_pSkyCube1RotateSinCosVariable = g_pSkyboxFX->GetVariableByName("g_SkyCube1RotateSinCos")->AsVector();
	g_pSkyCubeMultVariable = g_pSkyboxFX->GetVariableByName("g_SkyCubeMult")->AsVector();
	g_pSkyBoxLightningColorVariable = g_pSkyboxFX->GetVariableByName("g_LightningColor")->AsVector();
	g_pSkyBoxSunPositionVariable = g_pSkyboxFX->GetVariableByName("g_LightPos")->AsVector();
	g_pSkyBoxCloudFactorVariable = g_pSkyboxFX->GetVariableByName("g_CloudFactor")->AsScalar();

	const D3D11_INPUT_ELEMENT_DESC sky_layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	const UINT num_layout_elements = sizeof(sky_layout)/sizeof(sky_layout[0]);

	D3DX11_PASS_DESC PassDesc;
	V_RETURN(g_pSkyBoxTechnique->GetPassByIndex(0)->GetDesc(&PassDesc));

	V_RETURN(pd3dDevice->CreateInputLayout(	sky_layout, num_layout_elements,
												PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize,
												&g_pSkyboxLayout
												));

	ID3D11Resource* pD3D11Resource = NULL;
	for(int i = 0; i != NumSkyMaps; ++i) {
		OceanSkyMapInfo& sm = g_SkyMaps[i];
		V_RETURN(CreateTextureFromFileSRGB(pd3dDevice, sm.m_SkyDomeFileName, &pD3D11Resource));
		V_RETURN(pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &sm.m_pSkyDomeSRV));
		SAFE_RELEASE(pD3D11Resource);
		V_RETURN(CreateTextureFromFileSRGB(pd3dDevice, sm.m_ReflectFileName, &pD3D11Resource));
		V_RETURN(pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &sm.m_pReflectionSRV));
		SAFE_RELEASE(pD3D11Resource);
	}

	V_RETURN(CreateTextureFromFileSRGB(pd3dDevice, TEXT(".\\Media\\nvidia_logo.dds"), &pD3D11Resource));
	V_RETURN(pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &g_pLogoTex));
	SAFE_RELEASE(pD3D11Resource);

	V_RETURN(CreateTextureFromFileSRGB(pd3dDevice, TEXT(".\\Media\\fmod_logo.dds"), &pD3D11Resource));
	V_RETURN(pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &g_pFMODLogoTex));
	SAFE_RELEASE(pD3D11Resource);

	{
		D3D11_BUFFER_DESC vBufferDesc;
		vBufferDesc.ByteWidth = 4 * sizeof(D3DXVECTOR4);
		vBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vBufferDesc.MiscFlags = 0;
		V_RETURN(pd3dDevice->CreateBuffer(&vBufferDesc, NULL, &g_pSkyBoxVB));
	}

	// Readback marker
    {
        ID3DXBuffer* pEffectBuffer = NULL;
        V_RETURN(LoadFile(TEXT(".\\Media\\ocean_marker_d3d11.fxo"), &pEffectBuffer));
        V_RETURN(D3DX11CreateEffectFromMemory(pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, pd3dDevice, &g_pMarkerFX));
        pEffectBuffer->Release();
    }

	g_pMarkerTechnique = g_pMarkerFX->GetTechniqueByName("RenderMarkerTech");
	g_pMarkerMatViewProjVariable = g_pMarkerFX->GetVariableByName("g_matViewProj")->AsMatrix();

	const D3D11_INPUT_ELEMENT_DESC marker_layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	const UINT num_marker_layout_elements = sizeof(marker_layout)/sizeof(marker_layout[0]);

	V_RETURN(g_pMarkerTechnique->GetPassByIndex(0)->GetDesc(&PassDesc));

	V_RETURN(pd3dDevice->CreateInputLayout(	marker_layout, num_marker_layout_elements,
												PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize,
												&g_pMarkerLayout
												));

	{
		D3D11_BUFFER_DESC vBufferDesc;
		vBufferDesc.ByteWidth = 5 * sizeof(D3DXVECTOR4);
		vBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vBufferDesc.MiscFlags = 0;
		V_RETURN(pd3dDevice->CreateBuffer(&vBufferDesc, NULL, &g_pMarkerVB));
	}

	{
		static const WORD indices[] = {0,1,2, 0,2,3, 0,3,4, 0,4,1};

		D3D11_BUFFER_DESC iBufferDesc;
		iBufferDesc.ByteWidth = sizeof(indices);
		iBufferDesc.Usage = D3D11_USAGE_IMMUTABLE ;
		iBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		iBufferDesc.CPUAccessFlags = 0;
		iBufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA iBufferData;
		iBufferData.pSysMem = indices;
		iBufferData.SysMemPitch = 0;
		iBufferData.SysMemSlicePitch = 0;

		V_RETURN(pd3dDevice->CreateBuffer(&iBufferDesc, &iBufferData, &g_pMarkerIB));
	}

#if ENABLE_SSAO
    g_SSAO_Status = GFSDK_SSAO_CreateContext(pd3dDevice, &g_pSSAO_Context);

    g_SSAO_Params.Radius = 2.0f;
	g_SSAO_Params.Bias = 0.0f;
    g_SSAO_Params.PowerExponent = 3.0f;
	g_SSAO_Params.DetailAO = 1.f;
	g_SSAO_Params.CoarseAO = 1.f;
	g_SSAO_Params.GPUConfiguration = GFSDK_SSAO_SINGLE_GPU;
	g_SSAO_Params.DepthThreshold.Enable = FALSE;
	g_SSAO_Params.Blur.Enable = TRUE;
	g_SSAO_Params.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_4;
	g_SSAO_Params.Blur.Sharpness = 4.f;
	g_SSAO_Params.Blur.SharpnessProfile.Enable = FALSE;
	g_SSAO_Params.Output.BlendMode = GFSDK_SSAO_MULTIPLY_RGB;
#endif

#if ENABLE_TXAA
	ID3D11DeviceContext* pContext;
	pd3dDevice->GetImmediateContext(&pContext);
	g_TXAA_Status = TxaaOpenDX(&g_TXAA_Ctx, pd3dDevice, pContext);
	pContext->Release();
#endif
	
	// Spray
	g_pOceanSpray = new OceanSpray();
	g_pOceanSpray->init(g_pHeroVessel->getHullSensors());

	return S_OK;
}

void ReleaseViewDependentResources()
{
	// MSAA targets
	SAFE_RELEASE(g_pColorBuffer);
	SAFE_RELEASE(g_pColorBufferRTV);
	SAFE_RELEASE(g_pColorBufferSRV);

	SAFE_RELEASE(g_pDepthBuffer);
	SAFE_RELEASE(g_pDepthBufferDSV);
	SAFE_RELEASE(g_pDepthBufferReadDSV);
	SAFE_RELEASE(g_pDepthBufferSRV);

	// Resolved targets
	SAFE_RELEASE(g_pColorBufferResolved);
	SAFE_RELEASE(g_pColorBufferResolvedRTV);
	SAFE_RELEASE(g_pColorBufferResolvedSRV);

	SAFE_RELEASE(g_pDepthBufferResolved);
	SAFE_RELEASE(g_pDepthBufferResolvedDSV);
	SAFE_RELEASE(g_pDepthBufferResolvedReadDSV);
	SAFE_RELEASE(g_pDepthBufferResolvedSRV);

	SAFE_RELEASE(g_reflectionColorResource);
	SAFE_RELEASE(g_reflectionColorResourceSRV);
	SAFE_RELEASE(g_reflectionColorResourceRTV);
	SAFE_RELEASE(g_reflectionDepthResource);
	SAFE_RELEASE(g_reflectionDepthResourceDSV);
	SAFE_RELEASE(g_reflectionDepthResourceSRV);
	SAFE_RELEASE(g_sceneShadowMapResource);
	SAFE_RELEASE(g_sceneShadowMapResourceDSV);
	SAFE_RELEASE(g_sceneShadowMapResourceSRV);

	SAFE_RELEASE(g_pReflectionViewDepth);
	SAFE_RELEASE(g_pReflectionViewDepthRTV);
	SAFE_RELEASE(g_pReflectionViewDepthSRV);
}

HRESULT CreateViewDependentResources( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
	HRESULT hr;

	ReleaseViewDependentResources();

#if ENABLE_TXAA
	g_TXAAisActive = g_TXAA_Status == TXAA_RETURN_OK &&
		g_SwapChainDesc.SampleDesc.Count == 4 &&
		g_SwapChainDesc.SampleDesc.Quality == 0;
#endif

	{ // MSAA targets
#if 0
		{
			CD3D11_TEXTURE2D_DESC desc(pBackBufferSurfaceDesc->Format, pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
			desc.SampleDesc = g_SwapChainDesc.SampleDesc;
			V_RETURN(pd3dDevice->CreateTexture2D(&desc, NULL, &g_pColorBuffer));

			CD3D11_RENDER_TARGET_VIEW_DESC descRTV(desc.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D, pBackBufferSurfaceDesc->Format);
			V_RETURN(pd3dDevice->CreateRenderTargetView(g_pColorBuffer, &descRTV, &g_pColorBufferRTV));

			CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(desc.SampleDesc.Count > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D, pBackBufferSurfaceDesc->Format);
			V_RETURN(pd3dDevice->CreateShaderResourceView(g_pColorBuffer, &descSRV, &g_pColorBufferSRV));
		}
#endif

		{
			CD3D11_TEXTURE2D_DESC desc(DXGI_FORMAT_R24G8_TYPELESS, pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL);
			desc.SampleDesc = g_SwapChainDesc.SampleDesc;
			V_RETURN(pd3dDevice->CreateTexture2D(&desc, NULL, &g_pDepthBuffer));

			CD3D11_DEPTH_STENCIL_VIEW_DESC descDSV(desc.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D24_UNORM_S8_UINT);
			V_RETURN(pd3dDevice->CreateDepthStencilView(g_pDepthBuffer, &descDSV, &g_pDepthBufferDSV));

			descDSV.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;
			V_RETURN(pd3dDevice->CreateDepthStencilView(g_pDepthBuffer, &descDSV, &g_pDepthBufferReadDSV));

			CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(desc.SampleDesc.Count > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
			V_RETURN(pd3dDevice->CreateShaderResourceView(g_pDepthBuffer, &descSRV, &g_pDepthBufferSRV));
		}
	}

	//if (g_TXAAisActive) // Resolved targets
	{
		UINT bufferWidth = pBackBufferSurfaceDesc->Width / DOWNSAMPLE_SCALE;
		UINT bufferHeight = pBackBufferSurfaceDesc->Height / DOWNSAMPLE_SCALE;

		{
			CD3D11_TEXTURE2D_DESC desc(DXGI_FORMAT_R32_TYPELESS, bufferWidth, bufferHeight, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL);
			V_RETURN(pd3dDevice->CreateTexture2D(&desc, NULL, &g_pDepthBufferResolved));

			CD3D11_DEPTH_STENCIL_VIEW_DESC descDSV(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D32_FLOAT);
			V_RETURN(pd3dDevice->CreateDepthStencilView(g_pDepthBufferResolved, &descDSV, &g_pDepthBufferResolvedDSV));

			descDSV.Flags = D3D11_DSV_READ_ONLY_DEPTH;
			V_RETURN(pd3dDevice->CreateDepthStencilView(g_pDepthBufferResolved, &descDSV, &g_pDepthBufferResolvedReadDSV));

			CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R32_FLOAT);
			V_RETURN(pd3dDevice->CreateShaderResourceView(g_pDepthBufferResolved, &descSRV, &g_pDepthBufferResolvedSRV));
		}

		{
			DXGI_FORMAT format = pBackBufferSurfaceDesc->Format;
			CD3D11_TEXTURE2D_DESC desc(format, bufferWidth, bufferHeight, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
			V_RETURN(pd3dDevice->CreateTexture2D(&desc, NULL, &g_pColorBufferResolved));

			CD3D11_RENDER_TARGET_VIEW_DESC descRTV(D3D11_RTV_DIMENSION_TEXTURE2D, format);
			V_RETURN(pd3dDevice->CreateRenderTargetView(g_pColorBufferResolved, &descRTV, &g_pColorBufferResolvedRTV));

			CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURE2D, format);
			V_RETURN(pd3dDevice->CreateShaderResourceView(g_pColorBufferResolved, &descSRV, &g_pColorBufferResolvedSRV));
		}
	}

	// (Re)creating reflection buffers
	D3D11_TEXTURE2D_DESC tex_desc;
	D3D11_SHADER_RESOURCE_VIEW_DESC textureSRV_desc;
	D3D11_DEPTH_STENCIL_VIEW_DESC DSV_desc;
	float reflection_buffer_size_multiplier=1.0f;

	ZeroMemory(&textureSRV_desc,sizeof(textureSRV_desc));
	ZeroMemory(&tex_desc,sizeof(tex_desc));

	tex_desc.Width              = (UINT)(pBackBufferSurfaceDesc->Width*reflection_buffer_size_multiplier);
	tex_desc.Height             = (UINT)(pBackBufferSurfaceDesc->Height*reflection_buffer_size_multiplier);
	tex_desc.MipLevels          = (UINT)max(1,log(max((float)tex_desc.Width,(float)tex_desc.Height))/(float)log(2.0f));
	tex_desc.ArraySize          = 1;
	tex_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	tex_desc.SampleDesc.Count   = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage              = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags     = 0;
	tex_desc.MiscFlags          = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	textureSRV_desc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	textureSRV_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	textureSRV_desc.Texture2D.MostDetailedMip = 0;

	V_RETURN(pd3dDevice->CreateTexture2D( &tex_desc, NULL, &g_reflectionColorResource));
	V_RETURN(pd3dDevice->CreateShaderResourceView( g_reflectionColorResource, &textureSRV_desc, &g_reflectionColorResourceSRV));
	V_RETURN(pd3dDevice->CreateRenderTargetView( g_reflectionColorResource, NULL, &g_reflectionColorResourceRTV));

	tex_desc.Width              = (UINT)(pBackBufferSurfaceDesc->Width*reflection_buffer_size_multiplier);
	tex_desc.Height             = (UINT)(pBackBufferSurfaceDesc->Height*reflection_buffer_size_multiplier);
	tex_desc.MipLevels          = 1;
	tex_desc.ArraySize          = 1;
	tex_desc.Format             = DXGI_FORMAT_R32_TYPELESS;
	tex_desc.SampleDesc.Count   = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage              = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags          = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags     = 0;
	tex_desc.MiscFlags          = 0;

	DSV_desc.Format  = DXGI_FORMAT_D32_FLOAT;
	DSV_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DSV_desc.Flags = 0;
	DSV_desc.Texture2D.MipSlice = 0;

	textureSRV_desc.Format                    = DXGI_FORMAT_R32_FLOAT;
	textureSRV_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	textureSRV_desc.Texture2D.MostDetailedMip = 0;

	V_RETURN(pd3dDevice->CreateTexture2D( &tex_desc, NULL, &g_reflectionDepthResource));
	V_RETURN(pd3dDevice->CreateDepthStencilView(g_reflectionDepthResource, &DSV_desc, &g_reflectionDepthResourceDSV));
	V_RETURN(pd3dDevice->CreateShaderResourceView(g_reflectionDepthResource, &textureSRV_desc, &g_reflectionDepthResourceSRV));

	{
		CD3D11_TEXTURE2D_DESC desc(DXGI_FORMAT_R16G16B16A16_FLOAT, tex_desc.Width, tex_desc.Height, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
		V_RETURN(pd3dDevice->CreateTexture2D(&desc, NULL, &g_pReflectionViewDepth));

		CD3D11_RENDER_TARGET_VIEW_DESC descRTV(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R16G16B16A16_FLOAT);
		V_RETURN(pd3dDevice->CreateRenderTargetView(g_pReflectionViewDepth, &descRTV, &g_pReflectionViewDepthRTV));

		CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R16G16B16A16_FLOAT);
		V_RETURN(pd3dDevice->CreateShaderResourceView(g_pReflectionViewDepth, &descSRV, &g_pReflectionViewDepthSRV));
	}

	// (Re)creating shadowmap buffers
	tex_desc.Width              = (UINT)(SCENE_SHADOWMAP_SIZE);
	tex_desc.Height             = (UINT)(SCENE_SHADOWMAP_SIZE);
	tex_desc.MipLevels          = 1;
	tex_desc.ArraySize          = 1;
	tex_desc.Format             = DXGI_FORMAT_R32_TYPELESS;
	tex_desc.SampleDesc.Count   = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage              = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags          = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags     = 0;
	tex_desc.MiscFlags          = 0;

	DSV_desc.Format  = DXGI_FORMAT_D32_FLOAT;
	DSV_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DSV_desc.Flags = 0;
	DSV_desc.Texture2D.MipSlice = 0;

	textureSRV_desc.Format                    = DXGI_FORMAT_R32_FLOAT;
	textureSRV_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	textureSRV_desc.Texture2D.MostDetailedMip = 0;

	V_RETURN(pd3dDevice->CreateTexture2D( &tex_desc, NULL, &g_sceneShadowMapResource));
	V_RETURN(pd3dDevice->CreateDepthStencilView(g_sceneShadowMapResource, &DSV_desc, &g_sceneShadowMapResourceDSV));
	V_RETURN(pd3dDevice->CreateShaderResourceView(g_sceneShadowMapResource, &textureSRV_desc, &g_sceneShadowMapResourceSRV));

#if ENABLE_TXAA
	if (g_TXAA_Status == TXAA_RETURN_OK)
	{

	}
#endif


    return hr;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

	BOOL fullScreen = true;
	IDXGIOutput* pUnused=NULL;
	pSwapChain->GetFullscreenState(&fullScreen, &pUnused);
	//g_CursorVisibility.SetVisibility(!fullScreen);

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice,pBackBufferSurfaceDesc) );
    V_RETURN( g_SettingsDlg.OnD3D11ResizedSwapChain(pd3dDevice,pBackBufferSurfaceDesc) );

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(D3DX_PI/5, fAspectRatio, g_NearPlane, g_FarPlane);

	CreateViewDependentResources(pd3dDevice, pBackBufferSurfaceDesc);

	// UI
	g_HUD.SetLocation(pBackBufferSurfaceDesc->Width-180, 8);
	g_HUD.SetSize(172, 704);

	// Create Vb for nvidia logo
	float width = (float)200/pBackBufferSurfaceDesc->Width;
	float height = (float)160/pBackBufferSurfaceDesc->Height;
	float fmod_width = 2.f*(float)FMOD_logo_w/pBackBufferSurfaceDesc->Width;
	float fmod_height = 2.f*(float)FMOD_logo_h/pBackBufferSurfaceDesc->Height;
	float half_tex = 0;
	float vertices0[] = {0.94f - width, -0.96f + height, 0,    half_tex,      half_tex,
						 0.94f - width, -0.96f,          0,    half_tex,      half_tex+1.0f,
						 0.94f,         -0.96f + height, 0,    half_tex+1.0f, half_tex,
						 0.94f,         -0.96f,          0,    half_tex+1.0f, half_tex+1.0f};

	D3D11_BUFFER_DESC vBufferDesc;
	vBufferDesc.ByteWidth = sizeof(vertices0);
	vBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vBufferDesc.CPUAccessFlags = 0;
	vBufferDesc.MiscFlags = 0;
	vBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vSrd;
	vSrd.pSysMem = vertices0;
	vSrd.SysMemPitch = 0;
	vSrd.SysMemSlicePitch = 0;

	V_RETURN(pd3dDevice->CreateBuffer(&vBufferDesc, &vSrd, &g_pLogoVB));

	const float fmod_left = -1.f + 2.f * float((pBackBufferSurfaceDesc->Width-180)/2 - 150)/float(pBackBufferSurfaceDesc->Width);
	const float fmod_top = 1.f - 2.f * float(450+22)/float(pBackBufferSurfaceDesc->Height);

	float fmodvertices0[] = {fmod_left,              fmod_top,               0,    half_tex,      half_tex,
							 fmod_left,              fmod_top - fmod_height, 0,    half_tex,      half_tex+1.0f,
							 fmod_left + fmod_width, fmod_top,               0,    half_tex+1.0f, half_tex,
							 fmod_left + fmod_width, fmod_top - fmod_height, 0,    half_tex+1.0f, half_tex+1.0f};

	D3D11_SUBRESOURCE_DATA vFMODSrd;
	vFMODSrd.pSysMem = fmodvertices0;
	vFMODSrd.SysMemPitch = 0;
	vFMODSrd.SysMemSlicePitch = 0;

	V_RETURN(pd3dDevice->CreateBuffer(&vBufferDesc, &vFMODSrd, &g_pFMODLogoVB));

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	UpdatePresetTransition(fElapsedTime);

	// Update the wind speed readout
	const UINT buffer_len = 256;
	WCHAR buffer[buffer_len];
	swprintf_s(buffer,buffer_len,L"BEAUFORT: %0.1f", g_ocean_simulation_param.wind_speed);
	g_HUD.GetStatic(IDC_WIND_SPEED_READOUT)->SetText(buffer);
	g_HUD.GetStatic(IDC_WIND_SPEED_READOUT)->SetTextColor(0xFFFFFF00);

	// Update the time of day readout
	float tod = GetTimeOfDay();
	const int tod_hours = ((int)tod)%12;
	const WCHAR* tod_ampm = tod >= 12.f ? L"pm" : L"am";
	const int tod_minutes = (int)floor(60.f*(tod-floor(tod)));
	swprintf_s(buffer,buffer_len,L"Time of day: %d:%02d%s", tod_hours, tod_minutes, tod_ampm);
	g_HUD.GetStatic(IDC_TIMEOFDAY_READOUT)->SetText(buffer);

	// Cap sim time to make debugging less horrible!
	if(fElapsedTime > 0.1f) {
		g_TimeMultiplier = 0.1f/fElapsedTime;
	} else {
		g_TimeMultiplier = 1.f;
	}

	ID3D11DeviceContext* pDC = DXUTGetD3D11DeviceContext();
	if(g_bSimulateWater || g_ForceKick || (gfsdk_waveworks_result_NONE==GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hOceanSimulation,NULL)))
	{
		// Fill the simulation pipeline - this loop should execute once in all cases except the first
		// iteration, when the simulation pipeline is first 'primed'
		do {
			GFSDK_WaveWorks_Simulation_SetTime(g_hOceanSimulation, g_TotalSimulatedTime);
			GFSDK_WaveWorks_Simulation_KickD3D11(g_hOceanSimulation, NULL, pDC, g_hOceanSavestate);
			g_TotalSimulatedTime += (double)g_TimeMultiplier * (double)fElapsedTime;
		} while(gfsdk_waveworks_result_NONE==GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hOceanSimulation,NULL));
		GFSDK_WaveWorks_Savestate_RestoreD3D11(g_hOceanSavestate, pDC);
		g_ForceKick = false;
	}

	g_ocean_env.activeLightsNum = 0;

	g_pHeroVessel->updateVesselMotion(pDC, g_hOceanSimulation, g_ocean_param_quadtree.sea_level, fElapsedTime * g_TimeMultiplier, kWaterScale);
	g_pCameraVessel->updateVesselMotion(pDC, g_hOceanSimulation, g_ocean_param_quadtree.sea_level, fElapsedTime * g_TimeMultiplier, kWaterScale);

	// Put camera on vessel 0
	if(g_VesselMode == VesselMode_OnBoard) {
		g_Camera.SetVesselBorneXform(g_pCameraVessel->getCameraXform());
	}

	g_Camera.FrameMove( fElapsedTime );

	// Update lights in env
	float ship_lighting = ShipLightingOfPreset(g_CurrentPreset);
	if(tod<6.5f) ship_lighting = min(1.0f, ship_lighting + 6.5f - tod);

	D3DXMATRIX viewMatrix = *g_Camera.GetViewMatrix();
	g_pHeroVessel->updateVesselLightsInEnv(g_ocean_env, viewMatrix, ship_lighting, 0);

	g_pOceanSurf->setWorldToShipMatrix(*g_pHeroVessel->getWakeToWorldXform());

	const bool is_paused = DXUTGetGlobalTimer()->IsStopped();
	UpdateOceanAudio(fTime, fElapsedTime, g_ocean_simulation_param.wind_speed, g_pOceanSpray->getSplashPowerFromLastUpdate(), is_paused);

	// Run peak meter for splash power
	g_recentMaxSplashPower = max(g_recentMaxSplashPower,g_pOceanSpray->getSplashPowerFromLastUpdate());
	g_recentMaxSplashPower *= expf(-1.5f*fElapsedTime);
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pDC, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
    // getting simulation timings
	GFSDK_WaveWorks_Simulation_GetStats(g_hOceanSimulation,g_ocean_stats_simulation);

	// exponential filtering for stats
	g_ocean_stats_simulation_filtered.CPU_main_thread_wait_time			= g_ocean_stats_simulation_filtered.CPU_main_thread_wait_time*0.98f + 0.02f*g_ocean_stats_simulation.CPU_main_thread_wait_time;
	g_ocean_stats_simulation_filtered.CPU_threads_start_to_finish_time  = g_ocean_stats_simulation_filtered.CPU_threads_start_to_finish_time*0.98f + 0.02f*g_ocean_stats_simulation.CPU_threads_start_to_finish_time;
	g_ocean_stats_simulation_filtered.CPU_threads_total_time			= g_ocean_stats_simulation_filtered.CPU_threads_total_time*0.98f + 0.02f*g_ocean_stats_simulation.CPU_threads_total_time;
	g_ocean_stats_simulation_filtered.GPU_simulation_time				= g_ocean_stats_simulation_filtered.GPU_simulation_time*0.98f + 0.02f*g_ocean_stats_simulation.GPU_simulation_time;
	g_ocean_stats_simulation_filtered.GPU_FFT_simulation_time			= g_ocean_stats_simulation_filtered.GPU_FFT_simulation_time*0.98f + 0.02f*g_ocean_stats_simulation.GPU_FFT_simulation_time;
	g_ocean_stats_simulation_filtered.GPU_gfx_time						= g_ocean_stats_simulation_filtered.GPU_gfx_time*0.98f + 0.02f*g_ocean_stats_simulation.GPU_gfx_time;
	g_ocean_stats_simulation_filtered.GPU_update_time					= g_ocean_stats_simulation_filtered.GPU_update_time*0.98f + 0.02f*g_ocean_stats_simulation.GPU_update_time;

    // If the settings dialog is being shown, then
    // render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

	// Igniting the ligntnings
	float cloud_factor = CloudFactorOfPreset(g_CurrentPreset);
	switch(g_CloudMode) {
		case CloudMode_Enable:
			cloud_factor = 1.f;
			break;
		case CloudMode_Disable:
			cloud_factor = 0.f;
			break;
		default:
			break;
	}
	if((LightningsEnabledOfPreset(g_CurrentPreset))&&
		(g_ocean_env.lightning_time_to_next_strike < 0) &&
		(cloud_factor==1.0f))
	{
		g_ocean_env.lightning_current_cstrike=0;
		g_ocean_env.lightning_num_of_cstrikes = rand()%(int)AverageNumberOfConsecutiveLightningStrikesOfPreset(g_CurrentPreset);
		g_ocean_env.lightning_time_to_next_strike = AverageTimeToNextLightningStrikeOfPreset(g_CurrentPreset)*(0.5f+1.5f*(float)rand()/(float)RAND_MAX);
		g_ocean_env.lightning_time_to_next_cstrike = 0.25f*(float)rand()/(float)RAND_MAX;
		g_ocean_env.lightning_light_intensity = ((float)rand()/(float)RAND_MAX)*D3DXVECTOR3(20.0f,20.0f,20.0f);
		g_ocean_env.lightning_light_position.x = 2000.0f*(-0.5f + (float)rand()/(float)RAND_MAX) - 2.0f*g_ocean_simulation_param.wind_dir.x*(float)g_TotalSimulatedTime;
		g_ocean_env.lightning_light_position.y = 2000.0f*(-0.5f + (float)rand()/(float)RAND_MAX) - 2.0f*g_ocean_simulation_param.wind_dir.y*(float)g_TotalSimulatedTime;
		g_ocean_env.lightning_light_position.z = 200.0f;
		PlayLightningSound();
	}
	if((g_ocean_env.lightning_current_cstrike<g_ocean_env.lightning_num_of_cstrikes) && (g_ocean_env.lightning_time_to_next_cstrike<0))
	{
		g_ocean_env.lightning_light_intensity = ((float)rand()/(float)RAND_MAX)*D3DXVECTOR3(20.0f,20.0f,20.0f);
		g_ocean_env.lightning_time_to_next_cstrike = 0.25f*(float)rand()/(float)RAND_MAX;
		g_ocean_env.lightning_light_position.x += 400.0f*(-0.5f + (float)rand()/(float)RAND_MAX);
		g_ocean_env.lightning_light_position.y += 400.0f*(-0.5f + (float)rand()/(float)RAND_MAX);
		g_ocean_env.lightning_light_position.z = 200.0f;
		g_ocean_env.lightning_current_cstrike++;
	}

	// dimming the lightning over time
	g_ocean_env.lightning_light_intensity.x *=pow(0.95f,300.0f*fElapsedTime);
	g_ocean_env.lightning_light_intensity.y *=pow(0.945f,300.0f*fElapsedTime);
	g_ocean_env.lightning_light_intensity.z *=pow(0.94f,300.0f*fElapsedTime);

	// updating pauses between lightnings
	g_ocean_env.lightning_time_to_next_strike -= fElapsedTime;
	g_ocean_env.lightning_time_to_next_cstrike -= fElapsedTime;

	SkyMap lower_map;
	SkyMap upper_map;
	float map_interp;
	SkyMapsOfPreset(g_CurrentPreset,lower_map,upper_map,map_interp);
	g_ocean_env.pSky0 = &g_SkyMaps[lower_map];
	g_ocean_env.pSky1 = &g_SkyMaps[upper_map];
	g_ocean_env.sky_interp = map_interp;
	g_ocean_env.fog_exponent = FogExponentOfPreset(g_CurrentPreset);
	g_ocean_env.sky_map_color_mult = SkyColorMultOfPreset(g_CurrentPreset);

	// Wind gusts
	g_ocean_env.gust_UV.x -= 4.0f*fElapsedTime*g_ocean_simulation_param.wind_dir.x*SmokeSpeedOfPreset(g_CurrentPreset);
	g_ocean_env.gust_UV.y -= 4.0f*fElapsedTime*g_ocean_simulation_param.wind_dir.y*SmokeSpeedOfPreset(g_CurrentPreset);
	g_ocean_env.gust_UV.z += 2.0f*fElapsedTime*g_ocean_simulation_param.wind_dir.x*SmokeSpeedOfPreset(g_CurrentPreset);
	g_ocean_env.gust_UV.w += 2.0f*fElapsedTime*g_ocean_simulation_param.wind_dir.y*SmokeSpeedOfPreset(g_CurrentPreset);

	// Sun position
	const float tod = GetTimeOfDay();
	D3DXVECTOR3 sun_position = SunPositionOfTimeOfDay(tod);
	g_ocean_env.main_light_direction.x = sun_position.x;
	g_ocean_env.main_light_direction.y = sun_position.y;
	g_ocean_env.main_light_direction.z = sun_position.z;

	// Atmospheric colors
	D3DXVECTOR3 clear_sun_color;
	D3DXVECTOR3 clear_ambient_color;
	D3DXVECTOR3 cloudy_sun_color;
	D3DXVECTOR3 cloudy_ambient_color;

	// Clear sky colors
	AtmosphereColorsType atmospheric_colors = CalculateAtmosphericScattering(sun_position,sun_position,30.0f);
	clear_sun_color = atmospheric_colors.RayleighColor;
	atmospheric_colors = CalculateAtmosphericScattering(D3DXVECTOR3(0,0,1.0f),sun_position,20.0f);
	clear_ambient_color = atmospheric_colors.RayleighColor;
	//de-saturating the sun color based on time of day
	D3DXVECTOR3  LuminanceWeights = D3DXVECTOR3(0.299f,0.587f,0.114f);
	float Luminance = D3DXVec3Dot(&clear_sun_color,&LuminanceWeights);
	D3DXVECTOR3 LuminanceVec = D3DXVECTOR3(Luminance,Luminance,Luminance);
	D3DXVec3Lerp(&clear_sun_color,&clear_sun_color,&LuminanceVec, max(0.0f,min(1.0f,tod - 6.5f)));

	// Cloudy colors 
	cloudy_sun_color = DirLightColorOfPreset(g_CurrentPreset);
	cloudy_ambient_color = SkyColorOfPreset(g_CurrentPreset);

	// Dimming cloudy colors based on time of day
	cloudy_sun_color *= Luminance;
	cloudy_ambient_color *= Luminance;

	// Update env
	g_ocean_env.sky_color = clear_ambient_color + (cloudy_ambient_color - clear_ambient_color)*cloud_factor;
	g_ocean_env.main_light_color = clear_sun_color + (cloudy_sun_color - clear_sun_color)*cloud_factor;
	g_ocean_env.sky_map_color_mult.x *= Luminance;
	g_ocean_env.sky_map_color_mult.y *= Luminance; 
	g_ocean_env.sky_map_color_mult.z *= Luminance; 

	g_ocean_env.cloud_factor = cloud_factor;

	// Render GPU height maps
	g_pHeroVessel->getSurfaceHeights()->updateGPUHeights(pDC, g_hOceanSimulation, *g_Camera.GetViewMatrix());

	// Update vessel smoke
	const FLOAT smoke_speed = SmokeSpeedOfPreset(g_CurrentPreset);
	const FLOAT smoke_emit_rate_scale = g_bEnableSmoke ? SmokeEmitRateScaleOfPreset(g_CurrentPreset) : 0.f;
	g_pHeroVessel->updateSmokeSimulation(pDC, g_Camera, fElapsedTime * g_TimeMultiplier, -NvToDX(g_ocean_simulation_param.wind_dir), smoke_speed, smoke_emit_rate_scale);

	// Render vessel shadows
	g_pHeroVessel->updateVesselShadows(pDC);

	// Render reflection
	float ReflectionClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pDC->ClearRenderTargetView( g_reflectionColorResourceRTV, ReflectionClearColor );
	pDC->ClearRenderTargetView( g_pReflectionViewDepthRTV, ReflectionClearColor );
	pDC->ClearDepthStencilView( g_reflectionDepthResourceDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );
	ID3D11RenderTargetView* pRTVs[2] = {g_reflectionColorResourceRTV, g_pReflectionViewDepthRTV};
	pDC->OMSetRenderTargets(2, pRTVs, g_reflectionDepthResourceDSV);

    // Render vessel to reflection - set up a plane that passes through the target boat,
    // and match its attitude
	if(!g_bShowSpraySimulation && g_bRenderShip)
	{
        __declspec(align(16)) D3DXPLANE reflection_plane;
        D3DXVECTOR3 origin = D3DXVECTOR3(0,0,0);
        D3DXVECTOR3 up = D3DXVECTOR3(0,1,0);
        D3DXVECTOR3 forward = D3DXVECTOR3(0,0,1);
        D3DXVECTOR3 planar = D3DXVECTOR3(1,0,0);
        D3DXVec3TransformCoord(&origin,&origin,g_pHeroVessel->getWorldXform());

#if 0
        D3DXVec3TransformNormal(&up,&up,g_pHeroVessel->getWorldXform());
#else
        D3DXVec3TransformNormal(&forward,&forward,g_pHeroVessel->getWorldXform());
        D3DXVec3TransformNormal(&planar,&planar,g_pHeroVessel->getWorldXform());
        planar.y = 0;
        D3DXVec3Cross(&up, &planar, &forward);
        D3DXVec3Normalize(&up, &up);
#endif

        D3DXPlaneFromPointNormal(&reflection_plane, &origin, &up);

		g_ocean_env.lightFilter = 0;
 
        // do not render the reflection of the vessel we're at, if we're onboard
        g_pHeroVessel->renderReflectedVesselToScene(pDC, g_Camera, reflection_plane, g_ocean_env);
    }

	// Generate reflection mips
	pDC->GenerateMips(g_reflectionColorResourceSRV);

	// Update local foam map
	{
		D3D11_VIEWPORT original_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		UINT num_original_viewports = sizeof(original_viewports)/sizeof(original_viewports[0]);
		pDC->RSGetViewports( &num_original_viewports, original_viewports);

		D3DXMATRIX matWorldToFoam;
		g_pOceanSurf->beginRenderToLocalFoamMap(pDC, matWorldToFoam);
		g_pOceanSpray->renderToFoam(pDC, matWorldToFoam, g_pHeroVessel, fElapsedTime);
		D3DXVECTOR2 shift_vector;
		shift_vector.x = 0.0028f;	// set exactly as float2 g_WakeTexScale = {0.0028,0.0028}; in shader_common.fxh
		shift_vector.y =0;
		g_pOceanSurf->endRenderToLocalFoamMap(pDC,shift_vector*kVesselSpeed*fElapsedTime, 0.01f*fElapsedTime,1.f-0.1f*fElapsedTime);

		pDC->RSSetViewports(num_original_viewports, original_viewports);

		// Generate local foam map mips
		pDC->GenerateMips(g_pOceanSurf->m_pLocalFoamMapReceiverSRV);
	}

	D3D11_VIEWPORT defaultViewport;
	UINT numViewports = 1;
	pDC->RSGetViewports(&numViewports, &defaultViewport);

	// Render the scene to back buffer
	// Clear the render target and depth stencil
	float ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
	pDC->ClearRenderTargetView( pRTV, ClearColor );
	ID3D11DepthStencilView* pDSV = g_pDepthBufferDSV;
	pDC->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );
	pDC->OMSetRenderTargets(1, &pRTV, pDSV);

	// Render vessel
	g_ocean_env.lightFilter = 0;
	if(g_bRenderShip) {
		const bool b_vessel_wireframe = g_bShowSpraySimulation;
		g_pHeroVessel->renderVesselToScene(pDC, *g_Camera.GetViewMatrix(), *g_Camera.GetProjMatrix(), g_ocean_env, NULL, b_vessel_wireframe);
	}

	if(g_ShowReadbackMarkers)
		RenderMarkers(pDC);

#if ENABLE_SSAO
    while (g_RenderSSAO)
    {
        D3DXMATRIX projMatrix = *g_Camera.GetProjMatrix();

        UINT numViewports = 1;
        D3D11_VIEWPORT viewport;
        pDC->RSGetViewports(&numViewports, &viewport);
        
		GFSDK_SSAO_InputData InputData;
		InputData.pNormalData = NULL;

		GFSDK_SSAO_InputDepthData& InputDepthData = InputData.DepthData;
        InputDepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
        InputDepthData.pFullResDepthTextureSRV = g_pDepthBufferSRV;
        InputDepthData.pViewport = &viewport;
        InputDepthData.pProjectionMatrix = (const float*)projMatrix;
        InputDepthData.ProjectionMatrixLayout = GFSDK_SSAO_ROW_MAJOR_ORDER;
        InputDepthData.MetersToViewSpaceUnits = 1.0f;

		g_SSAO_Status = g_pSSAO_Context->RenderAO(pDC, &InputData, &g_SSAO_Params, pRTV);
		
        break;
    }
#endif

	const float foamSprayFade = g_bEnableFoamAndSpray ? FoamSprayFadeOfPreset(g_CurrentPreset) : 0.f;
	if(g_WaterRenderMode != WaterRenderMode_Off)
	{
		OceanHullProfile hp;
		g_pHeroVessel->renderVesselToHullProfile(pDC, hp);

		const D3DXMATRIX matView = D3DXMATRIX(kWaterScale,0,0,0,0,0,kWaterScale,0,0,kWaterScale,0,0,0,0,0,1) * *g_Camera.GetViewMatrix();
		const D3DXMATRIX matProj = *g_Camera.GetProjMatrix();

		// Pass the reflection SRV & DSV to ocean environment struct
		g_ocean_env.pPlanarReflectionSRV = g_reflectionColorResourceSRV;
		g_ocean_env.pPlanarReflectionPosSRV = g_pReflectionViewDepthSRV;
		g_ocean_env.lightFilter = -1;

		// Update ocean surface ready for rendering
		g_pOceanSurf->setHullProfiles(&hp,1);

		if (g_WaterRenderMode == WaterRenderMode_Wireframe)
			g_pOceanSurf->renderWireframe(pDC, matView,matProj,g_hOceanSimulation, g_hOceanSavestate, g_DebugCam);
		else if (g_WaterRenderMode == WaterRenderMode_Solid)
		{
			g_pOceanSurf->renderShaded(	pDC,
										*g_Camera.GetViewMatrix(),matView,matProj,
										g_hOceanSimulation, g_hOceanSavestate,
										g_ocean_env,
										g_DebugCam, foamSprayFade, g_bShowSpraySimulation, g_bShowFoamSimulation,
										g_bGustsEnabled, g_bWakeEnabled);}

		g_pOceanSurf->getQuadTreeStats(g_ocean_stats_quadtree);
	}

	if((g_WaterRenderMode == WaterRenderMode_Solid)&&(!g_bShowSpraySimulation)&&(!g_bShowFoamSimulation))
	{
		RenderSkybox(pDC);
	}

#if 0
	if (g_TXAAisActive)
	{
#if ENABLE_TXAA
#endif
	}
	else
	{
		ID3D11Resource* pResource;
		ID3D11RenderTargetView* pResourceRTV;
		pResourceRTV = DXUTGetD3D11RenderTargetView();
		pResourceRTV->GetResource(&pResource);
		pDC->ResolveSubresource(pResource, 0, g_pColorBuffer, 0, g_SwapChainDesc.BufferDesc.Format);

		pResource->Release();

		pRTV = pResourceRTV;
		pDSV = NULL;
	}
#endif

	ID3D11RenderTargetView* pTempRTV = pRTV;
	ID3D11DepthStencilView* pTempDSV = g_pDepthBufferReadDSV;
	ID3D11ShaderResourceView* pTempDepthSRV = g_pDepthBufferSRV;

#if DOWNSAMPLE_SCALE > 1
	DownsampleDepth(pDC);

	D3D11_TEXTURE2D_DESC desc;
	g_pColorBufferResolved->GetDesc(&desc);
	CD3D11_VIEWPORT viewport(0.0f, 0.0f, (float)desc.Width, (float)desc.Height);
	pDC->RSSetViewports(1, &viewport);

	pDC->OMSetRenderTargets(1, &g_pColorBufferResolvedRTV, g_pDepthBufferResolvedReadDSV);

	float colorBlack[4] = {0,0,0,1.0f};
	pDC->ClearRenderTargetView(g_pColorBufferResolvedRTV, colorBlack);

	pTempRTV = g_pColorBufferResolvedRTV;
	pTempDSV = g_pDepthBufferResolvedReadDSV;
	pTempDepthSRV = g_pDepthBufferResolvedSRV;
#endif

	// Spray
	gfsdk_float2 spray_wind;
	spray_wind.x = -g_ocean_simulation_param.wind_dir.x*SmokeSpeedOfPreset(g_CurrentPreset);
	spray_wind.y = -g_ocean_simulation_param.wind_dir.y*SmokeSpeedOfPreset(g_CurrentPreset);

	//updating spray generators
	g_pOceanSpray->updateSprayGenerators(g_hOceanSimulation, g_pHeroVessel, fElapsedTime, kWaterScale, spray_wind, foamSprayFade);

	// updating spray sensors for visualization
	if(g_bShowSpraySimulation)
	{
		g_pOceanSpray->UpdateSensorsVisualizationVertexBuffer(pDC, g_pHeroVessel, fElapsedTime);
		g_pOceanSpray->RenderSensors(pDC,g_Camera,g_pHeroVessel);
	}

	//updating spray particles
	g_pOceanSpray->updateSprayParticles(pDC, g_pHeroVessel, fElapsedTime, kWaterScale, spray_wind);

	//update PSM for spray
	//g_pOceanVessels[i]->beginRenderToPSM(pDC,-NvToDX(g_ocean_simulation_param.wind_dir));
	//g_pOceanSpray[i]->renderToPSM(pDC,g_pOceanVessels[i]->getPSM(),g_ocean_env);
	//g_pOceanVessels[i]->endRenderToPSM(pDC);

	g_ocean_env.lightFilter = 0;

	//rendering spray
	if(g_bEnableFoamAndSpray) {
		pDC->OMSetRenderTargets(1, &pTempRTV, pTempDSV);
		g_pOceanSpray->renderToScene(pDC, g_pHeroVessel, g_Camera, g_ocean_env, pTempDepthSRV, g_bShowSpraySimulation);
	}

	// Render smoke
	if(g_bEnableSmoke && (!g_bShowSpraySimulation))
	{
		//update PSM for smoke
		g_pHeroVessel->beginRenderToPSM(pDC,-NvToDX(g_ocean_simulation_param.wind_dir));
		g_pHeroVessel->renderSmokeToPSM(pDC, g_ocean_env);
		g_pHeroVessel->endRenderToPSM(pDC);

		g_ocean_env.lightFilter = -1;

		// render smoke
		pDC->OMSetRenderTargets(1, &pTempRTV, pTempDSV);
		g_pHeroVessel->renderSmokeToScene(pDC, g_Camera, g_ocean_env);
	}

	pDC->RSSetViewports(1, &defaultViewport);
	pDC->OMSetRenderTargets(1, &pRTV, NULL);

#if DOWNSAMPLE_SCALE > 1
	InterpolateDepth(pDC);
#endif


#if 0
	{
		ID3D11Resource* pResource;
		ID3D11RenderTargetView* pResourceRTV;
		pResourceRTV = DXUTGetD3D11RenderTargetView();
		pResourceRTV->GetResource(&pResource);
		pDC->ResolveSubresource(pResource, 0, g_pColorBuffer, 0, g_SwapChainDesc.BufferDesc.Format);

		pDC->OMSetRenderTargets(1, &pResourceRTV, NULL);
		pResource->Release();
	}
#endif

	// g_pHeroVessel->renderHullProfileInUI(pDC);

	if(g_bShowSprayAudioLevel) {
		const DXGI_SURFACE_DESC* pd3dsdBackBuffer = DXUTGetDXGIBackBufferSurfaceDesc();

		const float fAspectRatio = float(pd3dsdBackBuffer->Width)/float(pd3dsdBackBuffer->Height);
		const float height = 1.f;
		const float width = height * 0.1f / fAspectRatio;
		const float v_inset = 0.02f * height;
		const float h_inset = v_inset / fAspectRatio;
		const float v_margin = 0.02f * height;
		const float h_margin = v_margin / fAspectRatio;

		float splash_level = 0.5f * g_recentMaxSplashPower/GetOceanAudioSplashPowerThreshold();
		if(splash_level > 1.f) splash_level = 1.f;
		else if(splash_level < 0.f) splash_level = 0.f;
		g_pOceanSpray->renderAudioVisualization(pDC,-1.f+h_inset,-1.f+v_inset+height,-1.f+h_inset+width,-1.f+v_inset,h_margin,v_margin,splash_level);
	}

    if (g_ShowHUD)
    {
		float fUIElapsedTime = fElapsedTime;
		if( DXUTGetGlobalTimer()->IsStopped() ) {
			if (DXUTGetFPS() == 0.0f) fUIElapsedTime = 0;
			else fUIElapsedTime = 1.0f / DXUTGetFPS();
		}

        g_HUD.OnRender( fUIElapsedTime ); 
	    g_SampleUI.OnRender( fUIElapsedTime );
		RenderText( pDC, fTime );

		if(g_bShowCredits) {
			RenderLogo(pDC, g_pFMODLogoTex, g_pFMODLogoVB);
		}
    }

	RenderLogo(pDC, g_pLogoTex, g_pLogoVB);

	pDC->GSSetShader(NULL,NULL,0);
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText( ID3D11DeviceContext* pDC, double fTime )
{
    // Output statistics
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 0.9f, 0.9f, 0.9f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats(false) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

	const UINT buffer_len = 256;
	WCHAR buffer[buffer_len];

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    
    // Draw help
    if( g_bShowHelp )
    {
        const DXGI_SURFACE_DESC* pd3dsdBackBuffer = DXUTGetDXGIBackBufferSurfaceDesc();
        g_pTxtHelper->SetInsertionPos( 2, pd3dsdBackBuffer->Height-15*6 );
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f ) );

		g_pTxtHelper->SetInsertionPos( 20, pd3dsdBackBuffer->Height-200 );
		g_pTxtHelper->DrawTextLine( 
			L"Left mouse: camera look\
			\n\nTAB: toggle HUD\
			\nC: toggle mouse cursor\
			\n+/-: cycle simulation quality presets\
			\n\nHide help: F1\nShow credits: F3\nQuit: ESC" ); 
    }
    else
    {
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        g_pTxtHelper->DrawTextLine( L"Press F1 for help" );

		if(g_bShowStats) {
			swprintf_s(buffer,buffer_len,L"               Quad patches drawn: %d", g_ocean_stats_quadtree.num_patches_drawn);
			g_pTxtHelper->DrawTextLine(buffer);
			swprintf_s(buffer,buffer_len,L"                    GPU_gfx_time : %3.3f msec",g_ocean_stats_simulation_filtered.GPU_gfx_time);
			g_pTxtHelper->DrawTextLine(buffer);
			swprintf_s(buffer,buffer_len,L"                 GPU_update_time : %3.3f msec",g_ocean_stats_simulation_filtered.GPU_update_time);
			g_pTxtHelper->DrawTextLine(buffer);
			swprintf_s(buffer,buffer_len,L"       CPU_main_thread_wait_time : %3.3f msec",g_ocean_stats_simulation_filtered.CPU_main_thread_wait_time);
			g_pTxtHelper->DrawTextLine(buffer);
			swprintf_s(buffer,buffer_len,L"CPU_threads_start_to_finish_time : %3.3f msec",g_ocean_stats_simulation_filtered.CPU_threads_start_to_finish_time);
			g_pTxtHelper->DrawTextLine(buffer);
			swprintf_s(buffer,buffer_len,L"          CPU_threads_total_time : %3.3f msec",g_ocean_stats_simulation_filtered.CPU_threads_total_time);
			g_pTxtHelper->DrawTextLine(buffer);
			swprintf_s(buffer,buffer_len,L"             GPU_simulation_time : %3.3f msec",g_ocean_stats_simulation_filtered.GPU_simulation_time);
			g_pTxtHelper->DrawTextLine(buffer);
			swprintf_s(buffer,buffer_len,L"         GPU_FFT_simulation_time : %3.3f msec",g_ocean_stats_simulation_filtered.GPU_FFT_simulation_time);
			g_pTxtHelper->DrawTextLine(buffer);
			swprintf_s(buffer,buffer_len,L"        CPU_quadtree_update_time : %3.3f msec",g_ocean_stats_quadtree.CPU_quadtree_update_time);
			g_pTxtHelper->DrawTextLine(buffer);

			UINT particleCount = g_pOceanSpray->GetParticleCount(pDC);
			swprintf_s(buffer,buffer_len,L"          active_particles_spray : %d (%.1f%%)",particleCount, 100.0f*particleCount/g_pOceanSpray->GetParticleBudget());
			g_pTxtHelper->DrawTextLine(buffer);

			swprintf_s(buffer,buffer_len,L"                    splash_power : %.1f", g_pOceanSpray->getSplashPowerFromLastUpdate());
			g_pTxtHelper->DrawTextLine(buffer);

			const D3DXMATRIX* pMatVessel = g_pHeroVessel->getWorldXform();
			swprintf_s(buffer,buffer_len,L"                    hero_vessel@ : %.1f,%.1f,%.1f", pMatVessel->_41, pMatVessel->_42, pMatVessel->_43);
			g_pTxtHelper->DrawTextLine(buffer);

			swprintf_s(buffer,buffer_len,L"                            time : %.2f", g_TotalSimulatedTime);
			g_pTxtHelper->DrawTextLine(buffer);
		}
    }

	if(g_bShowCredits)
	{
        const DXGI_SURFACE_DESC* pd3dsdBackBuffer = DXUTGetDXGIBackBufferSurfaceDesc();
        g_pTxtHelper->SetInsertionPos( 2, pd3dsdBackBuffer->Height-15*6 );
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f ) );

		g_pTxtHelper->SetInsertionPos( (pd3dsdBackBuffer->Width-180)/2 - 150, 90 );
		g_pTxtHelper->DrawTextLine( 
			L"                         CREDITS                         \n"
			L"_________________________________________________________\n"
			L"\n"
			L"                Programming: Jon Jansen\n"
			L"                             Tim Tcheblokov\n"
			L"                             Evgeny Makarov\n"
			L"                             Iain Cantlay\n"
			L"                             Nikolay Chirkov\n"
			L"                             Calvin Lin\n"
			L"                             Phil Scott\n"
			L"\n"
			L"         Project Management: Miguel Sainz\n"
			L"                             Jon Jansen\n"
			L"\n"
			L"                        Art: Khariton Kantiev\n"
			L"                             Miguel Sainz\n"
			L"\n");

		g_pTxtHelper->SetInsertionPos( (pd3dsdBackBuffer->Width-180)/2 - 150, 432 );
		g_pTxtHelper->DrawTextLine( 
			L"_________________________________________________________\n"
			L"\n"
			L"              FMOD Sound System, Copyright (c) Firelight\n"
			L"              Technologies Pty, Ltd., 1994-2012\n"
			L"              \n"); 
	}

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext )
{
    // Always allow dialog resource manager calls to handle global messages
    // so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// As a convenience, DXUT inspects the incoming windows messages for
// keystroke messages and decodes the message parameters to pass relevant keyboard
// messages to the application.  The framework does not remove the underlying keystroke 
// messages, which are still passed to the application's MsgProc callback.
//--------------------------------------------------------------------------------------
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
		switch( nChar )
		{
			case VK_F1:
				g_bShowHelp = !g_bShowHelp;
				break;
			case VK_F3:
				g_bShowCredits = !g_bShowCredits;
				break;
			case VK_TAB: g_ShowHUD = !g_ShowHUD; break;
			case VK_OEM_MINUS:
				g_ocean_simulation_settings.detail_level = GFSDK_WaveWorks_Simulation_DetailLevel((g_ocean_simulation_settings.detail_level + g_max_detail_level) % (g_max_detail_level+1));
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
				GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
				break;
			case VK_OEM_PLUS:
				g_ocean_simulation_settings.detail_level = GFSDK_WaveWorks_Simulation_DetailLevel((g_ocean_simulation_settings.detail_level + 1) % (g_max_detail_level+1));
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
				GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
				break;
			case 'c':
			case 'C':
				g_CursorVisibility.ToggleVisibility();
				break;
			case VK_LEFT:
				g_TargetPreset = max(g_TargetPreset-1,0);
				g_HUD.GetComboBox(IDC_WEATHERPRESETS)->SetSelectedByData((void*)g_TargetPreset);
				break;
			case VK_RIGHT:
				g_TargetPreset = min(g_TargetPreset+1,NumPresets-1);
				g_HUD.GetComboBox(IDC_WEATHERPRESETS)->SetSelectedByData((void*)g_TargetPreset);
				break;
			/*
			case 'v':
			case 'V':
				CycleVesselMode();
				break;
			case 'h':
			case 'H':
				g_RenderSSAO = !g_RenderSSAO;
				break;
				*/
		}

		if('1' <= nChar && nChar <= '9') {
			g_TargetPreset = min(NumPresets-1,nChar - '1');
			g_HUD.GetComboBox(IDC_WEATHERPRESETS)->SetSelectedByData((void*)g_TargetPreset);
		}
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    if (!g_ShowHUD) return;

    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN: DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:        DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:     g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;

		case IDC_TOGGLEPAUSE:
			{
				const bool doPause = g_HUD.GetCheckBox(IDC_TOGGLEPAUSE)->GetChecked();
				g_DebugCam = doPause;
                if( doPause )
                    DXUTPause( true, false );
                else
                    DXUTPause( false, false );
			}
			break;

		case IDC_TOGGLESHOWSPRAYSIM:g_bShowSpraySimulation = g_HUD.GetCheckBox(IDC_TOGGLESHOWSPRAYSIM)->GetChecked(); break;
		case IDC_TOGGLESHOWFOAMSIM: g_bShowFoamSimulation = g_HUD.GetCheckBox(IDC_TOGGLESHOWFOAMSIM)->GetChecked(); break;
		case IDC_TOGGLESHOWAUDIOLEVEL: g_bShowSprayAudioLevel = g_HUD.GetCheckBox(IDC_TOGGLESHOWAUDIOLEVEL)->GetChecked(); break;

		case IDC_CLOUDSMODE:
			{
				DXUTComboBoxItem* pItem = ((CDXUTComboBox* ) pControl)->GetSelectedItem();
				if (pItem)
				{
					g_CloudMode = (CloudMode)((int)pItem->pData);
				}
			}
			break;

		case IDC_WATERMODE:
			{
				DXUTComboBoxItem* pItem = ((CDXUTComboBox* ) pControl)->GetSelectedItem();
				if (pItem)
				{
					g_WaterRenderMode = (WaterRenderMode)((int)pItem->pData);
				}
			}
			break;

		case IDC_TOGGLEFOAMSPRAY:  g_bEnableFoamAndSpray = g_HUD.GetCheckBox(IDC_TOGGLEFOAMSPRAY)->GetChecked(); break;
		case IDC_TOGGLESMOKE:  g_bEnableSmoke = g_HUD.GetCheckBox(IDC_TOGGLESMOKE)->GetChecked(); break;
		case IDC_TOGGLEWAKE:  g_bWakeEnabled = g_HUD.GetCheckBox(IDC_TOGGLEWAKE)->GetChecked(); break;
		case IDC_TOGGLEGUSTS:  g_bGustsEnabled = g_HUD.GetCheckBox(IDC_TOGGLEGUSTS)->GetChecked(); break;
		case IDC_TOGGLESHIP: g_bRenderShip = g_HUD.GetCheckBox(IDC_TOGGLESHIP)->GetChecked(); break;

		// No stats in shipping version: case IDC_TOGGLESTATS:	   g_bShowStats = g_HUD.GetCheckBox(IDC_TOGGLESTATS)->GetChecked()?1.0f:0.0f; break;

		case IDC_WEATHERPRESETS:
			{
				DXUTComboBoxItem* pItem = ((CDXUTComboBox* ) pControl)->GetSelectedItem();
				if (pItem)
				{
					g_TargetPreset = ((int)pItem->pData);
				}
			}
			break;
		case IDC_TOGGLETIMEOFDAY:
			{
				const bool prev_bUseCustomTimeOfDay = g_bUseCustomTimeOfDay;
				g_bUseCustomTimeOfDay = !g_HUD.GetCheckBox(IDC_TOGGLETIMEOFDAY)->GetChecked();
				if(prev_bUseCustomTimeOfDay && !g_bUseCustomTimeOfDay) {
					g_CustomTimeOfDay = TimeOfDayOfPreset(g_CurrentPreset);
					g_HUD.GetSlider(IDC_TIMEOFDAY_SLIDER)->SetValue((int)(kTimeOfDaySliderScaleFactor*(g_CustomTimeOfDay-kMinTimeOfDay)));
				}
				g_HUD.GetSlider(IDC_TIMEOFDAY_SLIDER)->SetEnabled(g_bUseCustomTimeOfDay);
			}
			break;
		case IDC_TIMEOFDAY_SLIDER:
			if((EVENT_SLIDER_VALUE_CHANGED_UP == nEvent) || (EVENT_SLIDER_VALUE_CHANGED == nEvent))
			{
				g_CustomTimeOfDay = kMinTimeOfDay + FLOAT(g_HUD.GetSlider(IDC_TIMEOFDAY_SLIDER)->GetValue())/kTimeOfDaySliderScaleFactor;
			}
			break;
   }
    
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
	g_DialogResourceManager.OnD3D11ReleasingSwapChain();

	SAFE_RELEASE(g_pLogoVB);
	SAFE_RELEASE(g_pFMODLogoVB);
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_SettingsDlg.OnD3D11DestroyDevice();
    CDXUTDirectionWidget::StaticOnD3D11DestroyDevice();
	SAFE_DELETE(g_pTxtHelper);
	
	// Spray
	SAFE_DELETE(g_pOceanSpray);
	
	// Ocean object
	SAFE_DELETE(g_pOceanSurf);
	SAFE_DELETE(g_pHeroVessel);
	SAFE_DELETE(g_pCameraVessel);

	if(g_hOceanSimulation)
	{
		GFSDK_WaveWorks_Simulation_Destroy(g_hOceanSimulation);
		g_hOceanSimulation = NULL;
	}

	SAFE_RELEASE(g_pMarkerFX);
	SAFE_RELEASE(g_pMarkerLayout);

	SAFE_RELEASE(g_pSkyboxFX);
	SAFE_RELEASE(g_pSkyboxLayout);
	SAFE_RELEASE(g_pLogoTex);
	SAFE_RELEASE(g_pFMODLogoTex);

	for(int i = 0; i != NumSkyMaps; ++i) {
		OceanSkyMapInfo& sm = g_SkyMaps[i];
		SAFE_RELEASE(sm.m_pSkyDomeSRV);
		SAFE_RELEASE(sm.m_pReflectionSRV);
	}

	SAFE_RELEASE(g_pSkyBoxVB);
	SAFE_RELEASE(g_pMarkerVB);
	SAFE_RELEASE(g_pMarkerIB);

	if(g_hOceanSavestate)
	{
		GFSDK_WaveWorks_Savestate_Destroy(g_hOceanSavestate);
		g_hOceanSavestate = NULL;
	}

	GFSDK_WaveWorks_ReleaseD3D11(DXUTGetD3D11Device());

    ReleaseViewDependentResources();

	DXUTGetGlobalResourceCache().OnDestroyDevice();

#if ENABLE_SSAO
    g_pSSAO_Context->Release();
#endif

#if ENABLE_TXAA
	TxaaCloseDX(&g_TXAA_Ctx);
#endif
}

void RenderMarkers(ID3D11DeviceContext* pDC)
{
	HRESULT hr;

	D3DXVECTOR3 eye_pos = *g_Camera.GetEyePt();

	// Find where the camera vector intersects mean sea level
	const D3DXVECTOR3 sea_level_pos = GetOceanSurfaceLookAtPoint();
	const D3DXVECTOR2 sea_level_xy(sea_level_pos.x, sea_level_pos.z);

	// Draw markers on the surface
	const int markers_x = 7;
	const int markers_y = 7;
	const int num_markers = markers_x * markers_y;
	gfsdk_float2 marker_pos[num_markers];
	for(int x = 0; x != markers_x; ++x)
	{
		for(int y = 0; y != markers_y; ++y)
		{
			marker_pos[y * markers_x + x] = NvFromDX(sea_level_xy/kWaterScale + D3DXVECTOR2(2.f * (x-((markers_x-1)/2)), 2.f * (y-((markers_y-1)/2))));
		}
	}
	gfsdk_float4 displacements[num_markers];
	V(GFSDK_WaveWorks_Simulation_GetDisplacements(g_hOceanSimulation, marker_pos, displacements, num_markers));

	D3DXVECTOR4 position[num_markers];
	for(int x = 0; x != markers_x; ++x)
	{
		for(int y = 0; y != markers_y; ++y)
		{
			const int ix = y * markers_x + x;
			position[ix] = D3DXVECTOR4(displacements[ix].x + marker_pos[ix].x,displacements[ix].z + g_ocean_param_quadtree.sea_level,displacements[ix].y + marker_pos[ix].y,1.f);
			position[ix].x *= kWaterScale;
			position[ix].y *= kWaterScale;
			position[ix].z *= kWaterScale;
		}
	}

	const UINT vbOffset = 0;
	const UINT vertexStride = sizeof(D3DXVECTOR4);
	pDC->IASetInputLayout(g_pMarkerLayout);
    pDC->IASetVertexBuffers(0, 1, &g_pMarkerVB, &vertexStride, &vbOffset);
	pDC->IASetIndexBuffer(g_pMarkerIB, DXGI_FORMAT_R16_UINT, 0);

	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3DXMATRIX matView = /*D3DXMATRIX(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1) **/ *g_Camera.GetViewMatrix();
	D3DXMATRIX matProj = *g_Camera.GetProjMatrix();
	D3DXMATRIX matVP = matView * matProj;
	g_pMarkerMatViewProjVariable->SetMatrix((FLOAT*)&matVP);

	g_pMarkerTechnique->GetPassByIndex(0)->Apply(0,pDC);
	for(int i = 0; i != num_markers; ++i)
	{
        D3D11_MAPPED_SUBRESOURCE msr;
        pDC->Map(g_pMarkerVB,0,D3D11_MAP_WRITE_DISCARD, 0, &msr);

        D3DXVECTOR4* marker_verts = (D3DXVECTOR4*)msr.pData;
		marker_verts[0] = position[i];
		marker_verts[1] = position[i] + D3DXVECTOR4( 0.5, 0.5, 0.5, 0.f);
		marker_verts[2] = position[i] + D3DXVECTOR4( 0.5, 0.5,-0.5, 0.f);
		marker_verts[3] = position[i] + D3DXVECTOR4(-0.5, 0.5,-0.5, 0.f);
		marker_verts[4] = position[i] + D3DXVECTOR4(-0.5, 0.5, 0.5, 0.f);
		pDC->Unmap(g_pMarkerVB,0);

		pDC->DrawIndexed(12, 0, 0);
	}
}

void DownsampleDepth(ID3D11DeviceContext* pDC)
{
	pDC->OMSetRenderTargets(0, NULL, g_pDepthBufferResolvedDSV);

	pDC->IASetInputLayout(NULL);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	g_pDepthMapMSVariable->SetResource(g_pDepthBufferSRV);
	g_pDownsampleDepthTechnique->GetPassByIndex(0)->Apply(0, pDC);

	pDC->Draw(4, 0);

	g_pDepthMapMSVariable->SetResource(NULL);
}

void InterpolateDepth(ID3D11DeviceContext* pDC)
{
	D3DXMATRIX matProj = *g_Camera.GetProjMatrix();
	D3DXMATRIX matProjInv;
	D3DXMatrixInverse(&matProjInv, NULL, &matProj);
	g_pMatProjInvVariable->SetMatrix(matProjInv);

	pDC->IASetInputLayout(NULL);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	g_pColorMapVariable->SetResource(g_pColorBufferResolvedSRV);
	g_pDepthMapVariable->SetResource(g_pDepthBufferResolvedSRV);
	g_pDepthMapMSVariable->SetResource(g_pDepthBufferSRV);
	g_pUpsampleParticlesTechnique->GetPassByIndex(0)->Apply(0, pDC);

	pDC->Draw(4, 0);

	g_pColorMapVariable->SetResource(NULL);
	g_pDepthMapVariable->SetResource(NULL);
	g_pDepthMapMSVariable->SetResource(NULL);
	g_pUpsampleParticlesTechnique->GetPassByIndex(0)->Apply(0, pDC);
}

void RenderSkybox(ID3D11DeviceContext* pDC)
{
	D3DXMATRIX matView = D3DXMATRIX(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1) * *g_Camera.GetViewMatrix();
	matView._41 = 0.f; // Zero out the translation, to avoid precision issues at large distances from origin
	matView._42 = 0.f;
	matView._43 = 0.f;
	D3DXMATRIX matProj = *g_Camera.GetProjMatrix();
	D3DXMATRIX matVP = matView * matProj;

    D3D11_MAPPED_SUBRESOURCE msr;
	pDC->Map(g_pSkyBoxVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    D3DXVECTOR4* far_plane_quad = (D3DXVECTOR4*)msr.pData;
	far_plane_quad[0] = D3DXVECTOR4(-g_FarPlane,  g_FarPlane, g_FarPlane, g_FarPlane);
	far_plane_quad[1] = D3DXVECTOR4(-g_FarPlane, -g_FarPlane, g_FarPlane, g_FarPlane);
	far_plane_quad[2] = D3DXVECTOR4( g_FarPlane,  g_FarPlane, g_FarPlane, g_FarPlane);
	far_plane_quad[3] = D3DXVECTOR4( g_FarPlane, -g_FarPlane, g_FarPlane, g_FarPlane);
	D3DXMATRIX matInvVP;
	D3DXMatrixInverse(&matInvVP, NULL, &matVP);
	D3DXVec4TransformArray(&far_plane_quad[0], sizeof(D3DXVECTOR4), &far_plane_quad[0], sizeof(D3DXVECTOR4), &matInvVP, 4);
	pDC->Unmap(g_pSkyBoxVB, 0);

	const UINT vbOffset = 0;
	const UINT vertexStride = sizeof(D3DXVECTOR4);
	pDC->IASetInputLayout(g_pSkyboxLayout);
    pDC->IASetVertexBuffers(0, 1, &g_pSkyBoxVB, &vertexStride, &vbOffset);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	g_pSkyBoxMatViewProjVariable->SetMatrix((FLOAT*)&matVP);
	g_pSkyBoxLightningColorVariable->SetFloatVector((FLOAT*)&g_ocean_env.lightning_light_intensity);
	g_pSkyBoxSunPositionVariable->SetFloatVector((FLOAT*)&g_ocean_env.main_light_direction);
	g_pSkyBoxCloudFactorVariable->SetFloat(g_ocean_env.cloud_factor);

	g_pSkyBoxFogColorVariable->SetFloatVector((FLOAT*)&g_ocean_env.sky_color);
	g_pSkyBoxFogExponentVariable->SetFloat(g_ocean_env.fog_exponent);

	g_pSkyBoxSkyCubeMap0Variable->SetResource(g_ocean_env.pSky0->m_pSkyDomeSRV);
	g_pSkyBoxSkyCubeMap1Variable->SetResource(g_ocean_env.pSky1->m_pSkyDomeSRV);
	g_pSkyCubeBlendVariable->SetFloat(g_ocean_env.sky_interp);

	D3DXVECTOR4 sincos0;
	sincos0.x = sinf(g_ocean_env.pSky0->m_Orientation);
	sincos0.y = cosf(g_ocean_env.pSky0->m_Orientation);
	g_pSkyCube0RotateSinCosVariable->SetFloatVector((FLOAT*)&sincos0);

	D3DXVECTOR4 sincos1;
	sincos1.x = sinf(g_ocean_env.pSky1->m_Orientation);
	sincos1.y = cosf(g_ocean_env.pSky1->m_Orientation);
	g_pSkyCube1RotateSinCosVariable->SetFloatVector((FLOAT*)&sincos1);

	g_pSkyCubeMultVariable->SetFloatVector((FLOAT*)&g_ocean_env.sky_map_color_mult);

	g_pSkyBoxTechnique->GetPassByIndex(0)->Apply(0, pDC);
	pDC->Draw(4, 0);
}

void RenderLogo(ID3D11DeviceContext* pDC, ID3D11ShaderResourceView* pSRV, ID3D11Buffer* pVB)
{
	g_pLogoTextureVariable->SetResource(pSRV);

	const UINT vbOffset = 0;
	const UINT vertexStride = 20;
	pDC->IASetInputLayout(g_pOceanSurf->m_pQuadLayout);
    pDC->IASetVertexBuffers(0, 1, &pVB, &vertexStride, &vbOffset);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	g_pLogoTechnique->GetPassByIndex(0)->Apply(0, pDC);
	pDC->Draw(4, 0);
}

void SetVesselMode_OnBoard()
{
	g_Camera.SetEnablePositionMovement(FALSE);
	
	// Reset the vessel-relative xform to look out to port at the other vessel
	D3DXVECTOR3 vecEye(0.f, 0.f, 0.f);
	D3DXVECTOR3 vecAt (-100.f, -3.f, 90.f);
	g_Camera.SetViewParams(&vecEye, &vecAt);

	g_VesselMode = VesselMode_OnBoard;
}

void SetVesselMode_External()
{
	g_Camera.SetEnablePositionMovement(TRUE);
	g_Camera.AssimilateVesselBorneXform();
	g_VesselMode = VesselMode_External;
}

void CycleVesselMode()
{
	g_VesselMode = (VesselMode)((g_VesselMode+1) % Num_VesselModes);

	if(g_VesselMode == VesselMode_OnBoard) {
		SetVesselMode_OnBoard();
	}
	else if(g_VesselMode == VesselMode_External) {

		SetVesselMode_External();
	}

	// g_ocean_simulation_settings.readback_displacements = g_ShowReadbackMarkers || (g_VesselMode != VesselMode_None);
	// GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
}

D3DXVECTOR3 GetOceanSurfaceLookAtPoint()
{
	D3DXVECTOR3 eye_pos = *g_Camera.GetEyePt();
	D3DXVECTOR3 lookat_pos = *g_Camera.GetLookAtPt();
	const FLOAT intersectionHeight = g_ocean_param_quadtree.sea_level;
	const FLOAT lambda = (intersectionHeight - eye_pos.y)/(lookat_pos.y - eye_pos.y);
	const D3DXVECTOR3 result = (1.f - lambda) * eye_pos + lambda * lookat_pos;
	return result;
}
