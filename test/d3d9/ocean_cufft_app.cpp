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

#include "../testing_src/testing.h"

#include "dxstdafx.h"
#include "resource.h"

#include "ocean_surface.h"
#include "GFSDK_WaveWorks.h"
#include "GFSDK_WaveWorks_D3D_Util.h"


#include "client.h"
#include "message_types.h"
#include "../d3d9/common/DXUT.h"

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 

// Disable warning "conditional expression is constant"
#pragma warning(disable:4127)

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
ID3DXFont*              g_pFont = NULL;         // Font for drawing text
ID3DXSprite*            g_pSprite = NULL;       // Sprite for batching draw text calls
bool                    g_bShowHelp = false;    // If true, it renders the UI control text
CFirstPersonCamera      g_Camera;               //
CDXUTDialogResourceManager g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg         g_SettingsDlg;          // Device settings dialog
CDXUTDialog             g_HUD;                  // manages the 3D UI
CDXUTDialog             g_SampleUI;             // dialog for sample specific controls

Client* g_pNetworkClient = NULL;					// For testing client/server use-cases

TestParams* g_pTestParams = NULL;

GFSDK_WaveWorks_SavestateHandle	g_hOceanSavestate = NULL;
GFSDK_WaveWorks_SimulationHandle g_hOceanSimulation = NULL;
OceanSurface*			g_pOceanSurf = NULL;
GFSDK_WaveWorks_Simulation_Params	g_ocean_simulation_param;
GFSDK_WaveWorks_Simulation_Settings	g_ocean_simulation_settings;
OceanSurfaceParameters		g_ocean_surface_param;
GFSDK_WaveWorks_Quadtree_Params		g_ocean_param_quadtree;
GFSDK_WaveWorks_Quadtree_Stats		g_ocean_stats_quadtree;
GFSDK_WaveWorks_Simulation_Stats    g_ocean_stats_simulation;
GFSDK_WaveWorks_Simulation_Stats    g_ocean_stats_simulation_filtered;
int   g_max_detail_level;

bool g_RenderWireframe = false;
bool g_RenderWater = true;
bool g_SimulateWater = true;
bool g_ForceKick = false;
bool g_DebugCam = false;
bool g_bShowRemoteMarkers = false;

LPDIRECT3DCUBETEXTURE9 g_pSkyCubeMap = NULL;
LPDIRECT3DTEXTURE9 g_pLogoTex = NULL;
ID3DXEffect* g_pSkyboxFX = NULL;
LPDIRECT3DVERTEXDECLARATION9 g_pSkyboxDecl = NULL;
LPDIRECT3DVERTEXDECLARATION9 g_pMarkerDecl = NULL;
ID3DXEffect* g_pMarkerFX = NULL;

float g_NearPlane = 200.0f;
float g_FarPlane = 300000.0f;
double g_SimulationTime = 0.0f;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_ENABLE_PRESHADER    5
#define IDC_TOGGLEWIREFRAME     6
#define IDC_BUFFERTYPE_STATIC	7
#define IDC_AMPLITUDE_SLIDER    9
#define IDC_CHOPPY_SCALE_SLIDER 10
#define IDC_WIND_SPEED_SLIDER   11
#define IDC_WIND_DEPENDENCY_SLIDER   12
#define IDC_TIME_SCALE_SLIDER   13
#define IDC_SKY_BLENDING_SLIDER 14
#define IDC_PATCH_LENGTH_SLIDER 17
#define IDC_FURTHEST_COVER_SLIDER 18
#define IDC_UPPER_GRID_COVER_SLIDER 19
#define IDC_TOGGLERENDER        20
#define IDC_TOGGLESIMULATE      21
#define IDC_ANISO_SLIDER         22
#define IDC_TOGGLEREADBACK      23
#define IDC_GEOMORPH_SLIDER     24
#define IDC_FOAM_THRESHOLD_SLIDER		25
#define IDC_FOAM_AMOUNT_SLIDER			26
#define IDC_FOAM_DISSIPATION_SLIDER		27
#define IDC_FOAM_FADEOUT_SLIDER			28

const FLOAT kAmplitudeSliderScaleFactor = 3.0f;
const FLOAT kChoppyScaleSliderScaleFactor = 100.0f;
const FLOAT kWindDependencySliderScaleFactor = 100.f;
const FLOAT kTimeScaleSliderScaleFactor = 100.f;
const FLOAT kSkyBlendingSliderScaleFactor = 10.f;
const FLOAT kWindSpeedSliderScaleFactor = 2.5f;
const FLOAT kGeomorphingDegreeSlideScaleFactor = 100.f;
const FLOAT kFoamGenerationThresholdFactor = 100.0f;
const FLOAT kFoamGenerationAmountFactor = 1000.0f;
const FLOAT kFoamDissipationSpeedFactor = 100.0f;
const FLOAT kFoamFadeoutSpeedFactor = 1000.0f;

const FLOAT kWaterScale = 50.f;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool    CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
bool    CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, const D3DCAPS9* pCaps, void* pUserContext );
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void    CALLBACK OnFrameMove( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
void    CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void    CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void    CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void    CALLBACK OnLostDevice( void* pUserContext );
void    CALLBACK OnDestroyDevice( void* pUserContext );

void    InitApp();
void    RenderText( double fTime );
void	AddGUISet();
void	UpdateServerControlledUI();
void	RenderSkybox(IDirect3DDevice9* pd3dDevice);
void	RenderLogo(IDirect3DDevice9* pd3dDevice);
void	RenderLocalMarkers(IDirect3DDevice9* pd3dDevice);
void	RenderRemoteMarkers(IDirect3DDevice9* pd3dDevice);
void	UpdateMarkers();
void	UpdateRemoteMarkerCoords();

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

	g_pTestParams = new TestParams(cmdline);

    // Set the callback functions. These functions allow DXUT to notify
    // the application about device changes, user input, and windows messages.  The 
    // callbacks are optional so you need only set callbacks for events you're interested 
    // in. However, if you don't handle the device reset/lost callbacks then the sample 
    // framework won't be able to reset your device since the application must first 
    // release all device resources before resetting.  Likewise, if you don't handle the 
    // device created/destroyed callbacks then DXUT won't be able to 
    // recreate your device resources.
    DXUTSetCallbackDeviceCreated( OnCreateDevice );
    DXUTSetCallbackDeviceReset( OnResetDevice );
    DXUTSetCallbackDeviceLost( OnLostDevice );
    DXUTSetCallbackDeviceDestroyed( OnDestroyDevice );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( KeyboardProc );
    DXUTSetCallbackFrameRender( OnFrameRender );
    DXUTSetCallbackFrameMove( OnFrameMove );

    // Show the cursor and clip it when in full screen
    DXUTSetCursorSettings( true, true );

    InitApp();

    // Initialize DXUT and create the desired Win32 window and Direct3D 
    // device for the application. Calling each of these functions is optional, but they
    // allow you to set several options which control the behavior of the framework.
    DXUTInit( true, true, true ); // Parse the command line, handle the default hotkeys, and show msgboxes
    DXUTCreateWindow( L"FFT Ocean" );
    DXUTCreateDevice( D3DADAPTER_DEFAULT, true, 1280, 720, IsDeviceAcceptable, ModifyDeviceSettings );

    // Pass control to DXUT for handling the message pump and 
    // dispatching render calls. DXUT will call your FrameMove 
    // and FrameRender callback when there is idle time between handling window messages.
    DXUTMainLoop();

    // Perform any application-level cleanup here. Direct3D device resources are released within the
    // appropriate callback functions and therefore don't require any cleanup code here.
	if(g_pNetworkClient) {
		Client::Destroy(g_pNetworkClient);
		g_pNetworkClient = NULL;
	}

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
	g_ocean_param_quadtree.min_patch_length		= 40.0f;
	g_ocean_param_quadtree.upper_grid_coverage	= 64.0f;
	g_ocean_param_quadtree.mesh_dim				= 128;
	g_ocean_param_quadtree.sea_level			= 0.0f; 
	g_ocean_param_quadtree.auto_root_lod		= 10;
	g_ocean_param_quadtree.tessellation_lod		= 100.0f;
	g_ocean_param_quadtree.geomorphing_degree	= 1.f;
	g_ocean_param_quadtree.enable_CPU_timers	= true;

	g_ocean_simulation_param.time_scale				= 0.5f;
	g_ocean_simulation_param.wave_amplitude			= 1.0f;
	g_ocean_simulation_param.wind_dir				= NvFromDX(D3DXVECTOR2(0.8f, 0.6f));
	g_ocean_simulation_param.wind_speed				= 9.0f;
	g_ocean_simulation_param.wind_dependency			= 0.98f;
	g_ocean_simulation_param.choppy_scale				= 1.f;
	g_ocean_simulation_param.small_wave_fraction		= 0.f;
	g_ocean_simulation_param.foam_dissipation_speed	= 0.6f;
	g_ocean_simulation_param.foam_falloff_speed		= 0.985f;
	g_ocean_simulation_param.foam_generation_amount	= 0.12f;
	g_ocean_simulation_param.foam_generation_threshold = 0.37f;

	g_ocean_simulation_settings.fft_period						= 1000.0f;
	g_ocean_simulation_settings.detail_level					= GFSDK_WaveWorks_Simulation_DetailLevel_Normal;
	g_ocean_simulation_settings.readback_displacements			= false;
	g_ocean_simulation_settings.num_readback_FIFO_entries		= 0;
	g_ocean_simulation_settings.aniso_level						= 4;
	g_ocean_simulation_settings.CPU_simulation_threading_model  = GFSDK_WaveWorks_Simulation_CPU_Threading_Model_Automatic;
	g_ocean_simulation_settings.use_Beaufort_scale				= true;
	g_ocean_simulation_settings.num_GPUs						= 1;
	g_ocean_simulation_settings.enable_CUDA_timers				= true;
	g_ocean_simulation_settings.enable_gfx_timers				= true;
	g_ocean_simulation_settings.enable_CPU_timers				= true;

	g_ocean_surface_param.sky_color			= D3DXVECTOR4(0.38f, 0.45f, 0.56f, 0);
	g_ocean_surface_param.waterbody_color		= D3DXVECTOR4(0.07f, 0.15f, 0.2f, 0);
	g_ocean_surface_param.sky_blending		= 16.0f;

	memset(&g_ocean_stats_simulation_filtered, 0, sizeof(g_ocean_stats_simulation_filtered));

    // Initialize dialogs
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

	g_Camera.SetRotateButtons( true, false, false );
	g_Camera.SetScalers(0.003f, 4000.0f);

	g_HUD.SetCallback( OnGUIEvent );
    g_SampleUI.SetCallback( OnGUIEvent );

	g_pNetworkClient = Client::Create();
	if(g_pNetworkClient) {
		UpdateWaveWorksParams(g_ocean_simulation_param,g_pNetworkClient->GetSimulationConfig());
	}

	AddGUISet();
	UpdateServerControlledUI();
}

void UpdateServerControlledUI()
{
	bool enable = true;
	if(g_pNetworkClient) {
		enable = false;

		// Server is in control, sync the UI values
		g_HUD.GetSlider(IDC_WIND_SPEED_SLIDER)->SetValue(int(g_ocean_simulation_param.wind_speed * kWindSpeedSliderScaleFactor));
		g_HUD.GetSlider(IDC_WIND_DEPENDENCY_SLIDER)->SetValue(int(kWindDependencySliderScaleFactor * (1.0f - g_ocean_simulation_param.wind_dependency)));
		g_HUD.GetSlider(IDC_TIME_SCALE_SLIDER)->SetValue(int(kTimeScaleSliderScaleFactor * g_ocean_simulation_param.time_scale));
	}

	g_HUD.GetControl(IDC_WIND_SPEED_SLIDER)->SetEnabled(enable);
	g_HUD.GetControl(IDC_WIND_DEPENDENCY_SLIDER)->SetEnabled(enable);
	g_HUD.GetControl(IDC_TIME_SCALE_SLIDER)->SetEnabled(enable);
}

void AddGUISet()
{
	int iY = 20;

	g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle Full Screen", 10, iY, 130, 24);
    g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change Device", 10, iY += 30, 130, 24, VK_F2);
    g_HUD.AddCheckBox(IDC_TOGGLEWIREFRAME, L"Wireframe", 10, iY += 30, 130, 24, g_RenderWireframe);
	g_HUD.AddCheckBox(IDC_TOGGLERENDER, L"Render Water", 10, iY += 30, 130, 24, g_RenderWater);
	g_HUD.AddCheckBox(IDC_TOGGLESIMULATE, L"Simulate Water", 10, iY += 30, 130, 24, g_SimulateWater);

	g_HUD.AddStatic(0, L"Wave amplitude", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_AMPLITUDE_SLIDER, 10, iY += 20, 130, 24, 0, 100, int(kAmplitudeSliderScaleFactor * g_ocean_simulation_param.wave_amplitude));

	g_HUD.AddStatic(0, L"Chop scale", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_CHOPPY_SCALE_SLIDER, 10, iY += 20, 130, 24, 0, 150, int(kChoppyScaleSliderScaleFactor * g_ocean_simulation_param.choppy_scale));

	g_HUD.AddStatic(0, L"Wind speed", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_WIND_SPEED_SLIDER, 10, iY += 20, 130, 24, 0, 100, int(g_ocean_simulation_param.wind_speed * kWindSpeedSliderScaleFactor));

	g_HUD.AddStatic(0, L"Wind dependency", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_WIND_DEPENDENCY_SLIDER, 10, iY += 20, 130, 24, 0, 100, int(kWindDependencySliderScaleFactor * (1.0f - g_ocean_simulation_param.wind_dependency)));

	g_HUD.AddStatic(0, L"Time scale", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_TIME_SCALE_SLIDER, 10, iY += 20, 130, 24, 0, 200, int(kTimeScaleSliderScaleFactor * g_ocean_simulation_param.time_scale));

	/*
	g_HUD.AddStatic(0, L"Sky blending", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_SKY_BLENDING_SLIDER, 10, iY += 20, 130, 24, 0, 1000, int(kSkyBlendingSliderScaleFactor * g_ocean_surface_param.sky_blending));
	*/

	g_HUD.AddStatic(0, L"Geomorphing", 10, iY += 40, 130, 24);
	g_HUD.AddSlider(IDC_GEOMORPH_SLIDER, 10, iY += 20, 130, 24, 0, 100, int(g_ocean_param_quadtree.geomorphing_degree * kGeomorphingDegreeSlideScaleFactor));

	g_HUD.AddStatic(0, L"Foam generation threshold", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_FOAM_THRESHOLD_SLIDER, 10, iY += 20, 130, 24, 0, 100, int(g_ocean_simulation_param.foam_generation_threshold*kFoamGenerationThresholdFactor));

	g_HUD.AddStatic(0, L"Foam generation amount", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_FOAM_AMOUNT_SLIDER, 10, iY += 20, 130, 24, 0, 100, int(g_ocean_simulation_param.foam_generation_amount*kFoamGenerationAmountFactor));

	g_HUD.AddStatic(0, L"Foam dissipation speed", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_FOAM_DISSIPATION_SLIDER, 10, iY += 20, 130, 24, 0, 100, int(g_ocean_simulation_param.foam_dissipation_speed*kFoamDissipationSpeedFactor));

	g_HUD.AddStatic(0, L"Foam fadeout speed", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_FOAM_FADEOUT_SLIDER, 10, iY += 20, 130, 24, 0, 100, int((g_ocean_simulation_param.foam_falloff_speed-0.9f)*kFoamFadeoutSpeedFactor));


	//g_HUD.AddCheckBox(IDC_ANISO_SLIDER, L"Allow Aniso", 10, iY += 35, 130, 24, g_ocean_simulation_settings.aniso);
	g_HUD.AddStatic(0, L"Aniso level", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_ANISO_SLIDER, 10, iY += 20, 130, 24, 1, 16, (int)g_ocean_simulation_settings.aniso_level);

	g_HUD.AddCheckBox(IDC_TOGGLEREADBACK, L"Readback", 10, iY += 25, 130, 24, g_ocean_simulation_settings.readback_displacements);
}

//--------------------------------------------------------------------------------------
// Called during device initialization, this code checks the device for some 
// minimum set of capabilities, and rejects those that don't pass by returning E_FAIL.
//--------------------------------------------------------------------------------------
bool CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, 
                                  D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // No fallback defined by this app, so reject any device that 
    // doesn't support at least ps1.1
    if( pCaps->PixelShaderVersion < D3DPS_VERSION(1,1) )
        return false;

    // Skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3DObject(); 
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                    AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, 
                    D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

	// Check ocean capability
	D3DADAPTER_IDENTIFIER9 adapter_id;
	pD3D->GetAdapterIdentifier(pCaps->AdapterOrdinal, 0, &adapter_id);
	if(!GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_D3D9(pD3D, adapter_id, g_ocean_simulation_settings.detail_level))
		return false;

    return true;
}


//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// DXUT will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise IDirect3D9::CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, const D3DCAPS9* pCaps, void* pUserContext )
{
    // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
    // return false.
    if( (pCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
         pCaps->VertexShaderVersion < D3DVS_VERSION(1,1) )
    {
        return false;
    }

    // Debugging vertex shaders requires either REF or software vertex processing 
    // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
    if( pDeviceSettings->DeviceType != D3DDEVTYPE_REF )
    {
        pDeviceSettings->BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
        pDeviceSettings->BehaviorFlags &= ~D3DCREATE_PUREDEVICE;                            
        pDeviceSettings->BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }
#endif
#ifdef DEBUG_PS
    pDeviceSettings->DeviceType = D3DDEVTYPE_REF;
#endif

	IDirect3D9* pD3D = DXUTGetD3DObject();
	D3DADAPTER_IDENTIFIER9 aid;
	pD3D->GetAdapterIdentifier(pDeviceSettings->AdapterOrdinal, 0, &aid);

	// NV PerfHUD
	if (pD3D != NULL)
	{
		for (UINT Adapter = 0; Adapter < pD3D->GetAdapterCount(); Adapter++)
		{
			D3DADAPTER_IDENTIFIER9 Identifier;
			pD3D->GetAdapterIdentifier(Adapter, 0, &Identifier);
			if (strstr(Identifier.Description, "PerfHUD") != 0)
			{
				pDeviceSettings->AdapterOrdinal = Adapter;
				pDeviceSettings->DeviceType = D3DDEVTYPE_REF;
				aid = Identifier;
				break;
			}
		}
	}

	static BOOL firstTime = TRUE;
	if(firstTime && aid.VendorId == 0x10DE)
	{
		if(g_pTestParams->AllowAA())
		{
			// 16x CSAA
			pDeviceSettings->pp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
			pDeviceSettings->pp.MultiSampleQuality = 4;
		}

		// Turn off vsync
		pDeviceSettings->pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

		firstTime = FALSE;
	}

	// Check detail level support
	int detail_level = 0;
	for(; detail_level != Num_GFSDK_WaveWorks_Simulation_DetailLevels; ++detail_level) {
		if(!GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_D3D9(pD3D, aid, (GFSDK_WaveWorks_Simulation_DetailLevel)detail_level))
			break;
	}
	if(0 == detail_level)
		return false;

	g_max_detail_level = (GFSDK_WaveWorks_Simulation_DetailLevel)(detail_level - 1);
	g_ocean_simulation_settings.detail_level = (GFSDK_WaveWorks_Simulation_DetailLevel)g_max_detail_level;
    return true;
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// created, which will happen during application initialization and windowed/full screen 
// toggles. This is the best location to create D3DPOOL_MANAGED resources since these 
// resources need to be reloaded whenever the device is destroyed. Resources created  
// here should be released in the OnDestroyDevice callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnCreateDevice( pd3dDevice ) );
    V_RETURN( g_SettingsDlg.OnCreateDevice( pd3dDevice ) );
    // Initialize the font
    V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
                              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                              L"Arial", &g_pFont ) );

	// Setup the camera's view parameters
	//D3DXVECTOR3 vecEye(1358.16f, 441.017f, -1558.43f);
	//D3DXVECTOR3 vecAt (881.419f, 340.248f, -1670.25f);

	D3DXVECTOR3 vecEye(0.f, 1210.534f, 0.f);
	D3DXVECTOR3 vecAt (4490.944f, 0.f, -3000.f);

	//D3DXVECTOR3 vecEye(1511.21f, 559.553f, -1164.19f);//(1691.43f, 503.88f, -1382.71f);
	//D3DXVECTOR3 vecAt (1821.63f, 429.548f, -1533.82f);
	//D3DXVECTOR3 vecEye(0, 1000, 0);
	//D3DXVECTOR3 vecAt (1000, 0, 0);
	//D3DXVECTOR3 vecEye(1691.43f, 503.88f, -1382.71f);
	//D3DXVECTOR3 vecAt (1732.75f, 429.646f, -1875.57f);
	//D3DXVECTOR3 vecEye(-1700, 1200, 1700);
	//D3DXVECTOR3 vecAt (-200, 0, 200);
	g_Camera.SetViewParams(&vecEye, &vecAt);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// reset, which will happen after a lost device scenario. This is the best location to 
// create D3DPOOL_DEFAULT resources since these resources need to be reloaded whenever 
// the device is lost. Resources created here should be released in the OnLostDevice 
// callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice, 
                                const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnResetDevice() );
    V_RETURN( g_SettingsDlg.OnResetDevice() );

    if( g_pFont )
        V_RETURN( g_pFont->OnResetDevice() );

    // Create a sprite to help batch calls when drawing many lines of text
    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pSprite ) );

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(D3DX_PI/4, fAspectRatio, g_NearPlane, g_FarPlane);
	//g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);

	// UI
	g_HUD.SetLocation(pBackBufferSurfaceDesc->Width-170, 0);
	g_HUD.SetSize(170, 170);

	GFSDK_WaveWorks_InitD3D9(pd3dDevice,NULL,GFSDK_WAVEWORKS_API_GUID);

	// Ocean sim
	GFSDK_WaveWorks_Simulation_CreateD3D9(g_ocean_simulation_settings, g_ocean_simulation_param, pd3dDevice, &g_hOceanSimulation);
	GFSDK_WaveWorks_Savestate_CreateD3D9(GFSDK_WaveWorks_StatePreserve_All, pd3dDevice, &g_hOceanSavestate);
	g_ForceKick = true;

	g_pTestParams->HookSimulation(g_hOceanSimulation);

	// Ocean object
	g_pOceanSurf = new OceanSurface();
	g_pOceanSurf->init(g_ocean_surface_param);
	g_pOceanSurf->initQuadTree(g_ocean_param_quadtree);
	GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
	GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));

	// Skybox
	TCHAR path[MAX_PATH];
	V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\skybox_d3d9.fxo")));
	V_RETURN(D3DXCreateEffectFromFile(pd3dDevice, path, NULL, NULL, D3DXFX_NOT_CLONEABLE, NULL, &g_pSkyboxFX, NULL));

	D3DVERTEXELEMENT9 sky_decl[] =
	{
		{0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		D3DDECL_END()
	};
	V_RETURN(pd3dDevice->CreateVertexDeclaration(sky_decl, &g_pSkyboxDecl));

	V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\sky_cube.dds")));
	V_RETURN(D3DXCreateCubeTextureFromFile(pd3dDevice, path, &g_pSkyCubeMap));
	V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\nvidia_logo.dds")));
	V_RETURN(D3DXCreateTextureFromFile(pd3dDevice, path, &g_pLogoTex));

	// Readback marker
	V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\ocean_marker_d3d9.fxo")));
	V_RETURN(D3DXCreateEffectFromFile(pd3dDevice, path, NULL, NULL, D3DXFX_NOT_CLONEABLE, NULL, &g_pMarkerFX, NULL));

	D3DVERTEXELEMENT9 marker_decl[] =
	{
		{0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		D3DDECL_END()
	};
	V_RETURN(pd3dDevice->CreateVertexDeclaration(marker_decl, &g_pMarkerDecl));

	return S_OK;
}

void UpdateSimulationTime(float fDXUTElapsedTime)
{
	if(g_pNetworkClient) {
		// Use server-sync'd time when running networked
		g_SimulationTime = g_pNetworkClient->GetSimulationTime();
	} else {
		g_SimulationTime += fDXUTElapsedTime;
	}
}

//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
	if(g_pNetworkClient)
	{
		if(g_pNetworkClient->IsUsable())
		{
			g_pNetworkClient->Tick();
		}

		while(g_pNetworkClient->HasPendingMessage() && g_pNetworkClient->IsUsable())
		{
			switch(g_pNetworkClient->ConsumePendingMessage())
			{
			case MessageTypeID_ServerSendConfigToClient:
				UpdateWaveWorksParams(g_ocean_simulation_param,g_pNetworkClient->GetSimulationConfig());
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
				GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
				UpdateServerControlledUI();
				break;
			}
		}

		// Abandon networking if the socket went bad for some reason
		if(!g_pNetworkClient->IsUsable())
		{
			Client::Destroy(g_pNetworkClient);
			g_pNetworkClient = NULL;
			UpdateServerControlledUI();
		}
	}

	UpdateSimulationTime(g_pTestParams->ShouldTakeScreenshot() ? 20.0f : fElapsedTime);

    // Update the camera's position based on user input 
	if(!g_pTestParams->ShouldTakeScreenshot())
	{
		g_Camera.FrameMove( fElapsedTime );
	}

	////DEBUG
	//D3DXVECTOR3 vEye = *g_Camera.GetEyePt();
	//D3DXVECTOR3 vLookAt = (*g_Camera.GetLookAtPt() - vEye) * 500 + vEye;
}

//--------------------------------------------------------------------------------------
// This callback function will be called at the end of every frame to perform all the 
// rendering calls for the scene, and it will also be called if the window needs to be 
// repainted. After this function has returned, DXUT will call 
// IDirect3DDevice9::Present to display the contents of the next buffer in the swap chain
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
	HRESULT hr = S_OK;

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
		if(g_SimulateWater || g_ForceKick || (gfsdk_waveworks_result_NONE==GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hOceanSimulation,NULL)))
		{
			// Fill the simulation pipeline - this loop should execute once in all cases except the first
			// iteration, when the simulation pipeline is first 'primed'
			do {
				GFSDK_WaveWorks_Simulation_SetTime(g_hOceanSimulation, g_SimulationTime);
				GFSDK_WaveWorks_Simulation_KickD3D9(g_hOceanSimulation, NULL, g_hOceanSavestate);
			} while(gfsdk_waveworks_result_NONE==GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hOceanSimulation,NULL));
			GFSDK_WaveWorks_Savestate_RestoreD3D9(g_hOceanSavestate);
			g_ForceKick = false;
		}

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

		if( g_SettingsDlg.IsActive() )
		{
			g_SettingsDlg.OnRender( fElapsedTime );
			return;
		}

		//if(g_RenderWater)
		{
			V(pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DXCOLOR(0.5f,0.6f,0.8f,0.0f), 1.0f, 0));
		}

		UpdateMarkers();
		if(g_ocean_simulation_settings.readback_displacements)
			RenderLocalMarkers(pd3dDevice);
		if(g_bShowRemoteMarkers)
			RenderRemoteMarkers(pd3dDevice);

		if(g_RenderWater)
		{
			const D3DXMATRIX matView = D3DXMATRIX(kWaterScale,0,0,0,0,0,kWaterScale,0,0,kWaterScale,0,0,0,0,0,1) * *g_Camera.GetViewMatrix();
			const D3DXMATRIX matProj = *g_Camera.GetProjMatrix();

			if (g_RenderWireframe)
				g_pOceanSurf->renderWireframe(matView,matProj,g_hOceanSimulation, g_hOceanSavestate, g_DebugCam);
			else
				g_pOceanSurf->renderShaded(matView,matProj,g_hOceanSimulation, g_hOceanSavestate, g_DebugCam);

			g_pOceanSurf->getQuadTreeStats(g_ocean_stats_quadtree);
		}

		//if(g_RenderWater)
		{
			RenderSkybox(pd3dDevice);
		}

		RenderLogo(pd3dDevice);

		if(!g_pTestParams->ShouldTakeScreenshot())
		{
			g_HUD.OnRender( fElapsedTime ); 
			g_SampleUI.OnRender( fElapsedTime );
			RenderText( fTime );
		}
    

		if(g_pTestParams != NULL )
		{
			g_pTestParams->Tick();

			if(g_pTestParams->IsTestingComplete() && g_pTestParams->ShouldTakeScreenshot())
			{
				int slength = (int)g_pTestParams->ScreenshotDirectory.length() + 1;
				int len = MultiByteToWideChar(CP_ACP, 0, g_pTestParams->ScreenshotDirectory.c_str(), slength, 0, 0);
				wchar_t* buf = new wchar_t[len];
				MultiByteToWideChar(CP_ACP, 0, g_pTestParams->ScreenshotDirectory.c_str(), slength, buf, len);

				IDirect3DSurface9* pBackBuffer = NULL;
				V( pd3dDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer ) );
				V( D3DXSaveSurfaceToFile( buf, D3DXIFF_BMP, pBackBuffer, NULL, NULL ) );
				SAFE_RELEASE( pBackBuffer );
				delete[] buf;

				DXUTShutdown();
				return;
			}

			V( pd3dDevice->EndScene() );
		}
    }
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText( double fTime )
{
    // The helper object simply helps keep track of text position, and color
    // and then it calls pFont->DrawText( m_pSprite, strMsg, -1, &rc, DT_NOCLIP, m_clr );
    // If NULL is passed in as the sprite object, then it will work fine however the 
    // pFont->DrawText() will not be batched together.  Batching calls will improves perf.
    CDXUTTextHelper txtHelper( g_pFont, g_pSprite, 15 );

    // Output statistics
    txtHelper.Begin();
    txtHelper.SetInsertionPos( 2, 0 );
    txtHelper.SetForegroundColor( D3DXCOLOR( 0.9f, 0.9f, 0.9f, 1.0f ) );
    txtHelper.DrawTextLine( DXUTGetFrameStats(true) );
    txtHelper.DrawTextLine( DXUTGetDeviceStats() );

	const UINT buffer_len = 256;
	WCHAR buffer[buffer_len];
	swprintf_s(buffer, buffer_len, L"Quad patches drawn: %d\n", g_ocean_stats_quadtree.num_patches_drawn);
	txtHelper.DrawTextLine(buffer);

	swprintf_s(buffer, buffer_len, L"Lib build: %S\n", GFSDK_WaveWorks_GetBuildString());
	txtHelper.DrawTextLine(buffer);
    
    txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    
    // Draw help
    if( g_bShowHelp )
    {
        const D3DSURFACE_DESC* pd3dsdBackBuffer = DXUTGetBackBufferSurfaceDesc();
        txtHelper.SetInsertionPos( 2, pd3dsdBackBuffer->Height-15*6 );
        txtHelper.SetForegroundColor( D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f ) );
        txtHelper.DrawTextLine( L"Controls:" );

        txtHelper.SetInsertionPos( 20, pd3dsdBackBuffer->Height-15*5 );
		txtHelper.DrawTextLine( L"Camera control: left mouse\n");
		txtHelper.DrawTextLine( L"W/S/A/D/Q/E to move camera" );

        txtHelper.SetInsertionPos( 250, pd3dsdBackBuffer->Height-15*5 );
        txtHelper.DrawTextLine( L"Hide help: F1\n" 
                                L"Quit: ESC\n" );
    }
    else
    {
        txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        txtHelper.DrawTextLine( L"Press F1 for help" );
		swprintf_s(buffer,buffer_len,L"GPU_gfx_time : %3.3f msec",g_ocean_stats_simulation_filtered.GPU_gfx_time);
		txtHelper.DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"GPU_update_time : %3.3f msec",g_ocean_stats_simulation_filtered.GPU_update_time);
		txtHelper.DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"CPU_main_thread_wait_time : %3.3f msec",g_ocean_stats_simulation_filtered.CPU_main_thread_wait_time);
		txtHelper.DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"CPU_threads_start_to_finish_time : %3.3f msec",g_ocean_stats_simulation_filtered.CPU_threads_start_to_finish_time);
		txtHelper.DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"CPU_threads_total_time : %3.3f msec",g_ocean_stats_simulation_filtered.CPU_threads_total_time);
		txtHelper.DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"GPU_simulation_time : %3.3f msec",g_ocean_stats_simulation_filtered.GPU_simulation_time);
		txtHelper.DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"GPU_FFT_simulation_time : %3.3f msec",g_ocean_stats_simulation_filtered.GPU_FFT_simulation_time);
		txtHelper.DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"CPU_quadtree_update_time : %3.3f msec",g_ocean_stats_quadtree.CPU_quadtree_update_time);
		txtHelper.DrawTextLine(buffer);

    }
    txtHelper.End();
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
            case VK_F1: g_bShowHelp = !g_bShowHelp; break;
			//case VK_F2: g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
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
			case VK_OEM_COMMA:
				g_ocean_param_quadtree.mesh_dim = max(g_ocean_param_quadtree.mesh_dim >> 1, 4);
				g_pOceanSurf->initQuadTree(g_ocean_param_quadtree);
				break;
			case VK_OEM_PERIOD:
				g_ocean_param_quadtree.mesh_dim = min(g_ocean_param_quadtree.mesh_dim << 1, 256);
				g_pOceanSurf->initQuadTree(g_ocean_param_quadtree);
				break;
			case 'c':
			case 'C':
				g_DebugCam = !g_DebugCam;
				break;
			case 'r':
			case 'R':
				g_bShowRemoteMarkers = !g_bShowRemoteMarkers;
				if(g_bShowRemoteMarkers)
				{
					UpdateRemoteMarkerCoords();
				}
        }
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN: DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:        DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:     g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
		case IDC_TOGGLEWIREFRAME:  g_RenderWireframe = g_HUD.GetCheckBox(IDC_TOGGLEWIREFRAME)->GetChecked(); break;
		case IDC_TOGGLERENDER:     g_RenderWater = g_HUD.GetCheckBox(IDC_TOGGLERENDER)->GetChecked(); break;
		case IDC_TOGGLESIMULATE:   g_SimulateWater =  g_HUD.GetCheckBox(IDC_TOGGLESIMULATE)->GetChecked(); break;

		case IDC_AMPLITUDE_SLIDER:
			if(EVENT_SLIDER_VALUE_FINALISED == nEvent)
			{
				g_ocean_simulation_param.wave_amplitude = FLOAT(g_HUD.GetSlider(IDC_AMPLITUDE_SLIDER)->GetValue())/kAmplitudeSliderScaleFactor;
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
				GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
			}
			break;

		case IDC_CHOPPY_SCALE_SLIDER:
			{
				g_ocean_simulation_param.choppy_scale = FLOAT(g_HUD.GetSlider(IDC_CHOPPY_SCALE_SLIDER)->GetValue())/kChoppyScaleSliderScaleFactor;
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
			}
			break;

		case IDC_WIND_SPEED_SLIDER:
			if(EVENT_SLIDER_VALUE_FINALISED == nEvent)
			{
				g_ocean_simulation_param.wind_speed = FLOAT(g_HUD.GetSlider(IDC_WIND_SPEED_SLIDER)->GetValue())/kWindSpeedSliderScaleFactor;
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
				GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
			}
			break;

		case IDC_WIND_DEPENDENCY_SLIDER:
			if(EVENT_SLIDER_VALUE_FINALISED == nEvent)
			{
				g_ocean_simulation_param.wind_dependency = 1.0f - FLOAT(g_HUD.GetSlider(IDC_WIND_DEPENDENCY_SLIDER)->GetValue())/kWindDependencySliderScaleFactor;
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
				GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
			}
			break;

		case IDC_TIME_SCALE_SLIDER:
			{
				g_ocean_simulation_param.time_scale = FLOAT(g_HUD.GetSlider(IDC_TIME_SCALE_SLIDER)->GetValue())/kTimeScaleSliderScaleFactor;
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
			}
			break;

		case IDC_SKY_BLENDING_SLIDER:
			{
				g_ocean_surface_param.sky_blending = FLOAT(g_HUD.GetSlider(IDC_SKY_BLENDING_SLIDER)->GetValue())/kSkyBlendingSliderScaleFactor;
				g_pOceanSurf->init(g_ocean_surface_param);
			}
			break;

		case IDC_GEOMORPH_SLIDER:
			{
				g_ocean_param_quadtree.geomorphing_degree = FLOAT(g_HUD.GetSlider(IDC_GEOMORPH_SLIDER)->GetValue())/kGeomorphingDegreeSlideScaleFactor;
				g_pOceanSurf->initQuadTree(g_ocean_param_quadtree);
			}
			break;

		case IDC_PATCH_LENGTH_SLIDER:
			if(EVENT_SLIDER_VALUE_FINALISED == nEvent)
			{
				g_ocean_param_quadtree.min_patch_length = FLOAT(g_HUD.GetSlider(IDC_PATCH_LENGTH_SLIDER)->GetValue());
				g_pOceanSurf->initQuadTree(g_ocean_param_quadtree);
			}
			break;

		case IDC_FURTHEST_COVER_SLIDER:
			{
				g_ocean_param_quadtree.auto_root_lod = g_HUD.GetSlider(IDC_FURTHEST_COVER_SLIDER)->GetValue();
				g_pOceanSurf->initQuadTree(g_ocean_param_quadtree);
			}
			break;

		case IDC_UPPER_GRID_COVER_SLIDER:
			{
				g_ocean_param_quadtree.upper_grid_coverage = FLOAT(g_HUD.GetSlider(IDC_UPPER_GRID_COVER_SLIDER)->GetValue());
				g_pOceanSurf->initQuadTree(g_ocean_param_quadtree);
			}
			break;

		case IDC_ANISO_SLIDER:
			{
				g_ocean_simulation_settings.aniso_level =  (gfsdk_U8)g_HUD.GetSlider(IDC_ANISO_SLIDER)->GetValue();
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
			}
			break;

		case IDC_TOGGLEREADBACK:
			{
				g_ocean_simulation_settings.readback_displacements =  g_HUD.GetCheckBox(IDC_TOGGLEREADBACK)->GetChecked();
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
			}
			break;
		case IDC_FOAM_THRESHOLD_SLIDER:
			{
				g_ocean_simulation_param.foam_generation_threshold = FLOAT(g_HUD.GetSlider(IDC_FOAM_THRESHOLD_SLIDER)->GetValue())/kFoamGenerationThresholdFactor;
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
			}
			break;
		case IDC_FOAM_AMOUNT_SLIDER:
			{
				g_ocean_simulation_param.foam_generation_amount = FLOAT(g_HUD.GetSlider(IDC_FOAM_AMOUNT_SLIDER)->GetValue())/kFoamGenerationAmountFactor;
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
			}
			break;
		case IDC_FOAM_DISSIPATION_SLIDER:
			{
				g_ocean_simulation_param.foam_dissipation_speed = FLOAT(g_HUD.GetSlider(IDC_FOAM_DISSIPATION_SLIDER)->GetValue())/kFoamDissipationSpeedFactor;
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
			}
			break;
		case IDC_FOAM_FADEOUT_SLIDER:
			{
				g_ocean_simulation_param.foam_falloff_speed = FLOAT(g_HUD.GetSlider(IDC_FOAM_FADEOUT_SLIDER)->GetValue())/kFoamFadeoutSpeedFactor + 0.9f;
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
			}
			break;
   }
    
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// entered a lost state and before IDirect3DDevice9::Reset is called. Resources created
// in the OnResetDevice callback should be released here, which generally includes all 
// D3DPOOL_DEFAULT resources. See the "Lost Devices" section of the documentation for 
// information about lost devices.
//--------------------------------------------------------------------------------------
void CALLBACK OnLostDevice( void* pUserContext )
{
	g_DialogResourceManager.OnLostDevice();
    g_SettingsDlg.OnLostDevice();
    CDXUTDirectionWidget::StaticOnLostDevice();
    if( g_pFont )
        g_pFont->OnLostDevice();
    SAFE_RELEASE(g_pSprite);

	// Ocean object
	SAFE_DELETE(g_pOceanSurf);

	if(g_hOceanSimulation)
	{
		GFSDK_WaveWorks_Simulation_Destroy(g_hOceanSimulation);
		g_hOceanSimulation = NULL;
	}

	if(g_hOceanSavestate)
	{
		GFSDK_WaveWorks_Savestate_Destroy(g_hOceanSavestate);
		g_hOceanSavestate = NULL;
	}

	GFSDK_WaveWorks_ReleaseD3D9(DXUTGetD3DDevice());

	SAFE_RELEASE(g_pSkyboxFX);
	SAFE_RELEASE(g_pSkyboxDecl);
	SAFE_RELEASE(g_pSkyCubeMap);
	SAFE_RELEASE(g_pLogoTex);
	SAFE_RELEASE(g_pMarkerFX);
	SAFE_RELEASE(g_pMarkerDecl);
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnCreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 
//--------------------------------------------------------------------------------------
void CALLBACK OnDestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnDestroyDevice();
    g_SettingsDlg.OnDestroyDevice();
    CDXUTDirectionWidget::StaticOnDestroyDevice();
    SAFE_RELEASE(g_pFont);
}

void RenderMarkers(IDirect3DDevice9* pd3dDevice, const gfsdk_float4* pMarkerPositions, int num_markers, const D3DXVECTOR4& color)
{
	g_pMarkerFX->SetVector("gMarkerColor",&color);
	g_pMarkerFX->SetTechnique("RenderMarkerTech");
	g_pMarkerFX->Begin(0, 0);

	pd3dDevice->SetVertexDeclaration(g_pMarkerDecl);

	D3DXMATRIX matView = /*D3DXMATRIX(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1) **/ *g_Camera.GetViewMatrix();
	D3DXMATRIX matProj = *g_Camera.GetProjMatrix();
	D3DXMATRIX matVP = matView * matProj;
	g_pMarkerFX->SetMatrix("g_matViewProj", &matVP);

	D3DXVECTOR4 marker_verts[5];
	static const WORD indices[] = {0,1,2, 0,2,3, 0,3,4, 0,4,1};

	g_pMarkerFX->BeginPass(0);
	for(int i = 0; i != num_markers; ++i)
	{
		D3DXVECTOR4 transformedMarkerPosition = D3DXVECTOR4(pMarkerPositions[i].x,pMarkerPositions[i].z + g_ocean_param_quadtree.sea_level,pMarkerPositions[i].y,1.f);
		transformedMarkerPosition.x *= kWaterScale;
		transformedMarkerPosition.y *= kWaterScale;
		transformedMarkerPosition.z *= kWaterScale;

		marker_verts[0] = transformedMarkerPosition;
		marker_verts[1] = transformedMarkerPosition + D3DXVECTOR4( 20.f, 20.f, 20.f, 0.f);
		marker_verts[2] = transformedMarkerPosition + D3DXVECTOR4( 20.f, 20.f,-20.f, 0.f);
		marker_verts[3] = transformedMarkerPosition + D3DXVECTOR4(-20.f, 20.f,-20.f, 0.f);
		marker_verts[4] = transformedMarkerPosition + D3DXVECTOR4(-20.f, 20.f, 20.f, 0.f);
		pd3dDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 5, 4, indices, D3DFMT_INDEX16, marker_verts, sizeof(D3DXVECTOR4));
	}
	g_pMarkerFX->EndPass();

	g_pMarkerFX->End();
}

enum { NumMarkersXY = 7, NumMarkers = NumMarkersXY*NumMarkersXY };

gfsdk_float2 g_local_marker_coords[NumMarkers];
gfsdk_float2 g_remote_marker_coords[NumMarkers];
gfsdk_float4 g_local_marker_positions[NumMarkers];

void RenderLocalMarkers(IDirect3DDevice9* pd3dDevice)
{
	RenderMarkers(pd3dDevice, g_local_marker_positions, NumMarkers, D3DXVECTOR4(1.f,0.f,0.f,1.f));
}

void RenderRemoteMarkers(IDirect3DDevice9* pd3dDevice)
{
	if(g_pNetworkClient) {
		const gfsdk_float4* pMarkerPositions = NULL;
		size_t numMarkers = 0;
		if(g_pNetworkClient->GetRemoteMarkerPositions(pMarkerPositions, numMarkers)) {
			RenderMarkers(pd3dDevice, pMarkerPositions, (int)numMarkers, D3DXVECTOR4(1.f,1.f,0.f,1.f));
		}
	}
}

void UpdateRemoteMarkerCoords()
{
	memcpy(g_remote_marker_coords,g_local_marker_coords,sizeof(g_remote_marker_coords));
}

void UpdateMarkers()
{
	HRESULT hr;

	// Find where the camera vector intersects mean sea level
	D3DXVECTOR3 eye_pos = *g_Camera.GetEyePt();
	D3DXVECTOR3 lookat_pos = *g_Camera.GetLookAtPt();
	const FLOAT intersectionHeight = g_ocean_param_quadtree.sea_level;
	const FLOAT lambda = (intersectionHeight - eye_pos.y)/(lookat_pos.y - eye_pos.y);
	const D3DXVECTOR3 sea_level_pos = (1.f - lambda) * eye_pos + lambda * lookat_pos;
	const D3DXVECTOR2 sea_level_xy(sea_level_pos.x, sea_level_pos.z);

	// Update local marker coords, we could need them any time for remote
	for(int x = 0; x != NumMarkersXY; ++x)
	{
		for(int y = 0; y != NumMarkersXY; ++y)
		{
			g_local_marker_coords[y * NumMarkersXY + x] = NvFromDX(sea_level_xy/kWaterScale + D3DXVECTOR2(2.f * (x-((NumMarkersXY-1)/2)), 2.f * (y-((NumMarkersXY-1)/2))));
		}
	}

	// Do local readback, if requested
	if(g_ocean_simulation_settings.readback_displacements)
	{
		gfsdk_float4 displacements[NumMarkers];
		V(GFSDK_WaveWorks_Simulation_GetDisplacements(g_hOceanSimulation, g_local_marker_coords, displacements, NumMarkers));

		for(int ix = 0; ix != NumMarkers; ++ix)
		{
			g_local_marker_positions[ix].x = displacements[ix].x + g_local_marker_coords[ix].x;
			g_local_marker_positions[ix].y = displacements[ix].y + g_local_marker_coords[ix].y;
			g_local_marker_positions[ix].z = displacements[ix].z;
			g_local_marker_positions[ix].w = 1.f;
		}
	}

	if(g_bShowRemoteMarkers && NULL != g_pNetworkClient)
	{
		g_pNetworkClient->RequestRemoteMarkerPositions(g_remote_marker_coords,NumMarkers);
	}
}

void RenderSkybox(IDirect3DDevice9* pd3dDevice)
{
	g_pSkyboxFX->SetTechnique("SkyboxTech");
	g_pSkyboxFX->Begin(0, 0);

	pd3dDevice->SetVertexDeclaration(g_pSkyboxDecl);

	D3DXMATRIX matView = D3DXMATRIX(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1) * *g_Camera.GetViewMatrix();
	D3DXMATRIX matProj = *g_Camera.GetProjMatrix();
	D3DXMATRIX matVP = matView * matProj;
	g_pSkyboxFX->SetMatrix("g_matViewProj", &matVP);

	D3DXVECTOR3 eye_pos = *g_Camera.GetEyePt();
	g_pSkyboxFX->SetVector("g_EyePos", (D3DXVECTOR4*)&eye_pos);

	g_pSkyboxFX->SetTexture("g_texSkyCube", g_pSkyCubeMap);

	D3DXVECTOR4 far_plane_quad[4];
	far_plane_quad[0] = D3DXVECTOR4(-g_FarPlane,  g_FarPlane, g_FarPlane * 0.999f, g_FarPlane);
	far_plane_quad[1] = D3DXVECTOR4(-g_FarPlane, -g_FarPlane, g_FarPlane * 0.999f, g_FarPlane);
	far_plane_quad[2] = D3DXVECTOR4( g_FarPlane,  g_FarPlane, g_FarPlane * 0.999f, g_FarPlane);
	far_plane_quad[3] = D3DXVECTOR4( g_FarPlane, -g_FarPlane, g_FarPlane * 0.999f, g_FarPlane);
	D3DXMATRIX matInvVP;
	D3DXMatrixInverse(&matInvVP, NULL, &matVP);
	D3DXVec4TransformArray(&far_plane_quad[0], sizeof(D3DXVECTOR4), &far_plane_quad[0], sizeof(D3DXVECTOR4), &matInvVP, 4);

	g_pSkyboxFX->BeginPass(0);
	pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &far_plane_quad, sizeof(D3DXVECTOR4));
	g_pSkyboxFX->EndPass();

	g_pSkyboxFX->End();
}

void RenderLogo(IDirect3DDevice9* pd3dDevice)
{
	g_pOceanSurf->m_pOceanFX->SetTechnique("DisplayBufferTech");

	g_pOceanSurf->m_pOceanFX->Begin(0, 0);
	{
		g_pOceanSurf->m_pOceanFX->SetTexture("g_texColorMap", g_pLogoTex);

		D3DVIEWPORT9 vp;
		pd3dDevice->GetViewport(&vp);
		float width = (float)200/vp.Width;
		float height = (float)160/vp.Height;

		// Draw slop-map
		pd3dDevice->SetVertexDeclaration(g_pOceanSurf->m_pQuadDecl);
		float half_tex = 0;
		float vertices0[] = {0.98f - width, -0.96f + height, 0,    half_tex,      half_tex,
							 0.98f - width, -0.96f,          0,    half_tex,      half_tex+1.0f,
							 0.98f,         -0.96f + height, 0,    half_tex+1.0f, half_tex,
							 0.98f,         -0.96f,          0,    half_tex+1.0f, half_tex+1.0f};
		g_pOceanSurf->m_pOceanFX->BeginPass(0);
		pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices0, 20);
		g_pOceanSurf->m_pOceanFX->EndPass();
	}
	g_pOceanSurf->m_pOceanFX->End();
}
