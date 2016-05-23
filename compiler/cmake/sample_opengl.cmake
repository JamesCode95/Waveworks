#
# Build sample_opengl
#

SET(GW_DEPS_ROOT $ENV{GW_DEPS_ROOT})

FIND_PACKAGE(AntTweakBar REQUIRED)

SET(SAMP_SOURCE_DIR ${PROJECT_SOURCE_DIR}/sample/opengl)
SET(TL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

IF(TARGET_BUILD_PLATFORM STREQUAL "Windows")

	FIND_PACKAGE(DirectX REQUIRED)

	SET(WW_PLATFORM_INCLUDES
	)
	
	SET(WW_PLATFORM_SRC_FILES
	)
	

	# Use generator expressions to set config specific preprocessor definitions
	SET(WW_COMPILE_DEFS
		# Common to all configurations
		WIN32;_WINDOWS;_CONSOLE;
		
		$<$<CONFIG:debug>:PROFILE;>
		$<$<CONFIG:release>:NDEBUG;>
	)

	SET(WW_LIBTYPE SHARED)
	
ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "PS4")

	SET(WW_PLATFORM_INCLUDES
		$ENV{SCE_ORBIS_SDK_DIR}/target/include
	)

	SET(WW_COMPILE_DEFS
	
		# Common to all configurations
		_LIB;_CRT_SECURE_NO_DEPRECATE;_CRT_NONSTDC_NO_DEPRECATE;PX_PHYSX_STATIC_LIB;

		$<$<CONFIG:debug>:_DEBUG;PX_DEBUG=1;PX_CHECKED=1;PX_SUPPORT_PVD=1;>
		$<$<CONFIG:release>:NDEBUG;PX_SUPPORT_PVD=0;>
	)

	SET(WW_LIBTYPE STATIC)

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "XBoxOne")
	SET(WW_PLATFORM_INCLUDES
	)

	# Use generator expressions to set config specific preprocessor definitions
	SET(WW_COMPILE_DEFS 
	
		# Common to all configurations
		PX_PHYSX_CORE_EXPORTS;_LIB;_CRT_SECURE_NO_DEPRECATE;_CRT_NONSTDC_NO_DEPRECATE;WINAPI_FAMILY=WINAPI_FAMILY_TV_TITLE;PX_PHYSX_STATIC_LIB;
		
		$<$<CONFIG:debug>:_DEBUG;PX_DEBUG=1;PX_CHECKED=1;PX_SUPPORT_PVD=1;>
		$<$<CONFIG:release>:NDEBUG;PX_SUPPORT_PVD=0;>
	)
	
	SET(WW_LIBTYPE STATIC)
	

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "Unix")
ENDIF()

SET(APP_FILES
	${SAMP_SOURCE_DIR}/math_code.cpp
	${SAMP_SOURCE_DIR}/math_code.h
	
	${SAMP_SOURCE_DIR}/ocean_surface.cpp
	${SAMP_SOURCE_DIR}/ocean_surface.h
	
	${SAMP_SOURCE_DIR}/sample_opengl.cpp
	
	${SAMP_SOURCE_DIR}/water.glsl
)


ADD_EXECUTABLE(SampleOpenGL WIN32
	${WW_PLATFORM_SRC_FILES}
	
	${APP_FILES}

)

SOURCE_GROUP("app" FILES ${APP_FILES})



# Target specific compile options


TARGET_INCLUDE_DIRECTORIES(SampleOpenGL 
	PRIVATE ${WW_PLATFORM_INCLUDES}

	PRIVATE ${ATB_INCLUDE_DIRS}
	PRIVATE ${TL_INCLUDE_DIR}
)

TARGET_COMPILE_DEFINITIONS(SampleOpenGL 

	# Common to all configurations
	PRIVATE ${WW_COMPILE_DEFS}
)



#TODO: Link flags

IF(TARGET_BUILD_PLATFORM STREQUAL "Windows")
	# Add linked libraries
	TARGET_LINK_LIBRARIES(SampleOpenGL PUBLIC WaveWorks comctl32.lib opengl32.lib ${ATB_LIBRARIES})

	IF(CMAKE_CL_64)
		SET(LIBPATH_SUFFIX "win64")
	ELSE(CMAKE_CL_64)
		SET(LIBPATH_SUFFIX "Win32")
	ENDIF(CMAKE_CL_64)				

	SET_TARGET_PROPERTIES(SampleOpenGL PROPERTIES 
		LINK_FLAGS_DEBUG "/MAP /DEBUG /SUBSYSTEM:CONSOLE"
		LINK_FLAGS_RELEASE "/MAP /INCREMENTAL:NO /SUBSYSTEM:CONSOLE"
	)

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "PS4")
#	TARGET_LINK_LIBRARIES(SampleOpenGL PUBLIC LowLevel LowLevelAABB LowLevelCloth LowLevelDynamics LowLevelParticles SampleD3D11Common PxFoundation PxPvdSDK PxTask SceneQuery SimulationController)

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "XBoxOne")

#	TARGET_LINK_LIBRARIES(SampleOpenGL PUBLIC LowLevel LowLevelAABB LowLevelCloth LowLevelDynamics LowLevelParticles SampleD3D11Common PxFoundation PxPvdSDK PxTask SceneQuery SimulationController)

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "Unix")
ENDIF()


