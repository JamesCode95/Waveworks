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

#ifndef _NVWAVEWORKS_GRAPHICS_CONTEXT_H
#define _NVWAVEWORKS_GRAPHICS_CONTEXT_H

#include "Internal.h"

namespace WaveWorks_Internal
{
	class Graphics_Context
	{
	public:

		Graphics_Context(ID3D11DeviceContext* pDC) :
			m_gfxAPI(nv_water_d3d_api_d3d11),
			m_ctx(pDC)
		{
		}

	#if WAVEWORKS_ENABLE_D3D11
		ID3D11DeviceContext* d3d11() const
		{
			assert(nv_water_d3d_api_d3d11 == m_gfxAPI);
			return m_ctx._d3d11;
		}
	#endif

		Graphics_Context(sce::Gnmx::LightweightGfxContext* pGC) :
			m_gfxAPI(nv_water_d3d_api_gnm),
			m_ctx(pGC)
		{
		}

	#if WAVEWORKS_ENABLE_GNM
		sce::Gnmx::LightweightGfxContext* gnm() const
		{
			assert(nv_water_d3d_api_gnm == m_gfxAPI);
			return m_ctx._gnm;
		}
	#endif

	private:

		nv_water_d3d_api m_gfxAPI;

		union Ctx
		{
			Ctx(ID3D11DeviceContext* pDC) : _d3d11(pDC) {}
			ID3D11DeviceContext* _d3d11;

			Ctx(sce::Gnmx::LightweightGfxContext* pGC) : _gnm(pGC) {}
			sce::Gnmx::LightweightGfxContext* _gnm;

		} m_ctx;

	};
}

#endif	// _NVWAVEWORKS_GRAPHICS_CONTEXT_H
