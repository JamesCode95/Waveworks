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

#ifndef _NVWAVEWORKS_FFT_SIMULATION_MANAGER_DIRECTCOMPUTE_IMPL_H
#define _NVWAVEWORKS_FFT_SIMULATION_MANAGER_DIRECTCOMPUTE_IMPL_H

#include "FFT_Simulation_Manager.h"
#include "Sim_Array.h"

class NVWaveWorks_FFT_Simulation_DirectCompute_Impl;

class NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl : public NVWaveWorks_FFT_Simulation_Manager
{
public:

	NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl();
	~NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl();

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

	HRESULT beforeReallocateSimulation();

private:

#if defined(_DEV) || defined(DEBUG)
	void VerifyReadbackLockstep();
#endif

	Sim_Array<NVWaveWorks_FFT_Simulation_DirectCompute_Impl> m_Simulations;

	gfsdk_U64 m_NextKickID;

	bool m_StagingCursorIsValid;
	gfsdk_U64 m_StagingCursorKickID;

	HRESULT checkForReadbackResults();
};

#endif	// _NVWAVEWORKS_FFT_SIMULATION_MANAGER_CUDA_IMPL_H
