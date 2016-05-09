// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright © 2008- 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

/*
 * This header defines the Orbis-specific parts of the WaveWorks API
 */

#ifndef _GFSDK_WAVEWORKS_ORBIS_H
#define _GFSDK_WAVEWORKS_ORBIS_H

#include <GFSDK_WaveWorks_Common.h>
#include <GFSDK_WaveWorks_Types.h>


/*===========================================================================
  Orbis-specific malloc hooks (replaces the definition in the generic header)
  ===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GFSDK_WAVEWORKS_MALLOC_HOOKS_DEFINED
#define GFSDK_WAVEWORKS_MALLOC_HOOKS_DEFINED

typedef void* (GFSDK_WAVEWORKS_CALL_CONV *GFSDK_WAVEWORKS_MALLOC) (size_t size);
typedef void (GFSDK_WAVEWORKS_CALL_CONV *GFSDK_WAVEWORKS_FREE) (void *p);
typedef void* (GFSDK_WAVEWORKS_CALL_CONV *GFSDK_WAVEWORKS_ALIGNED_MALLOC) (size_t size, size_t alignment);
typedef void (GFSDK_WAVEWORKS_CALL_CONV *GFSDK_WAVEWORKS_ALIGNED_FREE) (void *p);

struct GFSDK_WaveWorks_Malloc_Hooks
{
	GFSDK_WAVEWORKS_ALIGNED_MALLOC pOnionAlloc;
	GFSDK_WAVEWORKS_ALIGNED_FREE pOnionFree;
	GFSDK_WAVEWORKS_ALIGNED_MALLOC pGarlicAlloc;
	GFSDK_WAVEWORKS_ALIGNED_FREE pGarlicFree;
};

#endif

#ifdef __cplusplus
}; //extern "C" {
#endif


/*===========================================================================
  The generic public header
  ===========================================================================*/
#include <GFSDK_WaveWorks.h>


/*===========================================================================
  Orbis-specifics
  ===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

// Fwd. decls
namespace sce { namespace Gnmx { class LightweightGfxContext; } }
struct GFSDK_WaveWorks_GnmxWrap;
struct GFSDK_WaveWorks_CPU_Scheduler_Interface;

// Entrypoints
GFSDK_WAVEWORKS_DECL(gfsdk_waveworks_result   ) GFSDK_WaveWorks_InitGnm(const GFSDK_WaveWorks_Malloc_Hooks* pRequiredMallocHooks, const GFSDK_WaveWorks_API_GUID& apiGUID, GFSDK_WaveWorks_GnmxWrap* pRequiredGnmxWrap);
GFSDK_WAVEWORKS_DECL(gfsdk_waveworks_result   ) GFSDK_WaveWorks_ReleaseGnm();
GFSDK_WAVEWORKS_DECL(gfsdk_waveworks_result   ) GFSDK_WaveWorks_Simulation_CreateGnm(const GFSDK_WaveWorks_Simulation_Settings& settings, const GFSDK_WaveWorks_Simulation_Params& params, GFSDK_WaveWorks_CPU_Scheduler_Interface* pOptionalScheduler, GFSDK_WaveWorks_SimulationHandle* pResult);
GFSDK_WAVEWORKS_DECL(gfsdk_U32		          ) GFSDK_WaveWorks_Simulation_GetShaderInputCountGnm();
GFSDK_WAVEWORKS_DECL(gfsdk_waveworks_result   ) GFSDK_WaveWorks_Simulation_GetShaderInputDescGnm(gfsdk_U32 inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc);
GFSDK_WAVEWORKS_DECL(gfsdk_waveworks_result   ) GFSDK_WaveWorks_Simulation_SetRenderStateGnm(GFSDK_WaveWorks_SimulationHandle hSim, sce::Gnmx::LightweightGfxContext* pGC, const gfsdk_float4x4& matView, const gfsdk_U32 * pShaderInputRegisterMappings);
GFSDK_WAVEWORKS_DECL(gfsdk_waveworks_result   ) GFSDK_WaveWorks_Simulation_KickGnm(GFSDK_WaveWorks_SimulationHandle hSim, gfsdk_U64* pKickID, sce::Gnmx::LightweightGfxContext* pGC);
GFSDK_WAVEWORKS_DECL(gfsdk_waveworks_result   ) GFSDK_WaveWorks_Simulation_AdvanceStagingCursorGnm(GFSDK_WaveWorks_SimulationHandle hSim, bool block, sce::Gnmx::LightweightGfxContext* pGC);
GFSDK_WAVEWORKS_DECL(gfsdk_waveworks_result   ) GFSDK_WaveWorks_Quadtree_CreateGnm(const GFSDK_WaveWorks_Quadtree_Params& params, GFSDK_WaveWorks_QuadtreeHandle* pResult);
GFSDK_WAVEWORKS_DECL(gfsdk_U32                ) GFSDK_WaveWorks_Quadtree_GetShaderInputCountGnm();
GFSDK_WAVEWORKS_DECL(gfsdk_waveworks_result   ) GFSDK_WaveWorks_Quadtree_GetShaderInputDescGnm(gfsdk_U32 inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc);
GFSDK_WAVEWORKS_DECL(gfsdk_waveworks_result   ) GFSDK_WaveWorks_Quadtree_DrawGnm(GFSDK_WaveWorks_QuadtreeHandle hQuadtree, sce::Gnmx::LightweightGfxContext* pGC, const gfsdk_float4x4& matView, const gfsdk_float4x4& matProj, const gfsdk_float2& viewportDims, const gfsdk_U32 * pShaderInputRegisterMappings);

#ifdef __cplusplus
}; //extern "C" {
#endif

#endif	// _GFSDK_WAVEWORKS_ORBIS_H
