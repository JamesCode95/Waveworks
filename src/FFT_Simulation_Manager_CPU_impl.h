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

#ifndef _NVWAVEWORKS_FFT_SIMULATION_MANAGER_CPU_IMPL_H
#define _NVWAVEWORKS_FFT_SIMULATION_MANAGER_CPU_IMPL_H

#include "FFT_Simulation_Manager.h"
#include "Sim_Array.h"

class NVWaveWorks_FFT_Simulation_CPU_Impl;
struct GFSDK_WaveWorks_CPU_Scheduler_Interface;
struct NVWaveWorks_FFT_Simulation_CPU_Impl_Workqueue;

class NVWaveWorks_FFT_Simulation_Manager_CPU_Impl : public NVWaveWorks_FFT_Simulation_Manager
{
public:

	NVWaveWorks_FFT_Simulation_Manager_CPU_Impl(const GFSDK_WaveWorks_Detailed_Simulation_Params& params, GFSDK_WaveWorks_CPU_Scheduler_Interface* pOptionalScheduler);
	~NVWaveWorks_FFT_Simulation_Manager_CPU_Impl();

	// Mandatory NVWaveWorks_FFT_Simulation_Manager interface
	NVWaveWorks_FFT_Simulation* createSimulation(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params);
	void releaseSimulation(NVWaveWorks_FFT_Simulation* pSimulation);
	HRESULT beforeReinit(const GFSDK_WaveWorks_Detailed_Simulation_Params& params, bool reinitOnly);
	HRESULT kick(Graphics_Context* pGC, double dSimTime, gfsdk_U64& kickID);
	bool getStagingCursor(gfsdk_U64* pKickID);
	AdvanceCursorResult advanceStagingCursor(bool block);
	bool getReadbackCursor(gfsdk_U64* pKickID);
	AdvanceCursorResult advanceReadbackCursor(bool block);
	WaitCursorResult waitStagingCursor();
	HRESULT archiveDisplacements();
	HRESULT getTimings(GFSDK_WaveWorks_Simulation_Manager_Timings& timings);

private:

	Sim_Array<NVWaveWorks_FFT_Simulation_CPU_Impl> m_Simulations;

	gfsdk_U64 m_NextKickID;

	bool m_IsWaitingInFlightKick;
	bool m_HasPendingFlip;
	gfsdk_U64 m_InFlightKickID;

	bool m_HasReadyKick;
	gfsdk_U64 m_ReadyKickID;

	GFSDK_WaveWorks_CPU_Scheduler_Interface* m_queue;
	NVWaveWorks_FFT_Simulation_CPU_Impl_Workqueue* m_defaultQueue;

	gfsdk_U32* m_pushBuffer;
	size_t m_pushBufferCapacity;
	void ensurePushBufferCapacity(size_t n);	// Can caused existing contents to be lost!

	// timing stats
	float m_time_wait_for_tasks_completion;	// time spent on waitTasksCompletion

	bool m_enable_CPU_timers;

	static void ProcessTaskFn(void* pContext, gfsdk_U32 taskData);

	void ProcessTask(gfsdk_U32 taskData);
	void UpdateH0(gfsdk_U32 taskData);
	void UpdateHt(gfsdk_U32 taskData);
	void ComputeFFT_XY_NxN(gfsdk_U32 taskData);
	void ComputeFFT_X_N(gfsdk_U32 taskData);
	void ComputeFFT_Y_N(gfsdk_U32 taskData);
	void UpdateTexture(gfsdk_U32 taskData);

	void waitTasksCompletion();
	void flip();

	void processAllTasks();
};

#endif	// _NVWAVEWORKS_FFT_SIMULATION_MANAGER_CPU_IMPL_H
