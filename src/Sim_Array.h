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

#ifndef _NVWAVEWORKS_SIM_ARRAY_H
#define _NVWAVEWORKS_SIM_ARRAY_H

// Template container specifically for maintaining an array of simulation pointers
template<class SimType>
class Sim_Array
{
public:
	Sim_Array() :
		m_pSimulations(0),
		m_NumSimulationSlotsUsed(0),
		m_NumSimulationSlotsAllocated(0)
	{
	}

	~Sim_Array()
	{
		erase_all();
	}

	void push_back(SimType* pSim)
	{
		if(m_NumSimulationSlotsUsed == m_NumSimulationSlotsAllocated) {
			// Expand/allocate storage
			if(0 == m_NumSimulationSlotsAllocated) {
				assert(0 == m_pSimulations);
				m_NumSimulationSlotsAllocated = GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades;
				m_pSimulations = new SimType* [m_NumSimulationSlotsAllocated];
			} else {
				assert(0 != m_pSimulations);
				UINT newNumSlotsAllocated = m_NumSimulationSlotsAllocated * 2;
				SimType** pNewSlots = new SimType* [newNumSlotsAllocated];
				memcpy(pNewSlots, m_pSimulations, m_NumSimulationSlotsUsed * sizeof(m_pSimulations[0]));
				SAFE_DELETE_ARRAY(m_pSimulations);
				m_pSimulations = pNewSlots;
				m_NumSimulationSlotsAllocated = newNumSlotsAllocated;
			}
		}

		assert(m_NumSimulationSlotsUsed < m_NumSimulationSlotsAllocated);
		m_pSimulations[m_NumSimulationSlotsUsed] = pSim;
		++m_NumSimulationSlotsUsed;
	}

	template<class EraseType>
	void erase(EraseType* pSim)
	{
		SimType** pWritePtr = m_pSimulations;
		SimType** pReadPtr = m_pSimulations;
		SimType** pEndPtr = m_pSimulations + m_NumSimulationSlotsUsed;
		for(; pReadPtr != pEndPtr; ++pReadPtr) {
			if(*pReadPtr == pSim) {
				-- m_NumSimulationSlotsUsed;
			} else {
				*pWritePtr = *pReadPtr;
				++pWritePtr;
			}
		}
	}

	void erase_all()
	{
		SAFE_DELETE_ARRAY(m_pSimulations);
		m_NumSimulationSlotsUsed = 0;
		m_NumSimulationSlotsAllocated = 0;
	}

	SimType** begin() { return m_pSimulations; }
	SimType** end()   { return m_pSimulations+m_NumSimulationSlotsUsed; }
	SimType* operator[](int ix) { return m_pSimulations[ix]; }
	int size() const { return m_NumSimulationSlotsUsed; }

private:
	SimType** m_pSimulations;
	int m_NumSimulationSlotsUsed;
	int m_NumSimulationSlotsAllocated;
};

#endif	// _NVWAVEWORKS_SIM_ARRAY_H
