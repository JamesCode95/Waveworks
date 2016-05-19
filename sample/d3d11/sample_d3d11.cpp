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
#include "DXUTgui.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"

#include "resource.h"
#include "ocean_surface.h"
#include "distance_field.h"
#include "GFSDK_WaveWorks.h"
#include "GFSDK_WaveWorks_D3D_Util.h"
#include "terrain.h"
#include <windows.h> // for QueryPerformanceFrequency/QueryPerformanceCounter
#include "DDSTextureLoader.h"
#include "D3DX11Effect.h"
#include "D3D9types.h"

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 

// Disable warning "conditional expression is constant"
#pragma warning(disable:4127)

extern HRESULT LoadFile(LPCTSTR FileName, ID3DBlob** ppBuffer);

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
bool                    g_bShowHelp = false;    // If true, it renders the UI control text
bool                    g_bShowUI = true;       // UI can be hidden e.g. for video capture 
CFirstPersonCamera      g_Camera;               //
CDXUTDialogResourceManager g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg         g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*        g_pTxtHelper = NULL;    // Text helper
CDXUTDialog             g_HUD;                  // manages the 3D UI
CDXUTDialog             g_SampleUI;             // dialog for sample specific controls

GFSDK_WaveWorks_SavestateHandle			g_hOceanSavestate = NULL;
GFSDK_WaveWorks_SimulationHandle		g_hOceanSimulation = NULL;

gfsdk_U64 g_LastKickID = 0;
gfsdk_U64 g_LastArchivedKickID = GFSDK_WaveWorks_InvalidKickID;
gfsdk_U64 g_LastReadbackKickID = GFSDK_WaveWorks_InvalidKickID;
gfsdk_U32 g_RenderLatency = 0;
gfsdk_S32 g_ReadbackLatency = 0;
enum { ReadbackArchiveSize = 50 };
enum { ReadbackArchiveInterval = 5 };
float g_ReadbackCoord = 0.f;

OceanSurface*							g_pOceanSurf = NULL;
DistanceField*					        g_pDistanceField = NULL;

GFSDK_WaveWorks_Simulation_Params		g_ocean_simulation_param;
GFSDK_WaveWorks_Simulation_Settings		g_ocean_simulation_settings;
GFSDK_WaveWorks_Simulation_Stats		g_ocean_simulation_stats;
GFSDK_WaveWorks_Simulation_Stats		g_ocean_simulation_stats_filtered;

GFSDK_WaveWorks_Quadtree_Params			g_ocean_quadtree_param;
GFSDK_WaveWorks_Quadtree_Stats			g_ocean_quadtree_stats;
GFSDK_WaveWorks_Simulation_DetailLevel	g_max_detail_level = GFSDK_WaveWorks_Simulation_DetailLevel_Normal;

XMFLOAT2 g_WindDir = XMFLOAT2(0.8f, 0.6f);
bool g_Wireframe = false;
bool g_SimulateWater = true;
bool g_ForceKick = false;
bool g_QueryStats = false;
bool g_enableShoreEffects = true;
float g_TessellationLOD = 50.0f;
float g_NearPlane = 1.0f;
float g_FarPlane = 25000.0f;
double g_SimulationTime = 0.0;
float g_FrameTime = 0.0f;

float g_GerstnerSteepness = 1.0f;
float g_BaseGerstnerAmplitude = 0.279f;
float g_BaseGerstnerWavelength = 3.912f;
float g_BaseGerstnerSpeed = 2.472f;
float g_GerstnerParallelity = 0.2f;
float g_ShoreTime = 0.0f;

CTerrain			g_Terrain;
ID3DX11Effect*      g_pEffect       = NULL;

ID3DX11EffectTechnique* g_pLogoTechnique = NULL;
ID3DX11EffectShaderResourceVariable* g_pLogoTextureVariable = NULL;
ID3D11ShaderResourceView* g_pLogoTex = NULL;
ID3D11Buffer* g_pLogoVB = NULL;
ID3D11InputLayout* g_pSkyboxLayout = NULL;



D3D11_QUERY_DATA_PIPELINE_STATISTICS g_PipelineQueryData;
ID3D11Query*        g_pPipelineQuery= NULL;

// Readbacks and raycasts related globals
enum { NumMarkersXY = 10, NumMarkers = NumMarkersXY*NumMarkersXY };

gfsdk_float2 g_readback_marker_coords[NumMarkers];
gfsdk_float4 g_readback_marker_positions[NumMarkers];

XMVECTOR g_raycast_origins[NumMarkers];
XMVECTOR g_raycast_directions[NumMarkers];
XMVECTOR g_raycast_hitpoints[NumMarkers];
bool		g_raycast_hittestresults[NumMarkers];
static LARGE_INTEGER g_IntersectRaysPerfCounter, g_IntersectRaysPerfCounterOld, g_IntersectRaysPerfFrequency;
float		g_IntersectRaysTime;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN            1
#define IDC_TOGGLEREF                   2
#define IDC_CHANGEDEVICE                3
#define IDC_WIND_SPEED_SLIDER           4
#define IDC_WIND_DEPENDENCY_SLIDER      5
#define IDC_TIME_SCALE_SLIDER           6
#define IDC_TOGGLEWIREFRAME             7
#define IDC_TOGGLESIMULATE              8
#define IDC_TESSELLATION_LOD_SLIDER     9
#define IDC_TOGGLEQUERYSTATS	        10
#define IDC_DETAIL_LEVEL_COMBO          11
#define IDC_TOGGLEUSESHOREEFFECTS		12

CDXUTComboBox* g_pDetailLevelCombo = NULL;

const INT kSliderRange = 100;
const FLOAT kMinWindDep = 0.f;
const FLOAT kMaxWindDep = 1.f;
const FLOAT kMinTimeScale = 0.25f;
const FLOAT kMaxTimeScale = 1.f;
const FLOAT kMinWindSpeedBeaufort = 2.0f;
const FLOAT kMaxWindSpeedBeaufort = 4.0f;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
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

void	UpdateReadbackPositions();
void	UpdateRaycastPositions();

HRESULT a;

enum SynchronizationMode
{
	SynchronizationMode_None = 0,
	SynchronizationMode_RenderOnly,
	SynchronizationMode_Readback,
	Num_SynchronizationModes
};
SynchronizationMode g_bSyncMode = SynchronizationMode_None;

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
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1280, 720 );

    // Pass control to DXUT for handling the message pump and 
    // dispatching render calls. DXUT will call your FrameMove 
    // and FrameRender callback when there is idle time between handling window messages.
    DXUTMainLoop();

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
	g_ocean_quadtree_param.min_patch_length		= 40.f;
	g_ocean_quadtree_param.upper_grid_coverage	= 64.0f;
	g_ocean_quadtree_param.mesh_dim				= 128;
	g_ocean_quadtree_param.sea_level			= -2.f;
	g_ocean_quadtree_param.auto_root_lod		= 10;
	g_ocean_quadtree_param.use_tessellation		= true;
	g_ocean_quadtree_param.tessellation_lod		= 50.0f;
	g_ocean_quadtree_param.geomorphing_degree	= 1.f;
	g_ocean_quadtree_param.enable_CPU_timers	= true;

	g_ocean_simulation_param.time_scale				= 0.75f;
	g_ocean_simulation_param.wave_amplitude			= 0.8f;
	g_ocean_simulation_param.wind_dir				= NvFromDX(g_WindDir);
	g_ocean_simulation_param.wind_speed				= 2.52f;
	g_ocean_simulation_param.wind_dependency		= 0.85f;
	g_ocean_simulation_param.choppy_scale			= 1.2f;
	g_ocean_simulation_param.small_wave_fraction	= 0.f;
	g_ocean_simulation_param.foam_dissipation_speed	= 0.6f;
	g_ocean_simulation_param.foam_falloff_speed		= 0.985f;
	g_ocean_simulation_param.foam_generation_amount	= 0.12f;
	g_ocean_simulation_param.foam_generation_threshold = 0.37f;

	g_ocean_simulation_settings.fft_period						= 400.0f;
	g_ocean_simulation_settings.detail_level					= GFSDK_WaveWorks_Simulation_DetailLevel_Normal;
	g_ocean_simulation_settings.readback_displacements			= true;
	g_ocean_simulation_settings.num_readback_FIFO_entries		= ReadbackArchiveSize;
	g_ocean_simulation_settings.aniso_level						= 16;
	g_ocean_simulation_settings.CPU_simulation_threading_model = GFSDK_WaveWorks_Simulation_CPU_Threading_Model_Automatic;
	g_ocean_simulation_settings.use_Beaufort_scale				= true;
	g_ocean_simulation_settings.num_GPUs						= 1;
	g_ocean_simulation_settings.enable_CUDA_timers				= true;
	g_ocean_simulation_settings.enable_gfx_timers				= true;
	g_ocean_simulation_settings.enable_CPU_timers				= true;

    // Initialize dialogs
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

	g_Camera.SetRotateButtons( true, false, false );
	g_Camera.SetScalers(0.003f, 4000.0f);

	const D3DCOLOR kHUDBackgroundColor = 0x5F2F2F2F;
	g_HUD.SetBackgroundColors(kHUDBackgroundColor);
	g_HUD.SetCallback( OnGUIEvent );
    g_SampleUI.SetCallback( OnGUIEvent );

	AddGUISet();
}

void UpdateDetailLevelCombo()
{
	if(NULL == g_pDetailLevelCombo)
		return;

	g_pDetailLevelCombo->RemoveAllItems();

	for(int detail_level = 0; detail_level <= g_max_detail_level; ++detail_level)
	{
		switch(detail_level)
		{
		case GFSDK_WaveWorks_Simulation_DetailLevel_Normal:
			g_pDetailLevelCombo->AddItem( L"Normal", NULL );
			break;
		case GFSDK_WaveWorks_Simulation_DetailLevel_High:
			g_pDetailLevelCombo->AddItem( L"High", NULL );
			break;
		case GFSDK_WaveWorks_Simulation_DetailLevel_Extreme:
			g_pDetailLevelCombo->AddItem( L"Extreme", NULL );
			break;
		}
	}

	g_pDetailLevelCombo->SetSelectedByIndex(g_ocean_simulation_settings.detail_level);
	g_pDetailLevelCombo->SetDropHeight(32);
}

void AddGUISet()
{
    int iY = 10;
    const int kElementSize = 15;

    g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle Full Screen", 10, iY, 180, kElementSize);
    g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change Device", 10, iY += 20, 180, kElementSize, VK_F2);
    iY += 20;

    g_HUD.AddCheckBox(IDC_TOGGLESIMULATE, L"Simulate Water", 10, iY += 20, 180, kElementSize, g_SimulateWater);
    g_HUD.AddCheckBox(IDC_TOGGLEQUERYSTATS, L"Show Stats", 10, iY += 20, 180, kElementSize, g_QueryStats);
    g_HUD.AddCheckBox(IDC_TOGGLEUSESHOREEFFECTS, L"Shore Effects", 10, iY += 20, 180, kElementSize, g_enableShoreEffects);
    g_HUD.AddCheckBox(IDC_TOGGLEWIREFRAME, L"Wireframe", 10, iY += 20, 180, kElementSize, g_Wireframe);
    iY += 20;

	g_HUD.AddStatic(IDC_WIND_SPEED_SLIDER, L"Wind speed", 10, iY += kElementSize, 180, kElementSize);
	g_HUD.AddSlider(IDC_WIND_SPEED_SLIDER, 10, iY += 20, 180, kElementSize, 0, kSliderRange, int(float(kSliderRange) * (g_ocean_simulation_param.wind_speed-kMinWindSpeedBeaufort)/(kMaxWindSpeedBeaufort-kMinWindSpeedBeaufort)));

	g_HUD.AddStatic(IDC_WIND_DEPENDENCY_SLIDER, L"Wind dependency: %.2f", 10, iY += kElementSize, 180, kElementSize);
	g_HUD.AddSlider(IDC_WIND_DEPENDENCY_SLIDER, 10, iY += 20, 180, kElementSize, 0, 100, int(float(kSliderRange) * (g_ocean_simulation_param.wind_dependency-kMinWindDep)/(kMaxWindDep-kMinWindDep)));

	g_HUD.AddStatic(IDC_TIME_SCALE_SLIDER, L"Time scale", 10, iY += kElementSize, 180, kElementSize);
	g_HUD.AddSlider(IDC_TIME_SCALE_SLIDER, 10, iY += 20, 180, kElementSize, 0, kSliderRange, int(float(kSliderRange) * (g_ocean_simulation_param.time_scale-kMinTimeScale)/(kMaxTimeScale-kMinTimeScale)));

    iY += 20;

	g_HUD.AddStatic(IDC_TESSELLATION_LOD_SLIDER, L"Tessellation LOD", 10, iY += kElementSize, 180, kElementSize);
	g_HUD.AddSlider(IDC_TESSELLATION_LOD_SLIDER, 10, iY += 20, 180, kElementSize, 0, 200, int(g_TessellationLOD));
	
	g_HUD.AddStatic(0, L"Detail Level", 10, iY += kElementSize, 180, kElementSize);
    g_HUD.AddComboBox( IDC_DETAIL_LEVEL_COMBO, 10, iY += 20, 180, kElementSize, 0, false, &g_pDetailLevelCombo );

    iY += 20;
}

bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
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
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;

		// 16x CSAA
		pDeviceSettings->d3d11.sd.SampleDesc.Count = 1;
		pDeviceSettings->d3d11.sd.SampleDesc.Quality = 0;
		g_Terrain.MultiSampleCount = 1;
		g_Terrain.MultiSampleQuality = 0;

		// Turn off vsync
		pDeviceSettings->d3d11.SyncInterval = 0;
    }
	else
	{
		if(pDeviceSettings->d3d11.sd.SampleDesc.Count>4)
			pDeviceSettings->d3d11.sd.SampleDesc.Count = 4;
		g_Terrain.MultiSampleCount = pDeviceSettings->d3d11.sd.SampleDesc.Count;
		g_Terrain.MultiSampleQuality = pDeviceSettings->d3d11.sd.SampleDesc.Quality;
	}

	// It seems that AdapterInfo->m_pAdapter cannot be trusted (DXUT bug?), so enumerate our own
	IDXGIFactory1* pFactory = DXUTGetDXGIFactory();
	IDXGIAdapter* pAdapter = NULL;
	HRESULT hr = pFactory->EnumAdapters(pDeviceSettings->d3d11.AdapterOrdinal,&pAdapter);
	if(FAILED(hr) || NULL == pAdapter)
		return false;

	// Check detail level support
	int detail_level = 0;
	for(; detail_level != Num_GFSDK_WaveWorks_Simulation_DetailLevels; ++detail_level) {
		if(!GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_D3D11(pAdapter, (GFSDK_WaveWorks_Simulation_DetailLevel)detail_level))
			break;
	}
	if(0 == detail_level)
		return false;

	g_max_detail_level = (GFSDK_WaveWorks_Simulation_DetailLevel)(detail_level - 1);
	g_ocean_simulation_settings.detail_level = g_max_detail_level;

	SAFE_RELEASE(pAdapter);
	UpdateDetailLevelCombo();

	return true;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;
	TCHAR path[MAX_PATH];
	static int first_time=0;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_SettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

	if(first_time==0)
	{
		// Setup the camera's view parameters
		XMVECTOR vecEye = XMVectorSet(-100.f, 8.0f, 200.f, 0);
		XMVECTOR vecAt = XMVectorSet(100.f, 0.f, 200.f, 0);

		g_Camera.SetViewParams(vecEye, vecAt);
		g_Camera.SetScalers(0.005f, 50.0f);

	}

	float aspectRatio = (float)pBackBufferSurfaceDesc->Width / (float)pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams(camera_fov * XMVectorGetY(DirectX::g_XMPi) / 360.0f, aspectRatio, scene_z_near, scene_z_far);

	// Ocean sim
	GFSDK_WaveWorks_InitD3D11(pd3dDevice,NULL,GFSDK_WAVEWORKS_API_GUID);

	GFSDK_WaveWorks_Simulation_CreateD3D11(g_ocean_simulation_settings, g_ocean_simulation_param, pd3dDevice, &g_hOceanSimulation);
	GFSDK_WaveWorks_Savestate_CreateD3D11(GFSDK_WaveWorks_StatePreserve_All, pd3dDevice, &g_hOceanSavestate);
	g_ForceKick = true;

	// Ocean object
	g_pOceanSurf = new OceanSurface();
	g_pOceanSurf->init();
	g_pOceanSurf->initQuadTree(g_ocean_quadtree_param);
	GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
	GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));

	// Effect hooks borrowed from ocean object
	g_pLogoTechnique = g_pOceanSurf->m_pOceanFX->GetTechniqueByName("DisplayLogoTech");
	g_pLogoTextureVariable = g_pOceanSurf->m_pOceanFX->GetVariableByName("g_LogoTexture")->AsShaderResource();

	ID3D11Resource* pD3D11Resource = NULL;
	V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("nvidia_logo.dds")));
	V_RETURN(DirectX::CreateDDSTextureFromFile(pd3dDevice, static_cast<const wchar_t *>(path), NULL, &g_pLogoTex));
	SAFE_RELEASE(pD3D11Resource);

	// Terrain and sky fx
	ID3DBlob* pEffectBuffer = NULL;
	V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("sample_d3d11.fxo")));
	V_RETURN(D3DX11CreateEffectFromFile(path, 0, pd3dDevice, &g_pEffect));// pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, pd3dDevice, &g_pEffect));
	SAFE_RELEASE(pEffectBuffer);

    // Initialize shoreline interaction.
    g_pDistanceField = new DistanceField( &g_Terrain );
    g_pDistanceField->Init( pd3dDevice );
    g_pOceanSurf->AttachDistanceFieldModule( g_pDistanceField );

	// Initialize terrain 
	g_Terrain.Initialize(pd3dDevice,g_pEffect);
	V_RETURN(g_Terrain.LoadTextures());
	g_Terrain.BackbufferWidth=(float)pBackBufferSurfaceDesc->Width;
	g_Terrain.BackbufferHeight=(float)pBackBufferSurfaceDesc->Height;
	g_Terrain.ReCreateBuffers();

	// Creating pipeline query
	D3D11_QUERY_DESC queryDesc;
    queryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
    queryDesc.MiscFlags = 0;
    pd3dDevice->CreateQuery(&queryDesc, &g_pPipelineQuery);

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

	float aspectRatio = (float)pBackBufferSurfaceDesc->Width / (float)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(camera_fov * XMVectorGetY(DirectX::g_XMPi) / 360.0f, aspectRatio, scene_z_near, scene_z_far);

	// UI
	g_HUD.SetLocation(pBackBufferSurfaceDesc->Width-240, 8);
	g_HUD.SetSize(232, 704);

	g_Terrain.BackbufferWidth = (float)pBackBufferSurfaceDesc->Width;
	g_Terrain.BackbufferHeight = (float)pBackBufferSurfaceDesc->Height;

	g_Terrain.ReCreateBuffers();

	// Logo VB
	float width = (float)200/pBackBufferSurfaceDesc->Width;
	float height = (float)160/pBackBufferSurfaceDesc->Height;
	float half_tex = 0;
	float vertices0[] = {-0.98f,         -0.96f + height, 0,    half_tex,      half_tex,
						 -0.98f,         -0.96f,          0,    half_tex,      half_tex+1.0f,
						 -0.98f + width, -0.96f + height, 0,    half_tex+1.0f, half_tex,
						 -0.98f + width, -0.96f,          0,    half_tex+1.0f, half_tex+1.0f};

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
	g_Terrain.BackbufferWidth = (float)pBackBufferSurfaceDesc->Width;
	g_Terrain.BackbufferHeight = (float)pBackBufferSurfaceDesc->Height;

	V_RETURN(pd3dDevice->CreateBuffer(&vBufferDesc, &vSrd, &g_pLogoVB));

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
 	g_SimulationTime += fElapsedTime;
	if(g_SimulateWater)
	{
		g_ShoreTime += fElapsedTime*g_ocean_simulation_param.time_scale;
	}

	// Update the camera's position based on user input
    g_Camera.FrameMove( fElapsedTime );
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pDC, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
	CDXUTStatic* static_being_updated;
	WCHAR number_string[256];

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
	GFSDK_WaveWorks_Simulation_GetStats(g_hOceanSimulation,g_ocean_simulation_stats);

	// Performing treadbacks and raycasts
	UpdateReadbackPositions();
	UpdateRaycastPositions();

	// exponential filtering for stats
	g_ocean_simulation_stats_filtered.CPU_main_thread_wait_time			= g_ocean_simulation_stats_filtered.CPU_main_thread_wait_time*0.98f + 0.02f*g_ocean_simulation_stats.CPU_main_thread_wait_time;
	g_ocean_simulation_stats_filtered.CPU_threads_start_to_finish_time  = g_ocean_simulation_stats_filtered.CPU_threads_start_to_finish_time*0.98f + 0.02f*g_ocean_simulation_stats.CPU_threads_start_to_finish_time;
	g_ocean_simulation_stats_filtered.CPU_threads_total_time			= g_ocean_simulation_stats_filtered.CPU_threads_total_time*0.98f + 0.02f*g_ocean_simulation_stats.CPU_threads_total_time;
	g_ocean_simulation_stats_filtered.GPU_simulation_time				= g_ocean_simulation_stats_filtered.GPU_simulation_time*0.98f + 0.02f*g_ocean_simulation_stats.GPU_simulation_time;
	g_ocean_simulation_stats_filtered.GPU_FFT_simulation_time			= g_ocean_simulation_stats_filtered.GPU_FFT_simulation_time*0.98f + 0.02f*g_ocean_simulation_stats.GPU_FFT_simulation_time;
	g_ocean_simulation_stats_filtered.GPU_gfx_time						= g_ocean_simulation_stats_filtered.GPU_gfx_time*0.98f + 0.02f*g_ocean_simulation_stats.GPU_gfx_time;
	g_ocean_simulation_stats_filtered.GPU_update_time					= g_ocean_simulation_stats_filtered.GPU_update_time*0.98f + 0.02f*g_ocean_simulation_stats.GPU_update_time;

    // If the settings dialog is being shown, then
    // render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

	g_FrameTime = fElapsedTime;

	XMFLOAT2 ScreenSizeInv(1.0f / (g_Terrain.BackbufferWidth*main_buffer_size_multiplier), 1.0f / (g_Terrain.BackbufferHeight*main_buffer_size_multiplier));

	ID3DX11Effect* oceanFX = g_pOceanSurf->m_pOceanFX;

	oceanFX->GetVariableByName("g_ZNear")->AsScalar()->SetFloat(scene_z_near);
	oceanFX->GetVariableByName("g_ZFar")->AsScalar()->SetFloat(scene_z_far);
	XMFLOAT3 light_pos = XMFLOAT3(140000.0f,65000.0f,40000.0f);
	g_pEffect->GetVariableByName("g_LightPosition")->AsVector()->SetFloatVector((FLOAT*)&light_pos);
	g_pEffect->GetVariableByName("g_ScreenSizeInv")->AsVector()->SetFloatVector((FLOAT*)&ScreenSizeInv);
	oceanFX->GetVariableByName("g_ScreenSizeInv")->AsVector()->SetFloatVector((FLOAT*)&ScreenSizeInv);
	g_pEffect->GetVariableByName("g_DynamicTessFactor")->AsScalar()->SetFloat(g_ocean_quadtree_param.tessellation_lod * 0.25f + 0.1f);

	g_pOceanSurf->m_pOceanFX->GetVariableByName("g_enableShoreEffects")->AsScalar()->SetFloat(g_enableShoreEffects? 1.0f:0.0f);
	g_Terrain.pEffect->GetVariableByName("g_enableShoreEffects")->AsScalar()->SetFloat(g_enableShoreEffects? 1.0f:0.0f);

	g_Terrain.Render(&g_Camera);
    g_pDistanceField->GenerateDataTexture( pDC );


	
	RenderLogo(pDC);

	if(g_bShowUI) {
		
		const WCHAR* windSpeedFormatString = g_ocean_simulation_settings.use_Beaufort_scale ? L"Beaufort: %.1f" : L"Wind speed: %.1f";
		swprintf_s(number_string, 255, windSpeedFormatString, g_ocean_simulation_param.wind_speed);
		static_being_updated = g_HUD.GetStatic(IDC_WIND_SPEED_SLIDER);
		static_being_updated->SetText(number_string);

		swprintf_s(number_string, 255, L"Wind dependency: %.2f", g_ocean_simulation_param.wind_dependency);
		static_being_updated = g_HUD.GetStatic(IDC_WIND_DEPENDENCY_SLIDER);
		static_being_updated->SetText(number_string);

		swprintf_s(number_string, 255, L"Time scale: %.1f", g_ocean_simulation_param.time_scale);
		static_being_updated = g_HUD.GetStatic(IDC_TIME_SCALE_SLIDER);
		static_being_updated->SetText(number_string);

		swprintf_s(number_string, 255, L"Tessellation LOD: %.0f", g_ocean_quadtree_param.tessellation_lod);
		static_being_updated = g_HUD.GetStatic(IDC_TESSELLATION_LOD_SLIDER);
		static_being_updated->SetText(number_string);

		g_HUD.OnRender( fElapsedTime ); 
		g_SampleUI.OnRender( fElapsedTime );
		RenderText( fTime );
	}

	pDC->GSSetShader(NULL,NULL,0);
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText( double fTime )
{

	WCHAR number_string[256];
	WCHAR number_string_with_spaces[256];
	const UINT buffer_len = 2048;
	WCHAR buffer[buffer_len];

    // Output statistics
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( XMFLOAT4( 0.9f, 0.9f, 0.9f, 1.0f ) );

	swprintf_s(buffer, buffer_len, L"Lib build: %S\n", GFSDK_WaveWorks_GetBuildString());
	g_pTxtHelper->DrawTextLine(buffer);

    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats(true) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

	if(g_QueryStats)
	{
		g_pTxtHelper->SetForegroundColor( XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) );
		g_pTxtHelper->DrawTextLine(L"----------");
		wsprintf(buffer, L"Quad patches drawn                 : %d\n", g_ocean_quadtree_stats.num_patches_drawn);
		g_pTxtHelper->DrawTextLine(buffer);
		
		g_pTxtHelper->DrawTextLine(L"----------");
		g_pTxtHelper->DrawTextLine(L"Pipeline Stats for water surface");

		wsprintf(number_string, L"%d", (UINT)g_PipelineQueryData.IAPrimitives);
		number_string_with_spaces[0]=NULL;
		for(int i=0;i<(int)wcslen(number_string);i++)
		{
			if((((int)wcslen(number_string)-i)%3==0)&&(i!=0)) wcsncat_s(&number_string_with_spaces[0],256,L",",1);
			wcsncat_s(&number_string_with_spaces[0],256,&number_string[i],1);
		}
		wsprintf(buffer, L"Input Primitives                   : %s", number_string_with_spaces);
		g_pTxtHelper->DrawTextLine(buffer);


		wsprintf(number_string, L"%d", (UINT)g_PipelineQueryData.CInvocations);
		number_string_with_spaces[0]=NULL;
		for(int i=0;i<(int)wcslen(number_string);i++)
		{
			if((((int)wcslen(number_string)-i)%3==0)&&(i!=0)) wcsncat_s(&number_string_with_spaces[0],256,L",",1);
			wcsncat_s(&number_string_with_spaces[0],256,&number_string[i],1);
		}
		wsprintf(buffer, L"Primitives created                 : %s", number_string_with_spaces);
		g_pTxtHelper->DrawTextLine(buffer);


		wsprintf(number_string, L"%d", (UINT)((float)g_PipelineQueryData.CInvocations*(1.0f/g_FrameTime)/1000000.0f));
		number_string_with_spaces[0]=NULL;
		for(int i=0;i<(int)wcslen(number_string);i++)
		{
			if((((int)wcslen(number_string)-i)%3==0)&&(i!=0)) wcsncat_s(&number_string_with_spaces[0],256,L",",1);
			wcsncat_s(&number_string_with_spaces[0],256,&number_string[i],1);
		}
		wsprintf(buffer, L"Primitives created / sec           : %sM", number_string_with_spaces);
		g_pTxtHelper->DrawTextLine(buffer);

		wsprintf(number_string, L"%d", (UINT)g_PipelineQueryData.CPrimitives);
		number_string_with_spaces[0]=NULL;
		for(int i=0;i<(int)wcslen(number_string);i++)
		{
			if((((int)wcslen(number_string)-i)%3==0)&&(i!=0)) wcsncat_s(&number_string_with_spaces[0],256,L",",1);
			wcsncat_s(&number_string_with_spaces[0],256,&number_string[i],1);
		}
		wsprintf(buffer, L"Primitives passed clipping         : %s", number_string_with_spaces);
		g_pTxtHelper->DrawTextLine(buffer);
		
		if(g_PipelineQueryData.IAPrimitives>0)
		{
			wsprintf(number_string, L"%d", (UINT)(g_PipelineQueryData.CInvocations/g_PipelineQueryData.IAPrimitives));
			number_string_with_spaces[0]=NULL;
			for(int i=0;i<(int)wcslen(number_string);i++)
			{
				if((((int)wcslen(number_string)-i)%3==0)&&(i!=0)) wcsncat_s(&number_string_with_spaces[0],256,L",",1);
				wcsncat_s(&number_string_with_spaces[0],256,&number_string[i],1);
			}
			wsprintf(buffer, L"Average expansion ratio            : %s", number_string_with_spaces);
		}
		else
		{
			wsprintf(buffer, L"Average expansion ratio            : N/A");
		}
		g_pTxtHelper->DrawTextLine(buffer);
		g_pTxtHelper->DrawTextLine(L"----------");

		swprintf_s(buffer,buffer_len,L"GPU_gfx_time                       : %3.3f msec",g_ocean_simulation_stats_filtered.GPU_gfx_time);
		g_pTxtHelper->DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"GPU_update_time                    : %3.3f msec",g_ocean_simulation_stats_filtered.GPU_update_time);
		g_pTxtHelper->DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"CPU_main_thread_wait_time          : %3.3f msec",g_ocean_simulation_stats_filtered.CPU_main_thread_wait_time);
		g_pTxtHelper->DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"CPU_threads_start_to_finish_time   : %3.3f msec",g_ocean_simulation_stats_filtered.CPU_threads_start_to_finish_time);
		g_pTxtHelper->DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"CPU_threads_total_time             : %3.3f msec",g_ocean_simulation_stats_filtered.CPU_threads_total_time);
		g_pTxtHelper->DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"GPU_simulation_time                : %3.3f msec",g_ocean_simulation_stats_filtered.GPU_simulation_time);
		g_pTxtHelper->DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"GPU_FFT_simulation_time            : %3.3f msec",g_ocean_simulation_stats_filtered.GPU_FFT_simulation_time);
		g_pTxtHelper->DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"CPU_quadtree_update_time           : %3.3f msec",g_ocean_quadtree_stats.CPU_quadtree_update_time);
		g_pTxtHelper->DrawTextLine(buffer);

		swprintf_s(buffer,buffer_len,L"RenderLatency                      : %i",g_RenderLatency);
		g_pTxtHelper->DrawTextLine(buffer);
		if(g_ReadbackLatency >= 0)
		{
			swprintf_s(buffer,buffer_len,L"ReadbackLatency                    : %i",g_ReadbackLatency);
			g_pTxtHelper->DrawTextLine(buffer);
		}
		else
		{
			swprintf_s(buffer,buffer_len,L"ReadbackLatency                    : <NONE-AVAILABLE>");
			g_pTxtHelper->DrawTextLine(buffer);
		}
		
		swprintf_s(buffer,buffer_len,L"Num of raycasts                    : %i",NumMarkers);
		g_pTxtHelper->DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"Total time for raycasts            : %3.3f msec", 1000.0f * g_IntersectRaysTime);
		g_pTxtHelper->DrawTextLine(buffer);
		swprintf_s(buffer,buffer_len,L"Time per raycast                   : %3.3f msec", 1000.0f * g_IntersectRaysTime / NumMarkers);
		g_pTxtHelper->DrawTextLine(buffer);
		g_pTxtHelper->DrawTextLine(L"----------");
		swprintf_s(buffer,buffer_len,L"Readback cache coordinate          : %3.3f", g_ReadbackCoord);
		g_pTxtHelper->DrawTextLine(buffer);
	}
    
    // Draw help
    if( g_bShowHelp )
    {
		g_pTxtHelper->DrawTextLine(L"----------");
        g_pTxtHelper->DrawTextLine( L"Controls:" );
		g_pTxtHelper->DrawTextLine( L"Camera control: left mouse\n");
		g_pTxtHelper->DrawTextLine( L"W/S/A/D/Q/E to move camera" );
		g_pTxtHelper->DrawTextLine( L"'[' to cycle readback mode" );
		g_pTxtHelper->DrawTextLine( L"'N'/'M' to change readback cache coordinate" );
		g_pTxtHelper->DrawTextLine( L"'U' to toggle UI" );
        g_pTxtHelper->DrawTextLine( L"Hide help: F1\nQuit: ESC\n" );
    }
    else
    {
		g_pTxtHelper->DrawTextLine(L"----------");
        g_pTxtHelper->DrawTextLine( L"Press F1 for help" );
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
			case 'u':
			case 'U':
				g_bShowUI = !g_bShowUI;
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
        case IDC_TOGGLEFULLSCREEN:  DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:         DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:      g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
		case IDC_TOGGLESIMULATE:    g_SimulateWater =  g_HUD.GetCheckBox(IDC_TOGGLESIMULATE)->GetChecked(); break;
		case IDC_TOGGLEQUERYSTATS:  g_QueryStats =  g_HUD.GetCheckBox(IDC_TOGGLEQUERYSTATS)->GetChecked(); break;


		case IDC_TOGGLEUSESHOREEFFECTS:
			{
				g_enableShoreEffects = !g_enableShoreEffects;
				break;
			}
		
		case IDC_TOGGLEWIREFRAME:
			{
				g_Wireframe = !g_Wireframe;
				break;
			}

		case IDC_WIND_SPEED_SLIDER:
			if(EVENT_SLIDER_VALUE_CHANGED_UP == nEvent)
			{
				g_ocean_simulation_param.wind_speed = kMinWindSpeedBeaufort + FLOAT(g_HUD.GetSlider(IDC_WIND_SPEED_SLIDER)->GetValue()) * (kMaxWindSpeedBeaufort-kMinWindSpeedBeaufort)/float(kSliderRange);
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
				GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
				g_BaseGerstnerAmplitude = (GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation)/4.0f); // The estimate is based on significant wave height with 4x confidence: http://en.wikipedia.org/wiki/Significant_wave_height, we take it as our shore wave
				g_BaseGerstnerWavelength = 14.0f*g_BaseGerstnerAmplitude; // 7.0 is the min possible, according to Bascom reports: http://hyperphysics.phy-astr.gsu.edu/hbase/waves/watwav2.html
				g_BaseGerstnerSpeed = sqrtf(9.81f*g_BaseGerstnerWavelength/6.28f); // m/sec, let's use equation for deep water waves for simplicity, and slow it down a bit as we're working with shallow water
				break;
			}
			break;

		case IDC_WIND_DEPENDENCY_SLIDER:
			if(EVENT_SLIDER_VALUE_CHANGED_UP == nEvent)
			{
				g_ocean_simulation_param.wind_dependency = kMinWindDep + FLOAT(g_HUD.GetSlider(IDC_WIND_DEPENDENCY_SLIDER)->GetValue()) * (kMaxWindDep-kMinWindDep)/float(kSliderRange);
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
				GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
			}
			break;

		case IDC_TIME_SCALE_SLIDER:
			{
				g_ocean_simulation_param.time_scale = kMinTimeScale + FLOAT(g_HUD.GetSlider(IDC_TIME_SCALE_SLIDER)->GetValue()) * (kMaxTimeScale-kMinTimeScale)/float(kSliderRange);
				GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
			}
			break;

		case IDC_TESSELLATION_LOD_SLIDER:
			{
				g_ocean_quadtree_param.tessellation_lod = (float)g_HUD.GetSlider(IDC_TESSELLATION_LOD_SLIDER)->GetValue();
				g_pOceanSurf->initQuadTree(g_ocean_quadtree_param);
			}
			break;

		case IDC_DETAIL_LEVEL_COMBO:
			{
				g_ocean_simulation_settings.detail_level = (GFSDK_WaveWorks_Simulation_DetailLevel)g_HUD.GetComboBox(IDC_DETAIL_LEVEL_COMBO)->GetSelectedIndex();
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
    SAFE_DELETE(g_pDistanceField);

	if(g_hOceanSimulation)
	{
		GFSDK_WaveWorks_Simulation_Destroy(g_hOceanSimulation);
		g_hOceanSimulation = NULL;
	}
	SAFE_RELEASE(g_pLogoTex);
	SAFE_RELEASE(g_pEffect);
	SAFE_DELETE(g_pTxtHelper);
	g_Terrain.DeInitialize();

	if(g_hOceanSavestate)
	{
		GFSDK_WaveWorks_Savestate_Destroy(g_hOceanSavestate);
		g_hOceanSavestate = NULL;
	}

	GFSDK_WaveWorks_ReleaseD3D11(DXUTGetD3D11Device());

	SAFE_RELEASE(g_pPipelineQuery);
}

void RenderLogo(ID3D11DeviceContext* pDC)
{
	g_pLogoTextureVariable->SetResource(g_pLogoTex);

	const UINT vbOffset = 0;
	const UINT vertexStride = 20;
	pDC->IASetInputLayout(g_pOceanSurf->m_pQuadLayout);
    pDC->IASetVertexBuffers(0, 1, &g_pLogoVB, &vertexStride, &vbOffset);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pDC->GSSetShader(NULL,NULL,0);
	pDC->HSSetShader(NULL,NULL,0);
	pDC->DSSetShader(NULL,NULL,0);
	g_pLogoTechnique->GetPassByIndex(0)->Apply(0, pDC);
	pDC->Draw(4, 0);
}

void UpdateReadbackPositions()
{
	for(int x = 0; x != NumMarkersXY; ++x)
	{
		for(int y = 0; y != NumMarkersXY; ++y)
		{
			g_readback_marker_coords[y * NumMarkersXY + x].x = 5.0f*x;
			g_readback_marker_coords[y * NumMarkersXY + x].y = 5.0f*y;
		}
	}

	if(g_ocean_simulation_settings.readback_displacements)
	{
		gfsdk_float4 displacements[NumMarkers];
		if(g_ReadbackCoord >= 1.f)
		{
			const float coord = g_ReadbackCoord - (g_LastReadbackKickID-g_LastArchivedKickID) * 1.f/float(ReadbackArchiveInterval + 1);
			GFSDK_WaveWorks_Simulation_GetArchivedDisplacements(g_hOceanSimulation, coord, g_readback_marker_coords, displacements, NumMarkers);
		}
		else
		{
			GFSDK_WaveWorks_Simulation_GetDisplacements(g_hOceanSimulation, g_readback_marker_coords, displacements, NumMarkers);
		}

		for(int ix = 0; ix != NumMarkers; ++ix)
		{
			g_readback_marker_positions[ix].x = displacements[ix].x + g_readback_marker_coords[ix].x;
			g_readback_marker_positions[ix].y = displacements[ix].y + g_readback_marker_coords[ix].y;
			g_readback_marker_positions[ix].z = displacements[ix].z + g_ocean_quadtree_param.sea_level;
			g_readback_marker_positions[ix].w = 1.f;
		}
	}
}

// Returns true and sets Result to intersection point if intersection is found, or returns false and does not update Result 
// NB: The function does trace the water surface from above or from inside the water volume, but can be easily changed to trace from below water volume
// NB: y axiz is up
bool intersectRayWithOcean(XMVECTOR& Result, XMVECTOR Position, XMVECTOR Direction, GFSDK_WaveWorks_SimulationHandle hSim, float sea_level)
{
	gfsdk_float2 test_point;									// x,z coordinates of current test point
	gfsdk_float2 old_test_point;								// x,z coordinates of current test point
	gfsdk_float4 displacements;									// displacements returned by GFSDK_WaveWorks library
	float t;													// distance traveled along the ray while tracing
	int num_steps = 0;											// number of steps we've done while tracing
	float max_displacement = GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(hSim); 
																// the maximal possible displacements of ocean surface along y axis, 
																// defining volume we have to trace
	const int max_num_successive_steps = 16;					// we limit ourselves on #of successive steps
	const int max_num_binary_steps = 16;						// we limit ourselves on #of binary search steps
	const float t_threshold = 0.05f;							// we stop successive tracing when we don't progress more than 5 cm each step
	const float refinement_threshold_sqr = 0.1f*0.1f;			// we stop refinement step when we don't progress more than 10cm while doing refinement of current water altitude
	const float t_multiplier = 1.8f/(fabs(XMVectorGetY(Direction)) + 1.0f);	// we increase step length at steep angles to speed up the tracing, 
																// but less than 2 to make sure the process converges 
																// and to add some safety to minimize chance of overshooting
	XMVECTOR PositionBSStart;								// Vectors used at binary search step
	XMVECTOR PositionBSEnd;

	// normalizing direction 
	Direction = XMVector3Normalize(Direction);

	// checking if ray is outside of ocean surface volume 
	if((XMVectorGetY(Position) >= max_displacement + sea_level) && (XMVectorGetY(Direction) >=0)) return false;

	// getting to the top edge of volume where we can start
	if(XMVectorGetY(Position) > max_displacement  + sea_level) 
	{
		t = -(XMVectorGetY(Position) - max_displacement - sea_level) / XMVectorGetY(Direction);	
		Position += t*Direction;
	}

	// tracing the ocean surface:
	// moving along the ray by distance defined by vertical distance form current test point, increased/decreased by safety multiplier
	// this process will converge despite our assumption on local flatness of the surface because curvature of the surface is smooth
	// NB: this process guarantees we don't shoot through wave tips
	while(1)
	{
		displacements.x = 0;
		displacements.y = 0;
		old_test_point.x = 0;
		old_test_point.y = 0;
		for(int k = 0; k < 4; k++) // few refinement steps to converge at correct intersection point
		{
			// moving back sample points by the displacements read initially, 
			// to get a guess on which undisturbed water surface point moved to the actual sample point 
			// due to x,y motion of water surface, assuming the x,y disturbances are locally constant
			test_point.x = XMVectorGetX(Position) - displacements.x;
			test_point.y = XMVectorGetZ(Position) - displacements.y;
			GFSDK_WaveWorks_Simulation_GetDisplacements( hSim, &test_point, &displacements, 1 );
			if(refinement_threshold_sqr > (old_test_point.x - test_point.x)*(old_test_point.x - test_point.x) + (old_test_point.y - test_point.y)*(old_test_point.y - test_point.y)) break;
			old_test_point.x = test_point.x;
			old_test_point.y = test_point.y;
		}
		// getting t to travel along the ray
		t = t_multiplier * (XMVectorGetY(Position) - displacements.z - sea_level);

		// traveling along the ray
		Position += t*Direction;

		if(num_steps >= max_num_successive_steps)  break;
		if(t < t_threshold) break;
		++num_steps;
	}
	
	// exited the loop, checking if intersection is found
	if(t < t_threshold) 
	{
		Result = Position;
		return true;
	}

	// if we're looking down and we did not hit water surface, doing binary search to make sure we hit water surface,
	// but there is risk of shooting through wave tips if we are tracing at extremely steep angles
	if(XMVectorGetY(Direction) < 0) 
	{
		PositionBSStart = Position;

		// getting to the bottom edge of volume where we can start
		t = -(XMVectorGetY(Position) + max_displacement - sea_level) / XMVectorGetY(Direction);	
		PositionBSEnd = Position + t*Direction;

		for(int i = 0; i < max_num_binary_steps; i++)
		{
			Position = (PositionBSStart + PositionBSEnd)*0.5f;
			old_test_point.x = 0;
			old_test_point.y = 0;
			for(int k = 0; k < 4; k++) 
			{
				test_point.x = XMVectorGetX(Position) - displacements.x;
				test_point.y = XMVectorGetZ(Position) - displacements.y;
				GFSDK_WaveWorks_Simulation_GetDisplacements( hSim, &test_point, &displacements, 1 );
				if(refinement_threshold_sqr > (old_test_point.x - test_point.x)*(old_test_point.x - test_point.x) + (old_test_point.y - test_point.y)*(old_test_point.y - test_point.y)) break;
				old_test_point.x = test_point.x;
				old_test_point.y = test_point.y;
			}
			if(XMVectorGetY(Position) - displacements.z - sea_level > 0)
			{
				PositionBSStart = Position;
			}
			else
			{
				PositionBSEnd = Position;
			}
		}		

		Result = Position;

		return true;
	}
	return false;
}

void UpdateRaycastPositions()
{
	for(int x = 0; x != NumMarkersXY; ++x)
	{
		for(int y = 0; y != NumMarkersXY; ++y)
		{
			int i = x + y*NumMarkersXY;
			g_raycast_origins[i] = XMVectorSet(0, 10, terrain_gridpoints*terrain_geometry_scale, 0);
			g_raycast_directions[i] = XMVectorSet(5.0f*float(x - NumMarkersXY / 2.0f), -10.0f, 5.0f*float(y - NumMarkersXY / 2.0f), 0);

			g_raycast_directions[i] = XMVector3Normalize(g_raycast_directions[i]);
		}
	}
	g_IntersectRaysTime = 0.f;
	// Performing water hit test for rays
	QueryPerformanceFrequency(&g_IntersectRaysPerfFrequency);
	QueryPerformanceCounter(&g_IntersectRaysPerfCounterOld);
	for(int i = 0; i < NumMarkers; i++)
	{
		g_raycast_hittestresults[i] = intersectRayWithOcean(g_raycast_hitpoints[i], g_raycast_origins[i], g_raycast_directions[i], g_hOceanSimulation, g_ocean_quadtree_param.sea_level);
	}
	QueryPerformanceCounter(&g_IntersectRaysPerfCounter);
	g_IntersectRaysTime = (float)(((double)(g_IntersectRaysPerfCounter.QuadPart) - (double)(g_IntersectRaysPerfCounterOld.QuadPart))/(double)g_IntersectRaysPerfFrequency.QuadPart);
}
