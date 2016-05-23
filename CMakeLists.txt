cmake_minimum_required(VERSION 3.3)

project(WaveWorks CXX)

CMAKE_POLICY(SET CMP0057 NEW) # Enable IN_LIST

SET(GW_DEPS_ROOT $ENV{GW_DEPS_ROOT})

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
LIST(APPEND CMAKE_MODULE_PATH ${GW_DEPS_ROOT}CMakeModules)

MESSAGE(${CMAKE_MODULE_PATH})



# TODO: Fail if we didn't find deps

IF(CMAKE_CONFIGURATION_TYPES)
	SET(CMAKE_CONFIGURATION_TYPES debug release)
	SET(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
		"Reset config to what we need" 
		FORCE)
	
	SET(CMAKE_SHARED_LINKER_FLAGS_CHECKED "")
	SET(CMAKE_SHARED_LINKER_FLAGS_PROFILE "")
	
ENDIF()

SET(GW_DEPS_ROOT $ENV{GW_DEPS_ROOT})

# Handle different platforms. So far, Windows, PS4, XBone, Linux... More?
IF(NOT DEFINED TARGET_BUILD_PLATFORM) # Not defined, default to Windows
	SET(TARGET_BUILD_PLATFORM "Windows")
ENDIF()

SET(PLATFORM_LIST Windows PS4 XBoxOne OSX Android)

IF (NOT ${TARGET_BUILD_PLATFORM} IN_LIST PLATFORM_LIST)
	MESSAGE(FATAL_ERROR "Invalid platform:" ${TARGET_BUILD_PLATFORM})
ENDIF()

IF(TARGET_BUILD_PLATFORM STREQUAL "Windows")

	#/GS /Gm /Od /MTd  
	#/GS- /Gy /Gm- /Ox /Ob2 /GF /Oy /MT /Ot 

	SET(CMAKE_CXX_FLAGS "/W4 /Zc:wchar_t /Zi /errorReport:prompt /Zc:forScope /fp:fast /WX- /GR- /Gd /openmp- /nologo")
		
	SET(CMAKE_CXX_FLAGS_DEBUG "/GS /Gm /Od /MTd")
	SET(CMAKE_CXX_FLAGS_RELEASE "/GS- /Gy /Gm- /Ox /Ob2 /GF /Oy /MT /Ot")
	
	IF(CMAKE_CL_64)
		ADD_DEFINITIONS(-DWIN64)
	ENDIF(CMAKE_CL_64)			

	ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

	
ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "PS4")
	SET(CMAKE_CXX_FLAGS "-fdiagnostics-show-option -fno-rtti -fno-exceptions -ffast-math -ffunction-sections -fdata-sections -Werror -ferror-limit=0 -Wall -Wextra -fstrict-aliasing -Wstrict-aliasing=2 -pedantic -Weverything -Wno-documentation-deprecated-sync -Wno-documentation-unknown-command -Wno-float-equal -Wno-padded -Wno-weak-vtables -Wno-cast-align -Wno-conversion -Wno-missing-noreturn -Wno-missing-variable-declarations -Wno-shift-sign-overflow -Wno-covered-switch-default -Wno-exit-time-destructors -Wno-global-constructors -Wno-missing-prototypes -Wno-unreachable-code -Wno-unused-macros -Wno-unused-member-function -Wno-used-but-marked-unused -Wno-weak-template-vtables -Wno-deprecated")
		
	SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
	SET(CMAKE_CXX_FLAGS_CHECKED "-O2")
	SET(CMAKE_CXX_FLAGS_PROFILE "-O2")
	SET(CMAKE_CXX_FLAGS_RELEASE "-O2")

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "XBoxOne")
	SET(CMAKE_CXX_FLAGS "/Wall /wd4512 /wd4324 /wd4127 /wd4505 /wd4100 /wd4714 /WX /W4 /GF /GR- /Gd /fp:fast /Zp16 /favor:AMD64 /MDd")
	
	SET(CMAKE_CXX_FLAGS_DEBUG "/Od")
	SET(CMAKE_CXX_FLAGS_CHECKED "/Ox")
	SET(CMAKE_CXX_FLAGS_PROFILE "/Ox /Oi /Zi")
	SET(CMAKE_CXX_FLAGS_RELEASE "/0x /Oi")

	SET(CMAKE_SHARED_LINKER_FLAGS "/IGNORE:4221")


ENDIF()


# Default to appending "DEBUG", "PROFILE", etc to produced artifacts
IF(NOT DEFINED APPEND_CONFIG_NAME)
	SET(APPEND_CONFIG_NAME ON)
ENDIF()

IF (APPEND_CONFIG_NAME)
	MESSAGE("Appending config to output names")

	SET(CMAKE_DEBUG_POSTFIX "_debug")
	SET(CMAKE_RELEASE_POSTFIX "")
ENDIF()

IF (TARGET_BUILD_PLATFORM STREQUAL "Windows")
	IF(CMAKE_CL_64)
		SET(LIBPATH_SUFFIX "win64")
	ELSE(CMAKE_CL_64)
		SET(LIBPATH_SUFFIX "Win32")
	ENDIF(CMAKE_CL_64)				

	SET(CMAKE_DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}.${LIBPATH_SUFFIX}")
	SET(CMAKE_RELEASE_POSTFIX "${CMAKE_RELEASE_POSTFIX}.${LIBPATH_SUFFIX}")
ENDIF()


IF(NOT DEFINED WW_OUTPUT_DIR)
	# If there's no output dir set, default to something somewhat sensible.
	SET(WW_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/Lib/cmake/)
ENDIF()

# Only set this once, to avoid cascading sets.
IF(NOT DEFINED FINAL_OUTPUT_DIR)
	# We assume that WW_OUTPUT_DIR is a RELATIVE path.
	SET(FINAL_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/${WW_OUTPUT_DIR})
ENDIF()

MESSAGE("Final Output Directory: " ${FINAL_OUTPUT_DIR})

# Override the default output directories for all configurations.
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${FINAL_OUTPUT_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${FINAL_OUTPUT_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${FINAL_OUTPUT_DIR})

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${FINAL_OUTPUT_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${FINAL_OUTPUT_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${FINAL_OUTPUT_DIR})

# Include all of the projects
INCLUDE(compiler/cmake/NvWaveWorks.cmake)
INCLUDE(compiler/cmake/test_d3d11.cmake)
INCLUDE(compiler/cmake/sample_d3d11.cmake)
INCLUDE(compiler/cmake/sample_opengl.cmake)
