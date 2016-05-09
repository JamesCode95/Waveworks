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

#ifndef _THREADWRAP_H
#define _THREADWRAP_H

#ifdef TARGET_PLATFORM_NIXLIKE
#include <pthread.h>
#include <string.h>
#include <assert.h>

typedef size_t FAKE_SIZE_T;
typedef unsigned int FAKE_DWORD;
typedef void* FAKE_HANDLE;
typedef int FAKE_BOOL;
typedef const wchar_t* FAKE_LPCWSTR;
typedef const char* FAKE_LPCSTR;
#ifdef _UNICODE
typedef FAKE_LPCWSTR FAKE_LPCTSTR;
#else
typedef FAKE_LPCSTR FAKE_LPCTSTR;
#endif

#define INFINITE            0xFFFFFFFF  // Infinite timeout
#define FAKE_WAIT_OBJECT_0       ((FAKE_DWORD   )0x00000000L) 
#define FAKE_WAIT_TIMEOUT        ((FAKE_DWORD   )0x00000102L)
namespace
{
	typedef pthread_mutex_t CRITICAL_SECTION;
    void InitializeCriticalSection(pthread_mutex_t* mutex)
    {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); // todo: PTHREAD_MUTEX_ADAPTIVE_NP?
		pthread_mutex_init(mutex, &attr);
		pthread_mutexattr_destroy(&attr);
    }
	void InitializeCriticalSectionAndSpinCount(pthread_mutex_t* mutex, FAKE_DWORD spinCount)
	{
        InitializeCriticalSection(mutex);
	}
	void DeleteCriticalSection(pthread_mutex_t* mutex)
	{
		pthread_mutex_destroy(mutex);
	}
	void EnterCriticalSection(pthread_mutex_t* mutex)
	{
		pthread_mutex_lock(mutex);
	}
	void LeaveCriticalSection(pthread_mutex_t* mutex)
	{
		pthread_mutex_unlock( mutex );
	}

	struct Event
	{
		pthread_cond_t cond;
		pthread_mutex_t mutex;
		volatile bool signalled;
		bool manualReset;
	};
	typedef void* LPSECURITY_ATTRIBUTES;
	FAKE_HANDLE CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, FAKE_BOOL bManualReset, FAKE_BOOL bInitialState, FAKE_LPCTSTR lpName)
	{
		Event* event = new Event;
		pthread_cond_init(&event->cond, NULL);
		InitializeCriticalSectionAndSpinCount(&event->mutex, 0);
		event->signalled = bInitialState;
		event->manualReset = bManualReset;
		return event;
	}
	void SetEvent(FAKE_HANDLE handle)
	{
		Event* event = (Event*)handle;
		pthread_mutex_lock(&event->mutex);
		event->signalled = true;
		pthread_mutex_unlock(&event->mutex);
		pthread_cond_signal(&event->cond);
	}
	void ResetEvent(FAKE_HANDLE handle)
	{
		Event* event = (Event*)handle;
		event->signalled = false;
	}
	FAKE_DWORD WaitForSingleObject(FAKE_HANDLE handle, FAKE_DWORD milliseconds)
	{
		Event* event = (Event*)handle;

		if(0 == milliseconds)
		{
			// Simple non-blocking signalled/not-signalled check
			pthread_mutex_lock(&event->mutex);
			const bool was_signalled = event->signalled;
			pthread_mutex_unlock(&event->mutex);
			return was_signalled ? FAKE_WAIT_OBJECT_0 : FAKE_WAIT_TIMEOUT;
		}

		assert(milliseconds == INFINITE);
		pthread_mutex_lock(&event->mutex);
		while(!event->signalled)
			pthread_cond_wait(&event->cond, &event->mutex);
		if(!event->manualReset)
			event->signalled = false;
		pthread_mutex_unlock(&event->mutex);
		return FAKE_WAIT_OBJECT_0;
	}
	void CloseHandle(FAKE_HANDLE handle) // handle needs to point to return value of CreateEvent()!
	{
		Event* event = (Event*)handle;
		pthread_cond_destroy(&event->cond);
		pthread_mutex_destroy(&event->mutex);
		delete event;
	}

	typedef void* (LPTHREAD_START_ROUTINE) (void* lpThreadParameter);
	FAKE_HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, FAKE_SIZE_T dwStackSize, 
		LPTHREAD_START_ROUTINE* lpStartAddress, void* lpParameter, FAKE_DWORD dwCreationFlags, FAKE_DWORD* lpThreadId)
	{
		assert(lpThreadAttributes == NULL && !dwStackSize && !dwCreationFlags);

		pthread_t* thread = new pthread_t;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 1 << 21); // 2 MB
		pthread_create(thread, &attr, lpStartAddress, lpParameter);
		pthread_attr_destroy(&attr);
		lpThreadId = 0;

		return thread;
	}
}
#endif // TARGET_PLATFROM_NIXLIKE

#endif // _THREADWRAP_H

