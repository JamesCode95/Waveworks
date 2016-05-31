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
// Copyright  2008- 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#include "Internal.h"

#include "Simulation_impl.h"
#include "Quadtree_impl.h"
#include "Savestate_impl.h"
#include "Graphics_Context.h"

#include <cstdlib>

#ifdef SUPPORT_CUDA
#include <malloc.h>	// needed for _alloca
#endif

#if defined(TARGET_PLATFORM_NIXLIKE)
#include <stdarg.h>
#include <string.h>
#endif

#if WAVEWORKS_ENABLE_GNM
#include "orbis\GNM_Util.h"
#endif
#include "InternalLogger.h"

// Misc helper macros which can be used to bracket entrypoints to:
// - catch any and all exceptions, to keep them out of the app
// - do mundane checks for usage consistency
#ifdef TARGET_PLATFORM_PS4
// -fexceptions implies -frtti on PS4, so don't use them
// on PS4 (for now)
// (NB: main expected source of exceptions is CUDA, and
// there's no CUDA on PS4 anyway)
#define BEGIN_TRY_BLOCK			
#define CUSTOM_ENTRYPOINT_END(ret)			}
#else
#define BEGIN_TRY_BLOCK try
#define CUSTOM_ENTRYPOINT_END(ret)			} catch(...){return ret;}
#endif

#define ENTRYPOINT_BEGIN_NO_INIT_CHECK		BEGIN_TRY_BLOCK {

#define ENTRYPOINT_BEGIN_API(x)             BEGIN_TRY_BLOCK {  \
												if(g_InitialisedAPI != nv_water_d3d_api_##x) { \
													NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called but the library was not initialised for ") TSTR(#x)); \
													return gfsdk_waveworks_result_FAIL; \
												}

#define CUSTOM_ENTRYPOINT_BEGIN(r)			BEGIN_TRY_BLOCK {  \
												if(g_InitialisedAPI == nv_water_d3d_api_undefined) { \
													NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called but the library was not initialised\n")); \
													return r; \
												}
#define ENTRYPOINT_BEGIN					CUSTOM_ENTRYPOINT_BEGIN(gfsdk_waveworks_result_FAIL)


#define ENTRYPOINT_END						CUSTOM_ENTRYPOINT_END(gfsdk_waveworks_result_INTERNAL_ERROR)

namespace
{
	// Beaufort presets for water
	// Wave amplitude scaler in meters
	const static float BeaufortAmplitude[13] = {
											0.7f, // for Beaufort scale value 0
											0.7f, // for Beaufort scale value 1
											0.7f, // for Beaufort scale value 2
											0.7f, // for Beaufort scale value 3
											0.7f, // for Beaufort scale value 4
											0.7f, // for Beaufort scale value 5
											0.7f, // for Beaufort scale value 6
											0.7f, // for Beaufort scale value 7
											0.7f, // for Beaufort scale value 8
											0.7f, // for Beaufort scale value 9
											0.7f, // for Beaufort scale value 10
											0.7f, // for Beaufort scale value 11
											0.7f  // for Beaufort scale value 12 and above
										 };
	// Wind speed in meters per second
	const static float BeaufortWindSpeed[13] = {
											0.0f, // for Beaufort scale value 0
											0.6f, // for Beaufort scale value 1
											2.0f, // for Beaufort scale value 2
											3.0f, // for Beaufort scale value 3
											6.0f, // for Beaufort scale value 4
											8.1f, // for Beaufort scale value 5
											10.8f,// for Beaufort scale value 6
											13.9f,// for Beaufort scale value 7
											17.2f,// for Beaufort scale value 8
											20.8f,// for Beaufort scale value 9
											24.7f,// for Beaufort scale value 10
											28.6f,// for Beaufort scale value 11
											32.8f // for Beaufort scale value 12 and above
										 };
	// Choppy scale factor (unitless)
	const static float BeaufortChoppiness[13] = {
											1.0f, // for Beaufort scale value 0
											1.0f, // for Beaufort scale value 1
											1.0f, // for Beaufort scale value 2
											1.0f, // for Beaufort scale value 3
											1.0f, // for Beaufort scale value 4
											1.0f, // for Beaufort scale value 5
											1.0f, // for Beaufort scale value 6
											1.0f, // for Beaufort scale value 7
											1.0f, // for Beaufort scale value 8
											1.0f, // for Beaufort scale value 9
											1.0f, // for Beaufort scale value 10
											1.0f, // for Beaufort scale value 11
											1.0f  // for Beaufort scale value 12 and above
										 };

	// Foam generation threshold (unitless)
	const static float BeaufortFoamGenerationThreshold[13] = {
											0.3f, // for Beaufort scale value 0
											0.3f, // for Beaufort scale value 1
											0.3f, // for Beaufort scale value 2
											0.3f, // for Beaufort scale value 3
											0.24f,// for Beaufort scale value 4
											0.27f,// for Beaufort scale value 5
											0.27f, // for Beaufort scale value 6
											0.30f, // for Beaufort scale value 7
											0.30f, // for Beaufort scale value 8
											0.30f, // for Beaufort scale value 9
											0.30f, // for Beaufort scale value 10
											0.30f, // for Beaufort scale value 11
											0.30f  // for Beaufort scale value 12 and above
										 };

	// Foam generation amount (unitless)
	const static float BeaufortFoamGenerationAmount[13] = {
											0.0f, // for Beaufort scale value 0
											0.0f, // for Beaufort scale value 1
											0.0f, // for Beaufort scale value 2
											0.0f, // for Beaufort scale value 3
											0.13f,// for Beaufort scale value 4
											0.13f,// for Beaufort scale value 5
											0.13f,// for Beaufort scale value 6
											0.13f,// for Beaufort scale value 7
											0.13f,// for Beaufort scale value 8
											0.13f,// for Beaufort scale value 9
											0.13f,// for Beaufort scale value 10
											0.13f,// for Beaufort scale value 11
											0.13f // for Beaufort scale value 12 and above
										 };

	// Foam dissipation speed (unitless)
	const static float BeaufortFoamDissipationSpeed[13] = {
											1.0f, // for Beaufort scale value 0
											1.0f, // for Beaufort scale value 1
											1.0f, // for Beaufort scale value 2
											0.8f, // for Beaufort scale value 3
											0.7f,// for Beaufort scale value 4
											0.6f,// for Beaufort scale value 5
											0.6f,// for Beaufort scale value 6
											0.6f,// for Beaufort scale value 7
											0.7f,// for Beaufort scale value 8
											0.8f,// for Beaufort scale value 9
											0.9f,// for Beaufort scale value 10
											1.0f,// for Beaufort scale value 11
											1.1f // for Beaufort scale value 12 and above
										 };

	// Foam falloff speed (unitless)
	const static float BeaufortFoamFalloffSpeed[13] = {
											0.985f, // for Beaufort scale value 0
											0.985f, // for Beaufort scale value 1
											0.985f, // for Beaufort scale value 2
											0.985f, // for Beaufort scale value 3
											0.985f, // for Beaufort scale value 4
											0.985f, // for Beaufort scale value 5
											0.985f, // for Beaufort scale value 6
											0.988f, // for Beaufort scale value 7
											0.985f, // for Beaufort scale value 8
											0.985f, // for Beaufort scale value 9
											0.986f, // for Beaufort scale value 10
											0.988f, // for Beaufort scale value 11
											0.988f  // for Beaufort scale value 12 and above
										 };

	// Global init status
	nv_water_d3d_api g_InitialisedAPI = nv_water_d3d_api_undefined;
	bool g_CanUseCUDA = false;

#if defined(TARGET_PLATFORM_XBONE)
	gfsdk_bool EnsureD3D11API(void)
	{
		return true;
	}
#elif defined(TARGET_PLATFORM_WINDOWS)
	// Boilerplate for dynamic linkage to D3D11CreateDevice
	typedef HRESULT     (WINAPI * LPD3D11CREATEDEVICE)( IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT32, D3D_FEATURE_LEVEL*, UINT, UINT32, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** );
	LPD3D11CREATEDEVICE	g_DynamicD3D11CreateDevice = NULL;
	HMODULE				g_hModD3D11 = NULL;

	gfsdk_bool EnsureD3D11API(void)
	{
		if( g_hModD3D11 != NULL )
			return true;

		// This may fail if Direct3D 11 isn't installed
		g_hModD3D11 = LoadLibrary( TEXT("d3d11.dll") );
		if( g_hModD3D11 != NULL )
		{
			g_DynamicD3D11CreateDevice = ( LPD3D11CREATEDEVICE )GetProcAddress( g_hModD3D11, "D3D11CreateDevice" );
		}

		return ( g_hModD3D11 != NULL );
	}
#endif

	const float kCascadeScale = 5.23f; // Cascade - to - cascade ratio should be not integer, so repeats are less visible
	const float kLODCascadeMaxWaveNumber = kCascadeScale * 10.f;
	const int kLODCascadeResolution = 256; // Chosen to satisfy: kLODCascadeResolution >= (4*kLODCascadeMaxWaveNumber), for reasons of symmetry and Nyquist

#ifdef SUPPORT_CUDA
	gfsdk_bool cudaDeviceSupportsDoublePrecision(int device)
	{
		cudaDeviceProp cdp;
		if(cudaSuccess != cudaGetDeviceProperties(&cdp, device))
			return false;

		// double-precision is 1.3 onwards
		if(cdp.major < 1)
			return false;
		if(cdp.major == 1 && cdp.minor < 3)
			return false;

		return true;
	}
#endif

	gfsdk_waveworks_result SetMemoryManagementCallbacks(const GFSDK_WaveWorks_Malloc_Hooks& mallocHooks)
	{
#if !defined(TARGET_PLATFORM_PS4)
		if( !mallocHooks.pMalloc || !mallocHooks.pFree || !mallocHooks.pAlignedMalloc || !mallocHooks.pAlignedFree)
		{
			NV_ERROR(TEXT("SetMemoryManagementCallbacks received invalid pointer to memory allocation routines"));
			return gfsdk_waveworks_result_FAIL;
		}

		NVSDK_malloc = mallocHooks.pMalloc;
		NVSDK_free = mallocHooks.pFree;
		NVSDK_aligned_malloc = mallocHooks.pAlignedMalloc;
		NVSDK_aligned_free = mallocHooks.pAlignedFree;
#else
		if( !mallocHooks.pOnionAlloc || !mallocHooks.pOnionFree || !mallocHooks.pGarlicAlloc || !mallocHooks.pGarlicFree)
		{
			NV_ERROR(TEXT("SetMemoryManagementCallbacks received invalid pointer to memory allocation routines"));
			return gfsdk_waveworks_result_FAIL;
		}

		NVSDK_aligned_malloc = mallocHooks.pOnionAlloc;
		NVSDK_aligned_free = mallocHooks.pOnionFree;
		NVSDK_garlic_malloc = mallocHooks.pGarlicAlloc;
		NVSDK_garlic_free = mallocHooks.pGarlicFree;
#endif
		return gfsdk_waveworks_result_OK;
	}

	bool equal(const GFSDK_WaveWorks_API_GUID& lhs, const GFSDK_WaveWorks_API_GUID& rhs)
	{
		return lhs.Component1 == rhs.Component1 &&
			lhs.Component2 == rhs.Component2 &&
			lhs.Component3 == rhs.Component3 &&
			lhs.Component4 == rhs.Component4;
	}

	gfsdk_waveworks_result CheckDetailLevelSupport(GFSDK_WaveWorks_Simulation_DetailLevel dl, const char_type* CUDA_ONLY(szEntrypointFnName))
	{
		const nv_water_simulation_api simulationAPI = ToAPI(dl);
		switch(simulationAPI) {
			case nv_water_simulation_api_cuda:
				{
				#ifdef SUPPORT_CUDA

					if(g_CanUseCUDA)
						break;			// We detected CUDA, keep going

					NV_ERROR(TEXT("ERROR: %s failed because the hardware does not support the detail_level specified in the simulation settings\n"), szEntrypointFnName);
					return gfsdk_waveworks_result_FAIL;

				#else
					return gfsdk_waveworks_result_FAIL;
				#endif
				}
			case nv_water_simulation_api_cpu:
				{
				#ifdef SUPPORT_FFTCPU
					break;
				#else
					return gfsdk_waveworks_result_FAIL;
				#endif
				}
			case nv_water_simulation_api_direct_compute:
				{
				#ifdef SUPPORT_DIRECTCOMPUTE
					break;
				#else
					return gfsdk_waveworks_result_FAIL;
				#endif
				}
			default:
				return gfsdk_waveworks_result_FAIL;
		}

		return gfsdk_waveworks_result_OK;
	}
}

void Init_Detailed_Water_Simulation_Params(const GFSDK_WaveWorks_Simulation_Settings& global_settings, const GFSDK_WaveWorks_Simulation_Params& global_params, GFSDK_WaveWorks_Detailed_Simulation_Params* detailed_params)
{
	int BeaufortInteger=(int)(floor(global_params.wind_speed));
	float BeaufortFractional = global_params.wind_speed - floor(global_params.wind_speed); 
	const int fft_resolution = ToInt(global_settings.detail_level);
	
	// Clamping GPU count to 1..4 range internally
	gfsdk_S32 num_GPUs = global_settings.num_GPUs;
	if(num_GPUs < 1) num_GPUs = 1;
	if(num_GPUs > MaxNumGPUs) num_GPUs = MaxNumGPUs;

	// doing piece-wise linear interpolation between predefined values
	// and extrapolating last linear segment to higher Beaufort values
	if(BeaufortInteger>11)
	{
		BeaufortInteger=11;
		BeaufortFractional = global_params.wind_speed - 11;
	}

	detailed_params->num_cascades					= GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades;
	detailed_params->aniso_level					= max(1,min(16,int(global_settings.aniso_level)));
	detailed_params->simulation_api					= ToAPI(global_settings.detail_level);
	detailed_params->CPU_simulation_threading_model	= global_settings.CPU_simulation_threading_model;
	detailed_params->time_scale						= global_params.time_scale;
	detailed_params->num_GPUs						= num_GPUs;
	detailed_params->use_texture_arrays				= global_settings.use_texture_arrays;
	detailed_params->enable_gfx_timers				= global_settings.enable_gfx_timers;
	detailed_params->enable_CPU_timers				= global_settings.enable_CPU_timers;

	for(int i=0;i<GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades;i++)
	{
		const gfsdk_bool is_most_detailed_cascade_level = (i < (GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades-1)) ? false : true;

		detailed_params->cascades[i].fft_period						= global_settings.fft_period / pow(kCascadeScale,(float)i);
		detailed_params->cascades[i].readback_displacements			= global_settings.readback_displacements;
		detailed_params->cascades[i].num_readback_FIFO_entries		= global_settings.num_readback_FIFO_entries;
		detailed_params->cascades[i].fft_resolution					= is_most_detailed_cascade_level ? fft_resolution : min(fft_resolution,kLODCascadeResolution);
		detailed_params->cascades[i].small_wave_fraction			= global_params.small_wave_fraction;
		detailed_params->cascades[i].time_scale						= 1.0f;
		detailed_params->cascades[i].wind_dir						= global_params.wind_dir;
		detailed_params->cascades[i].wind_dependency				= global_params.wind_dependency;
		detailed_params->cascades[i].enable_CUDA_timers				= global_settings.enable_CUDA_timers;

		if(global_settings.use_Beaufort_scale)
		{
			// doing piece-wise linear interpolation between values predefined by Beaufort scale
			detailed_params->cascades[i].choppy_scale					= BeaufortChoppiness[BeaufortInteger] + BeaufortFractional*(BeaufortChoppiness[BeaufortInteger + 1] - BeaufortChoppiness[BeaufortInteger]);
			detailed_params->cascades[i].wave_amplitude					= BeaufortAmplitude[BeaufortInteger] + BeaufortFractional*(BeaufortAmplitude[BeaufortInteger + 1] - BeaufortAmplitude[BeaufortInteger]);
			detailed_params->cascades[i].wind_speed						= BeaufortWindSpeed[BeaufortInteger] + BeaufortFractional*(BeaufortWindSpeed[BeaufortInteger + 1] - BeaufortWindSpeed[BeaufortInteger]);
			detailed_params->cascades[i].foam_generation_threshold		= BeaufortFoamGenerationThreshold[BeaufortInteger] + BeaufortFractional*(BeaufortFoamGenerationThreshold[BeaufortInteger + 1] - BeaufortFoamGenerationThreshold[BeaufortInteger]);
			detailed_params->cascades[i].foam_generation_amount			= BeaufortFoamGenerationAmount[BeaufortInteger] + BeaufortFractional*(BeaufortFoamGenerationAmount[BeaufortInteger + 1] - BeaufortFoamGenerationAmount[BeaufortInteger]);
			detailed_params->cascades[i].foam_dissipation_speed			= BeaufortFoamDissipationSpeed[BeaufortInteger] + BeaufortFractional*(BeaufortFoamDissipationSpeed[BeaufortInteger + 1] - BeaufortFoamDissipationSpeed[BeaufortInteger]);
			detailed_params->cascades[i].foam_falloff_speed				= BeaufortFoamFalloffSpeed[BeaufortInteger] + BeaufortFractional*(BeaufortFoamFalloffSpeed[BeaufortInteger + 1] - BeaufortFoamFalloffSpeed[BeaufortInteger]);

		}
		else
		{
			// using values defined in global params
			detailed_params->cascades[i].choppy_scale					= global_params.choppy_scale;
			detailed_params->cascades[i].wave_amplitude					= global_params.wave_amplitude;
			detailed_params->cascades[i].wind_speed						= global_params.wind_speed;
			detailed_params->cascades[i].foam_generation_threshold		= global_params.foam_generation_threshold;
			detailed_params->cascades[i].foam_generation_amount			= global_params.foam_generation_amount;
			detailed_params->cascades[i].foam_dissipation_speed			= global_params.foam_dissipation_speed;
			detailed_params->cascades[i].foam_falloff_speed				= global_params.foam_falloff_speed;
		}

		// Windowing params to ensure we do not overlap wavelengths in different cascade levels
		if(is_most_detailed_cascade_level)
		{
			// Allow all high frequencies in most detailed level
			detailed_params->cascades[i].window_out = float(detailed_params->cascades[i].fft_resolution);
		}
		else
		{
			detailed_params->cascades[i].window_out = kLODCascadeMaxWaveNumber;
		}

		if(i > 0)
		{
			// Match the 'in' on this cascade to the 'out' on the previous
			detailed_params->cascades[i].window_in = detailed_params->cascades[i-1].window_out * detailed_params->cascades[i].fft_period/detailed_params->cascades[i-1].fft_period;
		}
		else
		{
			// This is the biggest cascade in world space, so we cover all the frequencies at the low end
			detailed_params->cascades[i].window_in= 0.f;
		}

	}
}

const char* GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_GetBuildString()
{
#if defined(TARGET_PLATFORM_MACOSX)
    return "MACOSX_TEST";
	// TIMT: TODO!!!
#elif defined(TARGET_PLATFORM_ANDROID)
    return "ANDROID_TEST";
	// TIMT: TODO!!!
#else   
	//TODO: Fix this build string thing.
	const char* kNVWaveWorks_build_string = "FixBuildString!";
	return kNVWaveWorks_build_string;
#endif
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV  GFSDK_WaveWorks_SetUserLogger(nv::ILogger* userLogger)
{
	g_UserLogger = userLogger;

	return gfsdk_waveworks_result_OK;
}

gfsdk_bool GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_GLAttribIsShaderInput(gfsdk_cstr attribName, const GFSDK_WaveWorks_ShaderInput_Desc& inputDesc)
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK

	// We have a match if the input desc name is the end fo the attrib name string
	// This is because we would like to support clients who embed the vertex attributes in their own GLSL structs, so any of
	// the following is considered a match for an attrib input named 'foo'...
	//    foo
	//    waveworks_struct.foo
	//    client_struct.foo
	//    client_struct.waveworks_struct.foo
	// ...etc, etc
	const size_t inputNameLen = strlen(inputDesc.Name);
	const size_t attribNameLen = strlen(attribName);
	if(attribNameLen < inputNameLen)
	{
		// Can't possibly match
		return false;
	}

	return 0 == strcmp(attribName + (attribNameLen - inputNameLen), inputDesc.Name);

	CUSTOM_ENTRYPOINT_END(false)
}

gfsdk_bool GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_D3D11(IDXGIAdapter* WIN_ONLY(D3D11_ONLY(adapter)), GFSDK_WaveWorks_Simulation_DetailLevel D3D11_ONLY(detailLevel))
{
#if WAVEWORKS_ENABLE_D3D11
	ENTRYPOINT_BEGIN_NO_INIT_CHECK

	// We avoid static linkage to D3D11CreateDevice() so that non-DX11 apps will successfully initialise
	// when DX11 is not installed
	if(!EnsureD3D11API())
		return false;
	
#ifndef _XBOX_ONE
	if(NULL == g_DynamicD3D11CreateDevice)
		return false;

	// Always check feature-level in DX11 - we need true DX11 for tessellation
	HRESULT hr;
	D3D_FEATURE_LEVEL FeatureLevel;
	hr = g_DynamicD3D11CreateDevice(	adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0,
										D3D11_SDK_VERSION, NULL, &FeatureLevel, NULL
										);

	if(FAILED(hr))
	{
		return false;
	}
	// removed 11.0 feature level check for Gaijin
	/*
	else if(FeatureLevel < D3D_FEATURE_LEVEL_11_0)
	{
		return false;
	}
	*/
#endif

	const nv_water_simulation_api simulationAPI = ToAPI(detailLevel);
	switch(simulationAPI) {
		case nv_water_simulation_api_cuda:
			{
			#ifdef SUPPORT_CUDA
				int device;
				cudaD3D11GetDevice(&device, adapter);
				if (cudaGetLastError() != cudaSuccess)
					return false;
				else
					return cudaDeviceSupportsDoublePrecision(device);
			#else
				return false;
			#endif
			}
		case nv_water_simulation_api_direct_compute:
			{
#ifdef SUPPORT_DIRECTCOMPUTE
				// todo: check D3D11 support
				return true;
#else
				return false;
#endif
			}
		case nv_water_simulation_api_cpu:
			{
			#ifdef SUPPORT_FFTCPU
				return true;
			#else
				return false;
			#endif
			}
		default:
			return false;
	}

	CUSTOM_ENTRYPOINT_END(false)
#else
	return false;
#endif
}

gfsdk_bool GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_NoGraphics(GFSDK_WaveWorks_Simulation_DetailLevel detailLevel)
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK

	const nv_water_simulation_api simulationAPI = ToAPI(detailLevel);
	switch(simulationAPI) {
		case nv_water_simulation_api_cuda:
			{
			#ifdef SUPPORT_CUDA
				int cuda_device;
				cudaError cu_err = cudaGetDevice(&cuda_device);
				if (cu_err != cudaSuccess)
					return false;
				else
					return cudaDeviceSupportsDoublePrecision(cuda_device);
			#else
				return false;
			#endif
			}
		case nv_water_simulation_api_cpu:
			{
			#ifdef SUPPORT_FFTCPU
				return true;
			#else
				return false;
			#endif
			}
		default:
			return false;
	}

	CUSTOM_ENTRYPOINT_END(false)
}

gfsdk_bool GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_GL2(GFSDK_WaveWorks_Simulation_DetailLevel GL_ONLY(detailLevel))
{
#if WAVEWORKS_ENABLE_GL
	ENTRYPOINT_BEGIN_NO_INIT_CHECK

	const nv_water_simulation_api simulationAPI = ToAPI(detailLevel);
	switch(simulationAPI) {
		case nv_water_simulation_api_cuda:
			{
			#ifdef SUPPORT_CUDA
				unsigned int num_devices;
				int cuda_device;
				cudaError cu_err = cudaGLGetDevices(&num_devices,&cuda_device,1,cudaGLDeviceListCurrentFrame);
				if (cu_err != cudaSuccess)
					return false;
				else
					return cudaDeviceSupportsDoublePrecision(cuda_device);
			#else
				return false;
			#endif
			}
		case nv_water_simulation_api_cpu:
			{
			#ifdef SUPPORT_FFTCPU
				return true;
			#else
				return false;
			#endif
			}
		default:
			return false;
	}

	CUSTOM_ENTRYPOINT_END(false)
#else
	return false;
#endif
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_InitD3D11(ID3D11Device* CUDA_ONLY(pD3DDevice), const GFSDK_WaveWorks_Malloc_Hooks* pRequiredMallocHooks, const GFSDK_WaveWorks_API_GUID& apiGUID)
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK

#if WAVEWORKS_ENABLE_D3D11

	NV_LOG(TEXT("Initing D3D11"));

	if(g_InitialisedAPI != nv_water_d3d_api_undefined) {
		NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called with the library already in an initialised state\n"));
		return gfsdk_waveworks_result_FAIL;
	}

	if(!equal(apiGUID,GFSDK_WAVEWORKS_API_GUID)) {
		NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called with an invalid API GUID"));
		return gfsdk_waveworks_result_FAIL;
	}

	if(pRequiredMallocHooks) {
		const gfsdk_waveworks_result smmcResult = SetMemoryManagementCallbacks(*pRequiredMallocHooks);
		if(smmcResult != gfsdk_waveworks_result_OK)
			return smmcResult;
	}

#if defined(SUPPORT_CUDA)
	// Associate all Cuda devices with the D3D11 device
	unsigned int numCudaDevices = 0;
	cudaError cu_err = cudaD3D11GetDevices(&numCudaDevices, NULL, 0, pD3DDevice, cudaD3D11DeviceListAll);
	if(cudaSuccess != cu_err)
	{
		// This is our first meaningful call to CUDA, so treat CUDA as unavailable if it fails for any reason
		g_InitialisedAPI = nv_water_d3d_api_d3d11;
		g_CanUseCUDA = false;
		return gfsdk_waveworks_result_OK;
	}

	int* pCudaDevices = (int*)_alloca(numCudaDevices * sizeof(int));
	CUDA_API_RETURN(cudaD3D11GetDevices(&numCudaDevices, pCudaDevices, numCudaDevices, pD3DDevice, cudaD3D11DeviceListAll));
	g_CanUseCUDA = numCudaDevices > 0;
	for(unsigned int cuda_dev_index = 0; cuda_dev_index != numCudaDevices; ++cuda_dev_index)
	{
		if(!cudaDeviceSupportsDoublePrecision(pCudaDevices[cuda_dev_index])) {
			// We can't use a CUDA device that does not have double-precision support
			g_CanUseCUDA = false;
		}
		CUDA_API_RETURN(cudaD3D11SetDirect3DDevice(pD3DDevice, pCudaDevices[cuda_dev_index]));
	}

	int currentFrameCudaDevice = 0;
	CUDA_API_RETURN(cudaD3D11GetDevices(&numCudaDevices, &currentFrameCudaDevice, 1, pD3DDevice, cudaD3D11DeviceListCurrentFrame));
	CUDA_API_RETURN(cudaSetDevice(currentFrameCudaDevice));

#else
	g_CanUseCUDA = false;
#endif
	g_InitialisedAPI = nv_water_d3d_api_d3d11;
	return gfsdk_waveworks_result_OK;

#else
	return gfsdk_waveworks_result_FAIL;
#endif

	ENTRYPOINT_END
}

struct GFSDK_WaveWorks_GnmxWrap;
gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_InitGnm(const GFSDK_WaveWorks_Malloc_Hooks* GNM_ONLY(pRequiredMallocHooks), const GFSDK_WaveWorks_API_GUID& GNM_ONLY(apiGUID), GFSDK_WaveWorks_GnmxWrap* GNM_ONLY(pRequiredGnmxWrap))
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK

#if WAVEWORKS_ENABLE_GNM

	if(g_InitialisedAPI != nv_water_d3d_api_undefined) {
		NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called with the library already in an initialised state"));
		return gfsdk_waveworks_result_FAIL;
	}

	if(!equal(apiGUID,GFSDK_WAVEWORKS_API_GUID)) {
		NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called with an invalid API GUID"));
		return gfsdk_waveworks_result_FAIL;
	}

	if(!pRequiredMallocHooks) {
		NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called with an invalid pRequiredMallocHooks"));
		return gfsdk_waveworks_result_FAIL;
	}

	if(!pRequiredGnmxWrap) {
		NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called with an invalid pRequiredGnmxWrap"));
		return gfsdk_waveworks_result_FAIL;
	}

	const gfsdk_waveworks_result smmcResult = SetMemoryManagementCallbacks(*pRequiredMallocHooks);
	if(smmcResult != gfsdk_waveworks_result_OK)
		return smmcResult;

	GFSDK_WaveWorks_GNM_Util::setGnmxWrap(pRequiredGnmxWrap);

	g_InitialisedAPI = nv_water_d3d_api_gnm;
	return gfsdk_waveworks_result_OK;

#else
	// Non-Gnm platform, just fail
	return gfsdk_waveworks_result_FAIL;
#endif

	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_InitGL2(const GFSDK_WAVEWORKS_GLFunctions* GL_ONLY(pGLFuncs), const GFSDK_WaveWorks_Malloc_Hooks* GL_ONLY(pOptionalMallocHooks), const GFSDK_WaveWorks_API_GUID& GL_ONLY(apiGUID))
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK

#if WAVEWORKS_ENABLE_GL
	if(g_InitialisedAPI != nv_water_d3d_api_undefined) {
		NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called with the library already in an initialised state"));
		return gfsdk_waveworks_result_FAIL;
	}

	if(!equal(apiGUID,GFSDK_WAVEWORKS_API_GUID)) {
		NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called with an invalid API GUID"));
		return gfsdk_waveworks_result_FAIL;
	}
 
	if(pOptionalMallocHooks) {
		const gfsdk_waveworks_result smmcResult = SetMemoryManagementCallbacks(*pOptionalMallocHooks);
		if(smmcResult != gfsdk_waveworks_result_OK)
			return smmcResult;
	}

    // Initializing internal GLFunctions struct
	if(pGLFuncs == 0) return gfsdk_waveworks_result_FAIL;
	memcpy((void*)&NVSDK_GLFunctions, (void*)pGLFuncs, sizeof(NVSDK_GLFunctions));

#if defined(SUPPORT_CUDA)
	// Associate all Cuda devices with the GL2 device
	unsigned int numCudaDevices = 0;
	cudaError cu_err = cudaGLGetDevices(&numCudaDevices, NULL, 0, cudaGLDeviceListAll);
	if(cudaSuccess != cu_err)
	{
		// This is our first meaningful call to CUDA, so treat CUDA as unavailable if it fails for any reason
		g_InitialisedAPI = nv_water_d3d_api_gl2;
		g_CanUseCUDA = false;
		return gfsdk_waveworks_result_OK;
	}

	int* pCudaDevices = (int*)_alloca(numCudaDevices * sizeof(int));
	CUDA_API_RETURN(cudaGLGetDevices(&numCudaDevices, pCudaDevices, numCudaDevices, cudaGLDeviceListAll));
	g_CanUseCUDA = numCudaDevices > 0;

	// It is no longer necessary (CUDA >= 5.0) to associate a CUDA context with an OpenGL
	// context in order to achieve maximum interoperability performance.
	// So returning OK

#else
	g_CanUseCUDA = false;
#endif
    
	g_InitialisedAPI = nv_water_d3d_api_gl2;
	return gfsdk_waveworks_result_OK;
#else
	return gfsdk_waveworks_result_FAIL;
#endif
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_ReleaseD3D11(ID3D11Device* CUDA_ONLY(pD3DDevice))
{
	ENTRYPOINT_BEGIN_API(d3d11)

	resetMemoryManagementCallbacksToDefaults();

	g_CanUseCUDA = false;

#if defined(SUPPORT_CUDA) && WAVEWORKS_ENABLE_D3D11
	unsigned int numCudaDevices = 0;
	cudaError cu_err = cudaD3D11GetDevices(&numCudaDevices, NULL, 0, pD3DDevice, cudaD3D11DeviceListAll);
	if(cudaErrorNoDevice == cu_err)
	{
		g_InitialisedAPI = nv_water_d3d_api_undefined;
		return gfsdk_waveworks_result_OK;	// Legit on systems that do not support CUDA - nothing to do here
	}
	else
		CUDA_API_RETURN(cu_err);

	int* pCudaDevices = (int*)_alloca(numCudaDevices * sizeof(int));
	CUDA_API_RETURN(cudaD3D11GetDevices(&numCudaDevices, pCudaDevices, numCudaDevices, pD3DDevice, cudaD3D11DeviceListAll));
	for(unsigned int cuda_dev_index = 0; cuda_dev_index != numCudaDevices; ++cuda_dev_index)
	{
		CUDA_API_RETURN(cudaSetDevice(pCudaDevices[cuda_dev_index]));
		cudaDeviceReset();
	}
#endif

	g_InitialisedAPI = nv_water_d3d_api_undefined;
	return gfsdk_waveworks_result_OK;

	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_ReleaseGnm()
{
	ENTRYPOINT_BEGIN_API(gnm)

#if WAVEWORKS_ENABLE_GNM
	GFSDK_WaveWorks_GNM_Util::setGnmxWrap(NULL);

	resetMemoryManagementCallbacksToDefaults();

	g_InitialisedAPI = nv_water_d3d_api_undefined;
	return gfsdk_waveworks_result_OK;
#else
	// Non-Gnm platform, just fail
	return gfsdk_waveworks_result_FAIL;
#endif

	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_ReleaseGL2()
{
	ENTRYPOINT_BEGIN_API(gl2)

	resetMemoryManagementCallbacksToDefaults();

	g_CanUseCUDA = false;

#if defined(SUPPORT_CUDA) && WAVEWORKS_ENABLE_GL
	unsigned int numCudaDevices = 0;
	cudaError cu_err = cudaGLGetDevices(&numCudaDevices, NULL, 0, cudaGLDeviceListAll);
	if(cudaErrorNoDevice == cu_err)
	{
		g_InitialisedAPI = nv_water_d3d_api_undefined;
		return gfsdk_waveworks_result_OK;	// Legit on systems that do not support CUDA - nothing to do here
	}
	else
		CUDA_API_RETURN(cu_err);

	int* pCudaDevices = (int*)_alloca(numCudaDevices * sizeof(int));
	CUDA_API_RETURN(cudaGLGetDevices(&numCudaDevices, pCudaDevices, numCudaDevices, cudaGLDeviceListAll));
	for(unsigned int cuda_dev_index = 0; cuda_dev_index != numCudaDevices; ++cuda_dev_index)
	{
		CUDA_API_RETURN(cudaSetDevice(pCudaDevices[cuda_dev_index]));
		cudaDeviceReset();
	}
#endif

	g_InitialisedAPI = nv_water_d3d_api_undefined;
	return gfsdk_waveworks_result_OK;

	ENTRYPOINT_END
}



gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_InitNoGraphics(const GFSDK_WaveWorks_Malloc_Hooks* pOptionalMallocHooks, const GFSDK_WaveWorks_API_GUID& apiGUID)
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK

	if(g_InitialisedAPI != nv_water_d3d_api_undefined) {
		NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called with the library already in an initialised state"));
		return gfsdk_waveworks_result_FAIL;
	}

	if(!equal(apiGUID,GFSDK_WAVEWORKS_API_GUID)) {
		NV_ERROR(TEXT("ERROR: ") __DEF_FUNCTION__ TEXT(" was called with an invalid API GUID"));
		return gfsdk_waveworks_result_FAIL;
	}

	if(pOptionalMallocHooks) {
		const gfsdk_waveworks_result smmcResult = SetMemoryManagementCallbacks(*pOptionalMallocHooks);
		if(smmcResult != gfsdk_waveworks_result_OK)
			return smmcResult;
	}

#ifdef SUPPORT_CUDA
	// We just need one device to qualify as CUDA-capable
	int cuda_device = 0;
	cudaError cu_err = cudaGetDevice(&cuda_device);
	if(cudaSuccess != cu_err)
	{
		// This is our first meaningful call to CUDA, so treat CUDA as unavailable if it fails for any reason
		g_InitialisedAPI = nv_water_d3d_api_none;
		g_CanUseCUDA = false;
		return gfsdk_waveworks_result_OK;
	}

	g_CanUseCUDA = cudaDeviceSupportsDoublePrecision(cuda_device);	// Must support double-precision
	g_InitialisedAPI = nv_water_d3d_api_none;
	return gfsdk_waveworks_result_OK;
#else
	g_InitialisedAPI = nv_water_d3d_api_none;
	g_CanUseCUDA = false;
	return gfsdk_waveworks_result_OK;
#endif


	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_ReleaseNoGraphics()
{
	ENTRYPOINT_BEGIN_API(none)

	resetMemoryManagementCallbacksToDefaults();

#ifdef SUPPORT_CUDA
	if(g_CanUseCUDA)
	{
		cudaDeviceReset();
		g_CanUseCUDA = false;
	}
#endif

	g_InitialisedAPI = nv_water_d3d_api_undefined;
	return gfsdk_waveworks_result_OK;

	ENTRYPOINT_END
}

namespace
{
    GFSDK_WaveWorks_Simulation* FromHandle(GFSDK_WaveWorks_SimulationHandle hSim)
    {
        return hSim;
    }

    GFSDK_WaveWorks_SimulationHandle ToHandle(GFSDK_WaveWorks_Simulation* pImpl)
    {
        return pImpl;
    }

    GFSDK_WaveWorks_Quadtree* FromHandle(GFSDK_WaveWorks_QuadtreeHandle hSim)
    {
        return hSim;
    }

    GFSDK_WaveWorks_QuadtreeHandle ToHandle(GFSDK_WaveWorks_Quadtree* pImpl)
    {
        return pImpl;
    }

    GFSDK_WaveWorks_Savestate* FromHandle(GFSDK_WaveWorks_SavestateHandle hSavestate)
    {
        return hSavestate;
    }

    GFSDK_WaveWorks_SavestateHandle ToHandle(GFSDK_WaveWorks_Savestate* pImpl)
    {
        return pImpl;
    }
}


gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Savestate_CreateD3D11(GFSDK_WaveWorks_StatePreserveFlags PreserveFlags, ID3D11Device* pD3DDevice, GFSDK_WaveWorks_SavestateHandle* pResult)
{
	ENTRYPOINT_BEGIN_API(d3d11)
	GFSDK_WaveWorks_Savestate* pImpl = new GFSDK_WaveWorks_Savestate(pD3DDevice, PreserveFlags);
	*pResult = ToHandle(pImpl);
	return gfsdk_waveworks_result_OK;
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Savestate_RestoreD3D11(GFSDK_WaveWorks_SavestateHandle hSavestate, ID3D11DeviceContext* pDC)
{
	ENTRYPOINT_BEGIN_API(d3d11)
	Graphics_Context gc(pDC);
	return ToAPIResult(FromHandle(hSavestate)->Restore(&gc));
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Savestate_Destroy(GFSDK_WaveWorks_SavestateHandle hSavestate)
{
	ENTRYPOINT_BEGIN
	GFSDK_WaveWorks_Savestate* pImpl = FromHandle(hSavestate);
	delete pImpl;

	return gfsdk_waveworks_result_OK;
	ENTRYPOINT_END
}

namespace
{
	gfsdk_waveworks_result Simulation_CreateD3D11_Generic(  const GFSDK_WaveWorks_Simulation_Settings& global_settings,
															const GFSDK_WaveWorks_Simulation_Params& global_params,
															GFSDK_WaveWorks_CPU_Scheduler_Interface* pOptionalScheduler,
															ID3D11Device* pD3DDevice,
															GFSDK_WaveWorks_SimulationHandle* pResult
															)
	{
		#if WAVEWORKS_ENABLE_D3D11
			// Don't assume the user checked GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_XXXX()...
			if(gfsdk_waveworks_result_OK != CheckDetailLevelSupport(global_settings.detail_level,__DEF_FUNCTION__))
			{
				return gfsdk_waveworks_result_FAIL;
			}

			GFSDK_WaveWorks_Simulation* pImpl = new GFSDK_WaveWorks_Simulation();
			GFSDK_WaveWorks_Detailed_Simulation_Params detailed_params;
			Init_Detailed_Water_Simulation_Params(global_settings, global_params, &detailed_params);
			HRESULT hr = pImpl->initD3D11(detailed_params, pOptionalScheduler, pD3DDevice);
			if(FAILED(hr))
			{
				delete pImpl;
				return ToAPIResult(hr);
			}
			*pResult = ToHandle(pImpl);
			return gfsdk_waveworks_result_OK;
		#else // WAVEWORKS_ENABLE_D3D11
			return gfsdk_waveworks_result_FAIL;
		#endif // WAVEWORKS_ENABLE_D3D11
	}
}

#if defined(WAVEWORKS_NDA_BUILD)
gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_CreateD3D11_NDA(   const GFSDK_WaveWorks_Simulation_Settings& settings,
																							   const GFSDK_WaveWorks_Simulation_Params& params,
																							   GFSDK_WaveWorks_CPU_Scheduler_Interface* pOptionalScheduler,
																							   ID3D11Device* pD3DDevice,
																							   GFSDK_WaveWorks_SimulationHandle* pResult
																							   )
{
	ENTRYPOINT_BEGIN_API(d3d11)
	return Simulation_CreateD3D11_Generic(settings, params, pOptionalScheduler, pD3DDevice, pResult);
	ENTRYPOINT_END
}
#endif

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_CreateD3D11(const GFSDK_WaveWorks_Simulation_Settings& settings, const GFSDK_WaveWorks_Simulation_Params& params, ID3D11Device* pD3DDevice, GFSDK_WaveWorks_SimulationHandle* pResult)
{
	ENTRYPOINT_BEGIN_API(d3d11)
	return Simulation_CreateD3D11_Generic(settings, params, NULL, pD3DDevice, pResult);
	ENTRYPOINT_END
}

gfsdk_waveworks_result  GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_CreateGnm(const GFSDK_WaveWorks_Simulation_Settings& GNM_ONLY(global_settings), const GFSDK_WaveWorks_Simulation_Params& GNM_ONLY(global_params), GFSDK_WaveWorks_CPU_Scheduler_Interface* GNM_ONLY(pOptionalScheduler), GFSDK_WaveWorks_SimulationHandle* GNM_ONLY(pResult))
{
	ENTRYPOINT_BEGIN_API(gnm)

#if WAVEWORKS_ENABLE_GNM
	// Don't assume the user checked GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_XXXX()...
	if(gfsdk_waveworks_result_OK != CheckDetailLevelSupport(global_settings.detail_level,__DEF_FUNCTION__))
	{
		return gfsdk_waveworks_result_FAIL;
	}

    GFSDK_WaveWorks_Simulation* pImpl = new GFSDK_WaveWorks_Simulation();
	GFSDK_WaveWorks_Detailed_Simulation_Params detailed_params;
	Init_Detailed_Water_Simulation_Params(global_settings, global_params, &detailed_params);
	HRESULT hr = pImpl->initGnm(detailed_params, pOptionalScheduler);
	if(FAILED(hr))
	{
		delete pImpl;
		return ToAPIResult(hr);
	}
	*pResult = ToHandle(pImpl);
	return gfsdk_waveworks_result_OK;
#else // WAVEWORKS_ENABLE_GNM
	return gfsdk_waveworks_result_FAIL;
#endif // WAVEWORKS_ENABLE_GNM

	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_CreateNoGraphics(const GFSDK_WaveWorks_Simulation_Settings& global_settings, const GFSDK_WaveWorks_Simulation_Params& global_params, GFSDK_WaveWorks_SimulationHandle* pResult)
{
	ENTRYPOINT_BEGIN_API(none)

	// Don't assume the user checked GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_XXXX()...
	if(gfsdk_waveworks_result_OK != CheckDetailLevelSupport(global_settings.detail_level,__DEF_FUNCTION__))
	{
		return gfsdk_waveworks_result_FAIL;
	}

    GFSDK_WaveWorks_Simulation* pImpl = new GFSDK_WaveWorks_Simulation();
	GFSDK_WaveWorks_Detailed_Simulation_Params detailed_params;
	Init_Detailed_Water_Simulation_Params(global_settings, global_params, &detailed_params);
	HRESULT hr = pImpl->initNoGraphics(detailed_params);
    if(FAILED(hr))
    {
        delete pImpl;
        return ToAPIResult(hr);
    }
    *pResult = ToHandle(pImpl);
    return gfsdk_waveworks_result_OK;

	ENTRYPOINT_END
}

gfsdk_waveworks_result  GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_CreateGL2(const GFSDK_WaveWorks_Simulation_Settings& GL_ONLY(global_settings), const GFSDK_WaveWorks_Simulation_Params& GL_ONLY(global_params), void* GL_ONLY(pGLContext), GFSDK_WaveWorks_SimulationHandle* GL_ONLY(pResult))
{
	ENTRYPOINT_BEGIN_API(gl2)

#if WAVEWORKS_ENABLE_GL
	// Don't assume the user checked GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_XXXX()...
	const nv_water_simulation_api simulationAPI = ToAPI(global_settings.detail_level);
	switch(simulationAPI) {
		case nv_water_simulation_api_cuda:
			{
				if(g_CanUseCUDA)
					break;			// We detected CUDA, keep going
			}
		case nv_water_simulation_api_cpu:
			{
			#ifdef SUPPORT_FFTCPU
				break;
			#else
				return gfsdk_waveworks_result_FAIL;
			#endif
			}
	}

    GFSDK_WaveWorks_Simulation* pImpl = new GFSDK_WaveWorks_Simulation();
	GFSDK_WaveWorks_Detailed_Simulation_Params detailed_params;
	Init_Detailed_Water_Simulation_Params(global_settings, global_params, &detailed_params);
	HRESULT hr = pImpl->initGL2(detailed_params, pGLContext);
	if(hr != S_OK)
	{
		delete pImpl;
		return ToAPIResult(hr);
	}
	*pResult = ToHandle(pImpl);
	return gfsdk_waveworks_result_OK;
#else // WAVEWORKS_ENABLE_GL
	return gfsdk_waveworks_result_FAIL;
#endif // WAVEWORKS_ENABLE_GL

	ENTRYPOINT_END
}

gfsdk_waveworks_result  GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_Destroy(GFSDK_WaveWorks_SimulationHandle hSim)
{
	ENTRYPOINT_BEGIN
	GFSDK_WaveWorks_Simulation* pImpl = FromHandle(hSim);
	delete pImpl;

	return gfsdk_waveworks_result_OK;
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_UpdateProperties(GFSDK_WaveWorks_SimulationHandle hSim, const GFSDK_WaveWorks_Simulation_Settings& global_settings, const GFSDK_WaveWorks_Simulation_Params& global_params)
{
	ENTRYPOINT_BEGIN

	// Don't assume the user checked GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_XXXX()...
	if(gfsdk_waveworks_result_OK != CheckDetailLevelSupport(global_settings.detail_level,__DEF_FUNCTION__))
	{
		return gfsdk_waveworks_result_FAIL;
	}

	GFSDK_WaveWorks_Detailed_Simulation_Params detailed_params;
	GFSDK_WaveWorks_Simulation* pImpl = FromHandle(hSim);
	Init_Detailed_Water_Simulation_Params(global_settings, global_params, &detailed_params);
    return ToAPIResult(pImpl->reinit(detailed_params));
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_SetTime(GFSDK_WaveWorks_SimulationHandle hSim, double dAppTime)
{
	ENTRYPOINT_BEGIN
    FromHandle(hSim)->setSimulationTime(dAppTime);
    return gfsdk_waveworks_result_OK;
	ENTRYPOINT_END
}

namespace
{
	gfsdk_waveworks_result Simulation_Kick_Generic(GFSDK_WaveWorks_SimulationHandle hSim, gfsdk_U64* pKickID, Graphics_Context* pGC, GFSDK_WaveWorks_SavestateHandle hSavestate)
	{
		GFSDK_WaveWorks_Savestate* pImpl = NULL;
		if(hSavestate)
		{
			pImpl = FromHandle(hSavestate);
		}

		return ToAPIResult(FromHandle(hSim)->kick(pKickID, pGC, pImpl));
	}
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_KickNoGraphics(GFSDK_WaveWorks_SimulationHandle hSim, gfsdk_U64* pKickID)
{
	ENTRYPOINT_BEGIN_API(none)
	return Simulation_Kick_Generic(hSim, pKickID, NULL, NULL);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_KickD3D11(GFSDK_WaveWorks_SimulationHandle hSim, gfsdk_U64* pKickID, ID3D11DeviceContext* pDC, GFSDK_WaveWorks_SavestateHandle hSavestate)
{
	ENTRYPOINT_BEGIN_API(d3d11)
	Graphics_Context gc(pDC);
	return Simulation_Kick_Generic(hSim, pKickID, &gc, hSavestate);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_KickGnm(GFSDK_WaveWorks_SimulationHandle hSim, gfsdk_U64* pKickID, sce::Gnmx::LightweightGfxContext* pGC)
{
	ENTRYPOINT_BEGIN_API(gnm)
	Graphics_Context gc(pGC);
	return Simulation_Kick_Generic(hSim, pKickID, &gc, NULL);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_KickGL2(GFSDK_WaveWorks_SimulationHandle hSim, gfsdk_U64* pKickID)
{
	ENTRYPOINT_BEGIN_API(gl2)
	return Simulation_Kick_Generic(hSim, pKickID, NULL, NULL);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_GetStats(GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_Simulation_Stats& stats)
{
	ENTRYPOINT_BEGIN
	(FromHandle(hSim))->getStats(stats);
	return gfsdk_waveworks_result_OK;
	ENTRYPOINT_END
}

gfsdk_U32 GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D11()
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
    return GFSDK_WaveWorks_Simulation::getShaderInputCountD3D11();
	CUSTOM_ENTRYPOINT_END((gfsdk_U32)-1)
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_GetShaderInputDescD3D11(gfsdk_U32 inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc)
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
    return ToAPIResult(GFSDK_WaveWorks_Simulation::getShaderInputDescD3D11(inputIndex, pDesc));
	ENTRYPOINT_END
}

gfsdk_U32 GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_GetShaderInputCountGnm()
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
	return GFSDK_WaveWorks_Simulation::getShaderInputCountGnm();
	CUSTOM_ENTRYPOINT_END((gfsdk_U32)-1)
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_GetShaderInputDescGnm(gfsdk_U32 inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc)
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
	return ToAPIResult(GFSDK_WaveWorks_Simulation::getShaderInputDescGnm(inputIndex, pDesc));
	ENTRYPOINT_END
}

gfsdk_U32 GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_GetShaderInputCountGL2()
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
	return GFSDK_WaveWorks_Simulation::getShaderInputCountGL2();
	CUSTOM_ENTRYPOINT_END((gfsdk_U32)-1)
}

gfsdk_U32 GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_GetTextureUnitCountGL2(gfsdk_bool useTextureArrays)
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
	return GFSDK_WaveWorks_Simulation::getTextureUnitCountGL2(useTextureArrays);
	CUSTOM_ENTRYPOINT_END((gfsdk_U32)-1)
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_GetShaderInputDescGL2(gfsdk_U32 inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc)
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
	return ToAPIResult(GFSDK_WaveWorks_Simulation::getShaderInputDescGL2(inputIndex, pDesc));
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WaveWorks_Simulation_GetStagingCursor(GFSDK_WaveWorks_SimulationHandle hSim, gfsdk_U64* pKickID)
{
	ENTRYPOINT_BEGIN
	if(FromHandle(hSim)->getStagingCursor(pKickID))
	{
		// Returned true, meaning the staging cursor points to a valid set of kick results
		return gfsdk_waveworks_result_OK;
	}
	else
	{
		// Returned false, there are no valid kick results (yet)
		return gfsdk_waveworks_result_NONE;
	}
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WaveWorks_Simulation_GetReadbackCursor(GFSDK_WaveWorks_SimulationHandle hSim, gfsdk_U64* pKickID)
{
	ENTRYPOINT_BEGIN
	if(FromHandle(hSim)->getReadbackCursor(pKickID))
	{
		// Returned true, meaning the readback cursor points to a valid set of kick results
		return gfsdk_waveworks_result_OK;
	}
	else
	{
		// Returned false, there are no valid kick results (yet)
		return gfsdk_waveworks_result_NONE;
	}
	ENTRYPOINT_END
}

namespace
{
	gfsdk_waveworks_result Simulation_AdvanceStagingCursor_Generic(GFSDK_WaveWorks_SimulationHandle hSim, bool block, Graphics_Context* pGC, GFSDK_WaveWorks_SavestateHandle hSavestate)
	{
		GFSDK_WaveWorks_Savestate* pImpl = NULL;
		if(hSavestate)
		{
			pImpl = FromHandle(hSavestate);
		}

		bool wouldBlock = false;
		HRESULT hr = FromHandle(hSim)->advanceStagingCursor(pGC,block,wouldBlock,pImpl);
		if(S_OK == hr)
		{
			// The staging cursor points to a new set of kick results
			return gfsdk_waveworks_result_OK;
		}
		else if(S_FALSE == hr)
		{
			// The staging cursor did not advance
			if(wouldBlock)
			{
				// Would have blocked
				return gfsdk_waveworks_result_WOULD_BLOCK;
			}
			else
			{
				// Would not have blocked
				return gfsdk_waveworks_result_NONE;
			}
		}
		else
		{
			// Sometheing bad happened
			return ToAPIResult(hr);
		}
	}
}

gfsdk_waveworks_result GFSDK_WaveWorks_Simulation_AdvanceStagingCursorNoGraphics(GFSDK_WaveWorks_SimulationHandle hSim, bool block)
{
	ENTRYPOINT_BEGIN_API(none)
	return Simulation_AdvanceStagingCursor_Generic(hSim,block,NULL,NULL);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WaveWorks_Simulation_AdvanceStagingCursorD3D11(GFSDK_WaveWorks_SimulationHandle hSim, bool block, ID3D11DeviceContext* pDC, GFSDK_WaveWorks_SavestateHandle hSavestate)
{
	ENTRYPOINT_BEGIN_API(d3d11)
	Graphics_Context gc(pDC);
	return Simulation_AdvanceStagingCursor_Generic(hSim,block,&gc,hSavestate);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WaveWorks_Simulation_AdvanceStagingCursorGL2(GFSDK_WaveWorks_SimulationHandle hSim, bool block)
{
	ENTRYPOINT_BEGIN_API(gl2)
	return Simulation_AdvanceStagingCursor_Generic(hSim,block,NULL,NULL);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WaveWorks_Simulation_AdvanceStagingCursorGnm(GFSDK_WaveWorks_SimulationHandle hSim, bool block, sce::Gnmx::LightweightGfxContext* pGC)
{
	ENTRYPOINT_BEGIN_API(gnm)
	Graphics_Context gc(pGC);
	return Simulation_AdvanceStagingCursor_Generic(hSim,block,&gc,NULL);
	ENTRYPOINT_END
}

namespace
{
	gfsdk_waveworks_result Simulation_WaitStagingCursor_Generic(GFSDK_WaveWorks_SimulationHandle hSim)
	{
		HRESULT hr = FromHandle(hSim)->waitStagingCursor();
		if(S_OK == hr)
		{
			// The staging cursor is ready to advance
			return gfsdk_waveworks_result_OK;
		}
		else if(S_FALSE == hr)
		{
			// The staging cursor did not advance
			return gfsdk_waveworks_result_NONE;
		}
		else
		{
			// Sometheing bad happened
			return ToAPIResult(hr);
		}
	}
}

gfsdk_waveworks_result GFSDK_WaveWorks_Simulation_WaitStagingCursor(GFSDK_WaveWorks_SimulationHandle hSim)
{
	ENTRYPOINT_BEGIN
	return Simulation_WaitStagingCursor_Generic(hSim);
	ENTRYPOINT_END
}

namespace
{
	gfsdk_waveworks_result Simulation_AdvanceReadbackCursor_Generic(GFSDK_WaveWorks_SimulationHandle hSim, bool block)
	{
		bool wouldBlock = false;
		HRESULT hr = FromHandle(hSim)->advanceReadbackCursor(block,wouldBlock);
		if(S_OK == hr)
		{
			// The staging cursor points to a new set of kick results
			return gfsdk_waveworks_result_OK;
		}
		else if(S_FALSE == hr)
		{
			// The staging cursor did not advance
			if(wouldBlock)
			{
				// Would have blocked
				return gfsdk_waveworks_result_WOULD_BLOCK;
			}
			else
			{
				// Would not have blocked
				return gfsdk_waveworks_result_NONE;
			}
		}
		else
		{
			// Sometheing bad happened
			return ToAPIResult(hr);
		}
	}
}

gfsdk_waveworks_result GFSDK_WaveWorks_Simulation_AdvanceReadbackCursor(GFSDK_WaveWorks_SimulationHandle hSim, bool block)
{
	ENTRYPOINT_BEGIN
	return Simulation_AdvanceReadbackCursor_Generic(hSim,block);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WaveWorks_Simulation_ArchiveDisplacements(GFSDK_WaveWorks_SimulationHandle hSim)
{
	ENTRYPOINT_BEGIN
	return ToAPIResult(FromHandle(hSim)->archiveDisplacements());
	ENTRYPOINT_END
}

namespace
{
	gfsdk_waveworks_result Simulation_SetRenderState_Generic(GFSDK_WaveWorks_SimulationHandle hSim, Graphics_Context* pGC, const gfsdk_float4x4& matView, const gfsdk_U32* pShaderInputRegisterMappings, GFSDK_WaveWorks_SavestateHandle hSavestate, const GFSDK_WaveWorks_Simulation_GL_Pool* pGlPool)
	{
		GFSDK_WaveWorks_Savestate* pImpl = NULL;
		if(hSavestate)
		{
			pImpl = FromHandle(hSavestate);
		}

		return ToAPIResult(FromHandle(hSim)->setRenderState(pGC, matView, pShaderInputRegisterMappings, pImpl, pGlPool));
	}
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_SetRenderStateD3D11(GFSDK_WaveWorks_SimulationHandle hSim, ID3D11DeviceContext* pDC, const gfsdk_float4x4& matView, const gfsdk_U32* pShaderInputRegisterMappings, GFSDK_WaveWorks_SavestateHandle hSavestate)
{
	ENTRYPOINT_BEGIN_API(d3d11)
	Graphics_Context gc(pDC);
	return Simulation_SetRenderState_Generic(hSim,&gc,matView,pShaderInputRegisterMappings,hSavestate,NULL);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_SetRenderStateGnm(GFSDK_WaveWorks_SimulationHandle hSim, sce::Gnmx::LightweightGfxContext* pGC, const gfsdk_float4x4& matView, const gfsdk_U32* pShaderInputRegisterMappings)
{
	ENTRYPOINT_BEGIN_API(gnm)
	Graphics_Context gc(pGC);
	return Simulation_SetRenderState_Generic(hSim,&gc,matView,pShaderInputRegisterMappings,NULL,NULL);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_SetRenderStateGL2(GFSDK_WaveWorks_SimulationHandle hSim, const gfsdk_float4x4& matView, const gfsdk_U32* pShaderInputRegisterMappings, const GFSDK_WaveWorks_Simulation_GL_Pool& glPool)
{
	ENTRYPOINT_BEGIN_API(gl2)
	return Simulation_SetRenderState_Generic(hSim,NULL,matView,pShaderInputRegisterMappings,NULL,&glPool);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_GetDisplacements(GFSDK_WaveWorks_SimulationHandle hSim, const gfsdk_float2* inSamplePoints, gfsdk_float4* outDisplacements, gfsdk_U32 numSamples)
{
	ENTRYPOINT_BEGIN
    FromHandle(hSim)->getDisplacements(inSamplePoints, outDisplacements, numSamples);
    return gfsdk_waveworks_result_OK;
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_GetArchivedDisplacements(GFSDK_WaveWorks_SimulationHandle hSim, float coord, const gfsdk_float2* inSamplePoints, gfsdk_float4* outDisplacements, gfsdk_U32 numSamples)
{
	ENTRYPOINT_BEGIN
    FromHandle(hSim)->getArchivedDisplacements(coord, inSamplePoints, outDisplacements, numSamples);
    return gfsdk_waveworks_result_OK;
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_CreateD3D11(const GFSDK_WaveWorks_Quadtree_Params& params, ID3D11Device* pD3DDevice, GFSDK_WaveWorks_QuadtreeHandle* pResult)
{
	ENTRYPOINT_BEGIN_API(d3d11)

    GFSDK_WaveWorks_Quadtree* pImpl = new GFSDK_WaveWorks_Quadtree();
    HRESULT hr = pImpl->initD3D11(params, pD3DDevice);
    if(FAILED(hr))
    {
        delete pImpl;
        return ToAPIResult(hr);
    }

    *pResult = ToHandle(pImpl);
    return gfsdk_waveworks_result_OK;

	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_CreateGnm(const GFSDK_WaveWorks_Quadtree_Params& params, GFSDK_WaveWorks_QuadtreeHandle* pResult)
{
	ENTRYPOINT_BEGIN_API(gnm)

	GFSDK_WaveWorks_Quadtree* pImpl = new GFSDK_WaveWorks_Quadtree();
	HRESULT hr = pImpl->initGnm(params);
	if(FAILED(hr))
	{
		delete pImpl;
		return ToAPIResult(hr);
	}

	*pResult = ToHandle(pImpl);
	return gfsdk_waveworks_result_OK;

	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_CreateGL2(const GFSDK_WaveWorks_Quadtree_Params& params, GLuint Program, GFSDK_WaveWorks_QuadtreeHandle* pResult)
{
	ENTRYPOINT_BEGIN_API(gl2)

    GFSDK_WaveWorks_Quadtree* pImpl = new GFSDK_WaveWorks_Quadtree();
    HRESULT hr = pImpl->initGL2(params, Program);
    if(FAILED(hr))
    {
        delete pImpl;
        return ToAPIResult(hr);
    }

    *pResult = ToHandle(pImpl);
    return gfsdk_waveworks_result_OK;

	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_Destroy(GFSDK_WaveWorks_QuadtreeHandle hQuadtree)
{
	ENTRYPOINT_BEGIN
	GFSDK_WaveWorks_Quadtree* pImpl = FromHandle(hQuadtree);
	delete pImpl;

	return gfsdk_waveworks_result_OK;
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_UpdateParams(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, const GFSDK_WaveWorks_Quadtree_Params& params)
{
	ENTRYPOINT_BEGIN
    return ToAPIResult(FromHandle(hQuadtree)->reinit(params));
	ENTRYPOINT_END
}

gfsdk_U32 GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_GetShaderInputCountD3D11()
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
	return GFSDK_WaveWorks_Quadtree::getShaderInputCountD3D11();
	CUSTOM_ENTRYPOINT_END((gfsdk_U32)-1)
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_GetShaderInputDescD3D11(gfsdk_U32 inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc)
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
	return ToAPIResult(GFSDK_WaveWorks_Quadtree::getShaderInputDescD3D11(inputIndex, pDesc));
	ENTRYPOINT_END
}

gfsdk_U32 GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_GetShaderInputCountGnm()
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
	return GFSDK_WaveWorks_Quadtree::getShaderInputCountGnm();
	CUSTOM_ENTRYPOINT_END((gfsdk_U32)-1)
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_GetShaderInputDescGnm(gfsdk_U32 inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc)
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
	return ToAPIResult(GFSDK_WaveWorks_Quadtree::getShaderInputDescGnm(inputIndex, pDesc));
	ENTRYPOINT_END
}

gfsdk_U32 GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_GetShaderInputCountGL2()
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
    return GFSDK_WaveWorks_Quadtree::getShaderInputCountGL2();
	CUSTOM_ENTRYPOINT_END((gfsdk_U32)-1)
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_GetShaderInputDescGL2(gfsdk_U32 inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc)
{
	ENTRYPOINT_BEGIN_NO_INIT_CHECK
	return ToAPIResult(GFSDK_WaveWorks_Quadtree::getShaderInputDescGL2(inputIndex, pDesc));
	ENTRYPOINT_END
}

namespace
{
	gfsdk_waveworks_result Quadtree_Draw_Generic(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, Graphics_Context* pGC, const gfsdk_float4x4& matView, const gfsdk_float4x4& matProj, const gfsdk_float2* pViewportDims, const gfsdk_U32* pShaderInputRegisterMappings, GFSDK_WaveWorks_SavestateHandle hSavestate)
	{
		GFSDK_WaveWorks_Savestate* pSavestateImpl = NULL;
		if(hSavestate)
		{
			pSavestateImpl = FromHandle(hSavestate);
		}

		HRESULT hr;
		GFSDK_WaveWorks_Quadtree* pImpl = FromHandle(hQuadtree);
		API_RETURN(pImpl->buildRenderList(pGC, matView, matProj, pViewportDims));
		API_RETURN(pImpl->flushRenderList(pGC, pShaderInputRegisterMappings, pSavestateImpl));

		return gfsdk_waveworks_result_OK;
	}
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_DrawD3D11(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, ID3D11DeviceContext* pDC, const gfsdk_float4x4& matView, const gfsdk_float4x4& matProj, const gfsdk_U32* pShaderInputRegisterMappings, GFSDK_WaveWorks_SavestateHandle hSavestate)
{
	ENTRYPOINT_BEGIN_API(d3d11)
	Graphics_Context gc(pDC);
	return Quadtree_Draw_Generic(hQuadtree,&gc,matView,matProj,NULL,pShaderInputRegisterMappings,hSavestate);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_DrawGnm(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, sce::Gnmx::LightweightGfxContext* pGC, const gfsdk_float4x4& matView, const gfsdk_float4x4& matProj, const gfsdk_float2& viewportDims, const gfsdk_U32* pShaderInputRegisterMappings)
{
	ENTRYPOINT_BEGIN_API(gnm)
	Graphics_Context gc(pGC);
	return Quadtree_Draw_Generic(hQuadtree,&gc,matView,matProj,&viewportDims,pShaderInputRegisterMappings,NULL);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_DrawGL2(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, const gfsdk_float4x4& matView, const gfsdk_float4x4& matProj, const gfsdk_U32* pShaderInputRegisterMappings)
{
	ENTRYPOINT_BEGIN_API(gl2)
	return Quadtree_Draw_Generic(hQuadtree,NULL,matView,matProj,NULL,pShaderInputRegisterMappings,NULL);
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_AllocPatch(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, gfsdk_S32 x, gfsdk_S32 y, gfsdk_U32 lod, gfsdk_bool enabled)
{
	ENTRYPOINT_BEGIN
	return ToAPIResult(FromHandle(hQuadtree)->allocPatch(x, y, lod, enabled));
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_FreePatch(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, gfsdk_S32 x, gfsdk_S32 y, gfsdk_U32 lod)
{
	ENTRYPOINT_BEGIN
	return ToAPIResult(FromHandle(hQuadtree)->freePatch(x, y, lod));
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_GetStats(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, GFSDK_WaveWorks_Quadtree_Stats& stats)
{
	ENTRYPOINT_BEGIN
	return ToAPIResult(FromHandle(hQuadtree)->getStats(stats));
	ENTRYPOINT_END
}

gfsdk_waveworks_result GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, gfsdk_F32 margin)
{
	ENTRYPOINT_BEGIN
	return ToAPIResult(FromHandle(hQuadtree)->setFrustumCullMargin(margin));
	ENTRYPOINT_END
}

gfsdk_F32 GFSDK_WAVEWORKS_CALL_CONV GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(GFSDK_WaveWorks_SimulationHandle hSim)
{	
	ENTRYPOINT_BEGIN
	return FromHandle(hSim)->getConservativeMaxDisplacementEstimate();
	ENTRYPOINT_END
}

namespace WaveWorks_Internal
{
	void diagnostic_message(const char_type *fmt, ...)
	{
#if defined(TARGET_PLATFORM_NIXLIKE)
#if defined (__ANDROID__)
		char s[65536];
		va_list arg;
		va_start(arg, fmt);
		vsnprintf (s, 65535, fmt, arg);
		__android_log_print(ANDROID_LOG_ERROR,"WaveWorks", s);
		va_end(arg);
#else	
		va_list arg;
		va_start(arg, fmt);
		vfprintf(stderr, fmt, arg);
		va_end(arg);
#endif	
#else
		va_list arg;
		va_start(arg, fmt);
		const int numChars = _vscwprintf(fmt,arg)+1;
		const int bufferSize = (numChars) * sizeof(char_type);
		va_end(arg);

		char_type* pStackBuffer = new char_type[numChars];
		va_start(arg, fmt);
		_vswprintf_p(pStackBuffer,bufferSize,fmt,arg);
		va_end(arg);

		OutputDebugString(pStackBuffer);

		delete pStackBuffer;
#endif
	}
}

#if defined (_DEV) || defined (DEBUG)
namespace
{
	void msg_and_break(const char_type* errMsg)
	{
		NV_ERROR(errMsg);
		DebugBreak();
	}
}

void handle_hr_error(HRESULT hr, const char_type* file, gfsdk_S32 line)
{
	char_type msg[1024];
	SPRINTF( SPRINTF_ARG0(msg), TEXT("%s(%i): hr error : %i\n"), file, line, hr );
	msg_and_break(msg);
}

#ifdef SUPPORT_CUDA
void handle_cuda_error(cudaError errCode, const char_type* file, gfsdk_S32 line)
{
	char_type msg[1024];
	SPRINTF( SPRINTF_ARG0(msg), TEXT("%s(%i): CUDA error : %S\n"), file, line, cudaGetErrorString(errCode) );
	msg_and_break(msg);
}

void handle_cufft_error(cufftResult errCode, const char_type* file, gfsdk_S32 line)
{
	char_type msg[1024];
	SPRINTF( SPRINTF_ARG0(msg), TEXT("%s(%i): cufft error : %i\n"), file, line, errCode );
	msg_and_break(msg);
}
#endif

#if WAVEWORKS_ENABLE_GL
void check_gl_errors(const char_type* file, gfsdk_S32 line)
{
	GLenum error;
	while (( error = NVSDK_GLFunctions.glGetError() ) != 0)
    {
		NV_ERROR(TEXT("\r\n%s(%i): OpenGL error : %i\n"), file, line, error);
	}
}
#endif // WAVEWORKS_ENABLE_GL
#endif // _DEV
