# Find DXUT

include(FindPackageHandleStandardArgs)

if (CMAKE_CL_64)
    set(DXUTARCH "x64")
else()
    set(DXUTARCH "Win32")
endif()

find_path(DXUT_SDK_PATH Core/DXUT.h
	HINTS ${GW_DEPS_ROOT}/DXUT
	)
   
# Need to be smart and get the proper VS library here

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 18.0.0.0 AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0.0.0)
	SET(VS_STR "Desktop_2013")
elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 19.0.0.0)
	SET(VS_STR "Desktop_2015")
else()
	MESSAGE(FATAL_ERROR "Failed to find compatible DXUT - Only supporting VS2013 and VS2015")
endif()
   
find_library(DXUT_LIBRARY_RELEASE 
    NAMES DXUT
    PATHS ${DXUT_SDK_PATH}/Core/Bin/${VS_STR}/${DXUTARCH}/Release)

find_library(DXUT_LIBRARY_DEBUG
    NAMES DXUT
    PATHS ${DXUT_SDK_PATH}/Core/Bin/${VS_STR}/${DXUTARCH}/Debug)
    
find_library(DXUT_Opt_LIBRARY_RELEASE
    NAMES DXUTOpt
    PATHS ${DXUT_SDK_PATH}/Optional/Bin/${VS_STR}/${DXUTARCH}/Release)

find_library(DXUT_Opt_LIBRARY_DEBUG
    NAMES DXUTOpt
    PATHS ${DXUT_SDK_PATH}/Optional/Bin/${VS_STR}/${DXUTARCH}/Debug)
    
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DXUT 
    DEFAULT_MSG 
    DXUT_LIBRARY_RELEASE 
    DXUT_Opt_LIBRARY_RELEASE 
    DXUT_SDK_PATH)
    
if(DXUT_FOUND)
    mark_as_advanced(DXUT_SDK_PATH DXUT_LIBRARY_RELEASE DXUT_LIBRARY_DEBUG DXUT_Opt_LIBRARY_RELEASE DXUT_Opt_LIBRARY_DEBUG)
    set(DXUT_INCLUDE_DIRS
        ${DXUT_SDK_PATH}/Core
        ${DXUT_SDK_PATH}/Optional
        CACHE STRING "")
    
    set(DXUT_LIBRARIES 
        optimized ${DXUT_LIBRARY_RELEASE} optimized ${DXUT_Opt_LIBRARY_RELEASE} 
        debug ${DXUT_LIBRARY_DEBUG} debug ${DXUT_Opt_LIBRARY_DEBUG} 
        CACHE STRING "")
endif()