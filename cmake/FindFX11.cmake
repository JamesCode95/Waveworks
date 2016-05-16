# Find FX11

include(FindPackageHandleStandardArgs)

if (CMAKE_CL_64)
    set(FX11ARCH "x64")
else()
    set(FX11ARCH "Win32")
endif()

find_path(FX11_SDK_PATH Effect.h
	HINTS ${GW_DEPS_ROOT}/FX11
	)
    
find_library(FX11_LIBRARY_RELEASE 
    NAMES Effects11
    PATHS ${FX11_SDK_PATH}/Bin/*/${FX11ARCH}/Release)

find_library(FX11_LIBRARY_DEBUG
    NAMES Effects11
    PATHS ${FX11_SDK_PATH}/Bin/*/${FX11ARCH}/Debug)
    
    
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FX11 
    DEFAULT_MSG 
    FX11_LIBRARY_RELEASE 
    FX11_SDK_PATH)
    
if(FX11_FOUND)
    mark_as_advanced(FX11_SDK_PATH FX11_LIBRARY_RELEASE FX11_LIBRARY_DEBUG)
    set(FX11_INCLUDE_DIRS
        ${FX11_SDK_PATH}
        ${FX11_SDK_PATH}/inc
        CACHE STRING "")
    
    set(FX11_LIBRARIES 
        optimized ${FX11_LIBRARY_RELEASE}
        debug ${FX11_LIBRARY_DEBUG}
        CACHE STRING "")
endif()