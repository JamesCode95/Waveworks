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
#include "resource.h"

#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"

#include "../testing_src/testing.h"

#include "ocean_surface.h"
#include "GFSDK_WaveWorks.h"
#include "GFSDK_WaveWorks_D3D_Util.h"

#include "client.h"
#include "message_types.h"
#include <shlwapi.h>
#include <tchar.h>
#include "DDSTextureLoader.h"


//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 

// Disable warning "conditional expression is constant"
#pragma warning(disable:4127)

extern HRESULT LoadFile(LPCTSTR FileName, ID3DBlob** ppBuffer);


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
bool                    g_bShowHelp = false;    // If true, it renders the UI control text
CFirstPersonCamera      g_Camera;               //
CDXUTDialogResourceManager g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg         g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*        g_pTxtHelper = NULL;    // Text helper
CDXUTDialog             g_HUD;                  // manages the 3D UI
CDXUTDialog             g_SampleUI;             // dialog for sample specific controls

Client* g_pNetworkClient = NULL;					// For testing client/server use-cases

TestParams* g_pTestParams = NULL;

GFSDK_WaveWorks_SavestateHandle	g_hOceanSavestate = NULL;
GFSDK_WaveWorks_SimulationHandle g_hOceanSimulation = NULL;
gfsdk_U64 g_LastKickID = 0;
gfsdk_U64 g_LastArchivedKickID = GFSDK_WaveWorks_InvalidKickID;
gfsdk_U64 g_LastReadbackKickID = GFSDK_WaveWorks_InvalidKickID;
gfsdk_U32 g_RenderLatency = 0;
gfsdk_S32 g_ReadbackLatency = 0;
enum { ReadbackArchiveSize = 32 };
enum { ReadbackArchiveInterval = 30 };
float g_ReadbackCoord = 0.f;
OceanSurface*			g_pOceanSurf = NULL;
GFSDK_WaveWorks_Simulation_Params	g_ocean_simulation_param;
GFSDK_WaveWorks_Simulation_Settings	g_ocean_simulation_settings;

OceanSurfaceParameters			g_ocean_surface_param;
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

enum SynchronizationMode
{
	SynchronizationMode_None = 0,
	SynchronizationMode_RenderOnly,
	SynchronizationMode_Readback,
	Num_SynchronizationModes
};
SynchronizationMode g_bSyncMode = SynchronizationMode_None;

float g_TessellationLOD = 8.0f;

ID3D11ShaderResourceView* g_pSkyCubeMap = NULL;
ID3D11ShaderResourceView* g_pLogoTex = NULL;
ID3DX11Effect* g_pSkyboxFX = NULL;
ID3DX11EffectTechnique* g_pSkyBoxTechnique = NULL;
ID3DX11EffectShaderResourceVariable* g_pSkyBoxSkyCubeMapVariable = NULL;
ID3DX11EffectVectorVariable* g_pSkyBoxEyePosVariable = NULL;
ID3DX11EffectMatrixVariable* g_pSkyBoxMatViewProjVariable = NULL;
ID3D11Buffer* g_pSkyBoxVB = NULL;
ID3D11Buffer* g_pLogoVB = NULL;

// These are borrowed from the effect in g_pOceanSurf
ID3DX11EffectTechnique* g_pLogoTechnique = NULL;
ID3DX11EffectShaderResourceVariable* g_pLogoTextureVariable = NULL;

ID3D11InputLayout* g_pSkyboxLayout = NULL;

ID3D11InputLayout* g_pMarkerLayout = NULL;
ID3DX11Effect* g_pMarkerFX = NULL;
ID3DX11EffectTechnique* g_pMarkerTechnique = NULL;
ID3DX11EffectMatrixVariable* g_pMarkerMatViewProjVariable = NULL;
ID3DX11EffectVectorVariable* g_pMarkerColor = NULL;
ID3D11Buffer* g_pMarkerVB = NULL;
ID3D11Buffer* g_pMarkerIB = NULL;

float g_NearPlane = 200.0f;
float g_FarPlane = 300000.0f;
double g_SimulationTime = 0.0;

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
#define IDC_FOAM_THRESHOLD_SLIDER		24
#define IDC_FOAM_AMOUNT_SLIDER			25
#define IDC_FOAM_DISSIPATION_SLIDER		26
#define IDC_FOAM_FADEOUT_SLIDER			27
#define IDC_TESSELLATION_LOD_SLIDER			28

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
//bool    CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
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
void    RenderText( double fTime );
void	AddGUISet();
void	RenderSkybox(ID3D11DeviceContext* pDC);
void	RenderLogo(ID3D11DeviceContext* pDC);
void	RenderMarkers(ID3D11DeviceContext* pDC);
void	UpdateServerControlledUI();
void	RenderLocalMarkers(ID3D11DeviceContext* pDC);
void	RenderRemoteMarkers(ID3D11DeviceContext* pDC);
void	UpdateMarkers();
void	UpdateRemoteMarkerCoords();

// custom memory management
void*	myMalloc(size_t size);
void    myFree(void* p);
size_t    AllocatedMemory = 0;
size_t    NumAllocations = 0;
size_t    AllocatedAlignedMemory = 0;
size_t    NumAlignedAllocations = 0;

void*   myMalloc(size_t size)
{
	AllocatedMemory += size;
	NumAllocations ++;
	return malloc(size);
}

void   myFree(void* p)
{
	NumAllocations --;
	free(p);
}

void*   myAlignedMalloc(size_t size, size_t alignment)
{
	AllocatedAlignedMemory += size;
	NumAlignedAllocations ++;
	return _aligned_malloc(size, alignment);
}

void   myAlignedFree(void* p)
{
	NumAlignedAllocations --;
	_aligned_free(p);
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

	g_pTestParams = new TestParams(cmdline);

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

//	DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

	// TBD: Switch to gamma-correct
	DXUTSetIsInGammaCorrectMode(false);

    // Show the cursor and clip it when in full screen
    DXUTSetCursorSettings( true, true );

    InitApp();

    // Initialize DXUT and create the desired Win32 window and Direct3D 
    // device for the application. Calling each of these functions is optional, but they
    // allow you to set several options which control the behavior of the framework.
    DXUTInit( true, true, NULL ); // Parse the command line, and show msgboxes
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"FFT Ocean" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 1280, 720 );

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
	g_ocean_param_quadtree.tessellation_lod		= 40.0f;
	g_ocean_param_quadtree.geomorphing_degree	= 1.f;
	g_ocean_param_quadtree.use_tessellation = true;
	g_ocean_param_quadtree.enable_CPU_timers	= true;

	g_ocean_simulation_param.time_scale				= 0.5f;
	g_ocean_simulation_param.wave_amplitude			= 1.0f;
	g_ocean_simulation_param.wind_dir					= NvFromDX(XMFLOAT2(0.8f, 0.6f));
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
	g_ocean_simulation_settings.num_readback_FIFO_entries		= ReadbackArchiveSize;
	g_ocean_simulation_settings.aniso_level						= 4;
	g_ocean_simulation_settings.CPU_simulation_threading_model  = GFSDK_WaveWorks_Simulation_CPU_Threading_Model_Automatic;
	g_ocean_simulation_settings.use_Beaufort_scale				= false;
	g_ocean_simulation_settings.num_GPUs						= 1;
	g_ocean_simulation_settings.enable_CUDA_timers				= true;
	g_ocean_simulation_settings.enable_gfx_timers				= true;
	g_ocean_simulation_settings.enable_CPU_timers				= true;
	
	g_ocean_surface_param.sky_color			= XMFLOAT4(0.38f, 0.45f, 0.56f, 0);
	g_ocean_surface_param.waterbody_color = XMFLOAT4(0.07f, 0.15f, 0.2f, 0);
	g_ocean_surface_param.sky_blending		= 100.0f;

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
	g_HUD.AddSlider(IDC_WIND_SPEED_SLIDER, 10, iY += 20, 130, 24, 0, 200, int(g_ocean_simulation_param.wind_speed * kWindSpeedSliderScaleFactor));

	g_HUD.AddStatic(0, L"Wind dependency", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_WIND_DEPENDENCY_SLIDER, 10, iY += 20, 130, 24, 0, 100, int(kWindDependencySliderScaleFactor * (1.0f - g_ocean_simulation_param.wind_dependency)));

	g_HUD.AddStatic(0, L"Time scale", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_TIME_SCALE_SLIDER, 10, iY += 20, 130, 24, 0, 200, int(kTimeScaleSliderScaleFactor * g_ocean_simulation_param.time_scale));

	g_HUD.AddStatic(0, L"Sky blending", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_SKY_BLENDING_SLIDER, 10, iY += 20, 130, 24, 0, 1000, int(kSkyBlendingSliderScaleFactor * g_ocean_surface_param.sky_blending));

	g_HUD.AddStatic(0, L"Foam generation threshold", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_FOAM_THRESHOLD_SLIDER, 10, iY += 20, 130, 24, 0, 100, int(g_ocean_simulation_param.foam_generation_threshold*kFoamGenerationThresholdFactor));

	g_HUD.AddStatic(0, L"Foam generation amount", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_FOAM_AMOUNT_SLIDER, 10, iY += 20, 130, 24, 0, 100, int(g_ocean_simulation_param.foam_generation_amount*kFoamGenerationAmountFactor));

	g_HUD.AddStatic(0, L"Foam dissipation speed", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_FOAM_DISSIPATION_SLIDER, 10, iY += 20, 130, 24, 0, 100, int(g_ocean_simulation_param.foam_dissipation_speed*kFoamDissipationSpeedFactor));

	g_HUD.AddStatic(0, L"Foam fadeout speed", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_FOAM_FADEOUT_SLIDER, 10, iY += 20, 130, 24, 0, 100, int((g_ocean_simulation_param.foam_falloff_speed-0.9f)*kFoamFadeoutSpeedFactor));

	g_HUD.AddStatic(0, L"Aniso level", 10, iY += 25, 130, 24);
	g_HUD.AddSlider(IDC_ANISO_SLIDER, 10, iY += 20, 130, 24, 1, 16, (int)g_ocean_simulation_settings.aniso_level);

	g_HUD.AddCheckBox(IDC_TOGGLEREADBACK, L"Readback", 10, iY += 40, 130, 24, g_ocean_simulation_settings.readback_displacements);

	g_HUD.AddStatic(0, L"Tessellation LOD", 10, iY += 40, 130, 24);
	g_HUD.AddSlider(IDC_TESSELLATION_LOD_SLIDER, 10, iY += 20, 130, 24, 0, 200, int(g_TessellationLOD));
}

//--------------------------------------------------------------------------------------
// Called during device initialization, this code checks the device for some 
// minimum set of capabilities, and rejects those that don't pass by returning E_FAIL.
//--------------------------------------------------------------------------------------
// bool CALLBACK IsD3D9DeviceAcceptable(	D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, 
// 										D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
// {
// 	return false;
// }

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

    static bool s_bFirstTime = true;
    if( s_bFirstTime && ad.VendorId == 0x10DE )
    {
        s_bFirstTime = false;

		if(g_pTestParams->AllowAA())
		{
			// 16x CSAA
			pDeviceSettings->d3d11.sd.SampleDesc.Count = 4;
			pDeviceSettings->d3d11.sd.SampleDesc.Quality = 16;
		}

		// Turn off vsync
		pDeviceSettings->d3d11.SyncInterval = 0;
    }

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

	// Setup the camera's view parameters
	//D3DXVECTOR3 vecEye(1358.16f, 441.017f, -1558.43f);
	//D3DXVECTOR3 vecAt (881.419f, 340.248f, -1670.25f);

	XMVECTOR vecEye = DirectX::XMVectorSet(0.f, 1210.534f, 0.f, 0.f);
	XMVECTOR vecAt = DirectX::XMVectorSet(4490.944f, 0.f, -3000.f, 0.f);

	//D3DXVECTOR3 vecEye(1511.21f, 559.553f, -1164.19f);//(1691.43f, 503.88f, -1382.71f);
	//D3DXVECTOR3 vecAt (1821.63f, 429.548f, -1533.82f);
	//D3DXVECTOR3 vecEye(0, 1000, 0);
	//D3DXVECTOR3 vecAt (1000, 0, 0);
	//D3DXVECTOR3 vecEye(1691.43f, 503.88f, -1382.71f);
	//D3DXVECTOR3 vecAt (1732.75f, 429.646f, -1875.57f);
	//D3DXVECTOR3 vecEye(-1700, 1200, 1700);
	//D3DXVECTOR3 vecAt (-200, 0, 200);
	g_Camera.SetViewParams(vecEye, vecAt);

	GFSDK_WaveWorks_Malloc_Hooks malloHooks;
	malloHooks.pMalloc = myMalloc;
	malloHooks.pFree = myFree;
	malloHooks.pAlignedMalloc = myAlignedMalloc;
	malloHooks.pAlignedFree = myAlignedFree;
	GFSDK_WaveWorks_InitD3D11(pd3dDevice,&malloHooks,GFSDK_WAVEWORKS_API_GUID);

	// Do an unnecessary release-init cycle, to test that specific use-case
	GFSDK_WaveWorks_InitD3D11(pd3dDevice,&malloHooks,GFSDK_WAVEWORKS_API_GUID);
	GFSDK_WaveWorks_ReleaseD3D11(pd3dDevice);
	GFSDK_WaveWorks_ReleaseD3D11(pd3dDevice);
	GFSDK_WaveWorks_InitD3D11(pd3dDevice,&malloHooks,GFSDK_WAVEWORKS_API_GUID);

	// Ocean sim
	GFSDK_WaveWorks_Simulation_CreateD3D11(g_ocean_simulation_settings, g_ocean_simulation_param, pd3dDevice, &g_hOceanSimulation);
	GFSDK_WaveWorks_Savestate_CreateD3D11(GFSDK_WaveWorks_StatePreserve_All, pd3dDevice, &g_hOceanSavestate);
	g_ForceKick = true;
	g_pTestParams->HookSimulation(g_hOceanSimulation);

	// Ocean object
	g_pOceanSurf = new OceanSurface();
	g_pOceanSurf->init(g_ocean_surface_param);
	g_pOceanSurf->initQuadTree(g_ocean_param_quadtree);
	GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));

	// Effect hooks borrowed from ocean object
	g_pLogoTechnique = g_pOceanSurf->m_pOceanFX->GetTechniqueByName("DisplayBufferTech");
	g_pLogoTextureVariable = g_pOceanSurf->m_pOceanFX->GetVariableByName("g_texBufferMap")->AsShaderResource();

	// Skybox
    {
		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\skybox_d3d11.fxo")));
        ID3DBlob* pEffectBuffer = NULL;
        V_RETURN(LoadFile(path, &pEffectBuffer));
        V_RETURN(D3DX11CreateEffectFromMemory(pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, pd3dDevice, &g_pSkyboxFX));
        pEffectBuffer->Release();
    }

	g_pSkyBoxTechnique = g_pSkyboxFX->GetTechniqueByName("SkyboxTech");
	g_pSkyBoxMatViewProjVariable = g_pSkyboxFX->GetVariableByName("g_matViewProj")->AsMatrix();
	g_pSkyBoxEyePosVariable = g_pSkyboxFX->GetVariableByName("g_EyePos")->AsVector();
	g_pSkyBoxSkyCubeMapVariable = g_pSkyboxFX->GetVariableByName("g_texSkyCube")->AsShaderResource();

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

	TCHAR path[MAX_PATH];
	V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\sky_cube.dds")));
	ID3D11Resource* pD3D11Resource = NULL;
	V_RETURN(DirectX::CreateDDSTextureFromFile(pd3dDevice, static_cast<const wchar_t *>(path), &pD3D11Resource, &g_pSkyCubeMap));
//	V_RETURN(pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &g_pSkyCubeMap));
	SAFE_RELEASE(pD3D11Resource);

	V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\nvidia_logo.dds")));
	V_RETURN(DirectX::CreateDDSTextureFromFile(pd3dDevice, static_cast<const wchar_t *>(path), &pD3D11Resource, &g_pLogoTex));
//	V_RETURN(pd3dDevice->CreateShaderResourceView(pD3D11Resource, NULL, &g_pLogoTex));
	SAFE_RELEASE(pD3D11Resource);

	{
		D3D11_BUFFER_DESC vBufferDesc;
		vBufferDesc.ByteWidth = 4 * sizeof(XMFLOAT4);
		vBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vBufferDesc.MiscFlags = 0;
		V_RETURN(pd3dDevice->CreateBuffer(&vBufferDesc, NULL, &g_pSkyBoxVB));
	}

	// Readback marker
    {
		TCHAR path[MAX_PATH];
		V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\ocean_marker_d3d11.fxo")))
        ID3DBlob* pEffectBuffer = NULL;
        V_RETURN(LoadFile(path, &pEffectBuffer));
        V_RETURN(D3DX11CreateEffectFromMemory(pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, pd3dDevice, &g_pMarkerFX));
        pEffectBuffer->Release();
    }

	g_pMarkerTechnique = g_pMarkerFX->GetTechniqueByName("RenderMarkerTech");
	g_pMarkerMatViewProjVariable = g_pMarkerFX->GetVariableByName("g_matViewProj")->AsMatrix();
	g_pMarkerColor = g_pMarkerFX->GetVariableByName("gMarkerColor")->AsVector();

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
		vBufferDesc.ByteWidth = 5 * sizeof(XMFLOAT4);
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

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice,pBackBufferSurfaceDesc) );
    V_RETURN( g_SettingsDlg.OnD3D11ResizedSwapChain(pd3dDevice,pBackBufferSurfaceDesc) );

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(DirectX::XMVectorGetX(g_XMPi/4), fAspectRatio, g_NearPlane, g_FarPlane);
	//g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);

	// UI
	g_HUD.SetLocation(pBackBufferSurfaceDesc->Width-200, 0);
	g_HUD.SetSize(170, 170);

	// Create Vb for logo
	float width = (float)200/pBackBufferSurfaceDesc->Width;
	float height = (float)160/pBackBufferSurfaceDesc->Height;
	float half_tex = 0;
	float vertices0[] = {0.98f - width, -0.96f + height, 0,    half_tex,      half_tex,
						 0.98f - width, -0.96f,          0,    half_tex,      half_tex+1.0f,
						 0.98f,         -0.96f + height, 0,    half_tex+1.0f, half_tex,
						 0.98f,         -0.96f,          0,    half_tex+1.0f, half_tex+1.0f};

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
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
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
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pDC, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
	if(g_SimulateWater || g_ForceKick || (gfsdk_waveworks_result_NONE==GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hOceanSimulation,NULL)))
	{
		GFSDK_WaveWorks_Simulation_SetTime(g_hOceanSimulation, g_SimulationTime);
		GFSDK_WaveWorks_Simulation_KickD3D11(g_hOceanSimulation, &g_LastKickID, pDC, g_hOceanSavestate);

		if(g_bSyncMode >= SynchronizationMode_RenderOnly)
		{
			// Block until the just-submitted kick is ready to render
			gfsdk_U64 stagingCursorKickID = g_LastKickID - 1;	// Just ensure that the initial value is different from last kick,
																// so that we continue waiting if the staging cursor is empty
			GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hOceanSimulation, &stagingCursorKickID);
			while(stagingCursorKickID != g_LastKickID) 
			{
				const bool doBlock = true;
				GFSDK_WaveWorks_Simulation_AdvanceStagingCursorD3D11(g_hOceanSimulation, doBlock, pDC, g_hOceanSavestate);
				GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hOceanSimulation, &stagingCursorKickID);
			}

			if(g_bSyncMode >= SynchronizationMode_Readback && g_ocean_simulation_settings.readback_displacements)
			{
				gfsdk_U64 readbackCursorKickID = g_LastKickID - 1;	// Just ensure that the initial value is different from last kick,
																	// so that we continue waiting if the staging cursor is empty
				while(readbackCursorKickID != g_LastKickID) 
				{
					const bool doBlock = true;
					GFSDK_WaveWorks_Simulation_AdvanceReadbackCursor(g_hOceanSimulation, doBlock);
					GFSDK_WaveWorks_Simulation_GetReadbackCursor(g_hOceanSimulation, &readbackCursorKickID);
				}
			}
		}
		else
		{
			// Keep feeding the simulation pipeline until it is full - this loop should skip in all
			// cases except the first iteration, when the simulation pipeline is first 'primed'
			while(gfsdk_waveworks_result_NONE==GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hOceanSimulation,NULL))
			{
				GFSDK_WaveWorks_Simulation_SetTime(g_hOceanSimulation, g_SimulationTime);
				GFSDK_WaveWorks_Simulation_KickD3D11(g_hOceanSimulation, &g_LastKickID, pDC, g_hOceanSavestate);
			}
		}

		GFSDK_WaveWorks_Savestate_RestoreD3D11(g_hOceanSavestate, pDC);
		g_ForceKick = false;

		// Exercise the readback archiving API
		if(gfsdk_waveworks_result_OK == GFSDK_WaveWorks_Simulation_GetReadbackCursor(g_hOceanSimulation, &g_LastReadbackKickID))
		{
			if((g_LastReadbackKickID-g_LastArchivedKickID) > ReadbackArchiveInterval)
			{
				GFSDK_WaveWorks_Simulation_ArchiveDisplacements(g_hOceanSimulation);
				g_LastArchivedKickID = g_LastReadbackKickID;
			}
		}
	}

	// deduce the rendering latency of the WaveWorks pipeline
	{
		gfsdk_U64 staging_cursor_kickID = 0;
		GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hOceanSimulation,&staging_cursor_kickID);
		g_RenderLatency = (gfsdk_U32)(g_LastKickID - staging_cursor_kickID);
	}

	// likewise with the readback latency
	if(g_ocean_simulation_settings.readback_displacements)
	{
		gfsdk_U64 readback_cursor_kickID = 0;
		if(gfsdk_waveworks_result_OK == GFSDK_WaveWorks_Simulation_GetReadbackCursor(g_hOceanSimulation,&readback_cursor_kickID))
		{
			g_ReadbackLatency = (gfsdk_S32)(g_LastKickID - readback_cursor_kickID);
		}
		else
		{
			g_ReadbackLatency = -1;
		}
	}
	else
	{
		g_ReadbackLatency = -1;
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

    // If the settings dialog is being shown, then
    // render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target and depth stencil
    float ClearColor[4] = { 0.0f, 0.5f, 0.6f, 0.8f };
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    pDC->ClearRenderTargetView( pRTV, ClearColor );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pDC->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

	UpdateMarkers();
	if(g_ocean_simulation_settings.readback_displacements)
		RenderLocalMarkers(pDC);
	if(g_bShowRemoteMarkers)
		RenderRemoteMarkers(pDC);

	if(g_RenderWater)
	{
		const XMMATRIX matView = XMMATRIX(kWaterScale, 0, 0, 0, 0, 0, kWaterScale, 0, 0, kWaterScale, 0, 0, 0, 0, 0, 1) *g_Camera.GetViewMatrix();
		const XMMATRIX matProj = g_Camera.GetProjMatrix();

		if (g_RenderWireframe)
			g_pOceanSurf->renderWireframe(pDC, matView,matProj,g_hOceanSimulation, g_hOceanSavestate, g_DebugCam);
		else
			g_pOceanSurf->renderShaded(pDC, matView,matProj,g_hOceanSimulation, g_hOceanSavestate, g_DebugCam);

		g_pOceanSurf->getQuadTreeStats(g_ocean_stats_quadtree);
	}

	//if(g_RenderWater)
	{
		RenderSkybox(pDC);
	}

	RenderLogo(pDC);

	if(!g_pTestParams->ShouldTakeScreenshot())
	{
		g_HUD.OnRender( fElapsedTime ); 
		g_SampleUI.OnRender( fElapsedTime );
		RenderText( fTime );
	}

	pDC->GSSetShader(NULL,NULL,0);

	if(g_pTestParams != NULL )
	{
		g_pTestParams->Tick();

		if(g_pTestParams->IsTestingComplete() && g_pTestParams->ShouldTakeScreenshot())
		{
			int slength = (int)g_pTestParams->ScreenshotDirectory.length() + 1;
			int len = MultiByteToWideChar(CP_ACP, 0, g_pTestParams->ScreenshotDirectory.c_str(), slength, 0, 0);
			wchar_t* buf = new wchar_t[len];
			MultiByteToWideChar(CP_ACP, 0, g_pTestParams->ScreenshotDirectory.c_str(), slength, buf, len);

			DXUTSnapD3D11Screenshot( buf, false );
			delete[] buf;

			DXUTShutdown();
		}
	}
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText( double fTime )
{
    // Output statistics
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
	g_pTxtHelper->SetForegroundColor(XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f));
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats(true) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

	const UINT buffer_len = 256;
	WCHAR buffer[buffer_len];
	swprintf_s(buffer, buffer_len, L"Quad patches drawn: %d\n", g_ocean_stats_quadtree.num_patches_drawn);
	g_pTxtHelper->DrawTextLine(buffer);
    
    g_pTxtHelper->SetForegroundColor( XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) );
    
    // Draw help
    if( g_bShowHelp )
    {
        const DXGI_SURFACE_DESC* pd3dsdBackBuffer = DXUTGetDXGIBackBufferSurfaceDesc();
        g_pTxtHelper->SetInsertionPos( 2, pd3dsdBackBuffer->Height-15*6 );
		g_pTxtHelper->SetForegroundColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
        g_pTxtHelper->DrawTextLine( L"Controls:" );

        g_pTxtHelper->SetInsertionPos( 20, pd3dsdBackBuffer->Height-15*5 );
		g_pTxtHelper->DrawTextLine( L"Camera control: left mouse\n");
		g_pTxtHelper->DrawTextLine( L"W/S/A/D/Q/E to move camera" );

        g_pTxtHelper->SetInsertionPos( 250, pd3dsdBackBuffer->Height-15*5 );
        g_pTxtHelper->DrawTextLine( L"Hide help: F1\n" 
                                L"Quit: ESC\n" );
    }
    else
    {
		g_pTxtHelper->SetForegroundColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
        g_pTxtHelper->DrawTextLine( L"Press F1 for help" );
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

		swprintf_s(buffer,buffer_len,L"        NumAllocations           : %i",NumAllocations);
		g_pTxtHelper->DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"        NumAlignedAllocations    : %i",NumAlignedAllocations);
		g_pTxtHelper->DrawTextLine(buffer);

		swprintf_s(buffer,buffer_len,L"        RenderLatency            : %i",g_RenderLatency);
		g_pTxtHelper->DrawTextLine(buffer);
		if(g_ReadbackLatency >= 0)
		{
			swprintf_s(buffer,buffer_len,L"        ReadbackLatency          : %i",g_ReadbackLatency);
			g_pTxtHelper->DrawTextLine(buffer);
		}
		else
		{
			swprintf_s(buffer,buffer_len,L"        ReadbackLatency          : <NONE-AVAILABLE>");
			g_pTxtHelper->DrawTextLine(buffer);
		}
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
				g_ocean_param_quadtree.mesh_dim = std::max(g_ocean_param_quadtree.mesh_dim >> 1, 4);
				g_pOceanSurf->initQuadTree(g_ocean_param_quadtree);
				break;
			case VK_OEM_PERIOD:
				g_ocean_param_quadtree.mesh_dim = std::min(g_ocean_param_quadtree.mesh_dim << 1, 256);
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
				break;
			case VK_OEM_4:	//  '[{' for US
				g_bSyncMode = (SynchronizationMode)((g_bSyncMode+1)%Num_SynchronizationModes);
				break;
			case 'n':
			case 'N':
				if(1.f == g_ReadbackCoord)
					g_ReadbackCoord = 0.f;
				else
				{
					g_ReadbackCoord -= 0.5f;
					if(g_ReadbackCoord < 0.f)
						g_ReadbackCoord = 0.f;
				}
				break;
			case 'm':
			case 'M':
				if(0.f == g_ReadbackCoord)
					g_ReadbackCoord = 1.f;
				else
				{
					g_ReadbackCoord += 0.5f;
					if(g_ReadbackCoord > (ReadbackArchiveSize-1))
						g_ReadbackCoord = (ReadbackArchiveSize-1);
				}
				break;
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
			if(EVENT_SLIDER_VALUE_CHANGED_UP == nEvent)
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
			if(EVENT_SLIDER_VALUE_CHANGED_UP == nEvent)
			{
				g_ocean_simulation_param.wind_speed = FLOAT(g_HUD.GetSlider(IDC_WIND_SPEED_SLIDER)->GetValue())/kWindSpeedSliderScaleFactor;
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
				GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
			}
			break;

		case IDC_WIND_DEPENDENCY_SLIDER:
			if(EVENT_SLIDER_VALUE_CHANGED_UP == nEvent)
			{
				g_ocean_simulation_param.wind_dependency = FLOAT(g_HUD.GetSlider(IDC_WIND_DEPENDENCY_SLIDER)->GetValue())/kWindDependencySliderScaleFactor;
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

		case IDC_PATCH_LENGTH_SLIDER:
			if(EVENT_SLIDER_VALUE_CHANGED_UP == nEvent)
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

		case IDC_TESSELLATION_LOD_SLIDER:
			{
				g_ocean_param_quadtree.tessellation_lod = (float)g_HUD.GetSlider(IDC_TESSELLATION_LOD_SLIDER)->GetValue();
				g_pOceanSurf->initQuadTree(g_ocean_param_quadtree);
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
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
	g_DialogResourceManager.OnD3D11ReleasingSwapChain();

	SAFE_RELEASE(g_pLogoVB);
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_SettingsDlg.OnD3D11DestroyDevice();
    CDXUTDirectionWidget::StaticOnD3D11DestroyDevice();

	// Ocean object
	SAFE_DELETE(g_pOceanSurf);

	if(g_hOceanSimulation)
	{
		GFSDK_WaveWorks_Simulation_Destroy(g_hOceanSimulation);
		g_hOceanSimulation = NULL;
	}

	SAFE_RELEASE(g_pMarkerFX);
	SAFE_RELEASE(g_pMarkerLayout);

	SAFE_RELEASE(g_pSkyboxFX);
	SAFE_RELEASE(g_pSkyboxLayout);
	SAFE_RELEASE(g_pSkyCubeMap);
	SAFE_RELEASE(g_pLogoTex);

	SAFE_RELEASE(g_pSkyBoxVB);
	SAFE_RELEASE(g_pMarkerVB);
	SAFE_RELEASE(g_pMarkerIB);

	if(g_hOceanSavestate)
	{
		GFSDK_WaveWorks_Savestate_Destroy(g_hOceanSavestate);
		g_hOceanSavestate = NULL;
	}

	GFSDK_WaveWorks_ReleaseD3D11(DXUTGetD3D11Device());

	SAFE_DELETE(g_pTxtHelper);
}

void RenderMarkers(ID3D11DeviceContext* pDC, const gfsdk_float4* pMarkerPositions, int num_markers, const XMFLOAT4& color)
{
	g_pMarkerColor->SetFloatVector((FLOAT*)&color);

	const UINT vbOffset = 0;
	const UINT vertexStride = sizeof(XMFLOAT4);
	pDC->IASetInputLayout(g_pMarkerLayout);
    pDC->IASetVertexBuffers(0, 1, &g_pMarkerVB, &vertexStride, &vbOffset);
	pDC->IASetIndexBuffer(g_pMarkerIB, DXGI_FORMAT_R16_UINT, 0);

	pDC->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX matView = /*D3DXMATRIX(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1) **/ g_Camera.GetViewMatrix();
	XMMATRIX matProj = g_Camera.GetProjMatrix();
	XMMATRIX matVP = matView * matProj;
	g_pMarkerMatViewProjVariable->SetMatrix((FLOAT*)&matVP);

	g_pMarkerTechnique->GetPassByIndex(0)->Apply(0,pDC);
	for(int i = 0; i != num_markers; ++i)
	{
		XMVECTOR transformedMarkerPosition = DirectX::XMVectorSet(pMarkerPositions[i].x, pMarkerPositions[i].z + g_ocean_param_quadtree.sea_level, pMarkerPositions[i].y, 1.f) * kWaterScale;
		DirectX::XMVectorSetW(transformedMarkerPosition, 1.0f);
// 		transformedMarkerPosition.x *= kWaterScale;
// 		transformedMarkerPosition.y *= kWaterScale;
// 		transformedMarkerPosition.z *= kWaterScale;

		D3D11_MAPPED_SUBRESOURCE msr;
		pDC->Map(g_pMarkerVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		XMFLOAT4* marker_verts = (XMFLOAT4*)msr.pData;

		DirectX::XMStoreFloat4(&marker_verts[0], transformedMarkerPosition);
		DirectX::XMStoreFloat4(&marker_verts[1], transformedMarkerPosition + DirectX::XMVectorSet(20.f, 20.f, 20.f, 0.f));
		DirectX::XMStoreFloat4(&marker_verts[2], transformedMarkerPosition + DirectX::XMVectorSet(20.f, 20.f, -20.f, 0.f));
		DirectX::XMStoreFloat4(&marker_verts[3], transformedMarkerPosition + DirectX::XMVectorSet(-20.f, 20.f, -20.f, 0.f));
		DirectX::XMStoreFloat4(&marker_verts[4], transformedMarkerPosition + DirectX::XMVectorSet(-20.f, 20.f, 20.f, 0.f));

		pDC->Unmap(g_pMarkerVB, 0);

		pDC->DrawIndexed(12, 0, 0);
	}
}

enum { NumMarkersXY = 7, NumMarkers = NumMarkersXY*NumMarkersXY };

gfsdk_float2 g_local_marker_coords[NumMarkers];
gfsdk_float2 g_remote_marker_coords[NumMarkers];
gfsdk_float4 g_local_marker_positions[NumMarkers];

void RenderLocalMarkers(ID3D11DeviceContext* pDC)
{
	RenderMarkers(pDC, g_local_marker_positions, NumMarkers, XMFLOAT4(1.f, 0.f, 0.f, 1.f));
}

void RenderRemoteMarkers(ID3D11DeviceContext* pDC)
{
	if(g_pNetworkClient) {
		const gfsdk_float4* pMarkerPositions = NULL;
		size_t numMarkers = 0;
		if(g_pNetworkClient->GetRemoteMarkerPositions(pMarkerPositions, numMarkers)) {
			RenderMarkers(pDC, pMarkerPositions, (int)numMarkers, XMFLOAT4(1.f, 1.f, 0.f, 1.f));
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
	XMVECTOR eye_pos = g_Camera.GetEyePt();
	XMVECTOR lookat_pos = g_Camera.GetLookAtPt();
	const FLOAT intersectionHeight = g_ocean_param_quadtree.sea_level;
	const FLOAT lambda = (intersectionHeight - XMVectorGetY(eye_pos))/(XMVectorGetY(lookat_pos) - XMVectorGetY(eye_pos));
	const XMVECTOR sea_level_pos = (1.f - lambda) * eye_pos + lambda * lookat_pos;
	const XMVECTOR sea_level_xy = XMVectorSet(XMVectorGetX(sea_level_pos), XMVectorGetZ(sea_level_pos), 0, 0);

	// Update local marker coords, we could need them any time for remote
	for(int x = 0; x != NumMarkersXY; ++x)
	{
		for(int y = 0; y != NumMarkersXY; ++y)
		{
			XMVECTOR offset = XMVectorSet(2.f * (x - ((NumMarkersXY - 1) / 2)), 2.f * (y - ((NumMarkersXY - 1) / 2)), 0, 0);
			XMVECTOR newPos = sea_level_xy / kWaterScale + offset;

			g_local_marker_coords[y * NumMarkersXY + x].x = XMVectorGetX(newPos);
			g_local_marker_coords[y * NumMarkersXY + x].x = XMVectorGetY(newPos);
		}
	}

	// Do local readback, if requested
	if(g_ocean_simulation_settings.readback_displacements)
	{
		gfsdk_float4 displacements[NumMarkers];
		if(g_ReadbackCoord >= 1.f)
		{
			const float coord = g_ReadbackCoord - (g_LastReadbackKickID-g_LastArchivedKickID) * 1.f/float(ReadbackArchiveInterval);
			V(GFSDK_WaveWorks_Simulation_GetArchivedDisplacements(g_hOceanSimulation, coord, g_local_marker_coords, displacements, NumMarkers));
		}
		else
		{
			V(GFSDK_WaveWorks_Simulation_GetDisplacements(g_hOceanSimulation, g_local_marker_coords, displacements, NumMarkers));
		}

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

void RenderSkybox(ID3D11DeviceContext* pDC)
{
	XMMATRIX matView = XMMATRIX(1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1) * g_Camera.GetViewMatrix();
	XMMATRIX matProj = g_Camera.GetProjMatrix();
	XMMATRIX matVP = matView * matProj;

    D3D11_MAPPED_SUBRESOURCE msr;
	pDC->Map(g_pSkyBoxVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	XMFLOAT4* far_plane_quad = (XMFLOAT4*)msr.pData;
	far_plane_quad[0] = XMFLOAT4(-g_FarPlane, g_FarPlane, g_FarPlane * 0.999f, g_FarPlane);
	far_plane_quad[1] = XMFLOAT4(-g_FarPlane, -g_FarPlane, g_FarPlane * 0.999f, g_FarPlane);
	far_plane_quad[2] = XMFLOAT4(g_FarPlane, g_FarPlane, g_FarPlane * 0.999f, g_FarPlane);
	far_plane_quad[3] = XMFLOAT4(g_FarPlane, -g_FarPlane, g_FarPlane * 0.999f, g_FarPlane);
	
	//NOTE: Need Determinant!
	XMMATRIX matInvVP = XMMatrixInverse(&DirectX::XMVectorSet(0, 0, 0, 0), matVP);

	XMVector4TransformStream(&far_plane_quad[0], sizeof(XMFLOAT4), &far_plane_quad[0], sizeof(XMFLOAT4), 4, matInvVP);
	pDC->Unmap(g_pSkyBoxVB, 0);

	const UINT vbOffset = 0;
	const UINT vertexStride = sizeof(XMFLOAT4);
	pDC->IASetInputLayout(g_pSkyboxLayout);
    pDC->IASetVertexBuffers(0, 1, &g_pSkyBoxVB, &vertexStride, &vbOffset);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	g_pSkyBoxMatViewProjVariable->SetMatrix((FLOAT*)&matVP);

	XMVECTOR eye_pos = g_Camera.GetEyePt();
	g_pSkyBoxEyePosVariable->SetFloatVector((FLOAT*)&eye_pos);

	g_pSkyBoxSkyCubeMapVariable->SetResource(g_pSkyCubeMap);

	g_pSkyBoxTechnique->GetPassByIndex(0)->Apply(0, pDC);
	pDC->Draw(4, 0);
}

void RenderLogo(ID3D11DeviceContext* pDC)
{
	g_pLogoTextureVariable->SetResource(g_pLogoTex);

	const UINT vbOffset = 0;
	const UINT vertexStride = 20;
	pDC->IASetInputLayout(g_pOceanSurf->m_pQuadLayout);
    pDC->IASetVertexBuffers(0, 1, &g_pLogoVB, &vertexStride, &vbOffset);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	g_pLogoTechnique->GetPassByIndex(0)->Apply(0, pDC);
	pDC->Draw(4, 0);
}
