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

#ifndef _GFSDK_WAVEWORKS_CPU_SCHEDULER_H
#define _GFSDK_WAVEWORKS_CPU_SCHEDULER_H

#include <GFSDK_WaveWorks_Common.h>
#include <GFSDK_WaveWorks_Types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fwd. decls
struct GFSDK_WaveWorks_CPU_Scheduler_Interface;

// Scheduler-variation entrypoints
GFSDK_WAVEWORKS_DECL(gfsdk_waveworks_result    ) GFSDK_WaveWorks_Simulation_CreateD3D11_NDA(   const GFSDK_WaveWorks_Simulation_Settings& settings,
																							   const GFSDK_WaveWorks_Simulation_Params& params,
																							   GFSDK_WaveWorks_CPU_Scheduler_Interface* pOptionalScheduler,
																							   ID3D11Device* pD3DDevice,
																							   GFSDK_WaveWorks_SimulationHandle* pResult
																							   );

// The scheduler interface
struct GFSDK_WaveWorks_CPU_Scheduler_Interface
{
	/*
		* This interface can be used to cause WaveWorks CPU simulation tasks to be handled
		* by the client's own scheduler.
		*
		* First, note that you will need to provide one instance of this interface per
		* WaveWorks simulation. The semantics of the interface imply a little bit of
		* statefulness which evolves over a simulation cycle, therefore it is necessary
		* to have a unique scheduler object per simulation
		*
		* A single simulation cycle consists of:
		*
		*   1/ one or more 'push' calls to queue the initial simulation tasks
		*
		*   2/ a call to 'kick()', which is a signal that the initial tasks are fully
		*      queued and simulation can commence
		*
		*   3/ scheduler calls 'taskHandler' for each queued task. This *may* result
		*      in further 'push' calls to queue further work
		*
		*   4/ the simulation cycle is complete when a 'taskHandler' call exits and
		*      there are no tasks left in the queue
		*
		*   5/ WaveWorks will either poll or wait for the simulation cycle to complete
		*      via isWorkDone() or waitForWorkDone(), depending on the calls made to
		*      the WaveWorks API by the client
		*
		* No more than ONE simulation cycle will be scheduled at a time. WaveWorks will
		* wait for the current cycle to complete before attempting to push tasks for a new
		* cycle.
		*/

	// Queue a single item of work
	virtual void push(gfsdk_U32 taskData) = 0;

	// Queue a batch of 'n' items of work
	virtual void push(const gfsdk_U32* taskData, int n) = 0;

	// Queue a single item of work but insert at a random location in the existing work
	// queue (reason: this gives better perf on some platforms by 'relaxing' the memory bus)
	virtual void pushRandom(gfsdk_U32 taskData) = 0;

	// Wait until the current simulation cycle is out of tasks and all handlers have
	// returned
	virtual void waitForWorkDone() = 0;

	// Test whether the current simulation cycle is out of tasks and all handlers have
	// returned
	virtual bool isWorkDone() = 0;

	// Signal the scheduler to begin work on a new simulation cycle
	typedef void (*ProcessTaskFn)(void* pContext, gfsdk_U32 taskData);
	virtual bool kick(ProcessTaskFn taskHandler, void* pContext) = 0;
};

#ifdef __cplusplus
}; //extern "C" {
#endif

#endif	// _GFSDK_WAVEWORKS_CPU_SCHEDULER_H
