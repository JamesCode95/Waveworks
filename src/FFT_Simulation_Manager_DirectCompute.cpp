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

#include "FFT_Simulation_Manager_DirectCompute_impl.h"
#include "FFT_Simulation_DirectCompute_impl.h"

#ifdef SUPPORT_DIRECTCOMPUTE

NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl() :
	m_NextKickID(0),
	m_StagingCursorIsValid(false),
	m_StagingCursorKickID(0)
{
}

NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::~NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl()
{
	assert(0 == m_Simulations.size());	// It is an error to destroy a non-empty manager
	m_Simulations.erase_all();
}

NVWaveWorks_FFT_Simulation* NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::createSimulation(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params)
{
	NVWaveWorks_FFT_Simulation_DirectCompute_Impl* pResult = new NVWaveWorks_FFT_Simulation_DirectCompute_Impl(this,params);
	m_Simulations.push_back(pResult);
	return pResult;
}

void NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::releaseSimulation(NVWaveWorks_FFT_Simulation* pSimulation)
{
	m_Simulations.erase(pSimulation);
	SAFE_DELETE(pSimulation);
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::beforeReinit(const GFSDK_WaveWorks_Detailed_Simulation_Params& /*params*/, bool /*reinitOnly*/)
{
	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::checkForReadbackResults()
{
	HRESULT hr;

	// The goal here is to evolve the readback state of all our simulations in lockstep, so that either all our simulations collect
	// a single readback or else none do (IOW: 'some' is *not* permitted, because it would break lockstep)

	NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pBeginSimulationsSrc = (NVWaveWorks_FFT_Simulation_DirectCompute_Impl**)_alloca(m_Simulations.size() * sizeof(NVWaveWorks_FFT_Simulation_DirectCompute_Impl*));
	memcpy(pBeginSimulationsSrc,m_Simulations.begin(),m_Simulations.size() * sizeof(NVWaveWorks_FFT_Simulation_DirectCompute_Impl*));
	NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pEndSimulationsSrc = pBeginSimulationsSrc + m_Simulations.size();

	NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pBeginSimulationsNoResult = (NVWaveWorks_FFT_Simulation_DirectCompute_Impl**)_alloca(m_Simulations.size() * sizeof(NVWaveWorks_FFT_Simulation_DirectCompute_Impl*));;
	NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pEndSimulationsNoResult = pBeginSimulationsNoResult;

	// Do an initial walk thru and see if any readbacks arrived (without blocking), and write any that did not get a readback result into dst
	for(NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pSim = pBeginSimulationsSrc; pSim != pEndSimulationsSrc; ++pSim)
	{
		hr = (*pSim)->collectSingleReadbackResult(false);
		if(FAILED(hr))
		{
			return hr;
		}

		if(S_FALSE == hr)
		{
			(*pEndSimulationsNoResult) = (*pSim);
			++pEndSimulationsNoResult;
		}
	}

	// If no results are ready, we're in sync so don't try again
	if((pEndSimulationsNoResult-pBeginSimulationsNoResult) != m_Simulations.size())
	{
		// Otherwise, wait on the remaining results
		for(NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pSim = pBeginSimulationsNoResult; pSim != pEndSimulationsNoResult; ++pSim)
		{
			V_RETURN((*pSim)->collectSingleReadbackResult(true));
		}
	}

#if defined(_DEV) || defined (DEBUG)
	VerifyReadbackLockstep();
#endif

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::kick(Graphics_Context* pGC, double dSimTime, gfsdk_U64& kickID)
{
	HRESULT hr;

	kickID = m_NextKickID;

	// Check for readback results - note that we do this at the manager level in order to guarantee lockstep between
	// the simulations that form a cascade. We either want all of simulations to collect a result, or none - some is
	// not an option
	checkForReadbackResults();

	// Kick all the sims
	for(NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		V_RETURN((*pSim)->kick(pGC,dSimTime,kickID));
	}

	m_StagingCursorIsValid = true;
	m_StagingCursorKickID = m_NextKickID;
	++m_NextKickID;
	return S_OK;
}

bool NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::getStagingCursor(gfsdk_U64* pKickID)
{
	if(pKickID && m_StagingCursorIsValid)
	{
		*pKickID = m_StagingCursorKickID;
	}

	return m_StagingCursorIsValid;
}

NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::advanceStagingCursor(bool /*block*/)
{
	// The DirectCompute pipeline is not async wrt the API, so there can never be any pending kicks and we can return immediately
	return AdvanceCursorResult_None;
}

NVWaveWorks_FFT_Simulation_Manager::WaitCursorResult NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::waitStagingCursor()
{
	// The DirectCompute pipeline is not async wrt the API, so there can never be any pending kicks and we can return immediately
	return WaitCursorResult_None;
}

#ifdef _DEV
void NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::VerifyReadbackLockstep()
{
	if(m_Simulations.size() > 1)
	{
		gfsdk_U64 sim0KickID;
		bool sim0GRCresult = m_Simulations[0]->getReadbackCursor(&sim0KickID);
		for(NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pSim = m_Simulations.begin()+1; pSim != m_Simulations.end(); ++pSim)
		{
			gfsdk_U64 simNKickID;
			bool simNGRCresult = (*pSim)->getReadbackCursor(&simNKickID);
			assert(simNGRCresult == sim0GRCresult);
			if(sim0GRCresult)
			{
				assert(sim0KickID == simNKickID);
			}
		}

	}
}
#endif

bool NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::getReadbackCursor(gfsdk_U64* pKickID)
{
	if(0 == m_Simulations.size())
		return false;

	// We rely on collectSingleReadbackResult() to maintain lockstep between the cascade members, therefore we can in theory
	// query any member to get the readback cursor...

	// ...but let's check that theory in debug builds!!!
#ifdef _DEV
	VerifyReadbackLockstep();
#endif

	return m_Simulations[0]->getReadbackCursor(pKickID);
}

NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::advanceReadbackCursor(bool block)
{
	if(0 == m_Simulations.size())
		return AdvanceCursorResult_None;

	// First, check whether we even have readbacks in-flight
	const bool hasReadbacksInFlightSim0 = m_Simulations[0]->hasReadbacksInFlight();

	// Usual paranoid verficiation that we're maintaining lockstep...
#ifdef _DEV
	VerifyReadbackLockstep();
#endif

	if(!hasReadbacksInFlightSim0)
	{
		return AdvanceCursorResult_None;
	}

	if(!block)
	{
		// Non-blocking case - in order to maintain lockstep, either all of the simulations should consume a readback,
		// or none. Therefore we need to do an initial pass to test whether the 'all' case applies (and bail if not)...
		for(NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
		{
			HRESULT hr = (*pSim)->canCollectSingleReadbackResultWithoutBlocking();
			if(FAILED(hr))
			{
				return AdvanceCursorResult_Failed;
			}
			else if(S_FALSE == hr)
			{
				// Cannot advance, would have blocked -> bail
				return AdvanceCursorResult_WouldBlock;
			}
		}
	}

	// We have readbacks in flight, and in the non-blocking case we *should* be in a position to consume them without
	// any waiting, so just visit each simulation in turn with a blocking wait for the next readback to complete...
	for(NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		if(FAILED((*pSim)->collectSingleReadbackResult(true)))
		{
			return AdvanceCursorResult_Failed;
		}
	}

#ifdef _DEV
	VerifyReadbackLockstep();
#endif

	return AdvanceCursorResult_Succeeded;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::archiveDisplacements()
{
	HRESULT hr;

	if(!getReadbackCursor(NULL))
	{
		return E_FAIL;
	}

	for(NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		V_RETURN((*pSim)->archiveDisplacements());
	}

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::getTimings(GFSDK_WaveWorks_Simulation_Manager_Timings& timings)
{
	// DirectCompute implementation doesn't update these CPU implementation related timings
	timings.time_start_to_stop = 0;
	timings.time_total = 0;
	timings.time_wait_for_completion = 0;
	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl::beforeReallocateSimulation()
{
	HRESULT hr;

	// A simulation is about to be reallocated...

	// Implication 1: at least some displacement map contents will become undefined and
	// will need a kick to make them valid again, which in turn means that we can no longer
	// consider any kick that was previously staged as still being staged...
	m_StagingCursorIsValid = false;

	// Implication 2: some of the readback tracking will be reset, meaning we break
	// lockstep. We can avoid this by forcible resetting all readback tracking
	for(NVWaveWorks_FFT_Simulation_DirectCompute_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		V_RETURN((*pSim)->resetReadbacks());
	}

	return S_OK;
}

#endif // SUPPORT_DIRECTCOMPUTE
