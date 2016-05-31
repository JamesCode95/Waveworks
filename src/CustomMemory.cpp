#include "Internal.h"
#include <cstdlib>

#if defined(TARGET_PLATFORM_LINUX)
#include <malloc.h>
#endif
#include "InternalLogger.h"

#if (defined(TARGET_PLATFORM_MACOSX) || defined(TARGET_PLATFORM_ANDROID))
#define _THROW0()
#endif

#if defined(TARGET_PLATFORM_NIXLIKE)
#if defined(TARGET_PLATFORM_MACOSX)
namespace
{
    void* default_aligned_malloc(size_t size, size_t alignment)
    {
        void* aPtr;
        if (posix_memalign (&aPtr, alignment, size))
        {
            aPtr = NULL;
        }
        return aPtr;
    }
}
#else
namespace
{
    void* default_aligned_malloc(size_t size, size_t alignment)
    {
        return memalign(alignment, size);
    }
}
#endif
GFSDK_WAVEWORKS_ALIGNED_MALLOC			NVSDK_aligned_malloc    = default_aligned_malloc;
GFSDK_WAVEWORKS_ALIGNED_FREE			NVSDK_aligned_free		= free;
#else
GFSDK_WAVEWORKS_ALIGNED_MALLOC			NVSDK_aligned_malloc	= _aligned_malloc;
GFSDK_WAVEWORKS_ALIGNED_FREE			NVSDK_aligned_free		= _aligned_free;
#endif

#if defined(TARGET_PLATFORM_PS4)
namespace
{
	void* malloc_wrapper(size_t size)
	{
		return NVSDK_aligned_malloc(size, 8);
	}

	void free_wrapper(void* ptr)
	{
		NVSDK_aligned_free(ptr);
	}
}

GFSDK_WAVEWORKS_MALLOC					NVSDK_malloc			= malloc_wrapper; // never changes
GFSDK_WAVEWORKS_FREE					NVSDK_free				= free_wrapper; // never changes
GFSDK_WAVEWORKS_ALIGNED_MALLOC			NVSDK_garlic_malloc     = default_aligned_malloc;
GFSDK_WAVEWORKS_ALIGNED_FREE			NVSDK_garlic_free		= free;
#define DELETE_THROWSPEC() _THROW0()
#else
GFSDK_WAVEWORKS_MALLOC					NVSDK_malloc			= malloc;
GFSDK_WAVEWORKS_FREE					NVSDK_free				= free;
#define DELETE_THROWSPEC() _THROW0()
#endif

void* internalMalloc( size_t size )
{
    void* p = NVSDK_malloc( size );
	if (!p)
		NV_ERROR(TEXT("WaveWorks: MEMORY ALLOCATION ERROR. Check memory allocation callback pointer\n"));
//        diagnostic_message( TEXT("WaveWorks: MEMORY ALLOCATION ERROR. Check memory allocation callback pointer\n") );
    return p;
}

void internalFree( void* p )
{
    NVSDK_free( p );
}


void* __CRTDECL operator new(size_t size)
{
	return internalMalloc( size );
}
void __CRTDECL operator delete(void *p) DELETE_THROWSPEC()
{
	internalFree( p );
}

void * __CRTDECL operator new[](size_t size)
{
	return internalMalloc( size );
}

void __CRTDECL operator delete[](void *p) DELETE_THROWSPEC()
{
	internalFree( p );
}

void resetMemoryManagementCallbacksToDefaults()
{
#if defined(TARGET_PLATFORM_PS4)
	NVSDK_aligned_malloc	= default_aligned_malloc;
	NVSDK_aligned_free		= free;
	NVSDK_garlic_malloc		= default_aligned_malloc;
	NVSDK_garlic_free		= free;
#elif defined(TARGET_PLATFORM_NIXLIKE)
	NVSDK_malloc			= malloc;
	NVSDK_free				= free;
	NVSDK_aligned_malloc	= default_aligned_malloc;
	NVSDK_aligned_free		= free;
#else
	NVSDK_malloc			= malloc;
	NVSDK_free				= free;
	NVSDK_aligned_malloc	= _aligned_malloc;
	NVSDK_aligned_free		= _aligned_free;
#endif
}


