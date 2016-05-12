#
# Build NvWaveWorks (PROJECT not SOLUTION)
#

SET(GW_DEPS_ROOT $ENV{GW_DEPS_ROOT})

FIND_PACKAGE(CUDA REQUIRED)


SET(WW_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
SET(SHADER_SRC_DIR ${WW_SOURCE_DIR}/shader)
SET(DISTRO_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
SET(GEN_SRC_DIR ${WW_SOURCE_DIR}/generated)

IF(TARGET_BUILD_PLATFORM STREQUAL "Windows")

	FIND_PACKAGE(DirectX REQUIRED)

	SET(WW_PLATFORM_INCLUDES
	)
	
	SET(WW_PLATFORM_SRC_FILES
	)
	

	# Use generator expressions to set config specific preprocessor definitions
	SET(WW_COMPILE_DEFS
		# Common to all configurations
		_LIB;NVWAVEWORKS_LIB_DLL_EXPORTS;WIN32;

		$<$<CONFIG:debug>:PROFILE;_DEV;>
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

SET(CPP_FILES
	${WW_SOURCE_DIR}/CustomMemory.cpp
	${WW_SOURCE_DIR}/D3DX_Replacement_code.cpp
	${WW_SOURCE_DIR}/Entrypoints.cpp
	
	${WW_SOURCE_DIR}/FFT_Simulation_CPU.cpp
	${WW_SOURCE_DIR}/FFT_Simulation_CUDA.cpp
	${WW_SOURCE_DIR}/FFT_Simulation_DirectCompute.cpp
	${WW_SOURCE_DIR}/FFT_Simulation_Manager_CPU.cpp
	${WW_SOURCE_DIR}/FFT_Simulation_Manager_CUDA.cpp
	${WW_SOURCE_DIR}/FFT_Simulation_Manager_DirectCompute.cpp
	
	${WW_SOURCE_DIR}/GFX_Timer.cpp
	${WW_SOURCE_DIR}/GLFunctions.cpp
	${WW_SOURCE_DIR}/Mesh.cpp
	${WW_SOURCE_DIR}/Quadtree.cpp
	${WW_SOURCE_DIR}/Savestate.cpp
	${WW_SOURCE_DIR}/Simulation.cpp
	${WW_SOURCE_DIR}/Simulation_Util.cpp
)

SET(H_FILES
	${WW_SOURCE_DIR}/CustomMemory.h
	${WW_SOURCE_DIR}/D3DX_Replacement_code.h
	${WW_SOURCE_DIR}/FFT_API_Support.h
	${WW_SOURCE_DIR}/FFT_Simulation.h
	${WW_SOURCE_DIR}/FFT_Simulation_CPU_impl.h
	${WW_SOURCE_DIR}/FFT_Simulation_CUDA_impl.h
	${WW_SOURCE_DIR}/FFT_Simulation_DirectCompute_impl.h
	${WW_SOURCE_DIR}/FFT_Simulation_Manager.h
	${WW_SOURCE_DIR}/FFT_Simulation_Manager_CPU_impl.h
	${WW_SOURCE_DIR}/FFT_Simulation_Manager_CUDA_impl.h
	${WW_SOURCE_DIR}/FFT_Simulation_Manager_DirectCompute_impl.h
	${WW_SOURCE_DIR}/Float16_Util.h
	${WW_SOURCE_DIR}/GFX_Timer_impl.h
	${WW_SOURCE_DIR}/Graphics_Context.h
	${WW_SOURCE_DIR}/internal.h
	${WW_SOURCE_DIR}/Mesh.h
	${WW_SOURCE_DIR}/Quadtree_impl.h
	${WW_SOURCE_DIR}/Savestate_impl.h
	${WW_SOURCE_DIR}/Simulation_impl.h
	${WW_SOURCE_DIR}/Simulation_Util.h
	${WW_SOURCE_DIR}/Spectrum_Util.h
)

SET(DISTRO_INCLUDE_FILES
	${DISTRO_INCLUDE_DIR}/GFSDK_WaveWorks.h
	${DISTRO_INCLUDE_DIR}/GFSDK_WaveWorks_Common.h
	${DISTRO_INCLUDE_DIR}/GFSDK_WaveWorks_D3D_Util.h
	${DISTRO_INCLUDE_DIR}/GFSDK_WaveWorks_GUID.h
	${DISTRO_INCLUDE_DIR}/GFSDK_WaveWorks_Types.h
)

SET(HLSL_FILES
	${SHADER_SRC_DIR}/Attributes.fxh
	${SHADER_SRC_DIR}/Common.fxh
	${SHADER_SRC_DIR}/Quadtree.fxh
	
	${SHADER_SRC_DIR}/CalcGradient.fx
	${SHADER_SRC_DIR}/CalcGradient_SM3.fx
	${SHADER_SRC_DIR}/CalcGradient_SM4.fx
	
	${SHADER_SRC_DIR}/FoamGeneration.fx
	${SHADER_SRC_DIR}/FoamGeneration_SM3.fx
	${SHADER_SRC_DIR}/FoamGeneration_SM4.fx
	
	${SHADER_SRC_DIR}/Quadtree_SM4_sig.fx
	${SHADER_SRC_DIR}/Quadtree_SM5_sig.fx
	
	${WW_SOURCE_DIR}/FFT_Simulation_DirectCompute_shader.hlsl
)

SET(CUDA_FILES
	${WW_SOURCE_DIR}/FFT_Simulation_CUDA_kernel.cu
)

SET(FX_FILES
	${SHADER_SRC_DIR}/Quadtree_SM4_sig.fx
	${SHADER_SRC_DIR}/Quadtree_SM5_sig.fx
	${SHADER_SRC_DIR}/FoamGeneration_SM4.fx
	${SHADER_SRC_DIR}/FoamGeneration_SM3.fx
	
	${WW_SOURCE_DIR}/FFT_Simulation_DirectCompute_shader.hlsl
)

SET(GENERATED_HLSL_FILES
	${GEN_SRC_DIR}/Quadtree_SM4_sig.h
	${GEN_SRC_DIR}/Quadtree_SM5_sig.h
	${GEN_SRC_DIR}/FoamGeneration_vs_4_0.h
	${GEN_SRC_DIR}/FoamGeneration_ps_4_0.h
	${GEN_SRC_DIR}/FoamGeneration_vs_3_0.h
	${GEN_SRC_DIR}/FoamGeneration_ps_3_0.h
	${GEN_SRC_DIR}/ComputeH0_cs_5_0.h
	${GEN_SRC_DIR}/ComputeRows_cs_5_0.h
	${GEN_SRC_DIR}/ComputeColumns_cs_5_0.h
)


INCLUDE(cmake/CompileFXToH.cmake)

#ADD_CUSTOM_TARGET(fx ALL)

#FUNCTION(CompileFXToH FILE OUTPUT_FILE TARGET INCLUDE_DIR ENTRYPOINT OPTIONS)

# Compile the .fx files to .h files so they can be loaded easily.
ADD_CUSTOM_TARGET(fx ALL)

CompileFXToH(${SHADER_SRC_DIR}/Quadtree_SM4_sig.fx ${GEN_SRC_DIR}/Quadtree_SM4_sig.h fx ${SHADER_SRC_DIR} ${DISTRO_INCLUDE_DIR} GFSDK_WAVEWORKS_VERTEX_INPUT_Sig /Tvs_4_0)
CompileFXToH(${SHADER_SRC_DIR}/Quadtree_SM5_sig.fx ${GEN_SRC_DIR}/Quadtree_SM5_sig.h fx ${SHADER_SRC_DIR} ${DISTRO_INCLUDE_DIR} GFSDK_WAVEWORKS_VERTEX_INPUT_Sig /Tvs_5_0)

# NOTE: This does a weird thing. Only the PS command invocation will show in VS. The other one is there, but is in some sort of hidden file.
#  It can still be seen in the project xml.
CompileFXToH(${SHADER_SRC_DIR}/FoamGeneration_SM4.fx ${GEN_SRC_DIR}/FoamGeneration_ps_4_0.h fx ${SHADER_SRC_DIR} ${GEN_SRC_DIR} ps /Tps_4_0)
CompileFXToH(${SHADER_SRC_DIR}/FoamGeneration_SM4.fx ${GEN_SRC_DIR}/FoamGeneration_vs_4_0.h fx ${SHADER_SRC_DIR} ${GEN_SRC_DIR} vs /Tvs_4_0)

CompileFXToH(${SHADER_SRC_DIR}/FoamGeneration_SM3.fx ${GEN_SRC_DIR}/FoamGeneration_ps_3_0.h fx ${SHADER_SRC_DIR} ${GEN_SRC_DIR} ps /Tps_3_0)
CompileFXToH(${SHADER_SRC_DIR}/FoamGeneration_SM3.fx ${GEN_SRC_DIR}/FoamGeneration_vs_3_0.h fx ${SHADER_SRC_DIR} ${GEN_SRC_DIR} vs /Tvs_3_0)

CompileFXToH(${SHADER_SRC_DIR}/CalcGradient_SM4.fx ${GEN_SRC_DIR}/CalcGradient_ps_4_0.h fx ${SHADER_SRC_DIR} ${GEN_SRC_DIR} ps /Tps_4_0)
CompileFXToH(${SHADER_SRC_DIR}/CalcGradient_SM4.fx ${GEN_SRC_DIR}/CalcGradient_vs_4_0.h fx ${SHADER_SRC_DIR} ${GEN_SRC_DIR} vs /Tvs_4_0)

CompileFXToH(${SHADER_SRC_DIR}/CalcGradient_SM3.fx ${GEN_SRC_DIR}/CalcGradient_ps_3_0.h fx ${SHADER_SRC_DIR} ${GEN_SRC_DIR} ps /Tps_3_0)
CompileFXToH(${SHADER_SRC_DIR}/CalcGradient_SM3.fx ${GEN_SRC_DIR}/CalcGradient_vs_3_0.h fx ${SHADER_SRC_DIR} ${GEN_SRC_DIR} vs /Tvs_3_0)

CompileFXToH(${WW_SOURCE_DIR}/FFT_Simulation_DirectCompute_shader.hlsl ${GEN_SRC_DIR}/ComputeH0_cs_5_0.h fx ${SHADER_SRC_DIR} ${GEN_SRC_DIR} ComputeH0 /Tcs_5_0 /Vng_ComputeH0)
CompileFXToH(${WW_SOURCE_DIR}/FFT_Simulation_DirectCompute_shader.hlsl ${GEN_SRC_DIR}/ComputeRows_cs_5_0.h fx ${SHADER_SRC_DIR} ${GEN_SRC_DIR} ComputeRows /Tcs_5_0 /Vng_ComputeRows)
CompileFXToH(${WW_SOURCE_DIR}/FFT_Simulation_DirectCompute_shader.hlsl ${GEN_SRC_DIR}/ComputeColumns_cs_5_0.h fx ${SHADER_SRC_DIR} ${GEN_SRC_DIR} ComputeColumns /Tcs_5_0 /Vng_ComputeColumns)


# Now the CUDA file
# CUDA!
SET(CUDA_NVCC_FLAGS "-G -g -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE -D_LIB -gencode arch=compute_30,code=compute_30 -gencode arch=compute_20,code=sm_20")

CUDA_INCLUDE_DIRECTORIES(
	${CUDA_INCLUDE_DIRS}
)

SET(CUDA_PROPAGATE_HOST_FLAGS OFF)

SET(CUDA_NVCC_FLAGS_DEBUG   "--compiler-options=/Zi,/W4,/nologo,/Od,/MTd")
SET(CUDA_NVCC_FLAGS_RELEASE "-use_fast_math -DNDEBUG --compiler-options=/W4,/nologo,/O2,/GF,/GS-,/Gy,/fp:fast,/GR-,/MT")

CUDA_COMPILE(GENERATED_CUDA_FILES_1
	${WW_SOURCE_DIR}/FFT_Simulation_CUDA_kernel.cu
)

ADD_LIBRARY(WaveWorks ${WW_LIBTYPE}
	${WW_PLATFORM_SRC_FILES}
	
	${CPP_FILES}
	${H_FILES}
	${DISTRO_INCLUDE_FILES}
	
	${FX_FILES}
	${GENERATED_HLSL_FILES}
	
	${GENERATED_CUDA_FILES_1}
	
#	${HLSL_FILES}
#	${CUDA_FILES}
)

SOURCE_GROUP("src" FILES ${CPP_FILES} ${H_FILES})
SOURCE_GROUP("include" FILES ${DISTRO_INCLUDE_FILES})
SOURCE_GROUP("generated" FILES ${GENERATED_HLSL_FILES})
SOURCE_GROUP("fx" FILES ${FX_FILES})
#SOURCE_GROUP("header" FILES ${H_FILES})
#SOURCE_GROUP("hlsl" FILES ${HLSL_FILES})
#SOURCE_GROUP("cuda" FILES ${CUDA_FILES})



# Target specific compile options


TARGET_INCLUDE_DIRECTORIES(WaveWorks 
	PRIVATE ${WW_PLATFORM_INCLUDES}
	PRIVATE ${CUDA_INCLUDE_DIRS}
	PRIVATE ${GEN_SRC_DIR}
	
	PRIVATE ${PROJECT_SOURCE_DIR}/include
	PRIVATE ${SHADER_SRC_DIR}
)

TARGET_COMPILE_DEFINITIONS(WaveWorks 

	# Common to all configurations
	PRIVATE ${WW_COMPILE_DEFS}
)



#TODO: Link flags

IF(TARGET_BUILD_PLATFORM STREQUAL "Windows")
	# Add linked libraries
	TARGET_LINK_LIBRARIES(WaveWorks PUBLIC ${CUDA_LIBRARIES} ${DirectX_DXGUID_LIBRARY})

	IF(CMAKE_CL_64)
		SET(LIBPATH_SUFFIX "win64")
	ELSE(CMAKE_CL_64)
		SET(LIBPATH_SUFFIX "Win32")
	ENDIF(CMAKE_CL_64)				

	SET_TARGET_PROPERTIES(WaveWorks PROPERTIES 
		LINK_FLAGS_DEBUG "/MAP /DEBUG"
		LINK_FLAGS_RELEASE "/MAP /INCREMENTAL:NO"
	)

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "PS4")
#	TARGET_LINK_LIBRARIES(WaveWorks PUBLIC LowLevel LowLevelAABB LowLevelCloth LowLevelDynamics LowLevelParticles WaveWorksCommon PxFoundation PxPvdSDK PxTask SceneQuery SimulationController)

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "XBoxOne")

#	TARGET_LINK_LIBRARIES(WaveWorks PUBLIC LowLevel LowLevelAABB LowLevelCloth LowLevelDynamics LowLevelParticles WaveWorksCommon PxFoundation PxPvdSDK PxTask SceneQuery SimulationController)

ELSEIF(TARGET_BUILD_PLATFORM STREQUAL "Unix")
ENDIF()


