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

#include "Internal.h"
#include "GFX_Timer_impl.h"
#include "Graphics_Context.h"

#if defined(TARGET_PLATFORM_NIXLIKE)
#include <unistd.h>
#include <string.h>
void Sleep(DWORD dwMilliseconds)
{
	assert(!dwMilliseconds);
	sleep(dwMilliseconds);
}
#endif


/* 
 * ********************************************************************************
 * Utility class for managing a pool of queries
 * ********************************************************************************
*/
namespace
{
	template<class QueryDataType>
	class GFSDK_WaveWorks_GFX_Query_Pool_Impl
	{
	public:
		GFSDK_WaveWorks_GFX_Query_Pool_Impl();
		~GFSDK_WaveWorks_GFX_Query_Pool_Impl();

		int getNumQueries() const { return m_NumQueries; }
		int getNumInactiveQueries() const { return m_NumInactiveQueries; }

		QueryDataType& addInactiveQuery();
		int activateQuery();

		void releaseQuery(int ix);
		void addRefQuery(int ix);

		QueryDataType& getQueryData(int ix);

	private:

		void releaseAll();

		QueryDataType* m_pQueriesData;
		int m_NumQueries;

		int* m_pInactiveQueries;
		int m_NumInactiveQueries;
	};

	template<class QueryDataType>
	GFSDK_WaveWorks_GFX_Query_Pool_Impl<QueryDataType>::GFSDK_WaveWorks_GFX_Query_Pool_Impl()
	{
		m_pQueriesData = 0;
		m_NumQueries = 0;
		m_pInactiveQueries = 0;
		m_NumInactiveQueries = 0;
	}

	template<class QueryDataType>
	GFSDK_WaveWorks_GFX_Query_Pool_Impl<QueryDataType>::~GFSDK_WaveWorks_GFX_Query_Pool_Impl()
	{
		releaseAll();
	}

	template<class QueryDataType>
	void GFSDK_WaveWorks_GFX_Query_Pool_Impl<QueryDataType>::releaseAll()
	{
		SAFE_DELETE_ARRAY(m_pQueriesData);
		SAFE_DELETE_ARRAY(m_pInactiveQueries);

		m_NumQueries = 0;
		m_NumInactiveQueries = 0;
	}

	template<class QueryDataType>
	QueryDataType& GFSDK_WaveWorks_GFX_Query_Pool_Impl<QueryDataType>::addInactiveQuery()
	{
		int newQueryIndex = m_NumQueries;
		int newNumQueries = m_NumQueries + 1;
		QueryDataType* pNewDatas = new QueryDataType[newNumQueries];
		int* pNewInactiveQueries = new int[newNumQueries];

		memcpy(pNewDatas, m_pQueriesData, m_NumQueries * sizeof(m_pQueriesData[0]));
		memcpy(pNewInactiveQueries, m_pInactiveQueries, m_NumInactiveQueries * sizeof(m_pInactiveQueries[0]));

		SAFE_DELETE_ARRAY(m_pQueriesData);
		SAFE_DELETE_ARRAY(m_pInactiveQueries);

		m_pQueriesData = pNewDatas;
		m_pInactiveQueries = pNewInactiveQueries;

		// Fixup newbies
		m_pQueriesData[newQueryIndex].m_refCount = 0;
		m_pInactiveQueries[m_NumInactiveQueries] = newQueryIndex;
		++m_NumInactiveQueries;

		m_NumQueries = newNumQueries;

		return m_pQueriesData[newQueryIndex];
	}

	template<class QueryDataType>
	int GFSDK_WaveWorks_GFX_Query_Pool_Impl<QueryDataType>::activateQuery()
	{
		assert(m_NumInactiveQueries > 0);

		--m_NumInactiveQueries;

		int result = m_pInactiveQueries[m_NumInactiveQueries];
		m_pQueriesData[result].m_status = S_FALSE;
		m_pQueriesData[result].m_refCount = 1;

		return result;
	}

	template<class QueryDataType>
	void GFSDK_WaveWorks_GFX_Query_Pool_Impl<QueryDataType>::releaseQuery(int ix)
	{
		assert(ix < m_NumQueries);
		assert(m_pQueriesData[ix].m_refCount > 0);

		--m_pQueriesData[ix].m_refCount;
		if(0 == m_pQueriesData[ix].m_refCount)
		{
			// return to inactive pool
			assert(m_NumInactiveQueries < m_NumQueries);
			m_pInactiveQueries[m_NumInactiveQueries] = ix;
			++m_NumInactiveQueries;
		}
	}

	template<class QueryDataType>
	void GFSDK_WaveWorks_GFX_Query_Pool_Impl<QueryDataType>::addRefQuery(int ix)
	{
		assert(ix < m_NumQueries);
		assert(m_pQueriesData[ix].m_refCount > 0);	// Because it is invalid to use a zero-ref'd query

		++m_pQueriesData[ix].m_refCount;
	}

	template<class QueryDataType>
	QueryDataType& GFSDK_WaveWorks_GFX_Query_Pool_Impl<QueryDataType>::getQueryData(int ix)
	{
		assert(ix < m_NumQueries);

		return m_pQueriesData[ix];
	}

	struct DisjointQueryData
	{
		int m_refCount;
		UINT64 m_freqResult;
		HRESULT m_status;


#if WAVEWORKS_ENABLE_D3D11
		struct D3D11Objects
		{
			ID3D11Query* m_pDisjointTimerQuery;
		};
#endif

		union
		{
#if WAVEWORKS_ENABLE_D3D11
			D3D11Objects _11;
#endif
		} m_d3d;
	};

	struct TimerQueryData
	{
		int m_refCount;
		UINT64 m_timestampResult;
		HRESULT m_status;

#if WAVEWORKS_ENABLE_D3D11
		struct D3D11Objects
		{
			ID3D11Query* m_pTimerQuery;
		};
#endif
#if WAVEWORKS_ENABLE_GL
		struct GL2Objects
		{
			GLuint m_GLTimerQuery;
		};
#endif
		union
		{
#if WAVEWORKS_ENABLE_D3D11
			D3D11Objects _11;
#endif
#if WAVEWORKS_ENABLE_GL
			GL2Objects _GL2;
#endif
		} m_d3d;
	};
}

class GFSDK_WaveWorks_GFX_DisjointQuery_Pool_Impl : public GFSDK_WaveWorks_GFX_Query_Pool_Impl<DisjointQueryData> {};
class GFSDK_WaveWorks_GFX_TimerQuery_Pool_Impl : public GFSDK_WaveWorks_GFX_Query_Pool_Impl<TimerQueryData> {};

/* 
 * ********************************************************************************
*/

NVWaveWorks_GFX_Timer_Impl::NVWaveWorks_GFX_Timer_Impl()
{
	memset(&m_d3d, 0, sizeof(m_d3d));
	m_d3dAPI = nv_water_d3d_api_undefined;

	m_pDisjointTimersPool = 0;
	m_pTimersPool = 0;

	m_CurrentDisjointTimerQuery = -1;
}

NVWaveWorks_GFX_Timer_Impl::~NVWaveWorks_GFX_Timer_Impl()
{
	releaseAll();
}

HRESULT NVWaveWorks_GFX_Timer_Impl::initD3D11(ID3D11Device* D3D11_ONLY(pD3DDevice))
{
#if WAVEWORKS_ENABLE_D3D11
    HRESULT hr;

    if(nv_water_d3d_api_d3d11 != m_d3dAPI)
    {
        releaseAll();
    }
    else if(m_d3d._11.m_pd3d11Device != pD3DDevice)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_d3d11;
        m_d3d._11.m_pd3d11Device = pD3DDevice;
        m_d3d._11.m_pd3d11Device->AddRef();

        V_RETURN(allocateAllResources());
    }

    return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT NVWaveWorks_GFX_Timer_Impl::initGnm()
{
    // No timers on PS4
#if WAVEWORKS_ENABLE_GNM
    return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT NVWaveWorks_GFX_Timer_Impl::initGL2(void* GL_ONLY(pGLContext))
{
#if WAVEWORKS_ENABLE_GL
	HRESULT hr;
	if(nv_water_d3d_api_gl2 != m_d3dAPI)
	{
		releaseAll();
	}
	else if(m_d3d._GL2.m_pGLContext != pGLContext)
	{
		releaseAll();
	}

	if(nv_water_d3d_api_undefined == m_d3dAPI)
	{
		m_d3dAPI = nv_water_d3d_api_gl2;
		m_d3d._GL2.m_pGLContext = pGLContext;
		V_RETURN(allocateAllResources());
	}
	return S_OK;
#else
	return S_FALSE;
#endif
}

HRESULT NVWaveWorks_GFX_Timer_Impl::allocateAllResources()
{
	SAFE_DELETE(m_pDisjointTimersPool);
	m_pDisjointTimersPool = new GFSDK_WaveWorks_GFX_DisjointQuery_Pool_Impl();

	SAFE_DELETE(m_pTimersPool);
	m_pTimersPool = new GFSDK_WaveWorks_GFX_TimerQuery_Pool_Impl();

    return S_OK;
}

void NVWaveWorks_GFX_Timer_Impl::releaseAll()
{
	releaseAllResources();

#if WAVEWORKS_ENABLE_GRAPHICS
    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			SAFE_RELEASE(m_d3d._11.m_pd3d11Device);
        }
        break;
#endif

#if WAVEWORKS_ENABLE_GL
    case nv_water_d3d_api_gl2:
        {
			// do nothing
        }
        break;
#endif
	default:
		break;
    }
#endif // WAVEWORKS_ENABLE_GRAPHICS

	m_d3dAPI = nv_water_d3d_api_undefined;
}

void NVWaveWorks_GFX_Timer_Impl::releaseAllResources()
{
#if WAVEWORKS_ENABLE_GRAPHICS
    switch(m_d3dAPI)
    {

#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			for(int i = 0; i != m_pDisjointTimersPool->getNumQueries(); ++i)
			{
				DisjointQueryData& dqd = m_pDisjointTimersPool->getQueryData(i);
				SAFE_RELEASE(dqd.m_d3d._11.m_pDisjointTimerQuery);
			}
			for(int i = 0; i != m_pTimersPool->getNumQueries(); ++i)
			{
				TimerQueryData& tqd = m_pTimersPool->getQueryData(i);
				SAFE_RELEASE(tqd.m_d3d._11.m_pTimerQuery);
			}
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			// Nothin doin
		}
		break;
#endif 

#if WAVEWORKS_ENABLE_GL
    case nv_water_d3d_api_gl2:
        {
			for(int i = 0; i != m_pTimersPool->getNumQueries(); ++i)
			{
				TimerQueryData& tqd = m_pTimersPool->getQueryData(i);
				if(tqd.m_d3d._GL2.m_GLTimerQuery > 0) NVSDK_GLFunctions.glDeleteQueries(1, &tqd.m_d3d._GL2.m_GLTimerQuery); CHECK_GL_ERRORS;
			}
		}
		break;
#endif
	default:
		break;
	}
#endif // WAVEWORKS_ENABLE_GRAPHICS

	SAFE_DELETE(m_pDisjointTimersPool);
	SAFE_DELETE(m_pTimersPool);
}

HRESULT NVWaveWorks_GFX_Timer_Impl::issueTimerQuery(Graphics_Context* pGC, int& ix)
{
	if(0 == m_pTimersPool->getNumInactiveQueries())
	{
		// Add D3D resources
#if WAVEWORKS_ENABLE_GRAPHICS
		switch(m_d3dAPI)
		{
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			{
				HRESULT hr;
				TimerQueryData& tqd = m_pTimersPool->addInactiveQuery();

				D3D11_QUERY_DESC query_desc;
				query_desc.Query = D3D11_QUERY_TIMESTAMP;
				query_desc.MiscFlags = 0;
				V_RETURN(m_d3d._11.m_pd3d11Device->CreateQuery(&query_desc, &tqd.m_d3d._11.m_pTimerQuery));
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			{
				/*TimerQueryData& tqd =*/ m_pTimersPool->addInactiveQuery();
			}
			break;
#endif

#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				TimerQueryData& tqd = m_pTimersPool->addInactiveQuery();
				NVSDK_GLFunctions.glGenQueries(1, &tqd.m_d3d._GL2.m_GLTimerQuery); CHECK_GL_ERRORS;
			}
			break;
#endif

		default:
			// Unexpected API
			return E_FAIL;
		}
#endif // WAVEWORKS_ENABLE_GRAPHICS
	}

	ix = m_pTimersPool->activateQuery();

	// Begin the query
#if WAVEWORKS_ENABLE_GRAPHICS
	switch(m_d3dAPI)
	{

#if WAVEWORKS_ENABLE_D3D11
	case nv_water_d3d_api_d3d11:
		{
			const TimerQueryData& tqd = m_pTimersPool->getQueryData(ix);

			ID3D11DeviceContext* pDC_d3d11 = pGC->d3d11();
			pDC_d3d11->End(tqd.m_d3d._11.m_pTimerQuery);
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
            const TimerQueryData& tqd = m_pTimersPool->getQueryData(ix);
            // Nothin doin
		}
		break;
#endif

#if WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_gl2:
		{
			const TimerQueryData& tqd = m_pTimersPool->getQueryData(ix);
			NVSDK_GLFunctions.glQueryCounter(tqd.m_d3d._GL2.m_GLTimerQuery, GL_TIMESTAMP); CHECK_GL_ERRORS;
		}
		break;
#endif

	default:
		// Unexpected API
		return E_FAIL;
	}
#endif // WAVEWORKS_ENABLE_GRAPHICS

	return S_OK;
}

void NVWaveWorks_GFX_Timer_Impl::releaseTimerQuery(int ix)
{
	m_pTimersPool->releaseQuery(ix);
}

HRESULT NVWaveWorks_GFX_Timer_Impl::getTimerQuery(Graphics_Context* pGC, int ix, UINT64& t)
{
	TimerQueryData& tqd = m_pTimersPool->getQueryData(ix);
	if(S_FALSE == tqd.m_status)
	{
		HRESULT hr = E_FAIL;
		UINT64 result = 0;

#if WAVEWORKS_ENABLE_GRAPHICS
		switch(m_d3dAPI)
		{
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			{
				ID3D11DeviceContext* pDC_d3d11 = pGC->d3d11();
				hr = pDC_d3d11->GetData(tqd.m_d3d._11.m_pTimerQuery, &result, sizeof(result), 0);
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			{
				hr = S_OK;
			}
			break;
#endif

#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				NVSDK_GLFunctions.glGetQueryObjectui64v(tqd.m_d3d._GL2.m_GLTimerQuery, GL_QUERY_RESULT_AVAILABLE, &result); CHECK_GL_ERRORS;
				if(result == GL_FALSE)
				{
					hr = S_FALSE;
				}
				else
				{
					NVSDK_GLFunctions.glGetQueryObjectui64v(tqd.m_d3d._GL2.m_GLTimerQuery, GL_QUERY_RESULT, &result); CHECK_GL_ERRORS;
					hr = S_OK;
				}
			}
			break;
#endif

	default:
			{
				// Unexpected API
				hr = E_FAIL;
			}
			break;
		}
#endif // WAVEWORKS_ENABLE_GRAPHICS

		switch(hr)
		{
		case S_FALSE:
			break;
		case S_OK:
			tqd.m_timestampResult = result;
			tqd.m_status = S_OK;
			break;
		default:
			tqd.m_timestampResult = 0;
			tqd.m_status = hr;
			break;
		}
	}

	if(S_FALSE != tqd.m_status)
	{
		t = tqd.m_timestampResult;
	}

	return tqd.m_status;
}

HRESULT NVWaveWorks_GFX_Timer_Impl::getTimerQueries(Graphics_Context* pGC, int ix1, int ix2, UINT64& tdiff)
{
	UINT64 stamp1;
	HRESULT hr1 = getTimerQuery(pGC, ix1, stamp1);
	if(S_FALSE == hr1)
		return S_FALSE;
	UINT64 stamp2;
	HRESULT hr2 = getTimerQuery(pGC, ix2, stamp2);
	if(S_FALSE == hr2)
		return S_FALSE;

	if(S_OK == hr1 && S_OK ==hr2)
	{
		tdiff = stamp2 - stamp1;
		return S_OK;
	}
	else if(S_OK == hr1)
	{
		return hr2;
	}
	else
	{
		return hr1;
	}
}

HRESULT NVWaveWorks_GFX_Timer_Impl::waitTimerQuery(Graphics_Context* pGC, int ix, UINT64& t)
{
	// No built-in sync in DX, roll our own as best we can...
	HRESULT status = S_FALSE;
	do
	{
		status = getTimerQuery(pGC, ix, t);
		if(S_FALSE == status)
		{
			Sleep(0);
		}
	}
	while(S_FALSE == status);

	return status;
}

HRESULT NVWaveWorks_GFX_Timer_Impl::waitTimerQueries(Graphics_Context* pGC, int ix1, int ix2, UINT64& tdiff)
{
	// No built-in sync in DX, roll our own as best we can...
	HRESULT status = S_FALSE;
	do
	{
		status = getTimerQueries(pGC, ix1, ix2, tdiff);
		if(S_FALSE == status)
		{
			Sleep(0);
		}
	}
	while(S_FALSE == status);

	return status;
}

HRESULT NVWaveWorks_GFX_Timer_Impl::beginDisjoint(Graphics_Context* pGC)
{
	if(0 == m_pDisjointTimersPool->getNumInactiveQueries())
	{
		// Add D3D resources
#if WAVEWORKS_ENABLE_GRAPHICS
		switch(m_d3dAPI)
		{

#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			{
				HRESULT hr;
				DisjointQueryData& dqd = m_pDisjointTimersPool->addInactiveQuery();
				D3D11_QUERY_DESC query_desc;
				query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
				query_desc.MiscFlags = 0;
				V_RETURN(m_d3d._11.m_pd3d11Device->CreateQuery(&query_desc, &dqd.m_d3d._11.m_pDisjointTimerQuery));
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			{
				/*DisjointQueryData& dqd = */ m_pDisjointTimersPool->addInactiveQuery();
			}
			break;
#endif

#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				/*DisjointQueryData& dqd =*/ m_pDisjointTimersPool->addInactiveQuery();
				// GL doesn't have disjoint queries atm, so doing nothing
			}
			break;
#endif

		default:
			// Unexpected API
			return E_FAIL;
		}
#endif // WAVEWORKS_ENABLE_GRAPHICS
	}

	// Make an inactive query current
	assert(m_CurrentDisjointTimerQuery == -1);
	m_CurrentDisjointTimerQuery = m_pDisjointTimersPool->activateQuery();

	// Begin the disjoint query
#if WAVEWORKS_ENABLE_GRAPHICS
	switch(m_d3dAPI)
	{
#if WAVEWORKS_ENABLE_D3D11
	case nv_water_d3d_api_d3d11:
		{
			ID3D11DeviceContext* pDC_d3d11 = pGC->d3d11();
			const DisjointQueryData& dqd = m_pDisjointTimersPool->getQueryData(m_CurrentDisjointTimerQuery);
			pDC_d3d11->Begin(dqd.m_d3d._11.m_pDisjointTimerQuery);
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			/*const DisjointQueryData& dqd =*/ m_pDisjointTimersPool->getQueryData(m_CurrentDisjointTimerQuery);
		}
		break;
#endif 

#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				// GL doesn't have disjoint queries atm, so doing nothing
			}
			break;
#endif
	default:
		// Unexpected API
		return E_FAIL;
	}
#endif // WAVEWORKS_ENABLE_GRAPHICS

	return S_OK;
}

HRESULT NVWaveWorks_GFX_Timer_Impl::endDisjoint(Graphics_Context* pGC)
{
	assert(m_CurrentDisjointTimerQuery != -1);

	// End the disjoint query
#if WAVEWORKS_ENABLE_GRAPHICS
	switch(m_d3dAPI)
	{

#if WAVEWORKS_ENABLE_D3D11
	case nv_water_d3d_api_d3d11:
		{
			ID3D11DeviceContext* pDC_d3d11 = pGC->d3d11();
			const DisjointQueryData& dqd = m_pDisjointTimersPool->getQueryData(m_CurrentDisjointTimerQuery);
			pDC_d3d11->End(dqd.m_d3d._11.m_pDisjointTimerQuery);
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			/*const DisjointQueryData& dqd =*/ m_pDisjointTimersPool->getQueryData(m_CurrentDisjointTimerQuery);
        }
		break;
#endif

#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				// GL doesn't have disjoint queries atm, so doing nothing
			}
			break;
#endif

	default:
		// Unexpected API
		return E_FAIL;
	}
#endif // WAVEWORKS_ENABLE_GRAPHICS

	// Release the query (but others may have referenced it by now...)
	m_pDisjointTimersPool->releaseQuery(m_CurrentDisjointTimerQuery);
	m_CurrentDisjointTimerQuery = -1;

	return S_OK;
}

int NVWaveWorks_GFX_Timer_Impl::getCurrentDisjointQuery()
{
	assert(m_CurrentDisjointTimerQuery != -1);

	m_pDisjointTimersPool->addRefQuery(m_CurrentDisjointTimerQuery);	// udpate ref-count
	return m_CurrentDisjointTimerQuery;
}

void NVWaveWorks_GFX_Timer_Impl::releaseDisjointQuery(int ix)
{
	m_pDisjointTimersPool->releaseQuery(ix);
}

HRESULT NVWaveWorks_GFX_Timer_Impl::getDisjointQuery(Graphics_Context* pGC, int ix, UINT64& f)
{
	DisjointQueryData& dqd = m_pDisjointTimersPool->getQueryData(ix);
	if(S_FALSE == dqd.m_status)
	{
		HRESULT hr = E_FAIL;
		BOOL WasDisjoint = FALSE;
		UINT64 RawF = 0;

#if WAVEWORKS_ENABLE_GRAPHICS
		switch(m_d3dAPI)
		{
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			{
				ID3D11DeviceContext* pDC_d3d11 = pGC->d3d11();

				D3D11_QUERY_DATA_TIMESTAMP_DISJOINT result;
				hr = pDC_d3d11->GetData(dqd.m_d3d._11.m_pDisjointTimerQuery, &result, sizeof(result), 0);

				RawF = result.Frequency;
				WasDisjoint = result.Disjoint;
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			{
				hr = S_OK;
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				// GL doesn't have disjoint queries atm, so assuming the queries are not disjoint
				hr = S_OK;
				RawF = 1000000000;
				WasDisjoint = false;
			}
			break;
#endif
		default:
			// Unexpected API
			return E_FAIL;
		}
#endif // WAVEWORKS_ENABLE_GRAPHICS

		switch(hr)
		{
		case S_FALSE:
			break;
		case S_OK:
			dqd.m_freqResult = WasDisjoint ? 0 : RawF;
			dqd.m_status = WasDisjoint ? E_FAIL : S_OK;
			break;
		default:
			dqd.m_freqResult = 0;
			dqd.m_status = hr;
			break;
		}
	}

	if(S_FALSE != dqd.m_status)
	{
		f = dqd.m_freqResult;
	}

	return dqd.m_status;
}

HRESULT NVWaveWorks_GFX_Timer_Impl::waitDisjointQuery(Graphics_Context* pGC, int ix, UINT64& f)
{
	// No built-in sync in DX, roll our own as best we can...
	HRESULT status = S_FALSE;
	do
	{
		status = getDisjointQuery(pGC, ix, f);
		if(S_FALSE == status)
		{
			Sleep(0);
		}
	}
	while(S_FALSE == status);

	return status;
}
