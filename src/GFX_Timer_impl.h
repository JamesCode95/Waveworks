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

#ifndef _NVWAVEWORKS_GFX_TIMER_IMPL_H
#define _NVWAVEWORKS_GFX_TIMER_IMPL_H

class GFSDK_WaveWorks_GFX_DisjointQuery_Pool_Impl;
class GFSDK_WaveWorks_GFX_TimerQuery_Pool_Impl;

class NVWaveWorks_GFX_Timer_Impl
{
public:

	NVWaveWorks_GFX_Timer_Impl();
	~NVWaveWorks_GFX_Timer_Impl();

    HRESULT initD3D9(IDirect3DDevice9* pD3DDevice);
    HRESULT initD3D10(ID3D10Device* pD3DDevice);
	HRESULT initD3D11(ID3D11Device* pD3DDevice);
	HRESULT initGnm();
	HRESULT initGL2(void* pGLContext);

	// Timer queries wrapper
	HRESULT issueTimerQuery(Graphics_Context* pGC, int& ix);
	void releaseTimerQuery(int ix);
	HRESULT waitTimerQuery(Graphics_Context* pGC, int ix, UINT64& t);
	HRESULT getTimerQuery(Graphics_Context* pGC, int ix, UINT64& t);

	// Pair-wise get/wait
	HRESULT getTimerQueries(Graphics_Context* pGC, int ix1, int ix2, UINT64& tdiff);
	HRESULT waitTimerQueries(Graphics_Context* pGC, int ix1, int ix2, UINT64& tdiff);

	// Disjoint queries wrapper
	HRESULT beginDisjoint(Graphics_Context* pGC);
	HRESULT endDisjoint(Graphics_Context* pGC);
	int getCurrentDisjointQuery();
	void releaseDisjointQuery(int ix);
	HRESULT waitDisjointQuery(Graphics_Context* pGC, int ix, UINT64& f);
	HRESULT getDisjointQuery(Graphics_Context* pGC, int ix, UINT64& f);

	enum { InvalidQueryIndex = -1 };

private:

	HRESULT allocateAllResources();
	void releaseAllResources();
	void releaseAll();

	GFSDK_WaveWorks_GFX_DisjointQuery_Pool_Impl* m_pDisjointTimersPool;
	int m_CurrentDisjointTimerQuery;

	GFSDK_WaveWorks_GFX_TimerQuery_Pool_Impl* m_pTimersPool;

	// D3D API handling
	nv_water_d3d_api m_d3dAPI;

#if WAVEWORKS_ENABLE_D3D9
    struct D3D9Objects
    {
		IDirect3DDevice9* m_pd3d9Device;
	};
#endif

#if WAVEWORKS_ENABLE_D3D10
    struct D3D10Objects
    {
		ID3D10Device* m_pd3d10Device;
	};
#endif

#if WAVEWORKS_ENABLE_D3D11
    struct D3D11Objects
    {
		ID3D11Device* m_pd3d11Device;
	};
#endif

#if WAVEWORKS_ENABLE_GNM
	struct GnmObjects
	{
	};
#endif
#if WAVEWORKS_ENABLE_GL
	struct GL2Objects
	{
		void* m_pGLContext;
	};
#endif
	union
    {
#if WAVEWORKS_ENABLE_D3D9
		D3D9Objects _9;
#endif
#if WAVEWORKS_ENABLE_D3D10
		D3D10Objects _10;
#endif
#if WAVEWORKS_ENABLE_D3D11
		D3D11Objects _11;
#endif
#if WAVEWORKS_ENABLE_GNM
		GnmObjects _gnm;
#endif
#if WAVEWORKS_ENABLE_GL
		GL2Objects _GL2;
#endif
    } m_d3d;
};

#endif	// _NVWAVEWORKS_GFX_TIMER_IMPL_H
