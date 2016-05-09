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

#ifndef _GNM_UTIL_H
#define _GNM_UTIL_H

#include <sdk_version.h>

#if SCE_ORBIS_SDK_VERSION < (0x01700000u)
#define DEPRICATED_IN_1_7( oo ) ,oo
#define NEW_IN_1_7( oo )
#else
#define DEPRICATED_IN_1_7( oo )
#define NEW_IN_1_7( oo ) ,oo
#endif

namespace sce
{
	// Forward Declarations
	namespace Gnm
	{
		class DrawCommandBuffer;
		class RenderTarget;
	}

	namespace Gnmx
	{
		class VsShader;
		class PsShader;
		class CsShader;
	}
}

struct GFSDK_WaveWorks_GnmxWrap;

namespace GFSDK_WaveWorks_GNM_Util
{
	////////////////////////////////////////////////////////////////////////
	// 
	// Begin functions adapted from Orbis SDK Toolkit -
	// - .\samples\sample_code\graphics\api_gnm\toolkit
	// ...
	//
	////////////////////////////////////////////////////////////////////////

	/* SCE CONFIDENTIAL
	PlayStation(R)4 Programmer Tool Runtime Library Release 01.500.111
	* Copyright (C) 2013 Sony Computer Entertainment Inc.
	* All Rights Reserved.
	*/

	void synchronizeComputeToGraphics( sce::Gnm::DrawCommandBuffer *dcb );
	void synchronizeComputeToCompute( sce::Gnm::DrawCommandBuffer *dcb );
	void synchronizeRenderTargetGraphicsToCompute(sce::Gnm::DrawCommandBuffer *dcb, const sce::Gnm::RenderTarget* renderTarget);


	////////////////////////////////////////////////////////////////////////
	// 
	// ...end functions adapted from Orbis SDK Toolkit
	//
	////////////////////////////////////////////////////////////////////////

	sce::Gnmx::VsShader* CreateVsMakeFetchShader(void*& fetchShader, const uint32_t* shaderData);
	void ReleaseVsShader(sce::Gnmx::VsShader*& vsShader, void*& fetchShader);

	sce::Gnmx::PsShader* CreatePsShader(const uint32_t* shaderData);
	void ReleasePsShader(sce::Gnmx::PsShader*& psShader);

	sce::Gnmx::CsShader* CreateCsShader(const uint32_t* shaderData);
	void ReleaseCsShader(sce::Gnmx::CsShader*& csShader);

	class RenderTargetClearer;
	RenderTargetClearer* CreateRenderTargetClearer();
	void ClearRenderTargetToZero(RenderTargetClearer* pRTC, sce::Gnmx::LightweightGfxContext &gfxc, const sce::Gnm::RenderTarget* renderTarget);
	void ReleaseRenderTargetClearer(RenderTargetClearer*& pRTC);

	sce::Gnmx::InputResourceOffsets* CreateInputResourceOffsets(sce::Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct);
	void ReleaseInputResourceOffsets(sce::Gnmx::InputResourceOffsets*& iros);

	void setGnmxWrap(GFSDK_WaveWorks_GnmxWrap*);
	GFSDK_WaveWorks_GnmxWrap* getGnmxWrap();
}

#endif	// _GNM_UTIL_H
