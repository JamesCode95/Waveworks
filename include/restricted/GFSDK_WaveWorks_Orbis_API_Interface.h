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

/*
 * This header defines ABC's for indirecting WaveWorks access to Gnmx
 * The raison d'etre of this header is to avoid generating linkage into Gnmx inside
 * WaveWorks. By generating linkage outside of the lib (i.e. in the client app), it
 * is possible for clients to use customised versions of Gnmx
 */

#ifndef _GFSDK_WAVEWORKS_ORBIS_API_INTERFACE_H
#define _GFSDK_WAVEWORKS_ORBIS_API_INTERFACE_H

#include <GFSDK_WaveWorks_Common.h>
#include <GFSDK_WaveWorks_Types.h>

#include <gnm/constants.h>

namespace sce {
	namespace Gnmx {
		class LightweightGfxContext; 
		class ShaderInfo;
		class CsShader;
		class PsShader;
		class VsShader;
		struct InputResourceOffsets;
	}
	namespace Gnm {
		class Buffer;
		class DrawCommandBuffer;
		class RenderTarget;
		class DepthRenderTarget;
		class Texture;
		class Sampler;
		class DepthStencilControl;
		class BlendControl;
		class PrimitiveSetup;
		class PsStageRegisters;
		class CsStageRegisters;
		class VsStageRegisters;
	}
}

	struct GFSDK_WaveWorks_GnmxWrap
	{
		// LightweightGfxContext wrappers
		virtual void setShaderType(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderType shaderType) = 0;
		virtual void setCsShader(sce::Gnmx::LightweightGfxContext& gfxc, const sce::Gnmx::CsShader *csb, sce::Gnmx::InputResourceOffsets* csros) = 0;
		virtual void setRwBuffers(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Buffer *rwBuffers) = 0;
		virtual void setBuffers(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Buffer *buffers) = 0;
		virtual void setConstantBuffers(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Buffer *buffers) = 0;
		virtual void dispatch(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) = 0;
		virtual void * allocateFromCommandBuffer(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t sizeInBytes, sce::Gnm::EmbeddedDataAlignment alignment) = 0;
		virtual void setVertexBuffers(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Buffer *buffers) = 0;
		virtual void setPrimitiveType(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::PrimitiveType primType) = 0;
		virtual void setIndexSize(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::IndexSize indexSize) = 0;
		virtual void setIndexCount(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t indexCount) = 0;
		virtual void setIndexOffset(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t offset) = 0;
		virtual void setIndexBuffer(sce::Gnmx::LightweightGfxContext& gfxc, const void * indexAddr) = 0;
		virtual void drawIndexOffset(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t indexOffset, uint32_t indexCount) = 0;
		virtual void pushMarker(sce::Gnmx::LightweightGfxContext& gfxc, const char * debugString) = 0;
		virtual void popMarker(sce::Gnmx::LightweightGfxContext& gfxc) = 0;
		virtual void setActiveShaderStages(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ActiveShaderStages activeStages) = 0;
		virtual void setVsShader(sce::Gnmx::LightweightGfxContext& gfxc, const sce::Gnmx::VsShader *vsb, uint32_t shaderModifier, void *fetchShaderAddr, sce::Gnmx::InputResourceOffsets* vsros) = 0;
		virtual void setPsShader(sce::Gnmx::LightweightGfxContext& gfxc, const sce::Gnmx::PsShader *psb, sce::Gnmx::InputResourceOffsets* psros) = 0;
		virtual void setRenderTarget(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t rtSlot, sce::Gnm::RenderTarget const *target) = 0;
		virtual void setDepthRenderTarget(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::DepthRenderTarget const * depthTarget) = 0;
		virtual void setupScreenViewport(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, float zScale, float zOffset) = 0;
		virtual void setTextures(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Texture *textures) = 0;
		virtual void setSamplers(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Sampler *samplers) = 0;
		virtual void setDepthStencilControl(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::DepthStencilControl depthControl) = 0;
		virtual void setBlendControl(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t rtSlot, sce::Gnm::BlendControl blendControl) = 0;
		virtual void setPrimitiveSetup(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::PrimitiveSetup reg) = 0;
		virtual void setRenderTargetMask(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t mask) = 0;
		virtual void waitForGraphicsWrites(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t baseAddr256, uint32_t sizeIn256ByteBlocks, uint32_t targetMask, sce::Gnm::CacheAction cacheAction, uint32_t extendedCacheMask, sce::Gnm::StallCommandBufferParserMode commandBufferStallMode) = 0;
		virtual void setRwTextures(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Texture *rwTextures) = 0;

		// Shader wrappers
		virtual void patchShaderGpuAddress(sce::Gnmx::PsShader& psShader, void *gpuAddress) = 0;
		virtual void patchShaderGpuAddress(sce::Gnmx::CsShader& csShader, void *gpuAddress) = 0;
		virtual void patchShaderGpuAddress(sce::Gnmx::VsShader& vsShader, void *gpuAddress) = 0;
		virtual void applyFetchShaderModifier(sce::Gnmx::VsShader& vsShader, uint32_t shaderModifier) = 0;
		virtual const sce::Gnm::PsStageRegisters& getPsStageRegisters(sce::Gnmx::PsShader& psShader) = 0;
		virtual const sce::Gnm::CsStageRegisters& getCsStageRegisters(sce::Gnmx::CsShader& csShader) = 0;
		virtual const sce::Gnm::VsStageRegisters& getVsStageRegisters(sce::Gnmx::VsShader& vsShader) = 0;
		virtual uint32_t computeVsFetchShaderSize(const sce::Gnmx::VsShader *vsb) = 0;
		virtual void generateVsFetchShader(void *fs, uint32_t* shaderModifier, const sce::Gnmx::VsShader *vsb, const sce::Gnm::FetchShaderInstancingMode *instancingData) = 0;
		virtual uint32_t computeSize(sce::Gnmx::PsShader& psShader) = 0;
		virtual uint32_t computeSize(sce::Gnmx::VsShader& vsShader) = 0;
		virtual uint32_t computeSize(sce::Gnmx::CsShader& csShader) = 0;

		// ShaderInfo wrappers
		virtual void parseShader(sce::Gnmx::ShaderInfo& shaderInfo, const void* data, int shaderType) = 0;
		virtual uint32_t getGpuShaderCodeSize(sce::Gnmx::ShaderInfo& shaderInfo) = 0;
		virtual const uint32_t* getGpuShaderCode(sce::Gnmx::ShaderInfo& shaderInfo) = 0;
		virtual sce::Gnmx::PsShader* getPsShader(sce::Gnmx::ShaderInfo& shaderInfo) = 0;
		virtual sce::Gnmx::CsShader* getCsShader(sce::Gnmx::ShaderInfo& shaderInfo) = 0;
		virtual sce::Gnmx::VsShader* getVsShader(sce::Gnmx::ShaderInfo& shaderInfo) = 0;

		// InputResourceOffsets wrappers
		virtual void generateInputResourceOffsetTable(sce::Gnmx::InputResourceOffsets* outTable, sce::Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct) = 0;

		// Synthesised wrappers/accessors
		virtual sce::Gnm::DrawCommandBuffer* getDcb(sce::Gnmx::LightweightGfxContext& gfxc) = 0;
		virtual int getVsShaderType() = 0;
		virtual int getPsShaderType() = 0;
		virtual int getCsShaderType() = 0;
		virtual size_t getSizeofShaderInfo() = 0;
		virtual size_t getSizeofInputResourceOffsets() = 0;
	};

#endif	// _GFSDK_WAVEWORKS_ORBIS_API_INTERFACE_H
