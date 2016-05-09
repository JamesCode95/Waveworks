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

#ifndef _NVWAVEWORKS_INTERNAL_H
#define _NVWAVEWORKS_INTERNAL_H

#include "Shared_Globals.h"

#define _HAS_EXCEPTIONS 0
#ifndef _XBOX_ONE
#define _STATIC_CPPLIB
#endif

#ifdef _NVWAVEWORKS_H
#error include Internal.h before GFSDK_WaveWorks.h
#endif

#if defined(_XBOX_ONE)
#define TARGET_PLATFORM_XBONE
#elif defined(WIN32)
#define TARGET_PLATFORM_WINDOWS
#elif defined(__ORBIS__)
#define TARGET_PLATFORM_PS4
#elif defined(__linux__) && (!defined(__ANDROID__))
#define TARGET_PLATFORM_LINUX
#elif defined(__APPLE__)
#define TARGET_PLATFORM_MACOSX
#elif defined(__ANDROID__)
#define TARGET_PLATFORM_ANDROID
#else
#error Unsupported platform!
#endif


#if defined(TARGET_PLATFORM_MACOSX) || defined(TARGET_PLATFORM_ANDROID)
#include <math.h>
#endif

#if defined(TARGET_PLATFORM_ANDROID)
#include <android/log.h>
#endif

#if defined(TARGET_PLATFORM_WINDOWS) || defined(TARGET_PLATFORM_XBONE)
#define TARGET_PLATFORM_MICROSOFT
#endif

#if defined(TARGET_PLATFORM_LINUX) || defined(TARGET_PLATFORM_PS4) || defined(TARGET_PLATFORM_MACOSX) || defined(TARGET_PLATFORM_ANDROID)
#define TARGET_PLATFORM_NIXLIKE
#endif

// Ensure all the lib symbols are hidden except the ones marked with "default" visibility attribute on Mac
#ifdef __APPLE__
#define NVWAVEWORKS_LIB_DLL_EXPORTS
#endif

#ifdef NVWAVEWORKS_LIB_DLL_EXPORTS
#ifdef TARGET_PLATFORM_PS4
#define GFSDK_WAVEWORKS_LINKAGE __declspec(dllexport)
#elif defined(__GNUC__)
#define GFSDK_WAVEWORKS_LINKAGE __attribute__ ((visibility ("default")))
#else
#define GFSDK_WAVEWORKS_LINKAGE __declspec(dllexport)
#endif
#endif

// Ensure STL implicit template instantiations are not exposed unnecessarily as weak symbols
#ifdef TARGET_PLATFORM_LINUX
#include <bits/c++config.h>
#undef _GLIBCXX_VISIBILITY_ATTR
#define _GLIBCXX_VISIBILITY_ATTR(V)
#endif

// Ensure expected/supported Orbis SDK version
#ifdef TARGET_PLATFORM_PS4


#include <sdk_version.h>
#if SCE_ORBIS_SDK_VERSION != EXPECTED_SCE_ORBIS_SDK_VERSION
#error Unexpected SCE_ORBIS_SDK_VERSION version
#endif
#if SCE_ORBIS_SDK_VERSION < 0x01500000
#error Unsupported SCE_ORBIS_SDK_VERSION version
#endif
#if SCE_ORBIS_SDK_VERSION > 0x02599999
#error Unsupported SCE_ORBIS_SDK_VERSION version
#endif
#include "restricted/GFSDK_WaveWorks_Orbis_API_Interface.h"

#if SCE_ORBIS_SDK_VERSION > 0x01700000u
#define SAMPLE_1 Gnm::kNumFragments1
#define GET_SIZE_IN_BYTES( rt ) rt->getSliceSizeInBytes()
#else		 
#define SAMPLE_1 Gnm::kNumSamples1
#define GET_SIZE_IN_BYTES( rt ) rt->getSizeInBytes()
#endif
#endif // TARGET_PLATFORM_PS4

// Ensure expected/supported Xbone SDK version
#ifdef TARGET_PLATFORM_XBONE
#include <xdk.h>
#if _XDK_VER != EXPECTED_XDK_VER
#error Unexpected _XDK_VER version
#endif
#if _XDK_VER < 11396
#error Unsupported _XDK_VER version
#endif
#if _XDK_VER > 12710
#error Unsupported _XDK_VER version
#endif
#endif // TARGET_PLATFORM_XBONE

#if defined(WAVEWORKS_NDA_BUILD)
// NB: This *must* be included before the main WaveWorks header in order to replace
// the default (public) GUID definitions in GFSDK_WaveWorks_GUID.h.
// NB: Also note that in the shipping distro, GFSDK_WaveWorks_GUID.h is replaced
// by <restricted/GFSDK_WaveWorks_GUID_NDA.h>, so consumers of the lib should
// have a 'seamless' experience with no need for such carefully controlled #include
// orderings
#include <restricted/GFSDK_WaveWorks_GUID_NDA.h>
#endif

#ifdef TARGET_PLATFORM_PS4
#include <restricted/GFSDK_WaveWorks_Orbis.h>
#else
#include <GFSDK_WaveWorks.h>
#endif

#if defined(WAVEWORKS_NDA_BUILD)
#include <restricted/GFSDK_WaveWorks_CPU_Scheduler.h>
#endif

#ifdef TARGET_PLATFORM_MICROSOFT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#pragma warning(disable:4005) 
#ifdef TARGET_PLATFORM_XBONE
#include <d3d11_x.h>

// We normally rely on the VS IDE to set these, but this sanity-check should help if we ever
// use some other build tool...
#ifndef WINAPI_FAMILY
#error WINAPI_FAMILY undefined
#endif
#if WINAPI_FAMILY!=WINAPI_FAMILY_TV_TITLE
#error Unexpected value for WINAPI_FAMILY, expected WINAPI_FAMILY_TV_TITLE
#endif

#else // TARGET_PLATFORM_XBONE
#include <dxgi.h>
#include <d3d11.h>
#include <d3d9.h>

// Check we're building against a recent-enough WinSDK
#include <winsdkver.h>
#ifndef _WIN32_MAXVER
#error _WIN32_MAXVER is undefined, expected _WIN32_MAXVER=0x0602
#endif
#if _WIN32_MAXVER < 0x0602
#error Expected _WIN32_MAXVER >= 0x0602, is Windows SDK 8.0 or greater correctly installed and configured?
#endif

#include <winnt.h>

#endif

inline LONG customInterlockedAdd(volatile LONG* pointer, LONG value)
{
	return InterlockedExchangeAdd(pointer,value)+value;
}

inline LONG customInterlockedSubtract(volatile LONG* pointer, LONG value)
{
	return InterlockedExchangeAdd(pointer,-value)-value;
}

// We use a little wrapper class for CB updates, so that we can encapsulate away
// the differences between preferred update mechanisms on Xbone vs PC
#ifdef TARGET_PLATFORM_XBONE
	#define D3D11_CB_CREATION_CPU_ACCESS_FLAGS D3D11_CPU_ACCESS_WRITE
	#define D3D11_CB_CREATION_USAGE D3D11_USAGE_DYNAMIC
	template<class T> struct D3D11_CB_Updater
	{
		D3D11_CB_Updater(ID3D11DeviceContext* pD3Dctxt, ID3D11Buffer* pD3Dcb) :
			m_pD3Dctxt(pD3Dctxt),
			m_pD3Dcb(pD3Dcb)
		{
			D3D11_MAPPED_SUBRESOURCE msr;
			m_pD3Dctxt->Map( m_pD3Dcb, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr );
			m_pMappedCb = (T*)msr.pData;
		}

		~D3D11_CB_Updater()
		{
			m_pD3Dctxt->Unmap(m_pD3Dcb,0);
		}

		T& cb() { return *m_pMappedCb; }

	private:
		T* m_pMappedCb;
		ID3D11DeviceContext* m_pD3Dctxt;
		ID3D11Buffer* m_pD3Dcb;
	};
#else // TARGET_PLATFORM_XBONE
	#define D3D11_CB_CREATION_CPU_ACCESS_FLAGS 0
	#define D3D11_CB_CREATION_USAGE D3D11_USAGE_DEFAULT
	template<class T> struct D3D11_CB_Updater
	{
		D3D11_CB_Updater(ID3D11DeviceContext* pD3Dctxt, ID3D11Buffer* pD3Dcb) :
			m_pD3Dctxt(pD3Dctxt),
			m_pD3Dcb(pD3Dcb)
		{
		}

		~D3D11_CB_Updater()
		{
			m_pD3Dctxt->UpdateSubresource(m_pD3Dcb,0,NULL,&m_cb,0,0);
		}

		T& cb() { return m_cb; }

	private:
		T m_cb;
		ID3D11DeviceContext* m_pD3Dctxt;
		ID3D11Buffer* m_pD3Dcb;
	};
#endif

#else // !TARGET_PLATFORM_MICROSOFT...
#include <stdint.h>
#include <stdio.h>

#include <algorithm> 
using std::min;
using std::max;

typedef int HRESULT;
#define S_OK ((HRESULT)0L)
#define S_FALSE ((HRESULT)1L)
#define E_FAIL ((HRESULT)0x80000008L)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define FALSE               0
#define TRUE                1

typedef int BOOL;
typedef unsigned char BYTE;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned int /*long*/ DWORD; // long is 64b on x64-GCC, but 32b on VC!
typedef size_t SIZE_T;
typedef int64_t __int64;
typedef uint64_t UINT64;
typedef void* HANDLE; 
typedef void* HMODULE;
typedef int /*long*/ LONG; // long is 64b on x64-GCC, but 32b on VC!
typedef const char* LPCSTR;
typedef float FLOAT;

inline void DebugBreak()
{
	__builtin_trap();	
}
inline LONG InterlockedDecrement(volatile LONG* pointer)
{
	return __sync_sub_and_fetch(pointer, 1);
}
inline LONG InterlockedIncrement(volatile LONG* pointer)
{
	return __sync_add_and_fetch(pointer, 1);
}
inline LONG customInterlockedAdd(volatile LONG* pointer, LONG value)
{
	return __sync_add_and_fetch(pointer, value);
}
inline LONG customInterlockedSubtract(volatile LONG* pointer, LONG value)
{
	return __sync_sub_and_fetch(pointer, value);
}
#endif

// Fwd. decls for common internal classes
namespace WaveWorks_Internal
{
	class Graphics_Context;
}

using namespace WaveWorks_Internal;

#include "FFT_API_support.h"
#include "CustomMemory.h"
#include "Mesh.h"

#include <assert.h>



// D3D/D3DX version checks
#ifdef TARGET_PLATFORM_XBONE
	#if !defined(D3D11_SDK_VERSION) || ((D3D11_SDK_VERSION >= 0x1000B) && (D3D11_SDK_VERSION <= 0x20011))
	#else
	#error Wrong D3D11_SDK_VERSION - expected 0x1000B
	#endif
#else // TARGET_PLATFORM_XBONE
	#if !defined(D3D11_SDK_VERSION) || (D3D11_SDK_VERSION == 7)
	#else
	#error Wrong D3D11_SDK_VERSION - expected 7
	#endif
#endif

#if !defined(D3D10_SDK_VERSION) || (D3D10_SDK_VERSION == 29)
#else
#error Wrong D3D10_SDK_VERSION - expected 29
#endif

#if !defined(D3D_SDK_VERSION) || (D3D_SDK_VERSION == 32)
#else
#error Wrong D3D_SDK_VERSION - expected 32
#endif

// Character/string types
#if defined(TARGET_PLATFORM_NIXLIKE)
typedef char char_type;
#define TEXT(x) x
#define TSTR(s) s
#define SPRINTF sprintf
#define SPRINTF_ARG0(x) x
#define ASCII_STR_FMT "%s"
#else
typedef WCHAR char_type;
#define TEXT(x) L##x
#define WSTR(s) L##s
#define TSTR(s) WSTR(s)
#define SPRINTF swprintf_s
#define SPRINTF_ARG0(x) x, sizeof(x)/sizeof(x[0])
#define ASCII_STR_FMT L"%S"
#endif

// Timestamp type
#if defined(TARGET_PLATFORM_PS4)
typedef unsigned long long TickType;
#elif defined(TARGET_PLATFORM_NIXLIKE)
typedef struct timespec TickType;
#else
typedef __int64 TickType;
#endif

#if defined (_DEV) || defined (DEBUG)
void handle_hr_error(HRESULT hr, const char_type* file, int line);
#define HANDLE_HR_ERROR(err) handle_hr_error(err, __DEF_FILE__, __LINE__)
#else
#define HANDLE_HR_ERROR(err)
#endif

#ifdef __GNUC__
// TODO: get some kind of __FUNCTION__ thing going on GCC
#define __DEF_FUNCTION__ TEXT("WaveWorks API function")
#define __DEF_FILE__ TSTR(__FILE__)
#else
#define __DEF_FUNCTION__ TSTR(__FUNCTION__)
#define __DEF_FILE__ TSTR(__FILE__)
#endif

#ifdef __GNUC__
#define ALIGN16_BEG
#define ALIGN16_END __attribute__ ((aligned(16)))
#else
// Assuming MSVC
#define ALIGN16_BEG __declspec(align(16))
#define ALIGN16_END 
#endif

#ifdef WAVEWORKS_FORCE_GFX_DISABLED
#define WAVEWORKS_ALLOW_GFX 0
#else
#define WAVEWORKS_ALLOW_GFX 1
#endif

#if D3D_SDK_VERSION
#define WAVEWORKS_ENABLE_D3D9 WAVEWORKS_ALLOW_GFX
#else
#define WAVEWORKS_ENABLE_D3D9 0
#endif

#ifdef D3D10_SDK_VERSION
#define WAVEWORKS_ENABLE_D3D10 WAVEWORKS_ALLOW_GFX
#else
#define WAVEWORKS_ENABLE_D3D10 0
#endif

#ifdef D3D11_SDK_VERSION
#define WAVEWORKS_ENABLE_D3D11 WAVEWORKS_ALLOW_GFX
#else
#define WAVEWORKS_ENABLE_D3D11 0
#endif

#ifdef TARGET_PLATFORM_PS4
#define WAVEWORKS_ENABLE_GNM WAVEWORKS_ALLOW_GFX
#else
#define WAVEWORKS_ENABLE_GNM 0
#endif

#ifdef TARGET_PLATFORM_WINDOWS
#define WAVEWORKS_ENABLE_GL WAVEWORKS_ALLOW_GFX
#else
#ifdef TARGET_PLATFORM_MACOSX
#define WAVEWORKS_ENABLE_GL WAVEWORKS_ALLOW_GFX
#else
#ifdef TARGET_PLATFORM_ANDROID
#define WAVEWORKS_ENABLE_GL WAVEWORKS_ALLOW_GFX
#else
#define WAVEWORKS_ENABLE_GL 0
#endif
#endif
#endif

#define WAVEWORKS_ENABLE_GRAPHICS (WAVEWORKS_ENABLE_D3D9 || WAVEWORKS_ENABLE_D3D10 || WAVEWORKS_ENABLE_D3D11 || WAVEWORKS_ENABLE_GNM || WAVEWORKS_ENABLE_GL)

#ifndef SUPPORT_CUDA
	typedef struct
	{
		float x;
		float y;
	} float2;

	typedef struct
	{
		float x;
		float y;
		float z;
		float w;
	} float4;
#else
	#pragma warning( push )
	#pragma warning( disable : 4201 )
	#pragma warning( disable : 4408 )

	#include <cuda.h>
	#include <builtin_types.h>
	#include <cufft.h>

	#pragma warning( pop )

	#if WAVEWORKS_ENABLE_D3D9
	#include <cuda_d3d9_interop.h>
	#endif

	#if WAVEWORKS_ENABLE_D3D10
	#include <cuda_d3d10_interop.h>
	#endif

	#if WAVEWORKS_ENABLE_D3D11
	#include <cuda_d3d11_interop.h>
	#endif
	
	#if WAVEWORKS_ENABLE_GL
	#include <cuda_gl_interop.h>
	#endif

	//#include <cutil.h>
	#include <cuda_runtime_api.h>

// 	#if (CUDA_VERSION == 5050)
// 	#else
// 	#error Wrong CUDA version - expected 5050 (5.5)
// 	#endif

	#if defined (_DEV) || defined (DEBUG)
		void handle_cuda_error(cudaError errCode, const char_type* file, int line);
		#define HANDLE_CUDA_ERROR(err) handle_cuda_error(err, __DEF_FILE__, __LINE__)
		void handle_cufft_error(cufftResult errCode, const char_type* file, int line);
		#define HANDLE_CUFFT_ERROR(err) handle_cufft_error(err, __DEF_FILE__, __LINE__)
	#else
		#define HANDLE_CUDA_ERROR(err)
		#define HANDLE_CUFFT_ERROR(err)
		#define HANDLE_HR_ERROR(err)
	#endif

	#ifndef CUDA_V_RETURN
		#define CUDA_V_RETURN(call) {                                            \
		cudaError err = call;                                                    \
		if( cudaSuccess != err) {                                                \
			HANDLE_CUDA_ERROR(err);                                              \
			return E_FAIL;                                                       \
		} }
	#endif

	#ifndef CUDA_API_RETURN
		#define CUDA_API_RETURN(call) {                                          \
		cudaError err = call;                                                    \
		if( cudaSuccess != err) {                                                \
			HANDLE_CUDA_ERROR(err);                                              \
			return gfsdk_waveworks_result_FAIL;                                            \
		} }
	#endif

	#ifndef CUFFT_V_RETURN
		#define CUFFT_V_RETURN(call) {                                           \
		cufftResult err = call;                                                  \
		if( CUFFT_SUCCESS != err) {                                              \
			HANDLE_CUFFT_ERROR(err);                                              \
			return E_FAIL;                                                       \
		} }
	#endif

	#define CUDA_SAFE_FREE(p)		{ if (p) { CUDA_V_RETURN(cudaFree(p)); (p)=NULL; } }
	#define CUDA_SAFE_FREE_HOST(p)	{ if (p) { CUDA_V_RETURN(cudaFreeHost(p)); (p)=NULL; } }

#endif //SUPPORT_CUDA

#if WAVEWORKS_ENABLE_GL
	#if defined (_DEV) || defined (DEBUG)
		void check_gl_errors(const char_type* file, int line);
		#define CHECK_GL_ERRORS check_gl_errors(__DEF_FILE__, __LINE__)
	#else
		#define CHECK_GL_ERRORS
	#endif
#endif // #if WAVEWORKS_ENABLE_GL

#ifndef V
	#define V(x)           { hr = x; }
#endif
#ifndef V_RETURN
	#define V_RETURN(x)    { hr = x; if( FAILED(hr) ) { HANDLE_HR_ERROR(hr); return hr; } }
#endif
#ifndef API_RETURN
	#define API_RETURN(x)  { hr = x; if( FAILED(hr) ) { HANDLE_HR_ERROR(hr); return gfsdk_waveworks_result_FAIL; } }
#endif

#ifndef SAFE_DELETE
	#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif


enum nv_water_d3d_api
{
	nv_water_d3d_api_undefined = 0,
	nv_water_d3d_api_none,				// Meaning: initialise and run without graphics e.g. server-mode
	nv_water_d3d_api_d3d9,
	nv_water_d3d_api_d3d10,
	nv_water_d3d_api_d3d11,
	nv_water_d3d_api_gnm,
	nv_water_d3d_api_gl2
};

enum nv_water_simulation_api
{
	nv_water_simulation_api_cuda = 0,
	nv_water_simulation_api_direct_compute,
	nv_water_simulation_api_cpu,
#if defined(SUPPORT_CUDA)
	nv_water_simulation_api_gpu_preferred = nv_water_simulation_api_cuda,
#elif defined(SUPPORT_DIRECTCOMPUTE)
	nv_water_simulation_api_gpu_preferred = nv_water_simulation_api_direct_compute,
#else
	nv_water_simulation_api_gpu_preferred = nv_water_simulation_api_cpu,
#endif
};

// As a readability convenience...
enum { nvrm_unused = GFSDK_WaveWorks_UnusedShaderInputRegisterMapping };

namespace WaveWorks_Internal
{
	// Convenience functions for resolving detail levels
	inline int ToInt(GFSDK_WaveWorks_Simulation_DetailLevel dl)
	{
		switch(dl)
		{
		case GFSDK_WaveWorks_Simulation_DetailLevel_Normal: return MAX_FFT_RESOLUTION/4;
		case GFSDK_WaveWorks_Simulation_DetailLevel_High: return MAX_FFT_RESOLUTION/2;
		case GFSDK_WaveWorks_Simulation_DetailLevel_Extreme: return MAX_FFT_RESOLUTION;
		default: return MAX_FFT_RESOLUTION;
		}
	}

	inline nv_water_simulation_api ToAPI(GFSDK_WaveWorks_Simulation_DetailLevel dl)
	{
		switch(dl)
		{
		case GFSDK_WaveWorks_Simulation_DetailLevel_Normal:
#if defined(SUPPORT_FFTCPU)
			return nv_water_simulation_api_cpu;
#else
			return nv_water_simulation_api_gpu_preferred;
#endif
		case GFSDK_WaveWorks_Simulation_DetailLevel_High: return nv_water_simulation_api_gpu_preferred;
		case GFSDK_WaveWorks_Simulation_DetailLevel_Extreme: return nv_water_simulation_api_gpu_preferred;
		default: return nv_water_simulation_api_gpu_preferred;
		}
	}

	inline gfsdk_waveworks_result ToAPIResult(HRESULT hr) {
		if(SUCCEEDED(hr)) {
			return gfsdk_waveworks_result_OK;
		}
		else {
			return gfsdk_waveworks_result_FAIL;
		}
	}

	void diagnostic_message(const char_type *fmt, ...);

	enum { MaxNumGPUs = 4 };
}

struct GFSDK_WaveWorks_Detailed_Simulation_Params
{
    // The simulation params for one of the frequency cascades
    struct Cascade
    {
        // Dimension of displacement texture (and, therefore, of the corresponding FFT step)
        int fft_resolution;

        // The repeat interval for the fft simulation, in world units
        float fft_period;

        // Simulation properties
        float time_scale;
        float wave_amplitude;
        gfsdk_float2 wind_dir;
        float wind_speed;
        float wind_dependency;
        float choppy_scale;
        float small_wave_fraction;

        // Should this cascade's displacement data be read back to the CPU?
        bool readback_displacements;

		// How big to make the readback FIFO?
		gfsdk_U32 num_readback_FIFO_entries;

		// Window params for setting up this cascade's spectrum, measured in pixels from DC
		float window_in;
		float window_out;

		// the foam related parameters are per-cascade as these might require per-cascade tweaking inside the lib

		// the factor characterizing critical wave amplitude/shape/energy to start generating foam
		float foam_generation_threshold;
		// the amount of foam generated in such areas on each simulation step  
		float foam_generation_amount;
		// the speed of foam spatial dissipation
		float foam_dissipation_speed;
		// the speed of foam dissipation over time
		float foam_falloff_speed;

		// whether to allow CUDA timers
		bool enable_CUDA_timers;
    };
	

    // A maximum of 4 cascades is supported - the first cascade (cascades[0]) is taken
    // to be the highest spatial size cascade 
    int num_cascades;
    enum { MaxNumCascades = 4 };
    Cascade cascades[MaxNumCascades];

    // The overall time scale for the simulation (FFT)
    float time_scale;

    // anisotropic degree for sampling of gradient maps
    int aniso_level;

	// # of GPUS (needed for foam simulation)
	int num_GPUs;

    nv_water_simulation_api simulation_api;

	GFSDK_WaveWorks_Simulation_CPU_Threading_Model CPU_simulation_threading_model;

	bool use_texture_arrays;

	bool enable_gfx_timers;

	bool enable_CPU_timers;
};

extern GFSDK_WAVEWORKS_MALLOC NVSDK_malloc;
extern GFSDK_WAVEWORKS_FREE NVSDK_free;
extern GFSDK_WAVEWORKS_ALIGNED_MALLOC NVSDK_aligned_malloc;
extern GFSDK_WAVEWORKS_ALIGNED_FREE NVSDK_aligned_free;
#ifdef TARGET_PLATFORM_PS4
extern GFSDK_WAVEWORKS_ALIGNED_MALLOC NVSDK_garlic_malloc;
extern GFSDK_WAVEWORKS_ALIGNED_FREE NVSDK_garlic_free;
#endif

// OpenGL related constants and structs
extern GFSDK_WAVEWORKS_GLFunctions		  NVSDK_GLFunctions;
#define GL_HALF_FLOAT                     0x140B
#define GL_FRAMEBUFFER                    0x8D40
#define GL_READ_FRAMEBUFFER               0x8CA8 
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_TEXTURE0                       0x84C0
#define GL_RGBA16F                        0x881A
#define GL_RGBA32F                        0x8814
#define GL_RGBA                           0x1908
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_R32F                           0x822E
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_TESS_EVALUATION_SHADER         0x8E87
#define GL_TESS_CONTROL_SHADER            0x8E88
#define GL_GEOMETRY_SHADER                0x8DD9
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_PATCHES                        0x000E
#define GL_PATCH_VERTICES                 0x8E72
#define GL_PIXEL_UNPACK_BUFFER            0x88EC
#define GL_STREAM_DRAW                    0x88E0
#define GL_WRITE_ONLY                     0x88B9
#define GL_READ_WRITE                     0x88BA
#define GL_TIMESTAMP					  0x8E28
#define GL_QUERY_RESULT_AVAILABLE		  0x8867
#define GL_QUERY_RESULT					  0x8866
#define GL_ACTIVE_ATTRIBUTES              0x8B89
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_RED                            0x1903
#define GL_TRUE                           1
#define GL_FALSE                          0
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_LINEAR_MIPMAP_LINEAR           0x2703
#define GL_FLOAT                          0x1406
#define GL_TEXTURE_2D                     0x0DE1
#define GL_TEXTURE_2D_ARRAY               0x8C1A
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_CULL_FACE                      0x0B44
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_REPEAT                         0x2901
#define GL_DEPTH_TEST                     0x0B71
#define GL_STENCIL_TEST                   0x0B90
#define GL_BLEND                          0x0BE2
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_UNSIGNED_INT                   0x1405
#define GL_VIEWPORT                       0x0BA2
#define GL_MAP_WRITE_BIT                  0x0002
#define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
#define GL_MAP_UNSYNCHRONIZED_BIT         0x0020


#if WAVEWORKS_ENABLE_D3D9
#define D3D9_ONLY(x) x
#else
#define D3D9_ONLY(x)
#endif

#if WAVEWORKS_ENABLE_D3D10
#define D3D10_ONLY(x) x
#else
#define D3D10_ONLY(x)
#endif

#if WAVEWORKS_ENABLE_D3D11
#define D3D11_ONLY(x) x
#else
#define D3D11_ONLY(x)
#endif

#if WAVEWORKS_ENABLE_GNM
#define GNM_ONLY(x) x
#else
#define GNM_ONLY(x)
#endif

#if WAVEWORKS_ENABLE_GL
#define GL_ONLY(x) x
#else
#define GL_ONLY(x)
#endif

#if WAVEWORKS_ENABLE_GRAPHICS
#define GFX_ONLY(x) x
#else
#define GFX_ONLY(x)
#endif

#if defined(TARGET_PLATFORM_WINDOWS)
#define WIN_ONLY(x) x
#else
#define WIN_ONLY(x)
#endif

#endif	// _NVWAVEWORKS_INTERNAL_H
