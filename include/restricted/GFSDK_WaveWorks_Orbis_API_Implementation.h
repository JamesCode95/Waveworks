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
 * This header defines a default implementation for GnmxWrap which can be
 * trivially #included in the client's compilation space to handle Gnmx calls
 */

#ifndef _GFSDK_WAVEWORKS_ORBIS_API_IMPLEMENTATION_H
#define _GFSDK_WAVEWORKS_ORBIS_API_IMPLEMENTATION_H

#include <GFSDK_WaveWorks_Orbis_API_Interface.h>

#include <gnmx/lwgfxcontext.h>
#include <gnmx/shader_parser.h>

struct GFSDK_WaveWorks_GnmxWrapImplementation : public GFSDK_WaveWorks_GnmxWrap
{
	virtual void setShaderType(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderType shaderType)
	{ gfxc.setShaderType(shaderType); }

	virtual void setCsShader(sce::Gnmx::LightweightGfxContext& gfxc, const sce::Gnmx::CsShader *csb, sce::Gnmx::InputResourceOffsets* csros)
	{ gfxc.setCsShader(csb, csros); }

	virtual void setRwBuffers(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Buffer *rwBuffers)
	{ gfxc.setRwBuffers(stage, startSlot, numSlots, rwBuffers); }

	virtual void setBuffers(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Buffer *buffers)
	{ gfxc.setBuffers(stage, startSlot, numSlots, buffers); }

	virtual void setConstantBuffers(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Buffer *buffers)
	{ gfxc.setConstantBuffers(stage, startSlot, numSlots, buffers); }

	virtual void dispatch(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
	{ gfxc.dispatch(threadGroupX, threadGroupY, threadGroupZ); }

	virtual void * allocateFromCommandBuffer(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t sizeInBytes, sce::Gnm::EmbeddedDataAlignment alignment)
	{ return gfxc.allocateFromCommandBuffer(sizeInBytes, alignment); }

	virtual void setVertexBuffers(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Buffer *buffers)
	{ gfxc.setVertexBuffers(stage, startSlot, numSlots, buffers); }

	virtual void setPrimitiveType(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::PrimitiveType primType)
	{ gfxc.setPrimitiveType(primType); }

	virtual void setIndexSize(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::IndexSize indexSize)
	{ gfxc.setIndexSize(indexSize); }

	virtual void setIndexCount(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t indexCount)
	{ gfxc.setIndexCount(indexCount); }

	virtual void setIndexOffset(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t offset)
	{ gfxc.setIndexOffset(offset); }

	virtual void setIndexBuffer(sce::Gnmx::LightweightGfxContext& gfxc, const void * indexAddr)
	{ gfxc.setIndexBuffer(indexAddr); }

	virtual void drawIndexOffset(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t indexOffset, uint32_t indexCount)
	{ gfxc.drawIndexOffset(indexOffset, indexCount); }

	virtual void pushMarker(sce::Gnmx::LightweightGfxContext& gfxc, const char * debugString)
	{ gfxc.pushMarker(debugString); }

	virtual void popMarker(sce::Gnmx::LightweightGfxContext& gfxc)
	{ gfxc.popMarker(); }

	virtual void setActiveShaderStages(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ActiveShaderStages activeStages)
	{ gfxc.setActiveShaderStages(activeStages); }

	virtual void setVsShader(sce::Gnmx::LightweightGfxContext& gfxc, const sce::Gnmx::VsShader *vsb, uint32_t shaderModifier, void *fetchShaderAddr, sce::Gnmx::InputResourceOffsets* vsros)
	{ gfxc.setVsShader(vsb, shaderModifier, fetchShaderAddr, vsros); }

	virtual void setPsShader(sce::Gnmx::LightweightGfxContext& gfxc, const sce::Gnmx::PsShader *psb, sce::Gnmx::InputResourceOffsets* psros)
	{ gfxc.setPsShader(psb, psros); }

	virtual void setRenderTarget(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t rtSlot, sce::Gnm::RenderTarget const *target)
	{ gfxc.setRenderTarget(rtSlot, target); }

	virtual void setDepthRenderTarget(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::DepthRenderTarget const * depthTarget)
	{ gfxc.setDepthRenderTarget(depthTarget); }

	virtual void setupScreenViewport(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, float zScale, float zOffset)
	{ gfxc.setupScreenViewport(left, top, right, bottom, zScale, zOffset); }

	virtual void setTextures(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Texture *textures)
	{ gfxc.setTextures(stage, startSlot, numSlots, textures); }

	virtual void setSamplers(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Sampler *samplers)
	{ gfxc.setSamplers(stage, startSlot, numSlots, samplers); }

	virtual void setDepthStencilControl(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::DepthStencilControl depthControl)
	{ gfxc.setDepthStencilControl(depthControl); }

	virtual void setBlendControl(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t rtSlot, sce::Gnm::BlendControl blendControl) 
	{ gfxc.setBlendControl(rtSlot, blendControl); }

	virtual void setPrimitiveSetup(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::PrimitiveSetup reg)
	{ gfxc.setPrimitiveSetup(reg); }

	virtual void setRenderTargetMask(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t mask)
	{ gfxc.setRenderTargetMask(mask); }

	virtual void waitForGraphicsWrites(sce::Gnmx::LightweightGfxContext& gfxc, uint32_t baseAddr256, uint32_t sizeIn256ByteBlocks, uint32_t targetMask, sce::Gnm::CacheAction cacheAction, uint32_t extendedCacheMask, sce::Gnm::StallCommandBufferParserMode commandBufferStallMode)
	{ gfxc.waitForGraphicsWrites(baseAddr256, sizeIn256ByteBlocks, targetMask, cacheAction, extendedCacheMask, commandBufferStallMode); }

	virtual void setRwTextures(sce::Gnmx::LightweightGfxContext& gfxc, sce::Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const sce::Gnm::Texture *rwTextures)
	{ gfxc.setRwTextures(stage, startSlot, numSlots, rwTextures); }

	virtual void generateInputResourceOffsetTable(sce::Gnmx::InputResourceOffsets* outTable, sce::Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct)
	{ sce::Gnmx::generateInputResourceOffsetTable(outTable, shaderStage, gnmxShaderStruct); }

	virtual void patchShaderGpuAddress(sce::Gnmx::PsShader& psShader, void *gpuAddress)
	{ psShader.patchShaderGpuAddress(gpuAddress); }

	virtual void patchShaderGpuAddress(sce::Gnmx::CsShader& csShader, void *gpuAddress)
	{ csShader.patchShaderGpuAddress(gpuAddress); }

	virtual void patchShaderGpuAddress(sce::Gnmx::VsShader& vsShader, void *gpuAddress)
	{ vsShader.patchShaderGpuAddress(gpuAddress); }

	virtual void applyFetchShaderModifier(sce::Gnmx::VsShader& vsShader, uint32_t shaderModifier)
	{ vsShader.applyFetchShaderModifier(shaderModifier); }

	virtual const sce::Gnm::PsStageRegisters& getPsStageRegisters(sce::Gnmx::PsShader& psShader)
	{ return psShader.m_psStageRegisters; }

	virtual const sce::Gnm::CsStageRegisters& getCsStageRegisters(sce::Gnmx::CsShader& csShader)
	{ return csShader.m_csStageRegisters; }

	virtual const sce::Gnm::VsStageRegisters& getVsStageRegisters(sce::Gnmx::VsShader& vsShader)
	{ return vsShader.m_vsStageRegisters; }

	virtual uint32_t computeVsFetchShaderSize(const sce::Gnmx::VsShader *vsb)
	{ return sce::Gnmx::computeVsFetchShaderSize(vsb); }

	virtual void generateVsFetchShader(void *fs, uint32_t* shaderModifier, const sce::Gnmx::VsShader *vsb, const sce::Gnm::FetchShaderInstancingMode *instancingData)
	{ return sce::Gnmx::generateVsFetchShader(fs, shaderModifier, vsb, instancingData); }

	virtual uint32_t computeSize(sce::Gnmx::PsShader& psShader)
	{ return psShader.computeSize(); }

	virtual uint32_t computeSize(sce::Gnmx::VsShader& vsShader)
	{ return vsShader.computeSize(); }

	virtual uint32_t computeSize(sce::Gnmx::CsShader& csShader)
	{ return csShader.computeSize(); }

	virtual void parseShader(sce::Gnmx::ShaderInfo& shaderInfo, const void* data, int /* deprecated: shaderType*/)
	{ sce::Gnmx::parseShader(&shaderInfo, data); }

	virtual uint32_t getGpuShaderCodeSize(sce::Gnmx::ShaderInfo& shaderInfo)
	{ return shaderInfo.m_gpuShaderCodeSize; }

	virtual const uint32_t* getGpuShaderCode(sce::Gnmx::ShaderInfo& shaderInfo)
	{ return shaderInfo.m_gpuShaderCode; }

	virtual sce::Gnmx::PsShader* getPsShader(sce::Gnmx::ShaderInfo& shaderInfo)
	{ return shaderInfo.m_psShader; }

	virtual sce::Gnmx::CsShader* getCsShader(sce::Gnmx::ShaderInfo& shaderInfo)
	{ return shaderInfo.m_csShader; }

	virtual sce::Gnmx::VsShader* getVsShader(sce::Gnmx::ShaderInfo& shaderInfo)
	{ return shaderInfo.m_vsShader; }

	// Synthesised wrappers/accessors
	virtual sce::Gnm::DrawCommandBuffer* getDcb(sce::Gnmx::LightweightGfxContext& gfxc)
	{ return &gfxc.m_dcb; }

	virtual int getVsShaderType()
	{ return sce::Gnmx::kVertexShader; }

	virtual int getPsShaderType()
	{ return sce::Gnmx::kPixelShader; }

	virtual int getCsShaderType()
	{ return sce::Gnmx::kComputeShader; }

	virtual size_t getSizeofShaderInfo()
	{ return sizeof(sce::Gnmx::ShaderInfo); }

	virtual size_t getSizeofInputResourceOffsets()
	{ return sizeof(sce::Gnmx::InputResourceOffsets); }
};

#endif	// _GFSDK_WAVEWORKS_ORBIS_API_IMPLEMENTATION_H
