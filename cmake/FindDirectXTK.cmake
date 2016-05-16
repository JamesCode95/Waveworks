# Find DirectXTK

include(FindPackageHandleStandardArgs)

if (CMAKE_CL_64)
    set(DXTKARCH "x64")
else()
    set(DXTKARCH "Win32")
endif()

find_path(DXTK_SDK_PATH DirectXHelpers.h
	HINTS ${GW_DEPS_ROOT}/DirectXTK/Inc
	)
    
find_library(DXTK_LIBRARY_RELEASE 
    NAMES DirectXTK
    PATHS ${DXTK_SDK_PATH}/Bin/*/${DXTKARCH}/Release)

find_library(DXTK_LIBRARY_DEBUG
    NAMES DirectXTK
    PATHS ${DXTK_SDK_PATH}/Bin/*/${DXTKARCH}/Debug)
    
    
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DXTK 
    DEFAULT_MSG 
    DXTK_LIBRARY_RELEASE 
    DXTK_SDK_PATH)
    
if(DXTK_FOUND)
    mark_as_advanced(DXTK_SDK_PATH DXTK_LIBRARY_RELEASE DXTK_LIBRARY_DEBUG)
    set(DXTK_INCLUDE_DIRS
        ${DXTK_SDK_PATH}/inc
        CACHE STRING "")
    
    set(DXTK_LIBRARIES 
        optimized ${DXTK_LIBRARY_RELEASE}
        debug ${DXTK_LIBRARY_DEBUG}
        CACHE STRING "")
endif()