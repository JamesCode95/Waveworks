#
# Build sample_d3d11
#

SET(GW_DEPS_ROOT $ENV{GW_DEPS_ROOT})

FIND_PACKAGE(DXUT REQUIRED)
FIND_PACKAGE(FX11 REQUIRED)
FIND_PACKAGE(DirectXTK REQUIRED)

SET(SAMP_SOURCE_DIR ${PROJECT_SOURCE_DIR}/sample/d3d11)
SET(SHARED_CS_DIR ${PROJECT_SOURCE_DIR}/test/client-server)
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
		WIN32;_WINDOWS

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
	${SAMP_SOURCE_DIR}/distance_field.cpp
	${SAMP_SOURCE_DIR}/distance_field.h
	
	${SAMP_SOURCE_DIR}/ocean_surface.cpp
	${SAMP_SOURCE_DIR}/ocean_surface.h
	
	${SAMP_SOURCE_DIR}/sample_d3d11.cpp
	
	${SAMP_SOURCE_DIR}/terrain.cpp
	${SAMP_SOURCE_DIR}/terrain.h
	
	${SAMP_SOURCE_DIR}/util.cpp
)

SET(FX_FILES
	${SAMP_SOURCE_DIR}/ocean_surface.fx
	${SAMP_SOURCE_DIR}/sample.fx
)

INCLUDE(cmake/CompileFXToFXO.cmake)

#ADD_CUSTOM_TARGET(fx ALL)

ADD_CUSTOM_TARGET(samp_d3d11fx ALL)

#FUNCTION(CompileFXToFXO FILE OUTPUT_FILE TARGET INCLUDE_DIR OPTIONS)
CompileFXToFXO(${SAMP_SOURCE_DIR}/ocean_surface.fx ${PROJECT_SOURCE_DIR}/media/sample/ocean_surface_d3d11.fxo samp_d3d11fx ${TL_INCLUDE_DIR} /O3 /Tfx_5_0)
CompileFXToFXO(${SAMP_SOURCE_DIR}/sample.fx ${PROJECT_SOURCE_DIR}/media/sample/sample_d3d11.fxo samp_d3d11fx ${TL_INCLUDE_DIR} /O3 /Tfx_5_0)

ADD_EXECUTABLE(SampleD3D11 WIN32
	${WW_PLATFORM_SRC_FILES}
	
	${APP_FILES}
	${SHARED_CS_FILES}
	${FX_FILES}

)

SOURCE_GROUP("app" FILES ${APP_FILES})
SOURCE_GROUP("fx" FILES ${FX_FILES})
#SOURCE_GROUP("header" FILES ${H_FILES})
#SOURCE_GROUP("hlsl" FILES ${HLSL_FILES})
#SOURCE_GROUP("cuda" FILES ${CUDA_FILES})



# Target specific compile options


TARGET_INCLUDE_DIRECTORIES(SampleD3D11 
	PRIVATE ${WW_PLATFORM_INCLUDES}

	PRIVATE ${TL_INCLUDE_DIR}
	PRIVATE ${SHADER_CS_DIR}
	PRIVATE ${SHARED_CS_DIR}
	
	PRIVATE ${DXUT_INCLUDE_DIRS}
	PRIVATE ${FX11_INCLUDE_DIRS}
	PRIVATE ${DXTK_INCLUDE_DIRS}
)

TARGET_COMPILE_DEFINITIONS(SampleD3D11 

	# Common to all configurations
	PRIVATE ${WW_COMPILE_DEFS}
)



#TODO: Link flags

IF(TARGET_BUILD_PLATFORM STREQUAL "Windows")
	# Add linked libraries
	TARGET_LINK_LIBRARIES(SampleD3D11 PUBLIC WaveWorks Ws2_32.lib comctl32.lib Usp10.lib ${CUDA_LIBRARIES} ${DirectX_DXGUID_LIBRARY} ${DirectX_D3D11_LIBRARY} ${DXUT_LIBRARIES} ${FX11_LIBRARIES} ${DXTK_LIBRARIES})

	IF(CMAKE_CL_64)
		SET(LIBPATH_SUFFIX "win64")
	ELSE(CMAKE_CL_64)
		SET(LIBPATH_SUFFIX "Win32")
	ENDIF(CMAKE_CL_64)				

	SET_TARGET_PROPERTIES(SampleD3D11 PROPERTIES 
		LINK_FLAGS_DEBUG "/MAP /DEBUG"
		LINK_FLAGS_RELEASE "/MAP /INCREMENTAL:NO"
	)

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "PS4")
#	TARGET_LINK_LIBRARIES(SampleD3D11 PUBLIC LowLevel LowLevelAABB LowLevelCloth LowLevelDynamics LowLevelParticles SampleD3D11Common PxFoundation PxPvdSDK PxTask SceneQuery SimulationController)

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "XBoxOne")

#	TARGET_LINK_LIBRARIES(SampleD3D11 PUBLIC LowLevel LowLevelAABB LowLevelCloth LowLevelDynamics LowLevelParticles SampleD3D11Common PxFoundation PxPvdSDK PxTask SceneQuery SimulationController)

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "Unix")
ENDIF()


