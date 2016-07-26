# Find DirectXTK

include(FindPackageHandleStandardArgs)

if (CMAKE_CL_64)
    set(DXTKARCH "x64")
else()
    set(DXTKARCH "Win32")
endif()

find_path(DXTK_SDK_PATH Inc/DirectXHelpers.h
	HINTS ${GW_DEPS_ROOT}/DirectXTK/${DirectXTK_FIND_VERSION}
	)
   
MESSAGE("DXTK SDK ${DXTK_SDK_PATH}")

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 18.0.0.0 AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0.0.0)
	SET(VS_STR "Desktop_2013")
elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 19.0.0.0)
	SET(VS_STR "Desktop_2015")
else()
	MESSAGE(FATAL_ERROR "Failed to find compatible FX11 - Only supporting VS2013 and VS2015")
endif()
   
find_library(DXTK_LIBRARY_RELEASE 
    NAMES DirectXTK
    PATHS ${DXTK_SDK_PATH}/Bin/${VS_STR}/${DXTKARCH}/Release)

find_library(DXTK_LIBRARY_DEBUG
    NAMES DirectXTK
    PATHS ${DXTK_SDK_PATH}/Bin/${VS_STR}/${DXTKARCH}/Debug)
    
    
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