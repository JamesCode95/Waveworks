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
 
#include "..\Internal.h"

#include "GNM_Util.h"
#include <gnm/drawcommandbuffer.h>
#include <gnm/rendertarget.h>
#include <gnm/buffer.h>
using namespace sce;

namespace PSSL
{
	const uint32_t g_NVWaveWorks_SetUintFastComputeShader[] = 
	{
		#include "cs_set_uint_fast_c_cs_gnm.h"
	};
};


namespace
{
	GFSDK_WaveWorks_GnmxWrap* g_pTheGnmxWrap = NULL;
}

namespace GFSDK_WaveWorks_GNM_Util
{
	class RenderTargetClearer
	{
	public:
		RenderTargetClearer(sce::Gnmx::CsShader* pSetUintFastComputeShader, sce::Gnmx::InputResourceOffsets* pSetUintFastComputeShaderResourceOffsets);
		~RenderTargetClearer();

		void ClearMemoryToUints(GFSDK_WaveWorks_GnmxWrap& gnmxWrap, sce::Gnmx::LightweightGfxContext &gfxc, void *destination, uint32_t destUints, uint32_t *source, uint32_t srcUints);

	private:
		sce::Gnmx::CsShader* m_pSetUintFastComputeShader;
		sce::Gnmx::InputResourceOffsets* m_pSetUintFastComputeShaderResourceOffsets;
	};

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

	void synchronizeComputeToGraphics( sce::Gnm::DrawCommandBuffer *dcb )
	{
		volatile uint64_t* label = (volatile uint64_t*)dcb->allocateFromCommandBuffer( sizeof(uint64_t), Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
		*label = 0x0; // set the memory to have the val 0
		dcb->writeAtEndOfShader( Gnm::kEosCsDone, const_cast<uint64_t*>(label), 0x1 ); // tell the CP to write a 1 into the memory only when all compute shaders have finished
		dcb->waitOnAddress( const_cast<uint64_t*>(label), 0xffffffff, Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
		dcb->flushShaderCachesAndWait(Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 0, Gnm::kStallCommandBufferParserDisable); // tell the CP to flush the L1$ and L2$
	}


	void synchronizeComputeToCompute( sce::Gnm::DrawCommandBuffer *dcb )
	{
		volatile uint64_t* label = (volatile uint64_t*)dcb->allocateFromCommandBuffer( sizeof(uint64_t), Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
		*label = 0x0; // set the memory to have the val 0
		dcb->writeAtEndOfShader( Gnm::kEosCsDone, const_cast<uint64_t*>(label), 0x1 ); // tell the CP to write a 1 into the memory only when all compute shaders have finished
		dcb->waitOnAddress( const_cast<uint64_t*>(label), 0xffffffff, Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
		dcb->flushShaderCachesAndWait(Gnm::kCacheActionInvalidateL1, 0, Gnm::kStallCommandBufferParserDisable); // tell the CP to flush the L1$, because presumably the consumers of compute shader output may run on different CUs
	}


	void synchronizeRenderTargetGraphicsToCompute(sce::Gnm::DrawCommandBuffer *dcb, const sce::Gnm::RenderTarget* renderTarget)
	{
		dcb->waitForGraphicsWrites(renderTarget->getBaseAddress256ByteBlocks(), GET_SIZE_IN_BYTES(renderTarget)>>8,
			Gnm::kWaitTargetSlotCb0 | Gnm::kWaitTargetSlotCb1 | Gnm::kWaitTargetSlotCb2 | Gnm::kWaitTargetSlotCb3 |
			Gnm::kWaitTargetSlotCb4 | Gnm::kWaitTargetSlotCb5 | Gnm::kWaitTargetSlotCb6 | Gnm::kWaitTargetSlotCb7,
			Gnm::kCacheActionWriteBackAndInvalidateL1andL2, Gnm::kExtendedCacheActionFlushAndInvalidateCbCache, Gnm::kStallCommandBufferParserDisable);
	}

	struct SurfaceFormatInfo
	{
		Gnm::SurfaceFormat m_format;
		uint8_t m_channels;
		uint8_t m_bitsPerElement;
		uint8_t m_bits[4];
		/* NOT NEEDED (YET)...
		void (*m_encoder)(const SurfaceFormatInfo *restrict info, uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict src, const Gnm::DataFormat dataFormat);
		void (*m_decoder)(const SurfaceFormatInfo *restrict info, Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict src, const Gnm::DataFormat dataFormat);
		uint8_t m_offset[4];
		double m_ooMaxUnormValue[4]; 
		double m_ooMaxSnormValue[4]; 
		inline uint32_t maxUnormValue(uint32_t channel) const {return (uint64_t(1) << (m_bits[channel]-0)) - 1;}
		inline uint32_t maxSnormValue(uint32_t channel) const {return (uint64_t(1) << (m_bits[channel]-1)) - 1;}
		inline  int32_t minSnormValue(uint32_t channel) const {return -maxSnormValue(channel) - 1;}
		*/
	};

	#define NONZERO(X) ((X) ? 1 : 0)
	#define SILENCE_DIVIDE_BY_ZERO_WARNING(X) ((X) ? (X) : 1)
	#define MAXUNORM(X) ((uint64_t(1) << (X))-1)
	#define MAXSNORM(X) (MAXUNORM(X) >> 1)
	#define OOMAXUNORM(X) (1.0 / SILENCE_DIVIDE_BY_ZERO_WARNING(MAXUNORM(X)))
	#define OOMAXSNORM(X) (1.0 / SILENCE_DIVIDE_BY_ZERO_WARNING(MAXSNORM(X)))
	#define DEFINE_SURFACEFORMATINFO(S,X,Y,Z,W,E,D) \
		{(S), NONZERO(X)+NONZERO(Y)+NONZERO(Z)+NONZERO(W), (X)+(Y)+(Z)+(W), {(X), (Y), (Z), (W)} /*, (E), (D), {0, (X), (X)+(Y), (X)+(Y)+(Z)}, {OOMAXUNORM(X), OOMAXUNORM(Y), OOMAXUNORM(Z), OOMAXUNORM(W)}, {OOMAXSNORM(X), OOMAXSNORM(Y), OOMAXSNORM(Z), OOMAXSNORM(W)}*/}

	static const SurfaceFormatInfo g_surfaceFormatInfo[] =
	{
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormatInvalid    ,  0,  0,  0,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat8          ,  8,  0,  0,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat16         , 16,  0,  0,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat8_8        ,  8,  8,  0,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat32         , 32,  0,  0,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat16_16      , 16, 16,  0,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat10_11_11   , 11, 11, 10,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat11_11_10   , 10, 11, 11,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat10_10_10_2 ,  2, 10, 10, 10, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat2_10_10_10 , 10, 10, 10,  2, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat8_8_8_8    ,  8,  8,  8,  8, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat32_32      , 32, 32,  0,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat16_16_16_16, 16, 16, 16, 16, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat32_32_32   , 32, 32, 32,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat32_32_32_32, 32, 32, 32, 32, simpleEncoder, simpleDecoder),
		{},
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat5_6_5      ,  5,  6,  5,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat1_5_5_5    ,  5,  5,  5,  1, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat5_5_5_1    ,  1,  5,  5,  5, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat4_4_4_4    ,  4,  4,  4,  4, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat8_24       , 24,  8,  0,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat24_8       ,  8, 24,  0,  0, simpleEncoder, simpleDecoder),
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormatGB_GR      ,  8,  8,  8,  8, sharedChromaEncoder, sharedChromaDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormatBG_RG      ,  8,  8,  8,  8, sharedChromaEncoder, sharedChromaDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat5_9_9_9    ,  9,  9,  9,  5, sharedExponentEncoder, sharedExponentDecoder),
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat4_4        ,  4,  4,  0,  0, simpleEncoder, simpleDecoder),
		DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat6_5_5      ,  5,  5,  6,  0, simpleEncoder, simpleDecoder), 
	};

	void RenderTargetClearer::ClearMemoryToUints(GFSDK_WaveWorks_GnmxWrap& gnmxWrap, sce::Gnmx::LightweightGfxContext &gfxc, void *destination, uint32_t destUints, uint32_t *source, uint32_t srcUints)
	{
		gnmxWrap.setShaderType(gfxc,Gnm::kShaderTypeCompute);

		const bool srcUintsIsPowerOfTwo = (srcUints & (srcUints-1)) == 0;

		assert(srcUintsIsPowerOfTwo);	// TBD: !srcUintsIsPowerOfTwo
		// gfxc.setCsShader(srcUintsIsPowerOfTwo ? s_set_uint_fast.m_shader : s_set_uint.m_shader);
		gnmxWrap.setCsShader(gfxc, m_pSetUintFastComputeShader, m_pSetUintFastComputeShaderResourceOffsets);

		Gnm::Buffer destinationBuffer;
		destinationBuffer.initAsDataBuffer(destination, Gnm::kDataFormatR32Uint, destUints);
		destinationBuffer.setResourceMemoryType(Gnm::kResourceMemoryTypeGC);
		gnmxWrap.setRwBuffers(gfxc, Gnm::kShaderStageCs, 0, 1, &destinationBuffer);

		Gnm::Buffer sourceBuffer;
		sourceBuffer.initAsDataBuffer(source, Gnm::kDataFormatR32Uint, srcUints);
		sourceBuffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO);
		gnmxWrap.setBuffers(gfxc, Gnm::kShaderStageCs, 0, 1, &sourceBuffer);

		struct Constants
		{
			uint32_t m_destUints;
			uint32_t m_srcUints;
		};
		Constants *constants = (Constants*)gnmxWrap.allocateFromCommandBuffer(gfxc, sizeof(Constants), Gnm::kEmbeddedDataAlignment4);
		constants->m_destUints = destUints;
		constants->m_srcUints = srcUints - (srcUintsIsPowerOfTwo ? 1 : 0);
		Gnm::Buffer constantBuffer;
		constantBuffer.initAsConstantBuffer(constants, sizeof(*constants));
		gnmxWrap.setConstantBuffers(gfxc, Gnm::kShaderStageCs, 0, 1, &constantBuffer);

		gnmxWrap.dispatch(gfxc, (destUints + Gnm::kThreadsPerWavefront - 1) / Gnm::kThreadsPerWavefront, 1, 1);

		synchronizeComputeToGraphics(gnmxWrap.getDcb(gfxc));
		gnmxWrap.setShaderType(gfxc, Gnm::kShaderTypeGraphics);
	}

	void ClearRenderTargetToZero(RenderTargetClearer* pRTC, sce::Gnmx::LightweightGfxContext &gfxc, const sce::Gnm::RenderTarget* renderTarget)
	{
		GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();

		uint32_t *source = static_cast<uint32_t*>(gnmxWrap->allocateFromCommandBuffer(gfxc, sizeof(uint32_t) * 4, Gnm::kEmbeddedDataAlignment4));
		source[0] = source[1] = source[2] = source[3] = 0;

		const Gnm::DataFormat dataFormat = renderTarget->getDataFormat();
		Gnm::SurfaceFormat surfaceFormat = dataFormat.getSurfaceFormat();
		assert(surfaceFormat < sizeof(g_surfaceFormatInfo)/sizeof(g_surfaceFormatInfo[0]));
		SCE_GNM_UNUSED(surfaceFormat);
		const SurfaceFormatInfo *info = &g_surfaceFormatInfo[dataFormat.getSurfaceFormat()];
		assert(info->m_format == surfaceFormat);

		const uint32_t num_dwords = info->m_bitsPerElement <= 32 ? 1 : info->m_bitsPerElement / 32;
		pRTC->ClearMemoryToUints(*gnmxWrap, gfxc, renderTarget->getBaseAddress(), GET_SIZE_IN_BYTES(renderTarget) / sizeof(uint32_t), source, num_dwords);
	}

	RenderTargetClearer* CreateRenderTargetClearer()
	{
		sce::Gnmx::CsShader* pSetUintFastComputeShader = CreateCsShader(PSSL::g_NVWaveWorks_SetUintFastComputeShader);
		if(NULL == pSetUintFastComputeShader)
			return NULL;

		sce::Gnmx::InputResourceOffsets* iros = CreateInputResourceOffsets(Gnm::kShaderStageCs,pSetUintFastComputeShader);
		if(NULL == iros)
		{
			ReleaseCsShader(pSetUintFastComputeShader);
			return NULL;
		}

		RenderTargetClearer* pResult = new RenderTargetClearer(pSetUintFastComputeShader, iros);
		return pResult;
	}

	////////////////////////////////////////////////////////////////////////
	// 
	// ...end functions adapted from Orbis SDK Toolkit
	//
	////////////////////////////////////////////////////////////////////////

	Gnmx::PsShader* CreatePsShader(const uint32_t* shaderData)
	{
		GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();

		sce::Gnmx::ShaderInfo* pShaderInfo = (sce::Gnmx::ShaderInfo*)alloca(gnmxWrap->getSizeofShaderInfo());
		gnmxWrap->parseShader(*pShaderInfo, shaderData, gnmxWrap->getPsShaderType());

		void *shaderBinary = NVSDK_garlic_malloc(gnmxWrap->getGpuShaderCodeSize(*pShaderInfo), Gnm::kAlignmentOfShaderInBytes);
		void* shaderHeader = NVSDK_aligned_malloc(gnmxWrap->computeSize(*gnmxWrap->getPsShader(*pShaderInfo)), Gnm::kAlignmentOfBufferInBytes);

		memcpy(shaderBinary, gnmxWrap->getGpuShaderCode(*pShaderInfo), gnmxWrap->getGpuShaderCodeSize(*pShaderInfo));
		memcpy(shaderHeader, gnmxWrap->getPsShader(*pShaderInfo), gnmxWrap->computeSize(*gnmxWrap->getPsShader(*pShaderInfo)));

		Gnmx::PsShader* pResult = static_cast<Gnmx::PsShader*>(shaderHeader);
		gnmxWrap->patchShaderGpuAddress(*pResult, shaderBinary);

		return pResult;
	}

	void ReleasePsShader(Gnmx::PsShader*& psShader)
	{
		if(psShader)
		{
			GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();
			uintptr_t addr = gnmxWrap->getPsStageRegisters(*psShader).m_spiShaderPgmLoPs;
			NVSDK_garlic_free((void*)(addr << 8));
			NVSDK_aligned_free(psShader);
			psShader = NULL;
		}
	}

	Gnmx::CsShader* CreateCsShader(const uint32_t* shaderData)
	{
		GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();

		sce::Gnmx::ShaderInfo* pShaderInfo = (sce::Gnmx::ShaderInfo*)alloca(gnmxWrap->getSizeofShaderInfo());
		gnmxWrap->parseShader(*pShaderInfo, shaderData, gnmxWrap->getCsShaderType());

		void *shaderBinary = NVSDK_garlic_malloc(gnmxWrap->getGpuShaderCodeSize(*pShaderInfo), Gnm::kAlignmentOfShaderInBytes);
		void* shaderHeader = NVSDK_aligned_malloc(gnmxWrap->computeSize(*gnmxWrap->getCsShader(*pShaderInfo)), Gnm::kAlignmentOfBufferInBytes);

		memcpy(shaderBinary, gnmxWrap->getGpuShaderCode(*pShaderInfo), gnmxWrap->getGpuShaderCodeSize(*pShaderInfo));
		memcpy(shaderHeader, gnmxWrap->getCsShader(*pShaderInfo), gnmxWrap->computeSize(*gnmxWrap->getCsShader(*pShaderInfo)));

		Gnmx::CsShader* pResult = static_cast<Gnmx::CsShader*>(shaderHeader);
		gnmxWrap->patchShaderGpuAddress(*pResult, shaderBinary);

		return pResult;
	}

	void ReleaseCsShader(Gnmx::CsShader*& csShader)
	{
		if(csShader)
		{
			GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();
			uintptr_t addr = gnmxWrap->getCsStageRegisters(*csShader).m_computePgmLo;
			NVSDK_garlic_free((void*)(addr << 8));
			NVSDK_aligned_free(csShader);
			csShader = NULL;
		}
	}

	Gnmx::VsShader* CreateVsMakeFetchShader(void*& fetchShader, const uint32_t* shaderData)
	{
		GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();

		sce::Gnmx::ShaderInfo* pShaderInfo = (sce::Gnmx::ShaderInfo*)alloca(gnmxWrap->getSizeofShaderInfo());
		gnmxWrap->parseShader(*pShaderInfo, shaderData, gnmxWrap->getVsShaderType());

		void *shaderBinary = NVSDK_garlic_malloc(gnmxWrap->getGpuShaderCodeSize(*pShaderInfo), Gnm::kAlignmentOfShaderInBytes);
		void* shaderHeader = NVSDK_aligned_malloc(gnmxWrap->computeSize(*gnmxWrap->getVsShader(*pShaderInfo)), Gnm::kAlignmentOfBufferInBytes);

		memcpy(shaderBinary, gnmxWrap->getGpuShaderCode(*pShaderInfo), gnmxWrap->getGpuShaderCodeSize(*pShaderInfo));
		memcpy(shaderHeader, gnmxWrap->getVsShader(*pShaderInfo), gnmxWrap->computeSize(*gnmxWrap->getVsShader(*pShaderInfo)));

		Gnmx::VsShader* pResult = static_cast<Gnmx::VsShader*>(shaderHeader);
		gnmxWrap->patchShaderGpuAddress(*pResult, shaderBinary);

		// VF is done by a separate shader
		fetchShader = NVSDK_garlic_malloc(gnmxWrap->computeVsFetchShaderSize(gnmxWrap->getVsShader(*pShaderInfo)), Gnm::kAlignmentOfBufferInBytes);
		uint32_t shaderModifier;
		gnmxWrap->generateVsFetchShader(fetchShader, &shaderModifier, pResult, NULL); 
		gnmxWrap->applyFetchShaderModifier(*pResult, shaderModifier);

		return pResult;
	}

	void ReleaseVsShader(Gnmx::VsShader*& vsShader, void*& fetchShader)
	{
		if(vsShader)
		{
			GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();
			uintptr_t addr = gnmxWrap->getVsStageRegisters(*vsShader).m_spiShaderPgmLoVs;
			NVSDK_garlic_free((void*)(addr << 8));
			NVSDK_aligned_free(vsShader);
			vsShader = NULL;
		}

		if(fetchShader)
		{
			NVSDK_garlic_free(fetchShader);
			fetchShader = NULL;
		}
	}


	sce::Gnmx::InputResourceOffsets* CreateInputResourceOffsets(sce::Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct)
	{
		GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();

		sce::Gnmx::InputResourceOffsets* pResult = (sce::Gnmx::InputResourceOffsets*)NVSDK_aligned_malloc(gnmxWrap->getSizeofInputResourceOffsets(),  Gnm::kAlignmentOfBufferInBytes);
		gnmxWrap->generateInputResourceOffsetTable(pResult, shaderStage, gnmxShaderStruct);

		return pResult;
	}

	void ReleaseInputResourceOffsets(sce::Gnmx::InputResourceOffsets*& iros)
	{
		if(iros)
		{
			NVSDK_aligned_free(iros);
			iros = NULL;
		}
	}


	RenderTargetClearer::RenderTargetClearer(sce::Gnmx::CsShader* pSetUintFastComputeShader, sce::Gnmx::InputResourceOffsets* pSetUintFastComputeShaderResourceOffsets) :
		m_pSetUintFastComputeShader(pSetUintFastComputeShader),
		m_pSetUintFastComputeShaderResourceOffsets(pSetUintFastComputeShaderResourceOffsets)
	{
	}

	RenderTargetClearer::~RenderTargetClearer()
	{
		ReleaseCsShader(m_pSetUintFastComputeShader);
		ReleaseInputResourceOffsets(m_pSetUintFastComputeShaderResourceOffsets);
	}

	void ReleaseRenderTargetClearer(RenderTargetClearer*& pRTC)
	{
		if(pRTC)
		{
			delete pRTC;
			pRTC = NULL;
		}
	}

	void setGnmxWrap(GFSDK_WaveWorks_GnmxWrap* pTheGnmxWrap)
	{
		assert((NULL == g_pTheGnmxWrap && NULL != pTheGnmxWrap) || (NULL != g_pTheGnmxWrap && NULL == pTheGnmxWrap));
		g_pTheGnmxWrap = pTheGnmxWrap;
	}

	GFSDK_WaveWorks_GnmxWrap* getGnmxWrap()
	{
		assert(g_pTheGnmxWrap != NULL);
		return g_pTheGnmxWrap;
	}
}