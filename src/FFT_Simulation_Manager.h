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

#ifndef _NVWAVEWORKS_FFT_SIMULATION_MANAGER_H
#define _NVWAVEWORKS_FFT_SIMULATION_MANAGER_H

#include "Internal.h"

class NVWaveWorks_FFT_Simulation;

struct GFSDK_WaveWorks_Simulation_Manager_Timings 
{
	// this struct is filled by simulation manager implementation
	float time_start_to_stop;		// time between starting the 1st thread's work and completing the last thread's work
	float time_total;				// sum of all time spent in worker threads doing actual work 
	float time_wait_for_completion;	// time spent on waitTasksCompletion
};

class NVWaveWorks_FFT_Simulation_Manager
{
public:

	virtual ~NVWaveWorks_FFT_Simulation_Manager() {};

	virtual HRESULT initD3D9(IDirect3DDevice9* /*pD3DDevice*/) { return S_OK; }
    virtual HRESULT initD3D10(ID3D10Device* /*pD3DDevice*/)    { return S_OK; }
    virtual HRESULT initD3D11(ID3D11Device* /*pD3DDevice*/)    { return S_OK; }
    virtual HRESULT initGL2(void* /*pGLContext*/)			   { return S_OK; }
	virtual HRESULT initNoGraphics()                           { return S_OK; }
	virtual HRESULT initGnm()								   { return S_OK; }

	// Simulation lifetime management
	virtual NVWaveWorks_FFT_Simulation* createSimulation(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params) = 0;
	virtual void releaseSimulation(NVWaveWorks_FFT_Simulation* pSimulation) = 0;

	// Pipeline synchronization
	virtual HRESULT kick(Graphics_Context* pGC, double dSimTime, gfsdk_U64& kickID) = 0;
	virtual bool getStagingCursor(gfsdk_U64* pKickID) = 0;					// Returns true iff the staging cursor is valid
	virtual bool getReadbackCursor(gfsdk_U64* pKickID) = 0;					// Returns true iff the readback cursor is valid

	enum AdvanceCursorResult
	{
		AdvanceCursorResult_Failed = -1,	// Something bad happened
		AdvanceCursorResult_Succeeded = 0,	// The cursor was advanced
		AdvanceCursorResult_None,			// The cursor was not advanced because there were no kicks in-flight
		AdvanceCursorResult_WouldBlock		// The cursor was not advanced because although there was a kick in-flight,
											// the function was called in non-blocking mode and the in-flight kick is not
											// yet ready
	};

	virtual AdvanceCursorResult advanceStagingCursor(bool block) = 0;
	virtual AdvanceCursorResult advanceReadbackCursor(bool block) = 0;

	enum WaitCursorResult
	{
		WaitCursorResult_Failed = -1,	// Something bad happened
		WaitCursorResult_Succeeded = 0,	// The cursor is ready to advance
		WaitCursorResult_None			// The cursor is not ready to advance because there were no kicks in-flight
	};

	virtual WaitCursorResult waitStagingCursor() = 0;

	virtual HRESULT archiveDisplacements() = 0;

	// Hooks
	virtual HRESULT beforeReinit(const GFSDK_WaveWorks_Detailed_Simulation_Params& params, bool reinitOnly) = 0;
	virtual HRESULT getTimings(GFSDK_WaveWorks_Simulation_Manager_Timings& timings) = 0;
};

#endif	// _NVWAVEWORKS_FFT_SIMULATION_MANAGER_H
