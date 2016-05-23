# Find AntTweakBar

include(FindPackageHandleStandardArgs)

if (CMAKE_CL_64)
    set(ATBARCH "64")
else()
    set(ATBARCH "")
endif()

find_path(ATB_PATH AntTweakBar.h
	HINTS ${GW_DEPS_ROOT}/AntTweakBar
	)
   
MESSAGE("ATB SDK ${ATB_PATH}")
   
find_library(ATB_LIBRARY_RELEASE 
    NAMES AntTweakBar${ATBARCH}
    PATHS ${ATB_PATH})

# find_library(ATB_LIBRARY_DEBUG
    # NAMES AntTweakBar${ATBARCH}
    # PATHS ${ATB_PATH}/debug)
    
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ATB 
    DEFAULT_MSG 
    ATB_LIBRARY_RELEASE 
    ATB_PATH)
    
if(ATB_FOUND)
    mark_as_advanced(ATB_PATH ATB_LIBRARY_RELEASE ATB_LIBRARY_DEBUG)
    set(ATB_INCLUDE_DIRS
        ${ATB_PATH}
        CACHE STRING "")
    
    set(ATB_LIBRARIES 
        optimized ${ATB_LIBRARY_RELEASE}
        debug ${ATB_LIBRARY_RELEASE}
        CACHE STRING "")
endif()
