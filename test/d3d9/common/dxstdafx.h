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
 
 //--------------------------------------------------------------------------------------
// File: DxStdAfx.h
//
// Desc: Standard includes and precompiled headers for DXUT
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#ifndef DXSDK_STDAFX_H
#define DXSDK_STDAFX_H

#ifndef STRICT
#define STRICT
#endif

// If app hasn't choosen, set to work with Windows XP and beyond
#ifndef WINVER
#define WINVER         0x0501
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0501 
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT   0x0501 
#endif

// #define DXUT_AUTOLIB to automatically include the libs needed for DXUT 
#ifdef DXUT_AUTOLIB
#pragma comment( lib, "dxerr.lib" )
#pragma comment( lib, "dxguid.lib" )
#if defined(DEBUG) || defined(_DEBUG)
#pragma comment( lib, "d3dx9d.lib" )
#else
#pragma comment( lib, "d3dx9.lib" )
#endif
#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "comctl32.lib" )
#endif

#pragma warning( disable : 4100 ) // disable unreference formal parameter warnings for /W4 builds
#pragma warning( disable : 4530 ) // disable warning "C++ exception handler used, but unwind semantics are not enabled."

#include <windows.h>
#include <assert.h>
#include <wchar.h>
#include <mmsystem.h>
#include <commctrl.h> // for InitCommonControls() 
#include <shellapi.h> // for ExtractIcon()
#include <new.h>      // for placement new
#include <math.h>      
#include <limits.h>      
#include <stdio.h>
// Not needed - #include <omp.h>
#include <XInput.h> // Header for XInput APIs

// CRT's memory leak detection
#if defined(DEBUG) || defined(_DEBUG)
#include <crtdbg.h>
#endif

// Enable extra D3D debugging in debug builds if using the debug DirectX runtime.  
// This makes D3D objects work well in the debugger watch window, but slows down 
// performance slightly.
#if defined(DEBUG) || defined(_DEBUG)
#ifndef D3D_DEBUG_INFO
#define D3D_DEBUG_INFO
#endif
#endif

// Direct3D includes
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>

// DirectSound includes
#include <mmsystem.h>
#include <mmreg.h>
#include <dsound.h>

// STD
#include <vector>
using namespace std;

// strsafe.h deprecates old unsecure string functions.  If you 
// really do not want to it to (not recommended), then uncomment the next line 
//#define STRSAFE_NO_DEPRECATE

#ifndef STRSAFE_NO_DEPRECATE
#pragma deprecated("strncpy")
#pragma deprecated("wcsncpy")
#pragma deprecated("_tcsncpy")
#pragma deprecated("wcsncat")
#pragma deprecated("strncat")
#pragma deprecated("_tcsncat")
#endif

#pragma warning( disable : 4995 ) // disable deprecated warning 
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 ) 

#include "DXUT.h"
#include "DXUTmisc.h"
#include "DXUTenum.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "DXUTRes.h"

#if defined(DEBUG) || defined(_DEBUG)
	#ifndef V
		#define V(x)           { hr = x; if( FAILED(hr) ) { DXUTTrace( __FILEW__, (DWORD)__LINE__, hr, L#x, true ); } }
	#endif
	#ifndef V_RETURN
		#define V_RETURN(x)    { hr = x; if( FAILED(hr) ) { return DXUTTrace( __FILEW__, (DWORD)__LINE__, hr, L#x, true ); } }
	#endif
#else
	#ifndef V
		#define V(x)           { hr = x; }
	#endif
	#ifndef V_RETURN
		#define V_RETURN(x)    { hr = x; if( FAILED(hr) ) { return hr; } }
	#endif
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

#define ENABLE_PIX_EVENT	1

#if ENABLE_PIX_EVENT
	#define PIX_BEGIN_EVENT(event_name)	D3DPERF_BeginEvent(0, TEXT(event_name))
	#define PIX_END_EVENT(event_name)	D3DPERF_EndEvent()
	#define PIX_SET_MARKER(marker_name)	D3DPERF_SetMarker(0, TEXT(marker_name))
	#define PIX_IS_RUNNING				D3DPERF_GetStatus()
#else
	#define PIX_BEGIN_EVENT(event_name)
	#define PIX_END_EVENT(event_name)
	#define PIX_SET_MARKER(marker_name)
	#define PIX_IS_RUNNING				0
#endif

#endif // !defined(DXSDK_STDAFX_H)
