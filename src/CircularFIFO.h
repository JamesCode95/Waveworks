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

#ifndef _CIRCULAR_FIFO_H
#define _CIRCULAR_FIFO_H

/*===========================================================================
  A template class to handle a fixed-maximum-size circular FIFO
  ===========================================================================*/
template<class T>
class CircularFIFO
{
public:
	CircularFIFO(int capacity) :
		m_ptr(new T [capacity]),
		m_capacity(capacity),
		m_range_begin_index(0),
		m_range_count(0)
	{
		memset(m_ptr,0,m_capacity*sizeof(T));
	}

	~CircularFIFO()
	{
		delete [] m_ptr;
	}

	int capacity() const { return m_capacity; }
	int range_count() const { return m_range_count; }

	T& raw_at(int ix)
	{
		assert(ix < m_capacity);
		return m_ptr[ix];
	}

	// NB: ix = 0 means 'most-recently-added', hence the reverse indexing...
	T& range_at(int ix)
	{
		assert(ix < m_range_count);
		return m_ptr[(m_range_begin_index+m_range_count-ix-1)%m_capacity];
	}

	// Recycles the oldest entry in the FIFO if necessary
	T& consume_one()
	{
		assert(m_capacity > 0);

		if(m_capacity == m_range_count)
		{
			// The FIFO is full, so free up the oldest entry
			m_range_begin_index = (m_range_begin_index+1) % m_capacity;
			--m_range_count;
		}

		const int raw_result_index = (m_range_begin_index+m_range_count) % m_capacity;
		++m_range_count;

		return m_ptr[raw_result_index];
	}

private:

	T* m_ptr;
	int m_capacity;
	int m_range_begin_index;
	int m_range_count;
};

#endif	// _CIRCULAR_FIFO_H
