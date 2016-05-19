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
#include "D3DX_replacement_code.h"
#include "Simulation_impl.h"
#include "Simulation_Util.h"
#include "Savestate_impl.h"
#include "FFT_Simulation.h"
#include "GFX_Timer_impl.h"
#include "Graphics_Context.h"
#ifdef SUPPORT_CUDA
#include "FFT_Simulation_Manager_CUDA_impl.h"
#endif
#ifdef SUPPORT_DIRECTCOMPUTE
#include "FFT_Simulation_Manager_DirectCompute_impl.h"
#endif
#ifdef SUPPORT_FFTCPU
#include "FFT_Simulation_Manager_CPU_impl.h"
#endif

#include <string.h>

#if WAVEWORKS_ENABLE_GNM
#include "orbis\GNM_Util.h"
using namespace sce;
#endif

namespace {
#if WAVEWORKS_ENABLE_GRAPHICS
// The contents of Attributes_map.h are generated somewhat indiscriminately, so
// use a pragma to suppress fluffy warnings under gcc
	#ifdef __GNUC__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-variable"
	#endif

	#ifdef __GNUC__
		#pragma GCC diagnostic pop
	#endif

#endif
#if WAVEWORKS_ENABLE_D3D9
namespace CalcGradient {
	#include "CalcGradient_ps_3_0.h"
	#include "CalcGradient_vs_3_0.h"
}
namespace FoamGeneration {
	#include "FoamGeneration_ps_3_0.h"
	#include "FoamGeneration_vs_3_0.h"
}
#endif

namespace SM4 {
#if WAVEWORKS_ENABLE_D3D10 || WAVEWORKS_ENABLE_D3D11
namespace CalcGradient {
	#include "CalcGradient_ps_4_0.h"
	#include "CalcGradient_vs_4_0.h"
}
namespace FoamGeneration {
	#include "FoamGeneration_ps_4_0.h"
	#include "FoamGeneration_vs_4_0.h"
}
#endif
}

#if WAVEWORKS_ENABLE_GNM
namespace PSSL
{
const uint32_t g_NVWaveWorks_CalcGradientPixelShader[] = 
{
#include "CalcGradient_ps_gnm.h"
};

const uint32_t g_NVWaveWorks_CalcGradientVertexShader[] = 
{
#include "CalcGradient_vs_gnm.h"
};

const uint32_t g_NVWaveWorks_FoamGenerationPixelShader[] = 
{
#include "FoamGeneration_ps_gnm.h"
};

const uint32_t g_NVWaveWorks_FoamGenerationVertexShader[] = 
{
#include "FoamGeneration_vs_gnm.h"
};

const uint32_t g_NVWaveWorks_MipMapGenerationComputeShader[] = 
{
#include "MipMapGeneration_cs_gnm.h"
};

}
#endif

	namespace GL {

	#if WAVEWORKS_ENABLE_GL

	const char* k_NVWaveWorks_CalcGradientVertexShader =
	#include "CalcGradient_glsl_vs.h"
	;

	const char* k_NVWaveWorks_CalcGradientFragmentShader =
	#include "CalcGradient_glsl_ps.h"
	;

	const char* k_NVWaveWorks_FoamGenerationVertexShader =
	#include "FoamGeneration_glsl_vs.h"
	;

	const char* k_NVWaveWorks_FoamGenerationFragmentShader =
	#include "FoamGeneration_glsl_ps.h"
	;

	#endif
	}

}

#if defined(TARGET_PLATFORM_MICROSOFT)
const DXGI_SAMPLE_DESC kNoSample = {1, 0};
#endif

#if WAVEWORKS_ENABLE_GL
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#endif

enum ShaderInputsD3D9
{
    ShaderInputD3D9_g_samplerDisplacementMap0 = 0,
    ShaderInputD3D9_g_samplerDisplacementMap1,
    ShaderInputD3D9_g_samplerDisplacementMap2,
    ShaderInputD3D9_g_samplerDisplacementMap3,
    ShaderInputD3D9_g_samplerGradientMap0,
    ShaderInputD3D9_g_samplerGradientMap1,
    ShaderInputD3D9_g_samplerGradientMap2,
    ShaderInputD3D9_g_samplerGradientMap3,
    ShaderInputD3D9_g_WorldEye,
    ShaderInputD3D9_g_Pad1,
	ShaderInputD3D9_g_UVScaleCascade0123,
    ShaderInputD3D9_g_TexelLength_x2_PS,
    ShaderInputD3D9_g_Cascade1Scale_PS,
    ShaderInputD3D9_g_Cascade1TexelScale_PS,
    ShaderInputD3D9_g_Cascade1UVOffset_PS,
    ShaderInputD3D9_g_Cascade2Scale_PS,
    ShaderInputD3D9_g_Cascade2TexelScale_PS,
    ShaderInputD3D9_g_Cascade2UVOffset_PS,
    ShaderInputD3D9_g_Cascade3Scale_PS,
    ShaderInputD3D9_g_Cascade3TexelScale_PS,
    ShaderInputD3D9_g_Cascade3UVOffset_PS,

    NumShaderInputsD3D9
};

enum ShaderInputsD3D10
{
    ShaderInputD3D10_vs_buffer = 0,
    ShaderInputD3D10_g_samplerDisplacementMap0,
    ShaderInputD3D10_g_samplerDisplacementMap1,
	ShaderInputD3D10_g_samplerDisplacementMap2,
	ShaderInputD3D10_g_samplerDisplacementMap3,
    ShaderInputD3D10_g_textureDisplacementMap0,
    ShaderInputD3D10_g_textureDisplacementMap1,
	ShaderInputD3D10_g_textureDisplacementMap2,
	ShaderInputD3D10_g_textureDisplacementMap3,
    ShaderInputD3D10_ps_buffer,
    ShaderInputD3D10_g_samplerGradientMap0,
    ShaderInputD3D10_g_samplerGradientMap1,
	ShaderInputD3D10_g_samplerGradientMap2,
	ShaderInputD3D10_g_samplerGradientMap3,
    ShaderInputD3D10_g_textureGradientMap0,
    ShaderInputD3D10_g_textureGradientMap1,
	ShaderInputD3D10_g_textureGradientMap2,
	ShaderInputD3D10_g_textureGradientMap3,

    NumShaderInputsD3D10
};

enum ShaderInputsD3D11
{
    ShaderInputD3D11_vs_buffer = 0,
    ShaderInputD3D11_vs_g_samplerDisplacementMap0,
    ShaderInputD3D11_vs_g_samplerDisplacementMap1,
	ShaderInputD3D11_vs_g_samplerDisplacementMap2,
	ShaderInputD3D11_vs_g_samplerDisplacementMap3,
    ShaderInputD3D11_vs_g_textureDisplacementMap0,
    ShaderInputD3D11_vs_g_textureDisplacementMap1,
	ShaderInputD3D11_vs_g_textureDisplacementMap2,
	ShaderInputD3D11_vs_g_textureDisplacementMap3,
    ShaderInputD3D11_ds_buffer,
    ShaderInputD3D11_ds_g_samplerDisplacementMap0,
    ShaderInputD3D11_ds_g_samplerDisplacementMap1,
	ShaderInputD3D11_ds_g_samplerDisplacementMap2,
	ShaderInputD3D11_ds_g_samplerDisplacementMap3,
    ShaderInputD3D11_ds_g_textureDisplacementMap0,
    ShaderInputD3D11_ds_g_textureDisplacementMap1,
	ShaderInputD3D11_ds_g_textureDisplacementMap2,
	ShaderInputD3D11_ds_g_textureDisplacementMap3,
    ShaderInputD3D11_ps_buffer,
    ShaderInputD3D11_g_samplerGradientMap0,
    ShaderInputD3D11_g_samplerGradientMap1,
	ShaderInputD3D11_g_samplerGradientMap2,
	ShaderInputD3D11_g_samplerGradientMap3,
    ShaderInputD3D11_g_textureGradientMap0,
    ShaderInputD3D11_g_textureGradientMap1,
	ShaderInputD3D11_g_textureGradientMap2,
	ShaderInputD3D11_g_textureGradientMap3,

    NumShaderInputsD3D11
};

enum ShaderInputsGnm
{
	ShaderInputGnm_vs_buffer = 0,
	ShaderInputGnm_vs_g_samplerDisplacementMap0,
	ShaderInputGnm_vs_g_samplerDisplacementMap1,
	ShaderInputGnm_vs_g_samplerDisplacementMap2,
	ShaderInputGnm_vs_g_samplerDisplacementMap3,
	ShaderInputGnm_vs_g_textureDisplacementMap0,
	ShaderInputGnm_vs_g_textureDisplacementMap1,
	ShaderInputGnm_vs_g_textureDisplacementMap2,
	ShaderInputGnm_vs_g_textureDisplacementMap3,
	ShaderInputGnm_ds_buffer,
	ShaderInputGnm_ds_g_samplerDisplacementMap0,
	ShaderInputGnm_ds_g_samplerDisplacementMap1,
	ShaderInputGnm_ds_g_samplerDisplacementMap2,
	ShaderInputGnm_ds_g_samplerDisplacementMap3,
	ShaderInputGnm_ds_g_textureDisplacementMap0,
	ShaderInputGnm_ds_g_textureDisplacementMap1,
	ShaderInputGnm_ds_g_textureDisplacementMap2,
	ShaderInputGnm_ds_g_textureDisplacementMap3,
	ShaderInputGnm_ps_buffer,
	ShaderInputGnm_g_samplerGradientMap0,
	ShaderInputGnm_g_samplerGradientMap1,
	ShaderInputGnm_g_samplerGradientMap2,
	ShaderInputGnm_g_samplerGradientMap3,
	ShaderInputGnm_g_textureGradientMap0,
	ShaderInputGnm_g_textureGradientMap1,
	ShaderInputGnm_g_textureGradientMap2,
	ShaderInputGnm_g_textureGradientMap3,

	NumShaderInputsGnm
};
enum ShaderInputsGL2
{
    ShaderInputGL2_g_textureBindLocationDisplacementMap0 = 0,
    ShaderInputGL2_g_textureBindLocationDisplacementMap1,
    ShaderInputGL2_g_textureBindLocationDisplacementMap2,
    ShaderInputGL2_g_textureBindLocationDisplacementMap3,
    ShaderInputGL2_g_textureBindLocationGradientMap0,
    ShaderInputGL2_g_textureBindLocationGradientMap1,
    ShaderInputGL2_g_textureBindLocationGradientMap2,
    ShaderInputGL2_g_textureBindLocationGradientMap3,
    ShaderInputGL2_g_textureBindLocationDisplacementMapArray,
    ShaderInputGL2_g_textureBindLocationGradientMapArray,
	ShaderInputGL2_g_WorldEye,
    ShaderInputGL2_g_UseTextureArrays,
	ShaderInputGL2_g_UVScaleCascade0123,
    ShaderInputGL2_g_TexelLength_x2_PS,
    ShaderInputGL2_g_Cascade1Scale_PS,
    ShaderInputGL2_g_Cascade1TexelScale_PS,
    ShaderInputGL2_g_Cascade1UVOffset_PS,
    ShaderInputGL2_g_Cascade2Scale_PS,
    ShaderInputGL2_g_Cascade2TexelScale_PS,
    ShaderInputGL2_g_Cascade2UVOffset_PS,
    ShaderInputGL2_g_Cascade3Scale_PS,
    ShaderInputGL2_g_Cascade3TexelScale_PS,
    ShaderInputGL2_g_Cascade3UVOffset_PS,
    NumShaderInputsGL2
};
// NB: These should be kept synchronisd with the shader source
#if WAVEWORKS_ENABLE_D3D9
const GFSDK_WaveWorks_ShaderInput_Desc ShaderInputDescsD3D9[NumShaderInputsD3D9] = {
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap3", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap1", 1 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap2", 2 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap3", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_FloatConstant, "nvsf_g_WorldEye", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_FloatConstant, "nvsf_g_Pad1", 1 },
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_FloatConstant, "nvsf_g_UVScaleCascade0123", 2 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant, "nvsf_g_TexelLength_x2_PS", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant, "nvsf_g_Cascade1Scale_PS", 1 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant, "nvsf_g_Cascade1TexelScale_PS", 2 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant, "nvsf_g_Cascade1UVOffset_PS", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant, "nvsf_g_Cascade2Scale_PS", 4 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant, "nvsf_g_Cascade2TexelScale_PS", 5 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant, "nvsf_g_Cascade2UVOffset_PS", 6 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant, "nvsf_g_Cascade3Scale_PS", 7 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant, "nvsf_g_Cascade3TexelScale_PS", 8 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant, "nvsf_g_Cascade3UVOffset_PS", 9 },
};
#endif // WAVEWORKS_ENABLE_D3D9

#if WAVEWORKS_ENABLE_D3D10
const GFSDK_WaveWorks_ShaderInput_Desc ShaderInputDescsD3D10[NumShaderInputsD3D10] = {
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_ConstantBuffer, "nvsf_attr_vs_buffer", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap3", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap3", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_ConstantBuffer, "nvsf_attr_ps_buffer", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap3", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap3", 3 }
};
#endif // WAVEWORKS_ENABLE_D3D10

#if WAVEWORKS_ENABLE_D3D11
const GFSDK_WaveWorks_ShaderInput_Desc ShaderInputDescsD3D11[NumShaderInputsD3D11] = {
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_ConstantBuffer, "nvsf_attr_vs_buffer", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap3", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap3", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_ConstantBuffer, "nvsf_attr_vs_buffer", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Sampler, "nvsf_g_samplerDisplacementMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Sampler, "nvsf_g_samplerDisplacementMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Sampler, "nvsf_g_samplerDisplacementMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Sampler, "nvsf_g_samplerDisplacementMap3", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Texture, "nvsf_g_textureDisplacementMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Texture, "nvsf_g_textureDisplacementMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Texture, "nvsf_g_textureDisplacementMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Texture, "nvsf_g_textureDisplacementMap3", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_ConstantBuffer, "nvsf_attr_ps_buffer", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap3", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap3", 3 }
};
#endif // WAVEWORKS_ENABLE_D3D11

#if WAVEWORKS_ENABLE_GNM
const GFSDK_WaveWorks_ShaderInput_Desc ShaderInputDescsGnm[NumShaderInputsGnm] = {
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_ConstantBuffer, "nvsf_attr_vs_buffer", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap0", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler, "nvsf_g_samplerDisplacementMap3", 3 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap0", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture, "nvsf_g_textureDisplacementMap3", 3 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_ConstantBuffer, "nvsf_attr_vs_buffer", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Sampler, "nvsf_g_samplerDisplacementMap0", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Sampler, "nvsf_g_samplerDisplacementMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Sampler, "nvsf_g_samplerDisplacementMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Sampler, "nvsf_g_samplerDisplacementMap3", 3 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Texture, "nvsf_g_textureDisplacementMap0", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Texture, "nvsf_g_textureDisplacementMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Texture, "nvsf_g_textureDisplacementMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Texture, "nvsf_g_textureDisplacementMap3", 3 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_ConstantBuffer, "nvsf_attr_ps_buffer", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap0", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler, "nvsf_g_samplerGradientMap3", 3 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap0", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap1", 1 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap2", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture, "nvsf_g_textureGradientMap3", 3 }
};
#endif // WAVEWORKS_ENABLE_GNM
#if WAVEWORKS_ENABLE_GL
const GFSDK_WaveWorks_ShaderInput_Desc ShaderInputDescsGL2[NumShaderInputsGL2] = {
	{ GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_TextureBindLocation, "nvsf_g_samplerDisplacementMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_TextureBindLocation, "nvsf_g_samplerDisplacementMap1", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_TextureBindLocation, "nvsf_g_samplerDisplacementMap2", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_TextureBindLocation, "nvsf_g_samplerDisplacementMap3", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_TextureBindLocation, "nvsf_g_samplerGradientMap0", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_TextureBindLocation, "nvsf_g_samplerGradientMap1", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_TextureBindLocation, "nvsf_g_samplerGradientMap2", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_TextureBindLocation, "nvsf_g_samplerGradientMap3", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_TextureArrayBindLocation, "nvsf_g_samplerDisplacementMapTextureArray", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_TextureArrayBindLocation, "nvsf_g_samplerGradientMapTextureArray", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_UniformLocation, "nvsf_g_WorldEye", 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_UniformLocation, "nvsf_g_UseTextureArrays", 1 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_UniformLocation, "nvsf_g_UVScaleCascade0123", 2 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_UniformLocation, "nvsf_g_TexelLength_x2_PS", 0 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_UniformLocation, "nvsf_g_Cascade1Scale_PS", 1 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_UniformLocation, "nvsf_g_Cascade1TexelScale_PS", 2 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_UniformLocation, "nvsf_g_Cascade1UVOffset_PS", 3 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_UniformLocation, "nvsf_g_Cascade2Scale_PS", 4 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_UniformLocation, "nvsf_g_Cascade2TexelScale_PS", 5 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_UniformLocation, "nvsf_g_Cascade2UVOffset_PS", 6 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_UniformLocation, "nvsf_g_Cascade3Scale_PS", 7 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_UniformLocation, "nvsf_g_Cascade3TexelScale_PS", 8 },
    { GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_UniformLocation, "nvsf_g_Cascade3UVOffset_PS", 9 },
};
#endif // __GL__
struct ps_calcgradient_cbuffer
{
    float g_ChoppyScale;
    float g_GradMap2TexelWSScale;
    float pad1;
	float pad2;

    gfsdk_float4 g_OneTexel_Left;
    gfsdk_float4 g_OneTexel_Right;
    gfsdk_float4 g_OneTexel_Back;
    gfsdk_float4 g_OneTexel_Front;
};

struct vs_attr_cbuffer
{
    float g_WorldEye[3];
	    float pad1;
    float g_UVScaleCascade0123[4];

};

struct ps_foamgeneration_cbuffer
{
	float nvsf_g_DissipationFactors_BlurExtents;
	float nvsf_g_DissipationFactors_Fadeout;
	float nvsf_g_DissipationFactors_Accumulation;
	float nvsf_g_FoamGenerationThreshold;
    gfsdk_float4 g_SourceComponents;
    gfsdk_float4 g_UVOffsets;
};

typedef vs_attr_cbuffer vs_ds_attr_cbuffer;

struct ps_attr_cbuffer
{
    float g_TexelLength_x2_PS;
    float g_Cascade1Scale_PS;
    float g_Cascade1TexelScale_PS;
    float g_Cascade1UVOffset_PS;
    float g_Cascade2Scale_PS;
    float g_Cascade2TexelScale_PS;
    float g_Cascade2UVOffset_PS;
    float g_Cascade3Scale_PS;
    float g_Cascade3TexelScale_PS;
    float g_Cascade3UVOffset_PS;
	float pad1;
	float pad2;
};

void GFSDK_WaveWorks_Simulation::TimerPool::reset()
{
	m_active_timer_slot = 0;
	m_end_inflight_timer_slots = 1;
	memset(m_timer_slots, 0, sizeof(m_timer_slots));
}

GFSDK_WaveWorks_Simulation::GFSDK_WaveWorks_Simulation()
{
    for(int i = 0; i != GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades; ++i)
    {
		cascade_states[i].m_pQuadMesh	= NULL;
        cascade_states[i].m_pFFTSimulation	= NULL;
		cascade_states[i].m_gradient_map_version = GFSDK_WaveWorks_InvalidKickID;
        memset(&cascade_states[i].m_d3d, 0, sizeof(cascade_states[i].m_d3d));
    }

    m_dSimTime = 0.f;
	m_numValidEntriesInSimTimeFIFO = 0;
	m_pSimulationManager = NULL;
	m_pOptionalScheduler = NULL;
	m_pGFXTimer = NULL;

    memset(&m_params, 0, sizeof(m_params));
    memset(&m_d3d, 0, sizeof(m_d3d));

    m_d3dAPI = nv_water_d3d_api_undefined;

	m_num_GPU_slots = 1;
	m_active_GPU_slot = 0;

	m_gpu_kick_timers.reset();
	m_gpu_wait_timers.reset();

	m_has_consumed_wait_timer_slot_since_last_kick = false;
}

GFSDK_WaveWorks_Simulation::~GFSDK_WaveWorks_Simulation()
{
    releaseAll();
}

void GFSDK_WaveWorks_Simulation::releaseAll()
{
    if(nv_water_d3d_api_undefined == m_d3dAPI)
        return;

    for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
    {
		releaseRenderingResources(cascade);
		releaseSimulation(cascade);
    }

	releaseGFXTimer();
	releaseSimulationManager();

	m_pOptionalScheduler = NULL;

    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        {
            SAFE_RELEASE(m_d3d._9.m_pd3d9GradCalcVS);
            SAFE_RELEASE(m_d3d._9.m_pd3d9GradCalcPS);
			SAFE_RELEASE(m_d3d._9.m_pd3d9FoamGenPS);
			SAFE_RELEASE(m_d3d._9.m_pd3d9FoamGenVS);
			SAFE_RELEASE(m_d3d._9.m_pd3d9Device);

            m_d3dAPI = nv_water_d3d_api_undefined;
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
            SAFE_RELEASE(m_d3d._10.m_pd3d10GradCalcVS);
            SAFE_RELEASE(m_d3d._10.m_pd3d10GradCalcPS);
            SAFE_RELEASE(m_d3d._10.m_pd3d10GradCalcPixelShaderCB);
            SAFE_RELEASE(m_d3d._10.m_pd3d10FoamGenVS);
            SAFE_RELEASE(m_d3d._10.m_pd3d10FoamGenPS);
            SAFE_RELEASE(m_d3d._10.m_pd3d10FoamGenPixelShaderCB);
            SAFE_RELEASE(m_d3d._10.m_pd3d10PointSampler);
            SAFE_RELEASE(m_d3d._10.m_pd3d10NoDepthStencil);
            SAFE_RELEASE(m_d3d._10.m_pd3d10AlwaysSolidRasterizer);
            SAFE_RELEASE(m_d3d._10.m_pd3d10CalcGradBlendState);
			SAFE_RELEASE(m_d3d._10.m_pd3d10AccumulateFoamBlendState);
			SAFE_RELEASE(m_d3d._10.m_pd3d10WriteAccumulatedFoamBlendState);
            SAFE_RELEASE(m_d3d._10.m_pd3d10LinearNoMipSampler);
            SAFE_RELEASE(m_d3d._10.m_pd3d10GradMapSampler);
            SAFE_RELEASE(m_d3d._10.m_pd3d10PixelShaderCB);
            SAFE_RELEASE(m_d3d._10.m_pd3d10VertexShaderCB);
            SAFE_RELEASE(m_d3d._10.m_pd3d10Device);

            m_d3dAPI = nv_water_d3d_api_undefined;
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
            SAFE_RELEASE(m_d3d._11.m_pd3d11GradCalcVS);
            SAFE_RELEASE(m_d3d._11.m_pd3d11GradCalcPS);
            SAFE_RELEASE(m_d3d._11.m_pd3d11GradCalcPixelShaderCB);
            SAFE_RELEASE(m_d3d._11.m_pd3d11FoamGenVS);
            SAFE_RELEASE(m_d3d._11.m_pd3d11FoamGenPS);
            SAFE_RELEASE(m_d3d._11.m_pd3d11FoamGenPixelShaderCB);
            SAFE_RELEASE(m_d3d._11.m_pd3d11PointSampler);
            SAFE_RELEASE(m_d3d._11.m_pd3d11NoDepthStencil);
            SAFE_RELEASE(m_d3d._11.m_pd3d11AlwaysSolidRasterizer);
            SAFE_RELEASE(m_d3d._11.m_pd3d11CalcGradBlendState);
			SAFE_RELEASE(m_d3d._11.m_pd3d11AccumulateFoamBlendState);
			SAFE_RELEASE(m_d3d._11.m_pd3d11WriteAccumulatedFoamBlendState);
            SAFE_RELEASE(m_d3d._11.m_pd3d11LinearNoMipSampler);
            SAFE_RELEASE(m_d3d._11.m_pd3d11GradMapSampler);
            SAFE_RELEASE(m_d3d._11.m_pd3d11PixelShaderCB);
            SAFE_RELEASE(m_d3d._11.m_pd3d11VertexDomainShaderCB);
            SAFE_RELEASE(m_d3d._11.m_pd3d11Device);

            m_d3dAPI = nv_water_d3d_api_undefined;
        }
        break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			GFSDK_WaveWorks_GNM_Util::ReleaseVsShader(m_d3d._gnm.m_pGnmGradCalcVS, m_d3d._gnm.m_pGnmGradCalcFS);
			GFSDK_WaveWorks_GNM_Util::ReleaseInputResourceOffsets(m_d3d._gnm.m_pGnmGradCalcVSResourceOffsets);
			GFSDK_WaveWorks_GNM_Util::ReleasePsShader(m_d3d._gnm.m_pGnmGradCalcPS);
			GFSDK_WaveWorks_GNM_Util::ReleaseInputResourceOffsets(m_d3d._gnm.m_pGnmGradCalcPSResourceOffsets);
			GFSDK_WaveWorks_GNM_Util::ReleaseVsShader(m_d3d._gnm.m_pGnmFoamGenVS, m_d3d._gnm.m_pGnmFoamGenFS);
			GFSDK_WaveWorks_GNM_Util::ReleaseInputResourceOffsets(m_d3d._gnm.m_pGnmFoamGenVSResourceOffsets);
			GFSDK_WaveWorks_GNM_Util::ReleasePsShader(m_d3d._gnm.m_pGnmFoamGenPS);
			GFSDK_WaveWorks_GNM_Util::ReleaseInputResourceOffsets(m_d3d._gnm.m_pGnmFoamGenPSResourceOffsets);
			GFSDK_WaveWorks_GNM_Util::ReleaseCsShader(m_d3d._gnm.m_pGnmMipMapGenCS);
			GFSDK_WaveWorks_GNM_Util::ReleaseInputResourceOffsets(m_d3d._gnm.m_pGnmMipMapGenCSResourceOffsets);
			GFSDK_WaveWorks_GNM_Util::ReleaseRenderTargetClearer(m_d3d._gnm.m_pGnmRenderTargetClearer);

			NVSDK_free(m_d3d._gnm.m_pGnmPixelShaderCB.getBaseAddress());
			NVSDK_free(m_d3d._gnm.m_pGnmVertexDomainShaderCB.getBaseAddress());

			m_d3dAPI = nv_water_d3d_api_undefined;
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GL
    case nv_water_d3d_api_gl2:
        {
			if(m_d3d._GL2.m_GradCalcProgram != 0) NVSDK_GLFunctions.glDeleteProgram(m_d3d._GL2.m_GradCalcProgram); CHECK_GL_ERRORS;
			if(m_d3d._GL2.m_FoamGenProgram != 0) NVSDK_GLFunctions.glDeleteProgram(m_d3d._GL2.m_FoamGenProgram); CHECK_GL_ERRORS;
			if(m_d3d._GL2.m_DisplacementsTextureArray != 0) NVSDK_GLFunctions.glDeleteTextures(1, &m_d3d._GL2.m_DisplacementsTextureArray); CHECK_GL_ERRORS;
			if(m_d3d._GL2.m_GradientsTextureArray != 0) NVSDK_GLFunctions.glDeleteTextures(1, &m_d3d._GL2.m_GradientsTextureArray); CHECK_GL_ERRORS;
			if(m_d3d._GL2.m_TextureArraysBlittingDrawFBO != 0) NVSDK_GLFunctions.glDeleteFramebuffers(1, &m_d3d._GL2.m_TextureArraysBlittingDrawFBO); CHECK_GL_ERRORS;
			if(m_d3d._GL2.m_TextureArraysBlittingReadFBO != 0) NVSDK_GLFunctions.glDeleteFramebuffers(1, &m_d3d._GL2.m_TextureArraysBlittingReadFBO); CHECK_GL_ERRORS;
            m_d3dAPI = nv_water_d3d_api_undefined;
        }
        break;
#endif
	case nv_water_d3d_api_none:
        {
            m_d3dAPI = nv_water_d3d_api_undefined;
        }
        break;
	default:
		break;
    }
}

HRESULT GFSDK_WaveWorks_Simulation::initGradMapSamplers()
{
#if WAVEWORKS_ENABLE_GRAPHICS
    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
	case nv_water_d3d_api_d3d9:
		{
		}
		break;
#endif 
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
			HRESULT hr;
            SAFE_RELEASE(m_d3d._10.m_pd3d10GradMapSampler);
            D3D10_SAMPLER_DESC anisoSamplerDesc;
            anisoSamplerDesc.Filter = m_params.aniso_level > 1 ? D3D10_FILTER_ANISOTROPIC : D3D10_FILTER_MIN_MAG_MIP_LINEAR;
            anisoSamplerDesc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;
            anisoSamplerDesc.AddressV = D3D10_TEXTURE_ADDRESS_WRAP;
            anisoSamplerDesc.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
            anisoSamplerDesc.MipLODBias = 0.f;
            anisoSamplerDesc.MaxAnisotropy = m_params.aniso_level;
            anisoSamplerDesc.ComparisonFunc = D3D10_COMPARISON_NEVER;
            anisoSamplerDesc.BorderColor[0] = 0.f;
            anisoSamplerDesc.BorderColor[1] = 0.f;
            anisoSamplerDesc.BorderColor[2] = 0.f;
            anisoSamplerDesc.BorderColor[3] = 0.f;
            anisoSamplerDesc.MinLOD = 0.f;
            anisoSamplerDesc.MaxLOD = FLT_MAX;
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateSamplerState(&anisoSamplerDesc, &m_d3d._10.m_pd3d10GradMapSampler));
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			HRESULT hr;
            SAFE_RELEASE(m_d3d._11.m_pd3d11GradMapSampler);
            D3D11_SAMPLER_DESC anisoSamplerDesc;
            anisoSamplerDesc.Filter = m_params.aniso_level > 1 ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            anisoSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            anisoSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            anisoSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            anisoSamplerDesc.MipLODBias = 0.f;
            anisoSamplerDesc.MaxAnisotropy = m_params.aniso_level;
            anisoSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            anisoSamplerDesc.BorderColor[0] = 0.f;
            anisoSamplerDesc.BorderColor[1] = 0.f;
            anisoSamplerDesc.BorderColor[2] = 0.f;
            anisoSamplerDesc.BorderColor[3] = 0.f;
            anisoSamplerDesc.MinLOD = 0.f;
            anisoSamplerDesc.MaxLOD = FLT_MAX;
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateSamplerState(&anisoSamplerDesc, &m_d3d._11.m_pd3d11GradMapSampler));

#ifdef TARGET_PLATFORM_XBONE 
			ID3D11DeviceX* pD3DDevX = NULL;
			hr = m_d3d._11.m_pd3d11Device->QueryInterface(IID_ID3D11DeviceX,(void**)&pD3DDevX);

			if(SUCCEEDED(hr))
			{
				// True fact: the Xbone docs recommends doing it this way... (!)
				//
				// "The easiest way to determine how to fill in all of the many confusing fields of D3D11X_SAMPLER_DESC
				// is to use CreateSamplerState to create the closest Direct3D equivalent, call GetDescX to get back the
				// corresponding D3D11X_SAMPLER_DESC structure, override the appropriate fields, and then call CreateSamplerStateX. 
				//
				D3D11X_SAMPLER_DESC anisoSamplerDescX;
				m_d3d._11.m_pd3d11GradMapSampler->GetDescX(&anisoSamplerDescX);
				anisoSamplerDescX.PerfMip = 10;	// Determined empirically at this stage
				SAFE_RELEASE(m_d3d._11.m_pd3d11GradMapSampler);
				V_RETURN(pD3DDevX->CreateSamplerStateX(&anisoSamplerDescX, &m_d3d._11.m_pd3d11GradMapSampler));
				SAFE_RELEASE(pD3DDevX);
			}
#endif // TARGET_PLATFORM_XBONE

        }
        break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			m_d3d._gnm.m_pGnmGradMapSampler.init();
			m_d3d._gnm.m_pGnmGradMapSampler.setMipFilterMode(Gnm::kMipFilterModeLinear);
			Gnm::FilterMode filterMode = m_params.aniso_level > 1 ? Gnm::kFilterModeAnisoBilinear : Gnm::kFilterModeBilinear;
			m_d3d._gnm.m_pGnmGradMapSampler.setXyFilterMode(filterMode, filterMode);
			m_d3d._gnm.m_pGnmGradMapSampler.setWrapMode(Gnm::kWrapModeWrap, Gnm::kWrapModeWrap, Gnm::kWrapModeWrap);
			int ratio = 0;
			for(int level = m_params.aniso_level; level > 1 && ratio < Gnm::kAnisotropyRatio16; level >>= 1)
				++ratio;
			m_d3d._gnm.m_pGnmGradMapSampler.setAnisotropyRatio(Gnm::AnisotropyRatio(ratio));
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_gl2:
		{
			// nothing to do here
		}
		break;
#endif
	case nv_water_d3d_api_none:
		break;
	default:
		// Unexpected API
		return E_FAIL;
	}

#endif // WAVEWORKS_ENABLE_GRAPHICS

    return S_OK;
}

HRESULT GFSDK_WaveWorks_Simulation::initShaders()
{
#if WAVEWORKS_ENABLE_GRAPHICS
    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        {
			HRESULT hr;
            SAFE_RELEASE(m_d3d._9.m_pd3d9GradCalcVS);
            SAFE_RELEASE(m_d3d._9.m_pd3d9GradCalcPS);
            SAFE_RELEASE(m_d3d._9.m_pd3d9FoamGenVS);
            SAFE_RELEASE(m_d3d._9.m_pd3d9FoamGenPS);
            V_RETURN(m_d3d._9.m_pd3d9Device->CreateVertexShader((DWORD*)CalcGradient::g_vs30_vs, &m_d3d._9.m_pd3d9GradCalcVS));
            V_RETURN(m_d3d._9.m_pd3d9Device->CreatePixelShader((DWORD*)CalcGradient::g_ps30_ps, &m_d3d._9.m_pd3d9GradCalcPS));
			V_RETURN(m_d3d._9.m_pd3d9Device->CreateVertexShader((DWORD*)FoamGeneration::g_vs30_vs, &m_d3d._9.m_pd3d9FoamGenVS));
			V_RETURN(m_d3d._9.m_pd3d9Device->CreatePixelShader((DWORD*)FoamGeneration::g_ps30_ps, &m_d3d._9.m_pd3d9FoamGenPS));
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
			HRESULT hr;
            SAFE_RELEASE(m_d3d._10.m_pd3d10GradCalcVS);
            SAFE_RELEASE(m_d3d._10.m_pd3d10GradCalcPS);
            SAFE_RELEASE(m_d3d._10.m_pd3d10FoamGenVS);
            SAFE_RELEASE(m_d3d._10.m_pd3d10FoamGenPS);
			V_RETURN(m_d3d._10.m_pd3d10Device->CreateVertexShader((void*)SM4::CalcGradient::g_vs, sizeof(SM4::CalcGradient::g_vs), &m_d3d._10.m_pd3d10GradCalcVS));
            V_RETURN(m_d3d._10.m_pd3d10Device->CreatePixelShader((void*)SM4::CalcGradient::g_ps, sizeof(SM4::CalcGradient::g_ps), &m_d3d._10.m_pd3d10GradCalcPS));
			V_RETURN(m_d3d._10.m_pd3d10Device->CreateVertexShader((void*)SM4::FoamGeneration::g_vs, sizeof(SM4::FoamGeneration::g_vs), &m_d3d._10.m_pd3d10FoamGenVS));
			V_RETURN(m_d3d._10.m_pd3d10Device->CreatePixelShader((void*)SM4::FoamGeneration::g_ps, sizeof(SM4::FoamGeneration::g_ps), &m_d3d._10.m_pd3d10FoamGenPS));

            D3D10_BUFFER_DESC cbDesc;
            cbDesc.ByteWidth = sizeof(ps_calcgradient_cbuffer);
            cbDesc.Usage = D3D10_USAGE_DEFAULT;
            cbDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
            cbDesc.CPUAccessFlags = 0;
            cbDesc.MiscFlags = 0;
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateBuffer(&cbDesc, NULL, &m_d3d._10.m_pd3d10GradCalcPixelShaderCB));

            cbDesc.ByteWidth = sizeof(ps_foamgeneration_cbuffer);
            cbDesc.Usage = D3D10_USAGE_DEFAULT;
            cbDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
            cbDesc.CPUAccessFlags = 0;
            cbDesc.MiscFlags = 0;
			V_RETURN(m_d3d._10.m_pd3d10Device->CreateBuffer(&cbDesc, NULL, &m_d3d._10.m_pd3d10FoamGenPixelShaderCB));

            cbDesc.ByteWidth = sizeof(ps_attr_cbuffer);
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateBuffer(&cbDesc, NULL, &m_d3d._10.m_pd3d10PixelShaderCB));

            cbDesc.ByteWidth = sizeof(vs_attr_cbuffer);
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateBuffer(&cbDesc, NULL, &m_d3d._10.m_pd3d10VertexShaderCB));

            D3D10_SAMPLER_DESC pointSamplerDesc;
            pointSamplerDesc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
            pointSamplerDesc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;
            pointSamplerDesc.AddressV = D3D10_TEXTURE_ADDRESS_WRAP;
            pointSamplerDesc.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
            pointSamplerDesc.MipLODBias = 0.f;
            pointSamplerDesc.MaxAnisotropy = 0;
            pointSamplerDesc.ComparisonFunc = D3D10_COMPARISON_NEVER;
            pointSamplerDesc.BorderColor[0] = 0.f;
            pointSamplerDesc.BorderColor[1] = 0.f;
            pointSamplerDesc.BorderColor[2] = 0.f;
            pointSamplerDesc.BorderColor[3] = 0.f;
            pointSamplerDesc.MinLOD = 0.f;
            pointSamplerDesc.MaxLOD = 0.f;	// NB: No mipping, effectively
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateSamplerState(&pointSamplerDesc, &m_d3d._10.m_pd3d10PointSampler));

            D3D10_SAMPLER_DESC linearNoMipSampleDesc = pointSamplerDesc;
            linearNoMipSampleDesc.Filter = D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateSamplerState(&linearNoMipSampleDesc, &m_d3d._10.m_pd3d10LinearNoMipSampler));

            const D3D10_DEPTH_STENCILOP_DESC defaultStencilOp = {D3D10_STENCIL_OP_KEEP, D3D10_STENCIL_OP_KEEP, D3D10_STENCIL_OP_KEEP, D3D10_COMPARISON_ALWAYS};
            D3D10_DEPTH_STENCIL_DESC dsDesc;
            dsDesc.DepthEnable = FALSE;
            dsDesc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ZERO;
            dsDesc.DepthFunc = D3D10_COMPARISON_LESS;
            dsDesc.StencilEnable = FALSE;
            dsDesc.StencilReadMask = D3D10_DEFAULT_STENCIL_READ_MASK;
            dsDesc.StencilWriteMask = D3D10_DEFAULT_STENCIL_WRITE_MASK;
            dsDesc.FrontFace = defaultStencilOp;
            dsDesc.BackFace = defaultStencilOp;
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateDepthStencilState(&dsDesc, &m_d3d._10.m_pd3d10NoDepthStencil));

            D3D10_RASTERIZER_DESC rastDesc;
            rastDesc.FillMode = D3D10_FILL_SOLID;
            rastDesc.CullMode = D3D10_CULL_NONE;
            rastDesc.FrontCounterClockwise = FALSE;
            rastDesc.DepthBias = 0;
            rastDesc.DepthBiasClamp = 0.f;
            rastDesc.SlopeScaledDepthBias = 0.f;
            rastDesc.DepthClipEnable = FALSE;
            rastDesc.ScissorEnable = FALSE;
            rastDesc.MultisampleEnable = FALSE;
            rastDesc.AntialiasedLineEnable = FALSE;
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateRasterizerState(&rastDesc, &m_d3d._10.m_pd3d10AlwaysSolidRasterizer));

            D3D10_BLEND_DESC blendDesc;
            blendDesc.AlphaToCoverageEnable = FALSE;
            blendDesc.BlendEnable[0] = FALSE;
            blendDesc.BlendEnable[1] = FALSE;
            blendDesc.BlendEnable[2] = FALSE;
            blendDesc.BlendEnable[3] = FALSE;
            blendDesc.BlendEnable[4] = FALSE;
            blendDesc.BlendEnable[5] = FALSE;
            blendDesc.BlendEnable[6] = FALSE;
            blendDesc.BlendEnable[7] = FALSE;
			blendDesc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_RED | D3D10_COLOR_WRITE_ENABLE_GREEN | D3D10_COLOR_WRITE_ENABLE_BLUE;
            blendDesc.RenderTargetWriteMask[1] = 0x0F;
            blendDesc.RenderTargetWriteMask[2] = 0x0F;
            blendDesc.RenderTargetWriteMask[3] = 0x0F;
            blendDesc.RenderTargetWriteMask[4] = 0x0F;
            blendDesc.RenderTargetWriteMask[5] = 0x0F;
            blendDesc.RenderTargetWriteMask[6] = 0x0F;
            blendDesc.RenderTargetWriteMask[7] = 0x0F;
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateBlendState(&blendDesc, &m_d3d._10.m_pd3d10CalcGradBlendState));

			blendDesc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateBlendState(&blendDesc, &m_d3d._10.m_pd3d10AccumulateFoamBlendState));

			blendDesc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALPHA;
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateBlendState(&blendDesc, &m_d3d._10.m_pd3d10WriteAccumulatedFoamBlendState));

		}
        break;
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			HRESULT hr;
            SAFE_RELEASE(m_d3d._11.m_pd3d11GradCalcVS);
            SAFE_RELEASE(m_d3d._11.m_pd3d11GradCalcPS);
			SAFE_RELEASE(m_d3d._11.m_pd3d11FoamGenPS);
			SAFE_RELEASE(m_d3d._11.m_pd3d11FoamGenVS);
			V_RETURN(m_d3d._11.m_pd3d11Device->CreateVertexShader((void*)SM4::CalcGradient::g_vs, sizeof(SM4::CalcGradient::g_vs), NULL, &m_d3d._11.m_pd3d11GradCalcVS));
            V_RETURN(m_d3d._11.m_pd3d11Device->CreatePixelShader((void*)SM4::CalcGradient::g_ps, sizeof(SM4::CalcGradient::g_ps), NULL, &m_d3d._11.m_pd3d11GradCalcPS));
			V_RETURN(m_d3d._11.m_pd3d11Device->CreateVertexShader((void*)SM4::FoamGeneration::g_vs, sizeof(SM4::FoamGeneration::g_vs), NULL, &m_d3d._11.m_pd3d11FoamGenVS));
			V_RETURN(m_d3d._11.m_pd3d11Device->CreatePixelShader((void*)SM4::FoamGeneration::g_ps, sizeof(SM4::FoamGeneration::g_ps), NULL, &m_d3d._11.m_pd3d11FoamGenPS));

            D3D11_BUFFER_DESC cbDesc;
            cbDesc.ByteWidth = sizeof(ps_calcgradient_cbuffer);
            cbDesc.Usage = D3D11_CB_CREATION_USAGE;
            cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cbDesc.CPUAccessFlags = D3D11_CB_CREATION_CPU_ACCESS_FLAGS;
            cbDesc.MiscFlags = 0;
			cbDesc.StructureByteStride = 0;
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateBuffer(&cbDesc, NULL, &m_d3d._11.m_pd3d11GradCalcPixelShaderCB));

            cbDesc.ByteWidth = sizeof(ps_foamgeneration_cbuffer);
            cbDesc.Usage = D3D11_CB_CREATION_USAGE;
            cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cbDesc.CPUAccessFlags = D3D11_CB_CREATION_CPU_ACCESS_FLAGS;
            cbDesc.MiscFlags = 0;
			cbDesc.StructureByteStride = 0;
			V_RETURN(m_d3d._11.m_pd3d11Device->CreateBuffer(&cbDesc, NULL, &m_d3d._11.m_pd3d11FoamGenPixelShaderCB));

            cbDesc.ByteWidth = sizeof(ps_attr_cbuffer);
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateBuffer(&cbDesc, NULL, &m_d3d._11.m_pd3d11PixelShaderCB));

            cbDesc.ByteWidth = sizeof(vs_ds_attr_cbuffer);
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateBuffer(&cbDesc, NULL, &m_d3d._11.m_pd3d11VertexDomainShaderCB));

            D3D11_SAMPLER_DESC pointSamplerDesc;
            pointSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            pointSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            pointSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            pointSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            pointSamplerDesc.MipLODBias = 0.f;
            pointSamplerDesc.MaxAnisotropy = 0;
            pointSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            pointSamplerDesc.BorderColor[0] = 0.f;
            pointSamplerDesc.BorderColor[1] = 0.f;
            pointSamplerDesc.BorderColor[2] = 0.f;
            pointSamplerDesc.BorderColor[3] = 0.f;
            pointSamplerDesc.MinLOD = 0.f;
            pointSamplerDesc.MaxLOD = 0.f;	// NB: No mipping, effectively
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateSamplerState(&pointSamplerDesc, &m_d3d._11.m_pd3d11PointSampler));

            D3D11_SAMPLER_DESC linearNoMipSampleDesc = pointSamplerDesc;
            linearNoMipSampleDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateSamplerState(&linearNoMipSampleDesc, &m_d3d._11.m_pd3d11LinearNoMipSampler));

            const D3D11_DEPTH_STENCILOP_DESC defaultStencilOp = {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS};
            D3D11_DEPTH_STENCIL_DESC dsDesc;
            dsDesc.DepthEnable = FALSE;
            dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
            dsDesc.StencilEnable = FALSE;
            dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
            dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
            dsDesc.FrontFace = defaultStencilOp;
            dsDesc.BackFace = defaultStencilOp;
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateDepthStencilState(&dsDesc, &m_d3d._11.m_pd3d11NoDepthStencil));

            D3D11_RASTERIZER_DESC rastDesc;
            rastDesc.FillMode = D3D11_FILL_SOLID;
            rastDesc.CullMode = D3D11_CULL_NONE;
            rastDesc.FrontCounterClockwise = FALSE;
            rastDesc.DepthBias = 0;
            rastDesc.DepthBiasClamp = 0.f;
            rastDesc.SlopeScaledDepthBias = 0.f;
            rastDesc.DepthClipEnable = FALSE;
            rastDesc.ScissorEnable = FALSE;
            rastDesc.MultisampleEnable = FALSE;
            rastDesc.AntialiasedLineEnable = FALSE;
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateRasterizerState(&rastDesc, &m_d3d._11.m_pd3d11AlwaysSolidRasterizer));

            D3D11_BLEND_DESC blendDesc;
            blendDesc.AlphaToCoverageEnable = FALSE;
            blendDesc.RenderTarget[0].BlendEnable = FALSE;
			blendDesc.RenderTarget[1].BlendEnable = FALSE;
			blendDesc.RenderTarget[2].BlendEnable = FALSE;
			blendDesc.RenderTarget[3].BlendEnable = FALSE;
			blendDesc.RenderTarget[4].BlendEnable = FALSE;
			blendDesc.RenderTarget[5].BlendEnable = FALSE;
			blendDesc.RenderTarget[6].BlendEnable = FALSE;
			blendDesc.RenderTarget[7].BlendEnable = FALSE;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
            blendDesc.RenderTarget[1].RenderTargetWriteMask = 0x0F;
            blendDesc.RenderTarget[2].RenderTargetWriteMask = 0x0F;
            blendDesc.RenderTarget[3].RenderTargetWriteMask = 0x0F;
            blendDesc.RenderTarget[4].RenderTargetWriteMask = 0x0F;
            blendDesc.RenderTarget[5].RenderTargetWriteMask = 0x0F;
            blendDesc.RenderTarget[6].RenderTargetWriteMask = 0x0F;
            blendDesc.RenderTarget[7].RenderTargetWriteMask = 0x0F;
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateBlendState(&blendDesc, &m_d3d._11.m_pd3d11CalcGradBlendState));

			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateBlendState(&blendDesc, &m_d3d._11.m_pd3d11AccumulateFoamBlendState));

			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA;
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateBlendState(&blendDesc, &m_d3d._11.m_pd3d11WriteAccumulatedFoamBlendState));
        }
        break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			GFSDK_WaveWorks_GNM_Util::ReleaseVsShader(m_d3d._gnm.m_pGnmGradCalcVS, m_d3d._gnm.m_pGnmGradCalcFS);
			GFSDK_WaveWorks_GNM_Util::ReleaseInputResourceOffsets(m_d3d._gnm.m_pGnmGradCalcVSResourceOffsets);
			GFSDK_WaveWorks_GNM_Util::ReleasePsShader(m_d3d._gnm.m_pGnmGradCalcPS);
			GFSDK_WaveWorks_GNM_Util::ReleaseInputResourceOffsets(m_d3d._gnm.m_pGnmGradCalcPSResourceOffsets);
			GFSDK_WaveWorks_GNM_Util::ReleaseVsShader(m_d3d._gnm.m_pGnmFoamGenVS, m_d3d._gnm.m_pGnmFoamGenFS);
			GFSDK_WaveWorks_GNM_Util::ReleaseInputResourceOffsets(m_d3d._gnm.m_pGnmFoamGenVSResourceOffsets);
			GFSDK_WaveWorks_GNM_Util::ReleasePsShader(m_d3d._gnm.m_pGnmFoamGenPS);
			GFSDK_WaveWorks_GNM_Util::ReleaseInputResourceOffsets(m_d3d._gnm.m_pGnmFoamGenPSResourceOffsets);
			GFSDK_WaveWorks_GNM_Util::ReleaseCsShader(m_d3d._gnm.m_pGnmMipMapGenCS);
			GFSDK_WaveWorks_GNM_Util::ReleaseInputResourceOffsets(m_d3d._gnm.m_pGnmMipMapGenCSResourceOffsets);
			GFSDK_WaveWorks_GNM_Util::ReleaseRenderTargetClearer(m_d3d._gnm.m_pGnmRenderTargetClearer);

			m_d3d._gnm.m_pGnmGradCalcVS = GFSDK_WaveWorks_GNM_Util::CreateVsMakeFetchShader(m_d3d._gnm.m_pGnmGradCalcFS, PSSL::g_NVWaveWorks_CalcGradientVertexShader);
			m_d3d._gnm.m_pGnmGradCalcVSResourceOffsets = GFSDK_WaveWorks_GNM_Util::CreateInputResourceOffsets(Gnm::kShaderStageVs, m_d3d._gnm.m_pGnmGradCalcVS);
			m_d3d._gnm.m_pGnmGradCalcPS = GFSDK_WaveWorks_GNM_Util::CreatePsShader(PSSL::g_NVWaveWorks_CalcGradientPixelShader);
			m_d3d._gnm.m_pGnmGradCalcPSResourceOffsets = GFSDK_WaveWorks_GNM_Util::CreateInputResourceOffsets(Gnm::kShaderStagePs, m_d3d._gnm.m_pGnmGradCalcPS);
			m_d3d._gnm.m_pGnmFoamGenVS = GFSDK_WaveWorks_GNM_Util::CreateVsMakeFetchShader(m_d3d._gnm.m_pGnmFoamGenFS, PSSL::g_NVWaveWorks_FoamGenerationVertexShader);
			m_d3d._gnm.m_pGnmFoamGenVSResourceOffsets = GFSDK_WaveWorks_GNM_Util::CreateInputResourceOffsets(Gnm::kShaderStageVs, m_d3d._gnm.m_pGnmFoamGenVS);
			m_d3d._gnm.m_pGnmFoamGenPS = GFSDK_WaveWorks_GNM_Util::CreatePsShader(PSSL::g_NVWaveWorks_FoamGenerationPixelShader);
			m_d3d._gnm.m_pGnmFoamGenPSResourceOffsets = GFSDK_WaveWorks_GNM_Util::CreateInputResourceOffsets(Gnm::kShaderStagePs, m_d3d._gnm.m_pGnmFoamGenPS);
			m_d3d._gnm.m_pGnmMipMapGenCS = GFSDK_WaveWorks_GNM_Util::CreateCsShader(PSSL::g_NVWaveWorks_MipMapGenerationComputeShader);
			m_d3d._gnm.m_pGnmMipMapGenCSResourceOffsets = GFSDK_WaveWorks_GNM_Util::CreateInputResourceOffsets(Gnm::kShaderStageCs, m_d3d._gnm.m_pGnmMipMapGenCS);
			m_d3d._gnm.m_pGnmRenderTargetClearer = GFSDK_WaveWorks_GNM_Util::CreateRenderTargetClearer();

			void* pixelShaderCB = NVSDK_aligned_malloc(sizeof(ps_attr_cbuffer), Gnm::kAlignmentOfBufferInBytes);
			m_d3d._gnm.m_pGnmPixelShaderCB.initAsConstantBuffer(pixelShaderCB, sizeof(ps_attr_cbuffer));
			m_d3d._gnm.m_pGnmPixelShaderCB.setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // it's a constant buffer, so read-only is OK

			void* vertexShaderCB = NVSDK_aligned_malloc(sizeof(vs_ds_attr_cbuffer), Gnm::kAlignmentOfBufferInBytes);
			m_d3d._gnm.m_pGnmVertexDomainShaderCB.initAsConstantBuffer(vertexShaderCB, sizeof(vs_ds_attr_cbuffer));
			m_d3d._gnm.m_pGnmVertexDomainShaderCB.setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // it's a constant buffer, so read-only is OK

			m_d3d._gnm.m_pGnmPointSampler.init();
			m_d3d._gnm.m_pGnmPointSampler.setMipFilterMode(Gnm::kMipFilterModeNone);
			m_d3d._gnm.m_pGnmPointSampler.setXyFilterMode(Gnm::kFilterModePoint, Gnm::kFilterModePoint);
			m_d3d._gnm.m_pGnmPointSampler.setWrapMode(Gnm::kWrapModeWrap, Gnm::kWrapModeWrap, Gnm::kWrapModeWrap);
			m_d3d._gnm.m_pGnmPointSampler.setDepthCompareFunction(Gnm::kDepthCompareNever);

			m_d3d._gnm.m_pGnmLinearNoMipSampler = m_d3d._gnm.m_pGnmPointSampler;
			m_d3d._gnm.m_pGnmLinearNoMipSampler.setXyFilterMode(Gnm::kFilterModeBilinear, Gnm::kFilterModeBilinear);
			
			m_d3d._gnm.m_pGnmNoDepthStencil.init();

			m_d3d._gnm.m_pGnmAlwaysSolidRasterizer.init();
			m_d3d._gnm.m_pGnmAlwaysSolidRasterizer.setFrontFace(Gnm::kPrimitiveSetupFrontFaceCw);
			m_d3d._gnm.m_pGnmAlwaysSolidRasterizer.setPolygonMode(Gnm::kPrimitiveSetupPolygonModeFill, Gnm::kPrimitiveSetupPolygonModeFill);

			m_d3d._gnm.m_pGnmCalcGradBlendState.init();
			m_d3d._gnm.m_pGnmAccumulateFoamBlendState.init();
			m_d3d._gnm.m_pGnmWriteAccumulatedFoamBlendState.init();
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_gl2:
		{
			// Creating gradient calculation program
			if(m_d3d._GL2.m_GradCalcProgram != 0) NVSDK_GLFunctions.glDeleteProgram(m_d3d._GL2.m_GradCalcProgram); CHECK_GL_ERRORS;
			m_d3d._GL2.m_GradCalcProgram = loadGLProgram(GL::k_NVWaveWorks_CalcGradientVertexShader,NULL,NULL,NULL,GL::k_NVWaveWorks_CalcGradientFragmentShader);
			if(m_d3d._GL2.m_GradCalcProgram == 0) return E_FAIL;

			// Gradient calculation program binding
			m_d3d._GL2.m_GradCalcUniformLocation_Scales = NVSDK_GLFunctions.glGetUniformLocation(m_d3d._GL2.m_GradCalcProgram,"nvsf_g_Scales"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_GradCalcUniformLocation_OneBack = NVSDK_GLFunctions.glGetUniformLocation(m_d3d._GL2.m_GradCalcProgram,"nvsf_g_OneTexel_Back"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_GradCalcUniformLocation_OneFront = NVSDK_GLFunctions.glGetUniformLocation(m_d3d._GL2.m_GradCalcProgram,"nvsf_g_OneTexel_Front"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_GradCalcUniformLocation_OneLeft = NVSDK_GLFunctions.glGetUniformLocation(m_d3d._GL2.m_GradCalcProgram,"nvsf_g_OneTexel_Left"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_GradCalcUniformLocation_OneRight = NVSDK_GLFunctions.glGetUniformLocation(m_d3d._GL2.m_GradCalcProgram,"nvsf_g_OneTexel_Right"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_GradCalcTextureBindLocation_DisplacementMap = NVSDK_GLFunctions.glGetUniformLocation(m_d3d._GL2.m_GradCalcProgram,"nvsf_g_samplerDisplacementMap"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_GradCalcTextureUnit_DisplacementMap = 0;
			m_d3d._GL2.m_GradCalcAttributeLocation_Pos = NVSDK_GLFunctions.glGetAttribLocation(m_d3d._GL2.m_GradCalcProgram, "nvsf_vInPos"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_GradCalcAttributeLocation_TexCoord = NVSDK_GLFunctions.glGetAttribLocation(m_d3d._GL2.m_GradCalcProgram, "nvsf_vInTexCoord"); CHECK_GL_ERRORS;

			// Creating foam generation program
			if(m_d3d._GL2.m_FoamGenProgram != 0) NVSDK_GLFunctions.glDeleteProgram(m_d3d._GL2.m_FoamGenProgram); CHECK_GL_ERRORS;
			m_d3d._GL2.m_FoamGenProgram = loadGLProgram(GL::k_NVWaveWorks_FoamGenerationVertexShader,NULL,NULL,NULL,GL::k_NVWaveWorks_FoamGenerationFragmentShader);
			if(m_d3d._GL2.m_FoamGenProgram == 0) return E_FAIL;

			// Foam accumulation program binding
			m_d3d._GL2.m_FoamGenUniformLocation_DissipationFactors = NVSDK_GLFunctions.glGetUniformLocation(m_d3d._GL2.m_FoamGenProgram,"nvsf_g_DissipationFactors"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_FoamGenUniformLocation_SourceComponents = NVSDK_GLFunctions.glGetUniformLocation(m_d3d._GL2.m_FoamGenProgram,"nvsf_g_SourceComponents"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_FoamGenUniformLocation_UVOffsets = NVSDK_GLFunctions.glGetUniformLocation(m_d3d._GL2.m_FoamGenProgram,"nvsf_g_UVOffsets"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_FoamGenTextureBindLocation_EnergyMap = NVSDK_GLFunctions.glGetUniformLocation(m_d3d._GL2.m_FoamGenProgram,"nvsf_g_samplerEnergyMap"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_FoamGenTextureUnit_EnergyMap = 0;
			m_d3d._GL2.m_FoamGenAttributeLocation_Pos = NVSDK_GLFunctions.glGetAttribLocation(m_d3d._GL2.m_FoamGenProgram, "nvsf_vInPos"); CHECK_GL_ERRORS;
			m_d3d._GL2.m_FoamGenAttributeLocation_TexCoord = NVSDK_GLFunctions.glGetAttribLocation(m_d3d._GL2.m_FoamGenProgram, "nvsf_vInTexCoord"); CHECK_GL_ERRORS;
		}
		break;
#endif
	case nv_water_d3d_api_none:
		break;
	default:
		// Unexpected API
		return E_FAIL;
	}

#endif // WAVEWORKS_ENABLE_GRAPHICS

    return S_OK;
}

HRESULT GFSDK_WaveWorks_Simulation::initTextureArrays()
{
#if WAVEWORKS_ENABLE_GRAPHICS
    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
	case nv_water_d3d_api_d3d9:
		break;
#endif 
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
		break;
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
		break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		break;
#endif
#if WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_gl2:
		{
			// last cascade is the closest cascade and it has the highest fft resolution
			UINT N = m_params.cascades[GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades-1].fft_resolution;

			// using the right texture format to avoid implicit format conversion (half float <-> float) when filling the texture arrays
			GLuint displacement_texture_array_format = (m_params.simulation_api == nv_water_simulation_api_cpu) ? GL_RGBA16F : GL_RGBA32F;
			GLuint displacement_texture_array_type = (m_params.simulation_api == nv_water_simulation_api_cpu) ? GL_HALF_FLOAT : GL_FLOAT;
			
			// creating displacement texture array
			if(m_d3d._GL2.m_DisplacementsTextureArray == 0) NVSDK_GLFunctions.glGenTextures(1,&m_d3d._GL2.m_DisplacementsTextureArray); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D_ARRAY, m_d3d._GL2.m_DisplacementsTextureArray); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, displacement_texture_array_format, N, N, 4, 0, GL_RGBA, displacement_texture_array_type, NULL); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D_ARRAY, 0); CHECK_GL_ERRORS;

			// creating gradients texture array
			if(m_d3d._GL2.m_GradientsTextureArray == 0) NVSDK_GLFunctions.glGenTextures(1,&m_d3d._GL2.m_GradientsTextureArray); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D_ARRAY, m_d3d._GL2.m_GradientsTextureArray); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA16F, N, N, 4, 0, GL_RGBA, GL_HALF_FLOAT, NULL); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glGenerateMipmap(GL_TEXTURE_2D_ARRAY); CHECK_GL_ERRORS; // allocating memory for mipmaps of gradient texture array
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D_ARRAY, 0); CHECK_GL_ERRORS;

			// creating FBOs used to blit from separate displacement/gradient textures to displacement/gradient texture arrays
			if(m_d3d._GL2.m_TextureArraysBlittingDrawFBO == 0) NVSDK_GLFunctions.glGenFramebuffers(1,&m_d3d._GL2.m_TextureArraysBlittingDrawFBO); CHECK_GL_ERRORS;
			if(m_d3d._GL2.m_TextureArraysBlittingReadFBO == 0) NVSDK_GLFunctions.glGenFramebuffers(1,&m_d3d._GL2.m_TextureArraysBlittingReadFBO); CHECK_GL_ERRORS;
		}
		break;
#endif
	case nv_water_d3d_api_none:
		break;
	default:
		// Unexpected API
		return E_FAIL;
	}

#endif // WAVEWORKS_ENABLE_GRAPHICS
    return S_OK;
}


HRESULT GFSDK_WaveWorks_Simulation::initD3D9(const GFSDK_WaveWorks_Detailed_Simulation_Params& D3D9_ONLY(params), IDirect3DDevice9* D3D9_ONLY(pD3DDevice))
{
#if WAVEWORKS_ENABLE_D3D9
    HRESULT hr;

    if(nv_water_d3d_api_d3d9 != m_d3dAPI)
    {
        releaseAll();
    }
    else if(m_d3d._9.m_pd3d9Device != pD3DDevice)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_d3d9;
        m_d3d._9.m_pd3d9Device = pD3DDevice;
        m_d3d._9.m_pd3d9Device->AddRef();

        m_params = params;
        V_RETURN(allocateAll());
    }
    else
    {
        V_RETURN(reinit(params));
    }

    return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::initD3D10(const GFSDK_WaveWorks_Detailed_Simulation_Params& D3D10_ONLY(params), ID3D10Device* D3D10_ONLY(pD3DDevice))
{
#if WAVEWORKS_ENABLE_D3D10
    HRESULT hr;

    if(nv_water_d3d_api_d3d10 != m_d3dAPI)
    {
        releaseAll();
    }
    else if(m_d3d._10.m_pd3d10Device != pD3DDevice)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_d3d10;
        m_d3d._10.m_pd3d10Device = pD3DDevice;
        m_d3d._10.m_pd3d10Device->AddRef();

        m_params = params;
        V_RETURN(allocateAll());
    }
    else
    {
        V_RETURN(reinit(params));
    }

    return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::initD3D11(const GFSDK_WaveWorks_Detailed_Simulation_Params& D3D11_ONLY(params), GFSDK_WaveWorks_CPU_Scheduler_Interface* D3D11_ONLY(pOptionalScheduler), ID3D11Device* D3D11_ONLY(pD3DDevice))
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

		m_pOptionalScheduler = pOptionalScheduler;

        m_params = params;
        V_RETURN(allocateAll());
    }
    else
    {
        V_RETURN(reinit(params));
    }
    return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::initGnm(const GFSDK_WaveWorks_Detailed_Simulation_Params& GNM_ONLY(params), GFSDK_WaveWorks_CPU_Scheduler_Interface* GNM_ONLY(pOptionalScheduler))
{
#if WAVEWORKS_ENABLE_GNM
	HRESULT hr;

	if(nv_water_d3d_api_gnm != m_d3dAPI)
	{
		releaseAll();
	}

	if(nv_water_d3d_api_undefined == m_d3dAPI)
	{
		m_d3dAPI = nv_water_d3d_api_gnm;
		m_params = params;
		m_pOptionalScheduler = pOptionalScheduler;
		V_RETURN(allocateAll());
	}
	else
	{
		V_RETURN(reinit(params));
	}
	return S_OK;
#else
	return E_FAIL;
#endif
}
HRESULT GFSDK_WaveWorks_Simulation::initGL2(const GFSDK_WaveWorks_Detailed_Simulation_Params& GL_ONLY(params), void* GL_ONLY(pGLContext))
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
        m_params = params;

        V_RETURN(allocateAll());
    }
    else
    {
        V_RETURN(reinit(params));
    }
	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::initNoGraphics(const GFSDK_WaveWorks_Detailed_Simulation_Params& params)
{
    HRESULT hr;

    if(nv_water_d3d_api_none != m_d3dAPI)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_none;
        m_params = params;
        V_RETURN(allocateAll());
		
    }
    else
    {
        V_RETURN(reinit(params));
    }
    return S_OK;
}

void GFSDK_WaveWorks_Simulation::releaseSimulation(int cascade)
{
	m_pSimulationManager->releaseSimulation(cascade_states[cascade].m_pFFTSimulation);
	cascade_states[cascade].m_pFFTSimulation = NULL;
}

HRESULT GFSDK_WaveWorks_Simulation::allocateSimulation(int cascade)
{
	NVWaveWorks_FFT_Simulation* pFFTSim = m_pSimulationManager ? m_pSimulationManager->createSimulation(m_params.cascades[cascade]) : NULL;
	cascade_states[cascade].m_pFFTSimulation = pFFTSim;
	if(pFFTSim) {
		switch(m_d3dAPI)
		{
#if WAVEWORKS_ENABLE_D3D9
		case nv_water_d3d_api_d3d9:
			return pFFTSim->initD3D9(m_d3d._9.m_pd3d9Device);
#endif
#if WAVEWORKS_ENABLE_D3D10
		case nv_water_d3d_api_d3d10:
			return pFFTSim->initD3D10(m_d3d._10.m_pd3d10Device);
#endif
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			return pFFTSim->initD3D11(m_d3d._11.m_pd3d11Device);
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			return pFFTSim->initGnm();
#endif		
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			return pFFTSim->initGL2(m_d3d._GL2.m_pGLContext);
#endif
		case nv_water_d3d_api_none:
			return pFFTSim->initNoGraphics();
		default:
			return E_FAIL;
		}
	} else {
		return E_FAIL;
	}
}

void GFSDK_WaveWorks_Simulation::releaseSimulationManager()
{
	SAFE_DELETE(m_pSimulationManager);
}

HRESULT GFSDK_WaveWorks_Simulation::allocateSimulationManager()
{
	switch(m_params.simulation_api)
	{
#ifdef SUPPORT_CUDA
	case nv_water_simulation_api_cuda:
		m_pSimulationManager = new NVWaveWorks_FFT_Simulation_Manager_CUDA_Impl();
		break;
#endif
#ifdef SUPPORT_FFTCPU
	case nv_water_simulation_api_cpu:
		m_pSimulationManager = new NVWaveWorks_FFT_Simulation_Manager_CPU_Impl(m_params,m_pOptionalScheduler);
		break;
#endif
#ifdef SUPPORT_DIRECTCOMPUTE
	case nv_water_simulation_api_direct_compute:
		m_pSimulationManager = new NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl();
		break;
#endif
	default:
		return E_FAIL;
	}

	if(m_pSimulationManager) {
		switch(m_d3dAPI)
		{
#if WAVEWORKS_ENABLE_D3D9
		case nv_water_d3d_api_d3d9:
			return m_pSimulationManager->initD3D9(m_d3d._9.m_pd3d9Device);
#endif
#if WAVEWORKS_ENABLE_D3D10
		case nv_water_d3d_api_d3d10:
			return m_pSimulationManager->initD3D10(m_d3d._10.m_pd3d10Device);
#endif
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			return m_pSimulationManager->initD3D11(m_d3d._11.m_pd3d11Device);
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			return m_pSimulationManager->initGnm();
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			return m_pSimulationManager->initGL2(m_d3d._GL2.m_pGLContext);
#endif
		case nv_water_d3d_api_none:
			return m_pSimulationManager->initNoGraphics();
		default:
			return E_FAIL;
		}
	} else {
		return E_FAIL;
	}
}

void  GFSDK_WaveWorks_Simulation::releaseGFXTimer()
{
	SAFE_DELETE(m_pGFXTimer);
}

HRESULT  GFSDK_WaveWorks_Simulation::allocateGFXTimer()
{
	SAFE_DELETE(m_pGFXTimer);

	if(!m_params.enable_gfx_timers)
		return S_OK;						// Timers not permitted by settings

	if(nv_water_d3d_api_none == m_d3dAPI)
		return S_OK;						// No GFX, no timers

#if WAVEWORKS_ENABLE_GRAPHICS
    if(nv_water_d3d_api_gnm != m_d3dAPI)
	{
        m_pGFXTimer = new NVWaveWorks_GFX_Timer_Impl();
    }

	m_gpu_kick_timers.reset();
	m_gpu_wait_timers.reset();

	switch(m_d3dAPI)
	{
#if WAVEWORKS_ENABLE_D3D9
	case nv_water_d3d_api_d3d9:
		return m_pGFXTimer->initD3D9(m_d3d._9.m_pd3d9Device);
#endif
#if WAVEWORKS_ENABLE_D3D10
	case nv_water_d3d_api_d3d10:
		return m_pGFXTimer->initD3D10(m_d3d._10.m_pd3d10Device);
#endif
#if WAVEWORKS_ENABLE_D3D11
	case nv_water_d3d_api_d3d11:
		return m_pGFXTimer->initD3D11(m_d3d._11.m_pd3d11Device);
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
        return m_pGFXTimer->initGnm();
#endif
#if WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_gl2:
		return m_pGFXTimer->initGL2(m_d3d._GL2.m_pGLContext);
#endif
	default:
		return E_FAIL;
	}
#else// WAVEWORKS_ENABLE_GRAPHICS
	return E_FAIL;
#endif // WAVEWORKS_ENABLE_GRAPHICS
}

HRESULT GFSDK_WaveWorks_Simulation::allocateAll()
{
    HRESULT hr;

    V_RETURN(initShaders());
    V_RETURN(initGradMapSamplers());
	if(m_params.use_texture_arrays)
	{
		V_RETURN(initTextureArrays());
	}

	V_RETURN(allocateSimulationManager());
	V_RETURN(allocateGFXTimer());

    for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
    {
		V_RETURN(allocateRenderingResources(cascade));
		V_RETURN(allocateSimulation(cascade));
    }

	updateRMS(m_params);

    return S_OK;
}

HRESULT GFSDK_WaveWorks_Simulation::reinit(const GFSDK_WaveWorks_Detailed_Simulation_Params& params)
{
    HRESULT hr;

	BOOL bReinitTextureArrays = FALSE;
	if(params.cascades[GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades - 1].fft_resolution != m_params.cascades[GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades - 1].fft_resolution)
    {
        bReinitTextureArrays = TRUE;
    }

    BOOL bReinitGradMapSamplers = FALSE;
    if(params.aniso_level != m_params.aniso_level)
    {
        bReinitGradMapSamplers = TRUE;
    }

	BOOL bReinitSimManager = FALSE;
	if(params.simulation_api != m_params.simulation_api)
	{
		bReinitSimManager = TRUE;
	}
	else if(nv_water_simulation_api_cpu == params.simulation_api && params.CPU_simulation_threading_model != m_params.CPU_simulation_threading_model)
	{
		bReinitSimManager = TRUE;
	}

	BOOL bAllocateSim[GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades];
	BOOL bReleaseSim[GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades];
    BOOL bReleaseRenderingResources[GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades];
    BOOL bAllocateRenderingResources[GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades];
	BOOL bReinitSim[GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades];
	int numReinitSims = 0;
	int numReleaseSims = 0;
	int numAllocSims = 0;

    for(int cascade = 0; cascade != GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades; ++cascade)
    {
        bAllocateSim[cascade] = FALSE;
        bReleaseSim[cascade] = FALSE;
        bReleaseRenderingResources[cascade] = FALSE;
        bAllocateRenderingResources[cascade] = FALSE;
		bReinitSim[cascade] = FALSE;

        if(cascade < params.num_cascades && cascade >= m_params.num_cascades)
        {
            // Cascade being activated
            bAllocateRenderingResources[cascade] = TRUE;
			bAllocateSim[cascade] = TRUE;
			++numAllocSims;
        }
        else if(cascade < m_params.num_cascades && cascade >= params.num_cascades)
        {
            // Cascade being deactivated
            bReleaseRenderingResources[cascade] = TRUE;
			bReleaseSim[cascade] = TRUE;
			++numReleaseSims;
        }
		else if(cascade < params.num_cascades)
		{
			// A kept cascade
			if(bReinitSimManager)
			{
				// Sim manager will be torn down and re-allocated, cascade needs the same treatment
				bReleaseSim[cascade] = TRUE;
				bAllocateSim[cascade] = TRUE;
				++numReleaseSims;
				++numAllocSims;
			}
			else
			{
				// Sim manager is not being touched: just prod cascade for an internal re-init
				bReinitSim[cascade] = TRUE;
				++numReinitSims;
			}

			if(params.cascades[cascade].fft_resolution != m_params.cascades[cascade].fft_resolution ||
				params.num_GPUs != m_params.num_GPUs)	// Need to re-alloc per-GPU resources
			{
				bReleaseRenderingResources[cascade] = TRUE;
				bAllocateRenderingResources[cascade] = TRUE;
			}
		}
    }

    m_params = params;

	if(numReinitSims) {
		bool reinitOnly = false;
		if(0 == numAllocSims && 0 == numReleaseSims && numReinitSims == m_params.num_cascades)
		{
			// This is a pure cascade-level reinit
			reinitOnly = true;
		}
		V_RETURN(m_pSimulationManager->beforeReinit(m_params, reinitOnly));
	}

    for(int cascade = 0; cascade != GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades; ++cascade)
    {
		if(bReleaseSim[cascade])
		{
			releaseSimulation(cascade);
		}
	}

	if(bReinitSimManager)
	{
		releaseSimulationManager();
		V_RETURN(allocateSimulationManager());
	}

    for(int cascade = 0; cascade != GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades; ++cascade)
    {
        if(bReleaseRenderingResources[cascade])
        {
            releaseRenderingResources(cascade);
        }

        if(bAllocateRenderingResources[cascade])
        {
            V_RETURN(allocateRenderingResources(cascade));
        }

		if(bAllocateSim[cascade])
		{
			V_RETURN(allocateSimulation(cascade));
		}

		if(bReinitSim[cascade])
		{
			V_RETURN(cascade_states[cascade].m_pFFTSimulation->reinit(m_params.cascades[cascade]));
		}
    }
	updateRMS(m_params);
	if(bReinitGradMapSamplers)
    {
        V_RETURN(initGradMapSamplers());
    }

	if(bReinitTextureArrays)
    {
		V_RETURN(initTextureArrays());
    }

    return S_OK;
}

void GFSDK_WaveWorks_Simulation::setSimulationTime(double dAppTime)
{
	m_dSimTime = dAppTime * (double)m_params.time_scale;

	if(m_numValidEntriesInSimTimeFIFO) {
		assert(m_numValidEntriesInSimTimeFIFO==(m_num_GPU_slots+1));
		for(int i=m_numValidEntriesInSimTimeFIFO-1;i>0;i--) {
			m_dSimTimeFIFO[i] = m_dSimTimeFIFO[i-1];
		}
		m_dSimTimeFIFO[0] = m_dSimTime;
	} else {
		// The FIFO is empty, so this must be first tick - prime it
		m_numValidEntriesInSimTimeFIFO=m_num_GPU_slots+1;
		for(int i = 0; i != m_numValidEntriesInSimTimeFIFO; ++i) {
			m_dSimTimeFIFO[i] = m_dSimTime;
		}
	}

	m_dFoamSimDeltaTime = m_dSimTimeFIFO[0] - m_dSimTimeFIFO[m_num_GPU_slots];
	if(m_dFoamSimDeltaTime <=0 ) m_dFoamSimDeltaTime = 0;
}

HRESULT GFSDK_WaveWorks_Simulation::updateGradientMaps(Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl)
{
	HRESULT result;

    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
		case nv_water_d3d_api_d3d9:
			result=updateGradientMapsD3D9(pSavestateImpl);
			break;
#endif
#if WAVEWORKS_ENABLE_D3D10
		case nv_water_d3d_api_d3d10:
			result=updateGradientMapsD3D10(pSavestateImpl);
			break;
#endif
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			result=updateGradientMapsD3D11(pGC, pSavestateImpl);
			break;
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			result=updateGradientMapsGnm(pGC, pSavestateImpl);
			break;
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			result=updateGradientMapsGL2(pGC);
			break;
#endif
		case nv_water_d3d_api_none:
			// No graphics, nothing to do
			result=S_OK;
			break;
		default:
			result=E_FAIL;
			break;
    }

	return result;
}

HRESULT GFSDK_WaveWorks_Simulation::updateGradientMapsD3D10(GFSDK_WaveWorks_Savestate* D3D10_ONLY(pSavestateImpl))
{
#if WAVEWORKS_ENABLE_D3D10
    HRESULT hr;

    // Preserve
    if(pSavestateImpl)
    {
        V_RETURN(pSavestateImpl->PreserveD3D10Viewport());
        V_RETURN(pSavestateImpl->PreserveD3D10RenderTargets());
        V_RETURN(pSavestateImpl->PreserveD3D10Shaders());
        V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderConstantBuffer(0));
        V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderSampler(0));
        V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderResource(0));
        V_RETURN(pSavestateImpl->PreserveD3D10DepthStencil());
        V_RETURN(pSavestateImpl->PreserveD3D10Blend());
        V_RETURN(pSavestateImpl->PreserveD3D10Raster());

        for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
        {
            V_RETURN(cascade_states[cascade].m_pQuadMesh->PreserveState(NULL, pSavestateImpl));
        }
    }

    for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
    {
		if(cascade_states[cascade].m_gradient_map_version == cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion())
            continue;
		
		// Clear the gradient map if necessary
		const FLOAT kBlack[] = {0.f,0.f,0.f,0.f};
		if(cascade_states[cascade].m_gradient_map_needs_clear[m_active_GPU_slot]) {
			m_d3d._10.m_pd3d10Device->ClearRenderTargetView(cascade_states[cascade].m_d3d._10.m_pd3d10GradientRenderTarget[m_active_GPU_slot],kBlack);
			cascade_states[cascade].m_gradient_map_needs_clear[m_active_GPU_slot] = false;
		}

		// Rendering folding to gradient map //////////////////////////////////
		
        // Render-targets + viewport
        m_d3d._10.m_pd3d10Device->OMSetRenderTargets(1, &cascade_states[cascade].m_d3d._10.m_pd3d10GradientRenderTarget[m_active_GPU_slot], NULL);

        int dmap_dim =m_params.cascades[cascade].fft_resolution;
        D3D10_VIEWPORT new_vp;
        new_vp.TopLeftX = 0;
        new_vp.TopLeftY = 0;
        new_vp.Width = dmap_dim;
        new_vp.Height = dmap_dim;
        new_vp.MinDepth = 0.f;
        new_vp.MaxDepth = 0.f;
        UINT num_new_vp = 1;
        m_d3d._10.m_pd3d10Device->RSSetViewports(num_new_vp, &new_vp);

        // Shaders
        m_d3d._10.m_pd3d10Device->VSSetShader(m_d3d._10.m_pd3d10GradCalcVS);
        m_d3d._10.m_pd3d10Device->GSSetShader(NULL);
        m_d3d._10.m_pd3d10Device->PSSetShader(m_d3d._10.m_pd3d10GradCalcPS);

        // Constants
        ps_calcgradient_cbuffer PSCB;
        PSCB.g_ChoppyScale = m_params.cascades[cascade].choppy_scale * dmap_dim / m_params.cascades[cascade].fft_period;
		if(m_params.cascades[0].fft_period > 1000.0f) PSCB.g_ChoppyScale *= 1.0f + 0.2f * log(m_params.cascades[0].fft_period/1000.0f);
		PSCB.g_GradMap2TexelWSScale = 0.5f*dmap_dim / m_params.cascades[cascade].fft_period ; 
		PSCB.g_OneTexel_Left = gfsdk_make_float4(-1.0f/dmap_dim, 0, 0, 0);
        PSCB.g_OneTexel_Right = gfsdk_make_float4( 1.0f/dmap_dim, 0, 0, 0);
        PSCB.g_OneTexel_Back = gfsdk_make_float4( 0,-1.0f/dmap_dim, 0, 0);
        PSCB.g_OneTexel_Front = gfsdk_make_float4( 0, 1.0f/dmap_dim, 0, 0);
        m_d3d._10.m_pd3d10Device->UpdateSubresource(m_d3d._10.m_pd3d10GradCalcPixelShaderCB, 0, NULL, &PSCB, 0, 0);
        m_d3d._10.m_pd3d10Device->PSSetConstantBuffers(0, 1, &m_d3d._10.m_pd3d10GradCalcPixelShaderCB);

        // Textures/samplers
        m_d3d._10.m_pd3d10Device->PSSetShaderResources(0, 1, cascade_states[cascade].m_pFFTSimulation->GetDisplacementMapD3D10());
        m_d3d._10.m_pd3d10Device->PSSetSamplers(0, 1, &m_d3d._10.m_pd3d10PointSampler);

		// Render state
		m_d3d._10.m_pd3d10Device->OMSetDepthStencilState(m_d3d._10.m_pd3d10NoDepthStencil, 0);
		m_d3d._10.m_pd3d10Device->OMSetBlendState(m_d3d._10.m_pd3d10CalcGradBlendState, NULL, 0xFFFFFFFF);
		m_d3d._10.m_pd3d10Device->RSSetState(m_d3d._10.m_pd3d10AlwaysSolidRasterizer);
		// Draw
		V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(NULL, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));


		// Accumulating energy in foam energy map //////////////////////////////////

		// Clear the foam map, to ensure inter-frame deps get broken on multi-GPU
		m_d3d._10.m_pd3d10Device->ClearRenderTargetView(cascade_states[cascade].m_d3d._10.m_pd3d10FoamEnergyRenderTarget,kBlack);

        // Render-targets + viewport
		m_d3d._10.m_pd3d10Device->OMSetRenderTargets(1, &cascade_states[cascade].m_d3d._10.m_pd3d10FoamEnergyRenderTarget, NULL);

        dmap_dim = m_params.cascades[cascade].fft_resolution;
        new_vp.TopLeftX = 0;
        new_vp.TopLeftY = 0;
        new_vp.Width = dmap_dim;
        new_vp.Height = dmap_dim;
        new_vp.MinDepth = 0.f;
        new_vp.MaxDepth = 0.f;
        num_new_vp = 1;
        m_d3d._10.m_pd3d10Device->RSSetViewports(num_new_vp, &new_vp);

        // Shaders
		m_d3d._10.m_pd3d10Device->VSSetShader(m_d3d._10.m_pd3d10FoamGenVS);
        m_d3d._10.m_pd3d10Device->GSSetShader(NULL);
		m_d3d._10.m_pd3d10Device->PSSetShader(m_d3d._10.m_pd3d10FoamGenPS);

		// Constants
		ps_foamgeneration_cbuffer fgcb;
		fgcb.g_SourceComponents = gfsdk_make_float4(0,0,0.0f,1.0f); // getting component W of grad map as source for energy
		fgcb.g_UVOffsets = gfsdk_make_float4(0,1.0f,0,0); // blurring by Y
		fgcb.nvsf_g_DissipationFactors_Accumulation = m_params.cascades[cascade].foam_generation_amount*(float)m_dFoamSimDeltaTime*50.0f; 
		fgcb.nvsf_g_DissipationFactors_Fadeout		= pow(m_params.cascades[cascade].foam_falloff_speed,(float)m_dFoamSimDeltaTime*50.0f);
		fgcb.nvsf_g_DissipationFactors_BlurExtents	= min(0.5f,m_params.cascades[cascade].foam_dissipation_speed*(float)m_dFoamSimDeltaTime*m_params.cascades[0].fft_period * (1000.0f/m_params.cascades[0].fft_period)/m_params.cascades[cascade].fft_period)/dmap_dim;
		fgcb.nvsf_g_FoamGenerationThreshold			= m_params.cascades[cascade].foam_generation_threshold;

		m_d3d._10.m_pd3d10Device->UpdateSubresource(m_d3d._10.m_pd3d10FoamGenPixelShaderCB, 0, NULL, &fgcb, 0, 0);	    
		m_d3d._10.m_pd3d10Device->PSSetConstantBuffers(0, 1, &m_d3d._10.m_pd3d10FoamGenPixelShaderCB);

        // Textures/samplers
        m_d3d._10.m_pd3d10Device->PSSetShaderResources(0, 1, &cascade_states[cascade].m_d3d._10.m_pd3d10GradientMap[m_active_GPU_slot]);
		m_d3d._10.m_pd3d10Device->PSSetSamplers(0, 1, &m_d3d._10.m_pd3d10LinearNoMipSampler);

		// Render state
		m_d3d._10.m_pd3d10Device->OMSetDepthStencilState(m_d3d._10.m_pd3d10NoDepthStencil, 0);
		m_d3d._10.m_pd3d10Device->OMSetBlendState(m_d3d._10.m_pd3d10AccumulateFoamBlendState, NULL, 0xFFFFFFFF);
		m_d3d._10.m_pd3d10Device->RSSetState(m_d3d._10.m_pd3d10AlwaysSolidRasterizer);
		// Draw
		V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(NULL, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));

		// Clear shader resource from inputs
		ID3D10ShaderResourceView* pNullSRV = NULL;
		m_d3d._10.m_pd3d10Device->PSSetShaderResources(0, 1, &pNullSRV);

		// Writing back energy to gradient map //////////////////////////////////

        // Render-targets + viewport
		m_d3d._10.m_pd3d10Device->OMSetRenderTargets(1, &cascade_states[cascade].m_d3d._10.m_pd3d10GradientRenderTarget[m_active_GPU_slot], NULL);

        dmap_dim = m_params.cascades[cascade].fft_resolution;
        new_vp.TopLeftX = 0;
        new_vp.TopLeftY = 0;
        new_vp.Width = dmap_dim;
        new_vp.Height = dmap_dim;
        new_vp.MinDepth = 0.f;
        new_vp.MaxDepth = 0.f;
        num_new_vp = 1;
        m_d3d._10.m_pd3d10Device->RSSetViewports(num_new_vp, &new_vp);

        // Shaders
		m_d3d._10.m_pd3d10Device->VSSetShader(m_d3d._10.m_pd3d10FoamGenVS);
        m_d3d._10.m_pd3d10Device->GSSetShader(NULL);
		m_d3d._10.m_pd3d10Device->PSSetShader(m_d3d._10.m_pd3d10FoamGenPS);

		// Constants
		fgcb.g_SourceComponents = gfsdk_make_float4(1.0f,0,0,0); // getting component R of energy map as source for energy
		fgcb.g_UVOffsets = gfsdk_make_float4(1.0f,0,0,0); // blurring by X
		fgcb.nvsf_g_DissipationFactors_Accumulation = 0.0f; 
		fgcb.nvsf_g_DissipationFactors_Fadeout		= 1.0f;
		fgcb.nvsf_g_DissipationFactors_BlurExtents	= min(0.5f,m_params.cascades[cascade].foam_dissipation_speed*(float)m_dFoamSimDeltaTime*m_params.cascades[0].fft_period * (1000.0f/m_params.cascades[0].fft_period)/m_params.cascades[cascade].fft_period)/dmap_dim;

		m_d3d._10.m_pd3d10Device->UpdateSubresource(m_d3d._10.m_pd3d10FoamGenPixelShaderCB, 0, NULL, &fgcb, 0, 0);	    
		m_d3d._10.m_pd3d10Device->PSSetConstantBuffers(0, 1, &m_d3d._10.m_pd3d10FoamGenPixelShaderCB);

        // Textures/samplers
		m_d3d._10.m_pd3d10Device->PSSetShaderResources(0, 1, &cascade_states[cascade].m_d3d._10.m_pd3d10FoamEnergyMap);
		m_d3d._10.m_pd3d10Device->PSSetSamplers(0, 1, &m_d3d._10.m_pd3d10LinearNoMipSampler);

		// Render state
		m_d3d._10.m_pd3d10Device->OMSetDepthStencilState(m_d3d._10.m_pd3d10NoDepthStencil, 0);
		m_d3d._10.m_pd3d10Device->OMSetBlendState(m_d3d._10.m_pd3d10WriteAccumulatedFoamBlendState, NULL, 0xFFFFFFFF);
		m_d3d._10.m_pd3d10Device->RSSetState(m_d3d._10.m_pd3d10AlwaysSolidRasterizer);
		
		// Draw
		V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(NULL, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));

        // Generate mips
        m_d3d._10.m_pd3d10Device->GenerateMips(cascade_states[cascade].m_d3d._10.m_pd3d10GradientMap[m_active_GPU_slot]);

        cascade_states[cascade].m_gradient_map_version = cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion();
    }

	// Clear any lingering displacement map reference
	ID3D10ShaderResourceView* pNullSRV = NULL;
	m_d3d._10.m_pd3d10Device->PSSetShaderResources(0, 1, &pNullSRV);

    return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::updateGradientMapsD3D9(GFSDK_WaveWorks_Savestate* D3D9_ONLY(pSavestateImpl))
{
#if WAVEWORKS_ENABLE_D3D9
    HRESULT hr;

    // Preserve
    const UINT NumPSConstants = 5;
    if(pSavestateImpl)
    {
        V_RETURN(pSavestateImpl->PreserveD3D9Viewport());
        V_RETURN(pSavestateImpl->PreserveD3D9RenderTargets());
        V_RETURN(pSavestateImpl->PreserveD3D9Shaders());

        V_RETURN(pSavestateImpl->PreserveD3D9PixelShaderConstantF(0, NumPSConstants));
        V_RETURN(pSavestateImpl->PreserveD3D9Texture(0));
        V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(0, D3DSAMP_MIPFILTER));
        V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(0, D3DSAMP_MINFILTER));
        V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(0, D3DSAMP_MAGFILTER));
        V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(0, D3DSAMP_ADDRESSU));
        V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(0, D3DSAMP_ADDRESSV));

        V_RETURN(pSavestateImpl->PreserveD3D9RenderState(D3DRS_ZENABLE));
        V_RETURN(pSavestateImpl->PreserveD3D9RenderState(D3DRS_ZWRITEENABLE));
        V_RETURN(pSavestateImpl->PreserveD3D9RenderState(D3DRS_FILLMODE));
        V_RETURN(pSavestateImpl->PreserveD3D9RenderState(D3DRS_CULLMODE));
        V_RETURN(pSavestateImpl->PreserveD3D9RenderState(D3DRS_ALPHABLENDENABLE));
        V_RETURN(pSavestateImpl->PreserveD3D9RenderState(D3DRS_ALPHATESTENABLE));
        V_RETURN(pSavestateImpl->PreserveD3D9RenderState(D3DRS_COLORWRITEENABLE));
        V_RETURN(pSavestateImpl->PreserveD3D9RenderState(D3DRS_STENCILENABLE));

        for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
        {
            V_RETURN(cascade_states[cascade].m_pQuadMesh->PreserveState(NULL, pSavestateImpl));
        }
    }

    for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
    {
        if(cascade_states[cascade].m_gradient_map_version == cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion())
            continue;

		// DX9 FOAM

		// Rendering folding to gradient map //////////////////////////////////
        // Set targets
        LPDIRECT3DSURFACE9 new_target_gradmap;
        V_RETURN(cascade_states[cascade].m_d3d._9.m_pd3d9GradientMap[m_active_GPU_slot]->GetSurfaceLevel(0, &new_target_gradmap));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderTarget(0, new_target_gradmap));
        SAFE_RELEASE(new_target_gradmap);

        V_RETURN(m_d3d._9.m_pd3d9Device->SetDepthStencilSurface(NULL));

		// Clear the gradient map if necessary
		const D3DCOLOR kBlack = 0x00000000;
		if(cascade_states[cascade].m_gradient_map_needs_clear[m_active_GPU_slot]) {
			V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_COLORWRITEENABLE	, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA));
			V_RETURN(m_d3d._9.m_pd3d9Device->Clear(0,NULL,D3DCLEAR_TARGET,kBlack,0.f,0));
			cascade_states[cascade].m_gradient_map_needs_clear[m_active_GPU_slot] = false;
		}

        // Shaders
        V_RETURN(m_d3d._9.m_pd3d9Device->SetVertexShader(m_d3d._9.m_pd3d9GradCalcVS));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShader(m_d3d._9.m_pd3d9GradCalcPS));

        // Constants
        int dmap_dim =m_params.cascades[cascade].fft_resolution;

		gfsdk_float4 oneLeft = gfsdk_make_float4(-1.0f/dmap_dim, 0, 0, 0);
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(1, (FLOAT*)&oneLeft, 1));
        gfsdk_float4 oneRight = gfsdk_make_float4( 1.0f/dmap_dim, 0, 0, 0);
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(2, (FLOAT*)&oneRight, 1));
        gfsdk_float4 oneBack = gfsdk_make_float4( 0,-1.0f/dmap_dim, 0, 0);
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(3, (FLOAT*)&oneBack, 1));
        gfsdk_float4 oneFront = gfsdk_make_float4( 0, 1.0f/dmap_dim, 0, 0);
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(4, (FLOAT*)&oneFront, 1));
        //
		gfsdk_F32 fGradMap2TexelWSScale = 0.5f*dmap_dim / m_params.cascades[cascade].fft_period;
		gfsdk_F32 fChoppyScale = m_params.cascades[cascade].choppy_scale * dmap_dim / m_params.cascades[cascade].fft_period;
		if(m_params.cascades[0].fft_period > 1000.0f) fChoppyScale *= 1.0f + 0.2f * log(m_params.cascades[0].fft_period/1000.0f);
		gfsdk_float4 g_Scales = gfsdk_make_float4(fChoppyScale,fGradMap2TexelWSScale,0.f,0.f);
		V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(0, (FLOAT*)&g_Scales, 1));

        // Textures/samplers
        V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(0, cascade_states[cascade].m_pFFTSimulation->GetDisplacementMapD3D9()));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));

        // Render state
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ZENABLE			, FALSE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ZWRITEENABLE		, FALSE)); 
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_FILLMODE			, D3DFILL_SOLID));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_CULLMODE			, D3DCULL_NONE ));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ALPHABLENDENABLE	, FALSE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ALPHATESTENABLE	, FALSE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_COLORWRITEENABLE	, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_STENCILENABLE		, FALSE));

		// Draw
		V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(NULL, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));

		// Accumulating energy in foam energy map //////////////////////////////////

        // Set targets
        LPDIRECT3DSURFACE9 new_target_foamenergymap;
        V_RETURN(cascade_states[cascade].m_d3d._9.m_pd3d9FoamEnergyMap->GetSurfaceLevel(0, &new_target_foamenergymap));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderTarget(0, new_target_foamenergymap));
        SAFE_RELEASE(new_target_foamenergymap);

        V_RETURN(m_d3d._9.m_pd3d9Device->SetDepthStencilSurface(NULL));

		// Clear the foam map, to ensure inter-frame deps get broken on multi-GPU
		V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_COLORWRITEENABLE	, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA));
		V_RETURN(m_d3d._9.m_pd3d9Device->Clear(0,NULL,D3DCLEAR_TARGET,kBlack,0.f,0));

        // Shaders
        V_RETURN(m_d3d._9.m_pd3d9Device->SetVertexShader(m_d3d._9.m_pd3d9FoamGenVS));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShader(m_d3d._9.m_pd3d9FoamGenPS));

        // Constants
		gfsdk_float4 g_DissipationFactors;
		g_DissipationFactors.z    = m_params.cascades[cascade].foam_generation_amount*(float)m_dFoamSimDeltaTime*50.0f; 
			//nvsf_g_DissipationFactors_Accumulation
		g_DissipationFactors.y	  = pow(m_params.cascades[cascade].foam_falloff_speed,(float)m_dFoamSimDeltaTime*50.0f);
			//nvsf_g_DissipationFactors_Fadeout
		g_DissipationFactors.x 	  = min(0.5f,m_params.cascades[cascade].foam_dissipation_speed*(float)m_dFoamSimDeltaTime*m_params.cascades[0].fft_period * (1000.0f/m_params.cascades[0].fft_period)/m_params.cascades[cascade].fft_period)/dmap_dim;
			//g_DissipationFactors_BlurExtents
		g_DissipationFactors.w    = m_params.cascades[cascade].foam_generation_threshold;
			//nvsf_g_FoamGenerationThreshold
		gfsdk_float4 g_SourceComponents = gfsdk_make_float4(0,0,0.0f,1.0f); // getting component W of grad map as source for energy
		gfsdk_float4 g_UVOffsets = gfsdk_make_float4(0,1.0f,0,0);			 // blurring by Y
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(0, (FLOAT*)&g_DissipationFactors, 1));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(1, (FLOAT*)&g_SourceComponents, 1));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(2, (FLOAT*)&g_UVOffsets, 1));

        // Textures / samplers
        V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(0, cascade_states[cascade].m_d3d._9.m_pd3d9GradientMap[m_active_GPU_slot]));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));

        // Render state
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ZENABLE			, FALSE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ZWRITEENABLE		, FALSE)); 
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_FILLMODE			, D3DFILL_SOLID));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_CULLMODE			, D3DCULL_NONE ));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ALPHABLENDENABLE	, FALSE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ALPHATESTENABLE	, FALSE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_COLORWRITEENABLE	, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_STENCILENABLE		, FALSE));

        // Draw
        V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(NULL, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));

		// Writing back energy to gradient map //////////////////////////////////

        // Set targets
        LPDIRECT3DSURFACE9 new_target_gradmap_writeback;
        V_RETURN(cascade_states[cascade].m_d3d._9.m_pd3d9GradientMap[m_active_GPU_slot]->GetSurfaceLevel(0, &new_target_gradmap_writeback));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderTarget(0, new_target_gradmap_writeback));
        SAFE_RELEASE(new_target_gradmap_writeback);

        V_RETURN(m_d3d._9.m_pd3d9Device->SetDepthStencilSurface(NULL));

        // Shaders
        V_RETURN(m_d3d._9.m_pd3d9Device->SetVertexShader(m_d3d._9.m_pd3d9FoamGenVS));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShader(m_d3d._9.m_pd3d9FoamGenPS));

        // Constants
		g_DissipationFactors.z    = 0; 
			//nvsf_g_DissipationFactors_Accumulation
		g_DissipationFactors.y	  = 1.0f;
			//nvsf_g_DissipationFactors_Fadeout
		g_DissipationFactors.x 	  = min(0.5f,m_params.cascades[cascade].foam_dissipation_speed*(float)m_dFoamSimDeltaTime*m_params.cascades[0].fft_period * (1000.0f/m_params.cascades[0].fft_period)/m_params.cascades[cascade].fft_period)/dmap_dim;
			//g_DissipationFactors_BlurExtents
		g_DissipationFactors.w    = 0;
			//nvsf_g_FoamGenerationThreshold
		g_SourceComponents = gfsdk_make_float4(1.0f,0,0,0); // getting component R of energy map as source for energy
		g_UVOffsets = gfsdk_make_float4(1.0f,0,0,0);			 // blurring by Y
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(0, (FLOAT*)&g_DissipationFactors, 1));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(1, (FLOAT*)&g_SourceComponents, 1));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(2, (FLOAT*)&g_UVOffsets, 1));

        // Textures / samplers
        V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(0, cascade_states[cascade].m_d3d._9.m_pd3d9FoamEnergyMap));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));

        // Render state
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ZENABLE			, FALSE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ZWRITEENABLE		, FALSE)); 
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_FILLMODE			, D3DFILL_SOLID));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_CULLMODE			, D3DCULL_NONE ));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ALPHABLENDENABLE	, FALSE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_ALPHATESTENABLE	, FALSE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_COLORWRITEENABLE	, D3DCOLORWRITEENABLE_ALPHA));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(D3DRS_STENCILENABLE		, FALSE));

        // Draw
        V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(NULL, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));
		
        cascade_states[cascade].m_gradient_map_version = cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion();
    }

    return S_OK;
#else
return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::updateGradientMapsD3D11(Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl)
{
#if WAVEWORKS_ENABLE_D3D11
    HRESULT hr;

	ID3D11DeviceContext* pDC_d3d11 = pGC->d3d11();

    // Preserve
    if(pSavestateImpl)
    {
        V_RETURN(pSavestateImpl->PreserveD3D11Viewport(pDC_d3d11));
        V_RETURN(pSavestateImpl->PreserveD3D11RenderTargets(pDC_d3d11));
        V_RETURN(pSavestateImpl->PreserveD3D11Shaders(pDC_d3d11));
        V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderConstantBuffer(pDC_d3d11,0));
        V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderSampler(pDC_d3d11,0));
        V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderResource(pDC_d3d11,0));
        V_RETURN(pSavestateImpl->PreserveD3D11DepthStencil(pDC_d3d11));
        V_RETURN(pSavestateImpl->PreserveD3D11Blend(pDC_d3d11));
        V_RETURN(pSavestateImpl->PreserveD3D11Raster(pDC_d3d11));

        for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
        {
            V_RETURN(cascade_states[cascade].m_pQuadMesh->PreserveState(pGC, pSavestateImpl));
        }
    }

    for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
    {
        if(cascade_states[cascade].m_gradient_map_version == cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion())
            continue;

		// Clear the gradient map if necessary
		const FLOAT kBlack[] = {0.f,0.f,0.f,0.f};
		if(cascade_states[cascade].m_gradient_map_needs_clear[m_active_GPU_slot]) {
			pDC_d3d11->ClearRenderTargetView(cascade_states[cascade].m_d3d._11.m_pd3d11GradientRenderTarget[m_active_GPU_slot],kBlack);
			cascade_states[cascade].m_gradient_map_needs_clear[m_active_GPU_slot] = false;
		}

				// Rendering folding to gradient map //////////////////////////////////
		
        // Render-targets + viewport
        pDC_d3d11->OMSetRenderTargets(1, &cascade_states[cascade].m_d3d._11.m_pd3d11GradientRenderTarget[m_active_GPU_slot], NULL);

        int dmap_dim =m_params.cascades[cascade].fft_resolution;
        D3D11_VIEWPORT new_vp;
        new_vp.TopLeftX = 0;
        new_vp.TopLeftY = 0;
        new_vp.Width = FLOAT(dmap_dim);
        new_vp.Height = FLOAT(dmap_dim);
        new_vp.MinDepth = 0.f;
        new_vp.MaxDepth = 0.f;
        UINT num_new_vp = 1;
        pDC_d3d11->RSSetViewports(num_new_vp, &new_vp);

        // Shaders
        pDC_d3d11->VSSetShader(m_d3d._11.m_pd3d11GradCalcVS, NULL, 0);
        pDC_d3d11->HSSetShader(NULL,NULL,0);
		pDC_d3d11->DSSetShader(NULL,NULL,0);
        pDC_d3d11->GSSetShader(NULL,NULL,0);
        pDC_d3d11->PSSetShader(m_d3d._11.m_pd3d11GradCalcPS, NULL, 0);

        // Constants
		{
			D3D11_CB_Updater<ps_calcgradient_cbuffer> cbu(pDC_d3d11,m_d3d._11.m_pd3d11GradCalcPixelShaderCB);
			cbu.cb().g_ChoppyScale = m_params.cascades[cascade].choppy_scale * dmap_dim / m_params.cascades[cascade].fft_period;
			if(m_params.cascades[0].fft_period > 1000.0f) cbu.cb().g_ChoppyScale *= 1.0f + 0.2f * log(m_params.cascades[0].fft_period/1000.0f);
			cbu.cb().g_GradMap2TexelWSScale = 0.5f*dmap_dim / m_params.cascades[cascade].fft_period ; 
			cbu.cb().g_OneTexel_Left = gfsdk_make_float4(-1.0f/dmap_dim, 0, 0, 0);
			cbu.cb().g_OneTexel_Right = gfsdk_make_float4( 1.0f/dmap_dim, 0, 0, 0);
			cbu.cb().g_OneTexel_Back = gfsdk_make_float4( 0,-1.0f/dmap_dim, 0, 0);
			cbu.cb().g_OneTexel_Front = gfsdk_make_float4( 0, 1.0f/dmap_dim, 0, 0);
		}
        pDC_d3d11->PSSetConstantBuffers(0, 1, &m_d3d._11.m_pd3d11GradCalcPixelShaderCB);

        // Textures/samplers
        pDC_d3d11->PSSetShaderResources(0, 1, cascade_states[cascade].m_pFFTSimulation->GetDisplacementMapD3D11());
        pDC_d3d11->PSSetSamplers(0, 1, &m_d3d._11.m_pd3d11PointSampler);

		// Render state
		pDC_d3d11->OMSetDepthStencilState(m_d3d._11.m_pd3d11NoDepthStencil, 0);
		pDC_d3d11->OMSetBlendState(m_d3d._11.m_pd3d11CalcGradBlendState, NULL, 0xFFFFFFFF);
		pDC_d3d11->RSSetState(m_d3d._11.m_pd3d11AlwaysSolidRasterizer);
		// Draw
		V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(pGC, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));

		// Accumulating energy in foam energy map //////////////////////////////////

		// Clear the foam map, to ensure inter-frame deps get broken on multi-GPU
		pDC_d3d11->ClearRenderTargetView(cascade_states[cascade].m_d3d._11.m_pd3d11FoamEnergyRenderTarget,kBlack);

        // Render-targets + viewport
		pDC_d3d11->OMSetRenderTargets(1, &cascade_states[cascade].m_d3d._11.m_pd3d11FoamEnergyRenderTarget, NULL);

        dmap_dim = m_params.cascades[cascade].fft_resolution;
        new_vp.TopLeftX = 0;
        new_vp.TopLeftY = 0;
        new_vp.Width = FLOAT(dmap_dim);
        new_vp.Height = FLOAT(dmap_dim);
        new_vp.MinDepth = 0.f;
        new_vp.MaxDepth = 0.f;
        num_new_vp = 1;
        pDC_d3d11->RSSetViewports(num_new_vp, &new_vp);

        // Shaders
		pDC_d3d11->VSSetShader(m_d3d._11.m_pd3d11FoamGenVS,NULL,0);
        pDC_d3d11->HSSetShader(NULL,NULL,0);
		pDC_d3d11->DSSetShader(NULL,NULL,0);
        pDC_d3d11->GSSetShader(NULL,NULL,0);
		pDC_d3d11->PSSetShader(m_d3d._11.m_pd3d11FoamGenPS,NULL,0);

		// Constants
		{
			D3D11_CB_Updater<ps_foamgeneration_cbuffer> cbu(pDC_d3d11,m_d3d._11.m_pd3d11FoamGenPixelShaderCB);
			cbu.cb().g_SourceComponents = gfsdk_make_float4(0,0,0.0f,1.0f); // getting component W of grad map as source for energy
			cbu.cb().g_UVOffsets = gfsdk_make_float4(0,1.0f,0,0); // blurring by Y
			cbu.cb().nvsf_g_DissipationFactors_Accumulation = m_params.cascades[cascade].foam_generation_amount*(float)m_dFoamSimDeltaTime*50.0f; 
			cbu.cb().nvsf_g_DissipationFactors_Fadeout		= pow(m_params.cascades[cascade].foam_falloff_speed,(float)m_dFoamSimDeltaTime*50.0f);
			cbu.cb().nvsf_g_DissipationFactors_BlurExtents	= min(0.5f,m_params.cascades[cascade].foam_dissipation_speed*(float)m_dFoamSimDeltaTime*m_params.cascades[0].fft_period * (1000.0f/m_params.cascades[0].fft_period)/m_params.cascades[cascade].fft_period)/dmap_dim;
			cbu.cb().nvsf_g_FoamGenerationThreshold			= m_params.cascades[cascade].foam_generation_threshold;
		}
		pDC_d3d11->PSSetConstantBuffers(0, 1, &m_d3d._11.m_pd3d11FoamGenPixelShaderCB);

        // Textures/samplers
        pDC_d3d11->PSSetShaderResources(0, 1, &cascade_states[cascade].m_d3d._11.m_pd3d11GradientMap[m_active_GPU_slot]);
		pDC_d3d11->PSSetSamplers(0, 1, &m_d3d._11.m_pd3d11LinearNoMipSampler);

		// Render state
		pDC_d3d11->OMSetDepthStencilState(m_d3d._11.m_pd3d11NoDepthStencil, 0);
		pDC_d3d11->OMSetBlendState(m_d3d._11.m_pd3d11AccumulateFoamBlendState, NULL, 0xFFFFFFFF);
		pDC_d3d11->RSSetState(m_d3d._11.m_pd3d11AlwaysSolidRasterizer);

		// Draw
		V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(pGC, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));

		// Clear shader resource from inputs
		ID3D11ShaderResourceView* pNullSRV = NULL;
		pDC_d3d11->PSSetShaderResources(0, 1, &pNullSRV);

		// Writing back energy to gradient map //////////////////////////////////

        // Render-targets + viewport
		pDC_d3d11->OMSetRenderTargets(1, &cascade_states[cascade].m_d3d._11.m_pd3d11GradientRenderTarget[m_active_GPU_slot], NULL);

        dmap_dim = m_params.cascades[cascade].fft_resolution;
        new_vp.TopLeftX = 0;
        new_vp.TopLeftY = 0;
        new_vp.Width = FLOAT(dmap_dim);
        new_vp.Height = FLOAT(dmap_dim);
        new_vp.MinDepth = 0.f;
        new_vp.MaxDepth = 0.f;
        num_new_vp = 1;
        pDC_d3d11->RSSetViewports(num_new_vp, &new_vp);

        // Shaders
		pDC_d3d11->VSSetShader(m_d3d._11.m_pd3d11FoamGenVS,NULL,0);
        pDC_d3d11->HSSetShader(NULL,NULL,0);
		pDC_d3d11->DSSetShader(NULL,NULL,0);
        pDC_d3d11->GSSetShader(NULL,NULL,0);
		pDC_d3d11->PSSetShader(m_d3d._11.m_pd3d11FoamGenPS,NULL,0);

		// Constants
		{
			D3D11_CB_Updater<ps_foamgeneration_cbuffer> cbu(pDC_d3d11,m_d3d._11.m_pd3d11FoamGenPixelShaderCB);
			cbu.cb().g_SourceComponents = gfsdk_make_float4(1.0f,0,0,0); // getting component R of energy map as source for energy
			cbu.cb().g_UVOffsets = gfsdk_make_float4(1.0f,0,0,0); // blurring by X
			cbu.cb().nvsf_g_DissipationFactors_Accumulation = 0.0f; 
			cbu.cb().nvsf_g_DissipationFactors_Fadeout		= 1.0f;
			cbu.cb().nvsf_g_DissipationFactors_BlurExtents	= min(0.5f,m_params.cascades[cascade].foam_dissipation_speed*(float)m_dFoamSimDeltaTime* (1000.0f/m_params.cascades[0].fft_period) * m_params.cascades[0].fft_period/m_params.cascades[cascade].fft_period)/dmap_dim;
			cbu.cb().nvsf_g_FoamGenerationThreshold			= m_params.cascades[cascade].foam_generation_threshold;
		}    
		pDC_d3d11->PSSetConstantBuffers(0, 1, &m_d3d._11.m_pd3d11FoamGenPixelShaderCB);

        // Textures/samplers
		pDC_d3d11->PSSetShaderResources(0, 1, &cascade_states[cascade].m_d3d._11.m_pd3d11FoamEnergyMap);
		pDC_d3d11->PSSetSamplers(0, 1, &m_d3d._11.m_pd3d11LinearNoMipSampler);

		// Render state
		pDC_d3d11->OMSetDepthStencilState(m_d3d._11.m_pd3d11NoDepthStencil, 0);
		pDC_d3d11->OMSetBlendState(m_d3d._11.m_pd3d11WriteAccumulatedFoamBlendState, NULL, 0xFFFFFFFF);
		pDC_d3d11->RSSetState(m_d3d._11.m_pd3d11AlwaysSolidRasterizer);
		
		// Draw
		V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(pGC, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));

        // Generate mips
        pDC_d3d11->GenerateMips(cascade_states[cascade].m_d3d._11.m_pd3d11GradientMap[m_active_GPU_slot]);

        cascade_states[cascade].m_gradient_map_version = cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion();
    }

	// Clear any lingering displacement map reference
	ID3D11ShaderResourceView* pNullSRV = NULL;
	pDC_d3d11->PSSetShaderResources(0, 1, &pNullSRV);

    return S_OK;
#else
return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::updateGradientMapsGL2(Graphics_Context* GL_ONLY(pGC))
{
#if WAVEWORKS_ENABLE_GL
	HRESULT hr;
    
	// No state preservation in GL

    for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
    {
        if(cascade_states[cascade].m_gradient_map_version == cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion()) continue;

		// Rendering folding to gradient map //////////////////////////////////
		// Set render target
		NVSDK_GLFunctions.glBindFramebuffer(GL_FRAMEBUFFER, cascade_states[cascade].m_d3d._GL2.m_GL2GradientFBO[m_active_GPU_slot]); CHECK_GL_ERRORS;
		const GLenum bufs = GL_COLOR_ATTACHMENT0;
		NVSDK_GLFunctions.glDrawBuffers(1, &bufs); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glViewport(0, 0, (GLsizei)m_params.cascades[cascade].fft_resolution,(GLsizei)m_params.cascades[cascade].fft_resolution); CHECK_GL_ERRORS;

		// Clear the gradient map if necessary
		if(cascade_states[cascade].m_gradient_map_needs_clear[m_active_GPU_slot]) 
		{
			NVSDK_GLFunctions.glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glClearColor(0.0f,0.0f,0.0f,0.0f); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glClear(GL_COLOR_BUFFER_BIT); CHECK_GL_ERRORS;
			cascade_states[cascade].m_gradient_map_needs_clear[m_active_GPU_slot] = false;
		}
        // Shaders
		NVSDK_GLFunctions.glUseProgram(m_d3d._GL2.m_GradCalcProgram); CHECK_GL_ERRORS;

        // Constants
        int dmap_dim =m_params.cascades[cascade].fft_resolution;

        float choppyScale = m_params.cascades[cascade].choppy_scale * dmap_dim / m_params.cascades[cascade].fft_period;
		if(m_params.cascades[0].fft_period > 1000.0f) choppyScale *= 1.0f + 0.2f * log(m_params.cascades[0].fft_period/1000.0f);
		float g_GradMap2TexelWSScale = 0.5f*dmap_dim / m_params.cascades[cascade].fft_period;

		gfsdk_float4 scales = gfsdk_make_float4(choppyScale, g_GradMap2TexelWSScale, 0, 0);
		NVSDK_GLFunctions.glUniform4fv(m_d3d._GL2.m_GradCalcUniformLocation_Scales, 1, (GLfloat*)&scales); CHECK_GL_ERRORS;

		gfsdk_float4 oneLeft = gfsdk_make_float4(-1.0f/dmap_dim, 0, 0, 0);
		NVSDK_GLFunctions.glUniform4fv(m_d3d._GL2.m_GradCalcUniformLocation_OneLeft, 1, (GLfloat*)&oneLeft); CHECK_GL_ERRORS;

        gfsdk_float4 oneRight = gfsdk_make_float4( 1.0f/dmap_dim, 0, 0, 0);
		NVSDK_GLFunctions.glUniform4fv(m_d3d._GL2.m_GradCalcUniformLocation_OneRight, 1, (GLfloat*)&oneRight); CHECK_GL_ERRORS;

        gfsdk_float4 oneBack = gfsdk_make_float4( 0,-1.0f/dmap_dim, 0, 0);
		NVSDK_GLFunctions.glUniform4fv(m_d3d._GL2.m_GradCalcUniformLocation_OneBack, 1, (GLfloat*)&oneBack); CHECK_GL_ERRORS;

        gfsdk_float4 oneFront = gfsdk_make_float4( 0, 1.0f/dmap_dim, 0, 0);
		NVSDK_GLFunctions.glUniform4fv(m_d3d._GL2.m_GradCalcUniformLocation_OneFront, 1, (GLfloat*)&oneFront); CHECK_GL_ERRORS;

        // Textures/samplers
		NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + m_d3d._GL2.m_GradCalcTextureUnit_DisplacementMap); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[cascade].m_pFFTSimulation->GetDisplacementMapGL2()); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;	
		NVSDK_GLFunctions.glUniform1i(m_d3d._GL2.m_GradCalcTextureBindLocation_DisplacementMap, m_d3d._GL2.m_GradCalcTextureUnit_DisplacementMap); CHECK_GL_ERRORS;

        // Render state
		NVSDK_GLFunctions.glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_FALSE); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_DEPTH_TEST); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_BLEND); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_STENCIL_TEST); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_CULL_FACE); CHECK_GL_ERRORS;

		// Draw
		const UINT calcGradAttribLocations[] = { m_d3d._GL2.m_GradCalcAttributeLocation_Pos, m_d3d._GL2.m_GradCalcAttributeLocation_TexCoord };
		V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(NULL, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, calcGradAttribLocations));
		NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERRORS;

		// Accumulating energy in foam energy map //////////////////////////////////
		
        // Set targets
		NVSDK_GLFunctions.glBindFramebuffer(GL_FRAMEBUFFER, cascade_states[cascade].m_d3d._GL2.m_GL2FoamEnergyFBO); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glViewport(0, 0, (GLsizei)m_params.cascades[cascade].fft_resolution,(GLsizei)m_params.cascades[cascade].fft_resolution); CHECK_GL_ERRORS;

		// Clear the foam map, to ensure inter-frame deps get broken on multi-GPU
		NVSDK_GLFunctions.glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glClearColor(0.0f,0.0f,0.0f,0.0f); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glClear(GL_COLOR_BUFFER_BIT); CHECK_GL_ERRORS;

        // Shaders
		NVSDK_GLFunctions.glUseProgram(m_d3d._GL2.m_FoamGenProgram); CHECK_GL_ERRORS;

        // Constants
		gfsdk_float4 g_DissipationFactors;
		g_DissipationFactors.z    = m_params.cascades[cascade].foam_generation_amount*(float)m_dFoamSimDeltaTime*50.0f; 
		g_DissipationFactors.y	  = pow(m_params.cascades[cascade].foam_falloff_speed,(float)m_dFoamSimDeltaTime*50.0f);
		g_DissipationFactors.x 	  = min(0.5f,m_params.cascades[cascade].foam_dissipation_speed*(float)m_dFoamSimDeltaTime*m_params.cascades[0].fft_period * (1000.0f/m_params.cascades[0].fft_period)/m_params.cascades[cascade].fft_period)/dmap_dim;
		g_DissipationFactors.w    = m_params.cascades[cascade].foam_generation_threshold;
		gfsdk_float4 g_SourceComponents = gfsdk_make_float4(0,0,0.0f,1.0f); // getting component W of grad map as source for energy
		gfsdk_float4 g_UVOffsets = gfsdk_make_float4(0,1.0f,0,0);			 // blurring by Y
		NVSDK_GLFunctions.glUniform4fv(m_d3d._GL2.m_FoamGenUniformLocation_DissipationFactors, 1, (GLfloat*)&g_DissipationFactors); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glUniform4fv(m_d3d._GL2.m_FoamGenUniformLocation_SourceComponents, 1, (GLfloat*)&g_SourceComponents); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glUniform4fv(m_d3d._GL2.m_FoamGenUniformLocation_UVOffsets, 1, (GLfloat*)&g_UVOffsets); CHECK_GL_ERRORS;

        // Textures / samplers
		NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + m_d3d._GL2.m_FoamGenTextureUnit_EnergyMap); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[cascade].m_d3d._GL2.m_GL2GradientMap[m_active_GPU_slot]); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;	
		NVSDK_GLFunctions.glUniform1i(m_d3d._GL2.m_FoamGenTextureBindLocation_EnergyMap, m_d3d._GL2.m_FoamGenTextureUnit_EnergyMap); CHECK_GL_ERRORS;

        // Render state
		NVSDK_GLFunctions.glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_DEPTH_TEST); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_BLEND); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_STENCIL_TEST); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_CULL_FACE); CHECK_GL_ERRORS;

        // Draw
		const UINT foamGenAttribLocations[] = { m_d3d._GL2.m_FoamGenAttributeLocation_Pos, m_d3d._GL2.m_FoamGenAttributeLocation_TexCoord };
        V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(pGC, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, foamGenAttribLocations));
		NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, 0);
		
		// Writing back energy to gradient map //////////////////////////////////
		
        // Set targets
		NVSDK_GLFunctions.glBindFramebuffer(GL_FRAMEBUFFER, cascade_states[cascade].m_d3d._GL2.m_GL2GradientFBO[m_active_GPU_slot]); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glViewport(0, 0, (GLsizei)m_params.cascades[cascade].fft_resolution,(GLsizei)m_params.cascades[cascade].fft_resolution); CHECK_GL_ERRORS;

        // Shaders
		NVSDK_GLFunctions.glUseProgram(m_d3d._GL2.m_FoamGenProgram); CHECK_GL_ERRORS;

        // Constants
		g_DissipationFactors.z    = 0; 
		g_DissipationFactors.y	  = 1.0f;
		g_DissipationFactors.x 	  = min(0.5f,m_params.cascades[cascade].foam_dissipation_speed*(float)m_dFoamSimDeltaTime*m_params.cascades[0].fft_period * (1000.0f/m_params.cascades[0].fft_period)/m_params.cascades[cascade].fft_period)/dmap_dim;
		g_DissipationFactors.w    = 0;
		g_SourceComponents = gfsdk_make_float4(1.0f,0,0,0); // getting component R of energy map as source for energy
		g_UVOffsets = gfsdk_make_float4(1.0f,0,0,0);			 // blurring by Y
		NVSDK_GLFunctions.glUniform4fv(m_d3d._GL2.m_FoamGenUniformLocation_DissipationFactors, 1, (GLfloat*)&g_DissipationFactors); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glUniform4fv(m_d3d._GL2.m_FoamGenUniformLocation_SourceComponents, 1, (GLfloat*)&g_SourceComponents); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glUniform4fv(m_d3d._GL2.m_FoamGenUniformLocation_UVOffsets, 1, (GLfloat*)&g_UVOffsets); CHECK_GL_ERRORS;

        // Textures / samplers
		NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + m_d3d._GL2.m_FoamGenTextureUnit_EnergyMap); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[cascade].m_d3d._GL2.m_GL2FoamEnergyMap); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;	
		NVSDK_GLFunctions.glUniform1i(m_d3d._GL2.m_FoamGenTextureBindLocation_EnergyMap, m_d3d._GL2.m_FoamGenTextureUnit_EnergyMap); CHECK_GL_ERRORS;

        // Render state
		NVSDK_GLFunctions.glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_DEPTH_TEST); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_BLEND); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_STENCIL_TEST); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glDisable(GL_CULL_FACE); CHECK_GL_ERRORS;

        // Draw
        V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(NULL, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, foamGenAttribLocations));
		
        // Enabling writing to all color components of RT
		NVSDK_GLFunctions.glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
		// building mipmaps for gradient texture if gradient texture arrays are not used
		if(m_params.use_texture_arrays == false)
		{
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[cascade].m_d3d._GL2.m_GL2GradientMap[m_active_GPU_slot]); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glGenerateMipmap(GL_TEXTURE_2D); CHECK_GL_ERRORS;
		}
		else
		{
			// if texture arrays are used, then mipmaps will be generated for the gradient texture array after blitting to it
		}
        cascade_states[cascade].m_gradient_map_version = cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion();

    }

    return S_OK;
#else
return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::updateGradientMapsGnm(Graphics_Context* GNM_ONLY(pGC), GFSDK_WaveWorks_Savestate* GNM_ONLY(pSavestateImpl))
{
#if WAVEWORKS_ENABLE_GNM
    HRESULT hr;

	sce::Gnmx::LightweightGfxContext* gfxContext = pGC->gnm();

    // Preserve
    if(pSavestateImpl)
    {
		/*
        V_RETURN(pSavestateImpl->PreserveGnmViewport(context));
        V_RETURN(pSavestateImpl->PreserveGnmRenderTargets(context));
        V_RETURN(pSavestateImpl->PreserveGnmShaders(context));
        V_RETURN(pSavestateImpl->PreserveGnmPixelShaderConstantBuffer(context,0));
        V_RETURN(pSavestateImpl->PreserveGnmPixelShaderSampler(context,0));
        V_RETURN(pSavestateImpl->PreserveGnmPixelShaderResource(context,0));
        V_RETURN(pSavestateImpl->PreserveGnmDepthStencil(context));
        V_RETURN(pSavestateImpl->PreserveGnmBlend(context));
        V_RETURN(pSavestateImpl->PreserveGnmRaster(context));

        for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
        {
            V_RETURN(cascade_states[cascade].m_pQuadMesh->PreserveState(pGC, pSavestateImpl));
        }
		*/
    }

	GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();
    for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
    {
        if(cascade_states[cascade].m_gradient_map_version == cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion()) continue;

		Gnm::RenderTarget* pCascadeGradientRT = &cascade_states[cascade].m_d3d._gnm.m_gnmGradientRenderTarget[m_active_GPU_slot];
		
		int dmap_dim = m_params.cascades[cascade].fft_resolution;

		// Clear the gradient map if necessary
		if(cascade_states[cascade].m_gradient_map_needs_clear[m_active_GPU_slot]) 
		{
			GFSDK_WaveWorks_GNM_Util::ClearRenderTargetToZero(m_d3d._gnm.m_pGnmRenderTargetClearer,*gfxContext, pCascadeGradientRT);
			cascade_states[cascade].m_gradient_map_needs_clear[m_active_GPU_slot] = false;
		}

		// Rendering folding to gradient map //////////////////////////////////
		// Shaders
		gnmxWrap->setActiveShaderStages(*gfxContext, Gnm::kActiveShaderStagesVsPs);
		gnmxWrap->setVsShader(*gfxContext, m_d3d._gnm.m_pGnmGradCalcVS, 0, m_d3d._gnm.m_pGnmGradCalcFS, m_d3d._gnm.m_pGnmGradCalcVSResourceOffsets);
		gnmxWrap->setPsShader(*gfxContext, m_d3d._gnm.m_pGnmGradCalcPS, m_d3d._gnm.m_pGnmGradCalcPSResourceOffsets);

		// Render-targets + viewport
		gnmxWrap->setRenderTarget(*gfxContext, 0, pCascadeGradientRT);
		gnmxWrap->setDepthRenderTarget(*gfxContext, NULL);
		gnmxWrap->setupScreenViewport(*gfxContext, 0, 0, dmap_dim, dmap_dim, 0.5f, 0.5f); // 1.0f, 0.0f for D3D style (?)

        
        // Constants
		ps_calcgradient_cbuffer* pPSCB = (ps_calcgradient_cbuffer*)gnmxWrap->allocateFromCommandBuffer(*gfxContext, sizeof(ps_calcgradient_cbuffer), Gnm::kEmbeddedDataAlignment4);
		pPSCB->g_ChoppyScale = m_params.cascades[cascade].choppy_scale * dmap_dim / m_params.cascades[cascade].fft_period;
		if(m_params.cascades[0].fft_period > 1000.0f) pPSCB->g_ChoppyScale *= 1.0f + 0.2f * log(m_params.cascades[0].fft_period/1000.0f);
		pPSCB->g_GradMap2TexelWSScale = 0.5f*dmap_dim / m_params.cascades[cascade].fft_period ; 
		pPSCB->g_OneTexel_Left = gfsdk_make_float4(-1.0f/dmap_dim, 0, 0, 0);
		pPSCB->g_OneTexel_Right = gfsdk_make_float4( 1.0f/dmap_dim, 0, 0, 0);
		pPSCB->g_OneTexel_Back = gfsdk_make_float4( 0,-1.0f/dmap_dim, 0, 0);
		pPSCB->g_OneTexel_Front = gfsdk_make_float4( 0, 1.0f/dmap_dim, 0, 0);

		Gnm::Buffer buffer;
		buffer.initAsConstantBuffer(pPSCB, sizeof(*pPSCB));
		buffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO);

        gnmxWrap->setConstantBuffers(*gfxContext, Gnm::kShaderStagePs, 0, 1, &buffer);

        // Textures/samplers
		gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStagePs, 0, 1, cascade_states[cascade].m_pFFTSimulation->GetDisplacementMapGnm());
		gnmxWrap->setSamplers(*gfxContext, Gnm::kShaderStagePs, 0, 1, &m_d3d._gnm.m_pGnmPointSampler);

		// Render state
		gnmxWrap->setDepthStencilControl(*gfxContext, m_d3d._gnm.m_pGnmNoDepthStencil);
		gnmxWrap->setBlendControl(*gfxContext, 0, m_d3d._gnm.m_pGnmCalcGradBlendState);
		gnmxWrap->setPrimitiveSetup(*gfxContext, m_d3d._gnm.m_pGnmAlwaysSolidRasterizer);
		gnmxWrap->setRenderTargetMask(*gfxContext, 0x7); // mask off alpha

		// Draw
		V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(pGC, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));
	}

	for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
	{
        if(cascade_states[cascade].m_gradient_map_version == cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion()) continue;

		Gnm::RenderTarget* pCascadeGradientRT = &cascade_states[cascade].m_d3d._gnm.m_gnmGradientRenderTarget[m_active_GPU_slot];
		Gnm::RenderTarget* pCascadeFoamEnergyRT = &cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyRenderTarget;

		int dmap_dim = m_params.cascades[cascade].fft_resolution;

		gnmxWrap->waitForGraphicsWrites(*gfxContext, pCascadeGradientRT->getBaseAddress256ByteBlocks(), GET_SIZE_IN_BYTES(pCascadeGradientRT)>>8,
			Gnm::kWaitTargetSlotCb0, Gnm::kCacheActionWriteBackAndInvalidateL1andL2, Gnm::kExtendedCacheActionFlushAndInvalidateCbCache, Gnm::kStallCommandBufferParserDisable);

		// Accumulating energy in foam energy map //////////////////////////////////

		// Clear the foam map, to ensure inter-frame deps get broken on multi-GPU
		// NB: PS4 is single-GPU so there *are* no inter-frame deps

        // Render-targets + viewport
		gnmxWrap->setRenderTarget(*gfxContext, 0, pCascadeFoamEnergyRT);
		gnmxWrap->setupScreenViewport(*gfxContext, 0, 0, dmap_dim, dmap_dim, 0.5f, 0.5f); // 1.0f, 0.0f for D3D style (?)

        // Shaders
		gnmxWrap->setActiveShaderStages(*gfxContext, Gnm::kActiveShaderStagesVsPs);
		gnmxWrap->setVsShader(*gfxContext, m_d3d._gnm.m_pGnmFoamGenVS, 0, m_d3d._gnm.m_pGnmFoamGenFS, m_d3d._gnm.m_pGnmFoamGenVSResourceOffsets);
		gnmxWrap->setPsShader(*gfxContext, m_d3d._gnm.m_pGnmFoamGenPS, m_d3d._gnm.m_pGnmFoamGenPSResourceOffsets);

		// Constants
		ps_foamgeneration_cbuffer* pFGCB = (ps_foamgeneration_cbuffer*)gnmxWrap->allocateFromCommandBuffer(*gfxContext, sizeof(ps_foamgeneration_cbuffer), Gnm::kEmbeddedDataAlignment4);
		pFGCB->g_SourceComponents = gfsdk_make_float4(0,0,0.0f,1.0f); // getting component W of grad map as source for energy
		pFGCB->g_UVOffsets = gfsdk_make_float4(0,1.0f,0,0); // blurring by Y
		pFGCB->nvsf_g_DissipationFactors_Accumulation = m_params.cascades[cascade].foam_generation_amount*(float)m_dFoamSimDeltaTime*50.0f; 
		pFGCB->nvsf_g_DissipationFactors_Fadeout		= pow(m_params.cascades[cascade].foam_falloff_speed,(float)m_dFoamSimDeltaTime*50.0f);
		pFGCB->nvsf_g_DissipationFactors_BlurExtents	= min(0.5f,m_params.cascades[cascade].foam_dissipation_speed*(float)m_dFoamSimDeltaTime*m_params.cascades[0].fft_period * (1000.0f/m_params.cascades[0].fft_period)/m_params.cascades[cascade].fft_period)/dmap_dim;
		pFGCB->nvsf_g_FoamGenerationThreshold			= m_params.cascades[cascade].foam_generation_threshold;

		Gnm::Buffer buffer;
		buffer.initAsConstantBuffer(pFGCB, sizeof(*pFGCB));
		buffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO);

		gnmxWrap->setConstantBuffers(*gfxContext, Gnm::kShaderStagePs, 0, 1, &buffer);

        // Textures/samplers
		gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStagePs, 0, 1, &cascade_states[cascade].m_d3d._gnm.m_gnmGradientMap[m_active_GPU_slot]);
		gnmxWrap->setSamplers(*gfxContext, Gnm::kShaderStagePs, 0, 1, &m_d3d._gnm.m_pGnmLinearNoMipSampler);

		// Render state
		gnmxWrap->setDepthStencilControl(*gfxContext, m_d3d._gnm.m_pGnmNoDepthStencil);
		gnmxWrap->setBlendControl(*gfxContext, 0, m_d3d._gnm.m_pGnmAccumulateFoamBlendState);
		gnmxWrap->setPrimitiveSetup(*gfxContext, m_d3d._gnm.m_pGnmAlwaysSolidRasterizer);
		gnmxWrap->setRenderTargetMask(*gfxContext, 0xf);

		// Draw
		V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(pGC, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));
	}
	for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
	{
        if(cascade_states[cascade].m_gradient_map_version == cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion()) continue;

		Gnm::RenderTarget* pCascadeGradientRT = &cascade_states[cascade].m_d3d._gnm.m_gnmGradientRenderTarget[m_active_GPU_slot];
		Gnm::RenderTarget* pCascadeFoamEnergyRT = &cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyRenderTarget;

		int dmap_dim = m_params.cascades[cascade].fft_resolution;

		gnmxWrap->waitForGraphicsWrites(*gfxContext, pCascadeFoamEnergyRT->getBaseAddress256ByteBlocks(), GET_SIZE_IN_BYTES(pCascadeFoamEnergyRT)>>8,
			Gnm::kWaitTargetSlotCb0, Gnm::kCacheActionWriteBackAndInvalidateL1andL2, Gnm::kExtendedCacheActionFlushAndInvalidateCbCache, Gnm::kStallCommandBufferParserDisable);

		// Writing back energy to gradient map //////////////////////////////////

        // Render-targets + viewport
		gnmxWrap->setRenderTarget(*gfxContext, 0, pCascadeGradientRT);
		gnmxWrap->setupScreenViewport(*gfxContext, 0, 0, dmap_dim, dmap_dim, 0.5f, 0.5f); // 1.0f, 0.0f for D3D style (?)

        // Shaders
		gnmxWrap->setActiveShaderStages(*gfxContext, Gnm::kActiveShaderStagesVsPs);
		gnmxWrap->setVsShader(*gfxContext, m_d3d._gnm.m_pGnmFoamGenVS, 0, m_d3d._gnm.m_pGnmFoamGenFS, m_d3d._gnm.m_pGnmFoamGenVSResourceOffsets);
		gnmxWrap->setPsShader(*gfxContext, m_d3d._gnm.m_pGnmFoamGenPS, m_d3d._gnm.m_pGnmFoamGenPSResourceOffsets);

		// Constants
		ps_foamgeneration_cbuffer* pFGCB = (ps_foamgeneration_cbuffer*)gnmxWrap->allocateFromCommandBuffer(*gfxContext, sizeof(ps_foamgeneration_cbuffer), Gnm::kEmbeddedDataAlignment4);
		pFGCB->g_SourceComponents = gfsdk_make_float4(1.0f,0,0,0); // getting component R of energy map as source for energy
		pFGCB->g_UVOffsets = gfsdk_make_float4(1.0f,0,0,0); // blurring by X
		pFGCB->nvsf_g_DissipationFactors_Accumulation = 0.0f; 
		pFGCB->nvsf_g_DissipationFactors_Fadeout		= 1.0f;
		pFGCB->nvsf_g_DissipationFactors_BlurExtents	= min(0.5f,m_params.cascades[cascade].foam_dissipation_speed*(float)m_dFoamSimDeltaTime* (1000.0f/m_params.cascades[0].fft_period) * m_params.cascades[0].fft_period/m_params.cascades[cascade].fft_period)/dmap_dim;

		Gnm::Buffer buffer;
		buffer.initAsConstantBuffer(pFGCB, sizeof(*pFGCB));
		buffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO);

		gnmxWrap->setConstantBuffers(*gfxContext, Gnm::kShaderStagePs, 0, 1, &buffer);

        // Textures/samplers
		gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStagePs, 0, 1, &cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyMap);
		gnmxWrap->setSamplers(*gfxContext, Gnm::kShaderStagePs, 0, 1, &m_d3d._gnm.m_pGnmLinearNoMipSampler);

		// Render state
		gnmxWrap->setDepthStencilControl(*gfxContext, m_d3d._gnm.m_pGnmNoDepthStencil);
		gnmxWrap->setBlendControl(*gfxContext, 0, m_d3d._gnm.m_pGnmWriteAccumulatedFoamBlendState);
		gnmxWrap->setPrimitiveSetup(*gfxContext, m_d3d._gnm.m_pGnmAlwaysSolidRasterizer);
		gnmxWrap->setRenderTargetMask(*gfxContext, 0x8); // write alpha only

		// Draw
		V_RETURN(cascade_states[cascade].m_pQuadMesh->Draw(pGC, NVWaveWorks_Mesh::PT_TriangleStrip, 0, 0, 4, 0, 2, NULL));

	}

	gnmxWrap->setShaderType(*gfxContext, Gnm::kShaderTypeCompute);

	for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
	{
        if(cascade_states[cascade].m_gradient_map_version == cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion()) continue;

		int dmap_dim =m_params.cascades[cascade].fft_resolution;

		// build mip-map		
		gnmxWrap->setCsShader(*gfxContext, m_d3d._gnm.m_pGnmMipMapGenCS, m_d3d._gnm.m_pGnmMipMapGenCSResourceOffsets);
		Gnm::Texture mipTexture = cascade_states[cascade].m_d3d._gnm.m_gnmGradientMap[m_active_GPU_slot];
		mipTexture.setMipLevelRange(0, 0); // probably not necessary
		for(uint32_t level=1, width = dmap_dim / 2; width > 0; ++level, width >>= 1)
		{
			gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStageCs, 0, 1, &mipTexture);
			mipTexture.setMipLevelRange(level, level);
			gnmxWrap->setRwTextures(*gfxContext, Gnm::kShaderStageCs, 0, 1, &mipTexture);
			unsigned int widthInGroups = (width + 7) / 8;
			gnmxWrap->dispatch(*gfxContext, widthInGroups, widthInGroups, 1);
		}

        cascade_states[cascade].m_gradient_map_version = cascade_states[cascade].m_pFFTSimulation->getDisplacementMapVersion();
    }

	GFSDK_WaveWorks_GNM_Util::synchronizeComputeToGraphics( gnmxWrap->getDcb(*gfxContext) );
	gnmxWrap->setShaderType(*gfxContext, Gnm::kShaderTypeGraphics);

    return S_OK;
#else
return S_FALSE;
#endif
}

bool GFSDK_WaveWorks_Simulation::getStagingCursor(gfsdk_U64* pKickID)
{
	return m_pSimulationManager->getStagingCursor(pKickID);
}

bool GFSDK_WaveWorks_Simulation::getReadbackCursor(gfsdk_U64* pKickID)
{
	return m_pSimulationManager->getReadbackCursor(pKickID);
}

HRESULT GFSDK_WaveWorks_Simulation::advanceReadbackCursor(bool block, bool& wouldBlock)
{
	NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult advance_result = m_pSimulationManager->advanceReadbackCursor(block);
	switch(advance_result)
	{
	case NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult_Succeeded:
		wouldBlock = false;
		return S_OK;
	case NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult_WouldBlock:
		wouldBlock = true;
		return S_FALSE;
	case NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult_None:
		wouldBlock = false;
		return S_FALSE;
	case NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult_Failed:
	default:	// Drop-thru from prior case is intentional
		return E_FAIL;
	}
}

HRESULT GFSDK_WaveWorks_Simulation::advanceStagingCursor(Graphics_Context* pGC, bool block, bool& wouldBlock, GFSDK_WaveWorks_Savestate* pSavestateImpl)
{
	HRESULT hr;

	NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult advance_result = m_pSimulationManager->advanceStagingCursor(block);
	switch(advance_result)
	{
	case NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult_Succeeded:
		wouldBlock = false;
		break; // result, carry on...
	case NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult_WouldBlock:
		wouldBlock = true;
		return S_FALSE;
	case NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult_None:
		wouldBlock = false;
		return S_FALSE;
	case NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult_Failed:
	default:	// Drop-thru from prior case is intentional
		return E_FAIL;
	}

	TimerSlot* pTimerSlot = NULL;
	if(m_pGFXTimer)
	{
		// Check for completed queries
		V_RETURN(queryAllGfxTimers(pGC, m_pGFXTimer));

		// Bracket GPU work with a disjoint timer query
		V_RETURN(m_pGFXTimer->beginDisjoint(pGC));

		V_RETURN(consumeAvailableTimerSlot(pGC, m_pGFXTimer, m_gpu_wait_timers, &pTimerSlot));
		m_pGFXTimer->issueTimerQuery(pGC, pTimerSlot->m_StartQueryIndex);
		m_pGFXTimer->issueTimerQuery(pGC, pTimerSlot->m_StartGFXQueryIndex);

		m_has_consumed_wait_timer_slot_since_last_kick = true;
	}

	// If new simulation results have become available, it will be necessary to update the gradient maps
	V_RETURN(updateGradientMaps(pGC,pSavestateImpl));

	if(m_pGFXTimer)
	{
		m_pGFXTimer->issueTimerQuery(pGC, pTimerSlot->m_StopGFXQueryIndex);
		m_pGFXTimer->issueTimerQuery(pGC, pTimerSlot->m_StopQueryIndex);
		V_RETURN(m_pGFXTimer->endDisjoint(pGC));
	}

	return S_OK;
}

HRESULT GFSDK_WaveWorks_Simulation::waitStagingCursor()
{
	NVWaveWorks_FFT_Simulation_Manager::WaitCursorResult wait_result = m_pSimulationManager->waitStagingCursor();
	switch(wait_result)
	{
	case NVWaveWorks_FFT_Simulation_Manager::WaitCursorResult_Succeeded:
		return S_OK;
	case NVWaveWorks_FFT_Simulation_Manager::WaitCursorResult_None:
		return S_FALSE;
	case NVWaveWorks_FFT_Simulation_Manager::WaitCursorResult_Failed:
	default:	// Drop-thru from prior case is intentional
		return E_FAIL;
	}
}

HRESULT GFSDK_WaveWorks_Simulation::archiveDisplacements()
{
	return m_pSimulationManager->archiveDisplacements();
}

HRESULT GFSDK_WaveWorks_Simulation::setRenderState(	Graphics_Context* pGC,
													const gfsdk_float4x4& GFX_ONLY(matView),
													const UINT* GFX_ONLY(pShaderInputRegisterMappings),
													GFSDK_WaveWorks_Savestate* GFX_ONLY(pSavestateImpl),
													const GFSDK_WaveWorks_Simulation_GL_Pool* GL_ONLY(pGlPool)
													)
{
#if WAVEWORKS_ENABLE_GRAPHICS
	if(!getStagingCursor(NULL))
		return E_FAIL;

    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        return setRenderStateD3D9(matView, pShaderInputRegisterMappings, pSavestateImpl);
#endif
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        return setRenderStateD3D10(matView, pShaderInputRegisterMappings, pSavestateImpl);
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
		{
			return setRenderStateD3D11(pGC->d3d11(), matView, pShaderInputRegisterMappings, pSavestateImpl);
		}
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			return setRenderStateGnm(pGC->gnm(), matView, pShaderInputRegisterMappings, pSavestateImpl);
		}
#endif
#if WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_gl2:
		{
			if(NULL == pGlPool)
			{
				WaveWorks_Internal::diagnostic_message(TEXT("ERROR: a valid gl pool is required when setting simulation state for gl rendering\n"));
				return E_FAIL;
			}

			HRESULT hr = setRenderStateGL2(matView, pShaderInputRegisterMappings, *pGlPool);
			return hr;
		}
#endif
    default:
        return E_FAIL;
    }
#else// WAVEWORKS_ENABLE_GRAPHICS
	return E_FAIL;
#endif // WAVEWORKS_ENABLE_GRAPHICS
}

HRESULT GFSDK_WaveWorks_Simulation::setRenderStateD3D9(	const gfsdk_float4x4& D3D9_ONLY(matView),
															const UINT* D3D9_ONLY(pShaderInputRegisterMappings),
															GFSDK_WaveWorks_Savestate* D3D9_ONLY(pSavestateImpl)
															)
{
#if WAVEWORKS_ENABLE_D3D9
    HRESULT hr;

	const UINT rm_g_samplerDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputD3D9_g_samplerDisplacementMap0];
    const UINT rm_g_samplerDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputD3D9_g_samplerDisplacementMap1];
	const UINT rm_g_samplerDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputD3D9_g_samplerDisplacementMap2];
    const UINT rm_g_samplerDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputD3D9_g_samplerDisplacementMap3];
    const UINT rm_g_samplerGradientMap0 = pShaderInputRegisterMappings[ShaderInputD3D9_g_samplerGradientMap0];
    const UINT rm_g_samplerGradientMap1 = pShaderInputRegisterMappings[ShaderInputD3D9_g_samplerGradientMap1];
    const UINT rm_g_samplerGradientMap2 = pShaderInputRegisterMappings[ShaderInputD3D9_g_samplerGradientMap2];
    const UINT rm_g_samplerGradientMap3 = pShaderInputRegisterMappings[ShaderInputD3D9_g_samplerGradientMap3];
    const UINT rm_g_WorldEye = pShaderInputRegisterMappings[ShaderInputD3D9_g_WorldEye];
    const UINT rm_g_UVScaleCascade0123 = pShaderInputRegisterMappings[ShaderInputD3D9_g_UVScaleCascade0123];
    const UINT rm_g_TexelLength_x2_PS = pShaderInputRegisterMappings[ShaderInputD3D9_g_TexelLength_x2_PS];
    const UINT rm_g_Cascade1Scale_PS = pShaderInputRegisterMappings[ShaderInputD3D9_g_Cascade1Scale_PS];
    const UINT rm_g_Cascade1TexelScale_PS = pShaderInputRegisterMappings[ShaderInputD3D9_g_Cascade1TexelScale_PS];
    const UINT rm_g_Cascade1UVOffset_PS = pShaderInputRegisterMappings[ShaderInputD3D9_g_Cascade1UVOffset_PS];
    const UINT rm_g_Cascade2Scale_PS = pShaderInputRegisterMappings[ShaderInputD3D9_g_Cascade2Scale_PS];
    const UINT rm_g_Cascade2TexelScale_PS = pShaderInputRegisterMappings[ShaderInputD3D9_g_Cascade2TexelScale_PS];
    const UINT rm_g_Cascade2UVOffset_PS = pShaderInputRegisterMappings[ShaderInputD3D9_g_Cascade2UVOffset_PS];
    const UINT rm_g_Cascade3Scale_PS = pShaderInputRegisterMappings[ShaderInputD3D9_g_Cascade3Scale_PS];
    const UINT rm_g_Cascade3TexelScale_PS = pShaderInputRegisterMappings[ShaderInputD3D9_g_Cascade3TexelScale_PS];
    const UINT rm_g_Cascade3UVOffset_PS = pShaderInputRegisterMappings[ShaderInputD3D9_g_Cascade3UVOffset_PS];

    const DWORD gradMapMinFilterMode = m_params.aniso_level > 1 ? D3DTEXF_ANISOTROPIC : D3DTEXF_LINEAR;

    // Preserve state as necessary
    if(pSavestateImpl)
    {
        if(rm_g_samplerDisplacementMap0 != nvrm_unused)
        {
            const UINT displacementMapSampler = D3DVERTEXTEXTURESAMPLER0 + rm_g_samplerDisplacementMap0;
            V_RETURN(pSavestateImpl->PreserveD3D9Texture(displacementMapSampler));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MIPFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MINFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MAGFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_ADDRESSU));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_ADDRESSV));
        }
        if(rm_g_samplerDisplacementMap1 != nvrm_unused)
        {
            const UINT displacementMapSampler = D3DVERTEXTEXTURESAMPLER0 + rm_g_samplerDisplacementMap1;
            V_RETURN(pSavestateImpl->PreserveD3D9Texture(displacementMapSampler));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MIPFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MINFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MAGFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_ADDRESSU));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_ADDRESSV));
        }
        if(rm_g_samplerDisplacementMap2 != nvrm_unused)
        {
            const UINT displacementMapSampler = D3DVERTEXTEXTURESAMPLER0 + rm_g_samplerDisplacementMap2;
            V_RETURN(pSavestateImpl->PreserveD3D9Texture(displacementMapSampler));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MIPFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MINFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MAGFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_ADDRESSU));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_ADDRESSV));
        }
        if(rm_g_samplerDisplacementMap3 != nvrm_unused)
        {
            const UINT displacementMapSampler = D3DVERTEXTEXTURESAMPLER0 + rm_g_samplerDisplacementMap3;
            V_RETURN(pSavestateImpl->PreserveD3D9Texture(displacementMapSampler));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MIPFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MINFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_MAGFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_ADDRESSU));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(displacementMapSampler, D3DSAMP_ADDRESSV));
        }
        //
        if(rm_g_samplerGradientMap0 != nvrm_unused)
        {
            const UINT gradMapSampler = rm_g_samplerGradientMap0;
            V_RETURN(pSavestateImpl->PreserveD3D9Texture(gradMapSampler));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MIPFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MINFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MAGFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_ADDRESSU));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_ADDRESSV));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MAXANISOTROPY));
        }
        if(rm_g_samplerGradientMap1 != nvrm_unused)
        {
            const UINT gradMapSampler = rm_g_samplerGradientMap1;
            V_RETURN(pSavestateImpl->PreserveD3D9Texture(gradMapSampler));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MIPFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MINFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MAGFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_ADDRESSU));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_ADDRESSV));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MAXANISOTROPY));
        }
        if(rm_g_samplerGradientMap2 != nvrm_unused)
        {
            const UINT gradMapSampler = rm_g_samplerGradientMap2;
            V_RETURN(pSavestateImpl->PreserveD3D9Texture(gradMapSampler));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MIPFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MINFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MAGFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_ADDRESSU));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_ADDRESSV));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MAXANISOTROPY));
        }
        if(rm_g_samplerGradientMap3 != nvrm_unused)
        {
            const UINT gradMapSampler = rm_g_samplerGradientMap3;
            V_RETURN(pSavestateImpl->PreserveD3D9Texture(gradMapSampler));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MIPFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MINFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MAGFILTER));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_ADDRESSU));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_ADDRESSV));
            V_RETURN(pSavestateImpl->PreserveD3D9SamplerState(gradMapSampler, D3DSAMP_MAXANISOTROPY));
        }
        //

        if(rm_g_WorldEye != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9VertexShaderConstantF(rm_g_WorldEye, 1));
        if(rm_g_UVScaleCascade0123 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9VertexShaderConstantF(rm_g_UVScaleCascade0123, 1));

		if(rm_g_TexelLength_x2_PS != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9PixelShaderConstantF(rm_g_TexelLength_x2_PS, 1));
        if(rm_g_Cascade1Scale_PS != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9PixelShaderConstantF(rm_g_Cascade1Scale_PS, 1));
        if(rm_g_Cascade1TexelScale_PS != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9PixelShaderConstantF(rm_g_Cascade1TexelScale_PS, 1));
        if(rm_g_Cascade1UVOffset_PS != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9PixelShaderConstantF(rm_g_Cascade1UVOffset_PS, 1));
		if(rm_g_Cascade2Scale_PS != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9PixelShaderConstantF(rm_g_Cascade2Scale_PS, 1));
        if(rm_g_Cascade2TexelScale_PS != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9PixelShaderConstantF(rm_g_Cascade2TexelScale_PS, 1));
        if(rm_g_Cascade2UVOffset_PS != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9PixelShaderConstantF(rm_g_Cascade2UVOffset_PS, 1));
        if(rm_g_Cascade3Scale_PS != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9PixelShaderConstantF(rm_g_Cascade3Scale_PS, 1));
        if(rm_g_Cascade3TexelScale_PS != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9PixelShaderConstantF(rm_g_Cascade3TexelScale_PS, 1));
        if(rm_g_Cascade3UVOffset_PS != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D9PixelShaderConstantF(rm_g_Cascade3UVOffset_PS, 1));
    }

    // Textures
    if(rm_g_samplerDisplacementMap0 != nvrm_unused)
    {
        const UINT displacementMapSampler = D3DVERTEXTEXTURESAMPLER0 + rm_g_samplerDisplacementMap0;
        V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(displacementMapSampler, cascade_states[0].m_pFFTSimulation->GetDisplacementMapD3D9()));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));
    }
    if(rm_g_samplerDisplacementMap1 != nvrm_unused)
    {
        const UINT displacementMapSampler = D3DVERTEXTEXTURESAMPLER0 + rm_g_samplerDisplacementMap1;
        V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(displacementMapSampler, cascade_states[1].m_pFFTSimulation->GetDisplacementMapD3D9()));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));
    }
    if(rm_g_samplerDisplacementMap2 != nvrm_unused)
    {
        const UINT displacementMapSampler = D3DVERTEXTEXTURESAMPLER0 + rm_g_samplerDisplacementMap2;
        V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(displacementMapSampler, cascade_states[2].m_pFFTSimulation->GetDisplacementMapD3D9()));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));
    }
    if(rm_g_samplerDisplacementMap3 != nvrm_unused)
    {
        const UINT displacementMapSampler = D3DVERTEXTEXTURESAMPLER0 + rm_g_samplerDisplacementMap3;
        V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(displacementMapSampler, cascade_states[3].m_pFFTSimulation->GetDisplacementMapD3D9()));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(displacementMapSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));
    }
    //
    if(rm_g_samplerGradientMap0 != nvrm_unused)
    {
        const UINT gradMapSampler = rm_g_samplerGradientMap0;
        V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(gradMapSampler, cascade_states[0].m_d3d._9.m_pd3d9GradientMap[m_active_GPU_slot]));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MINFILTER, gradMapMinFilterMode));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MAXANISOTROPY, m_params.aniso_level));
    }
    if(rm_g_samplerGradientMap1 != nvrm_unused)
    {
        const UINT gradMapSampler = rm_g_samplerGradientMap1;
        V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(gradMapSampler, cascade_states[1].m_d3d._9.m_pd3d9GradientMap[m_active_GPU_slot]));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MINFILTER, gradMapMinFilterMode));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MAXANISOTROPY, m_params.aniso_level));
    }
    if(rm_g_samplerGradientMap2 != nvrm_unused)
    {
        const UINT gradMapSampler = rm_g_samplerGradientMap2;
        V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(gradMapSampler, cascade_states[2].m_d3d._9.m_pd3d9GradientMap[m_active_GPU_slot]));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MINFILTER, gradMapMinFilterMode));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MAXANISOTROPY, m_params.aniso_level));
    }
    if(rm_g_samplerGradientMap3 != nvrm_unused)
    {
        const UINT gradMapSampler = rm_g_samplerGradientMap3;
        V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(gradMapSampler, cascade_states[3].m_d3d._9.m_pd3d9GradientMap[m_active_GPU_slot]));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MINFILTER, gradMapMinFilterMode));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));
        V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(gradMapSampler, D3DSAMP_MAXANISOTROPY, m_params.aniso_level));
    }
    // Constants
	gfsdk_float4 UVScaleCascade0123;
	UVScaleCascade0123.x = 1.0f / m_params.cascades[0].fft_period;
	UVScaleCascade0123.y = 1.0f / m_params.cascades[1].fft_period;
	UVScaleCascade0123.z = 1.0f / m_params.cascades[2].fft_period;
	UVScaleCascade0123.w = 1.0f / m_params.cascades[3].fft_period;

	gfsdk_float4x4 inv_mat_view;
	gfsdk_float4 vec_original = {0,0,0,1};
	gfsdk_float4 vec_transformed;
	mat4Inverse(inv_mat_view,matView);
	vec4Mat4Mul(vec_transformed, vec_original, inv_mat_view);
	gfsdk_float4 vGlobalEye = vec_transformed;

	const gfsdk_float4 texel_len = gfsdk_make_float4(m_params.cascades[0].fft_period / m_params.cascades[0].fft_resolution,0,0,0);
    const gfsdk_float4 cascade1Scale = gfsdk_make_float4(m_params.cascades[0].fft_period/m_params.cascades[1].fft_period,0,0,0);
	const gfsdk_float4 cascade1TexelScale = gfsdk_make_float4((m_params.cascades[0].fft_period * m_params.cascades[1].fft_resolution) / (m_params.cascades[1].fft_period * m_params.cascades[0].fft_resolution),0,0,0);
	const gfsdk_float4 cascade1UVOffset = gfsdk_make_float4(0,0,0,0);
    const gfsdk_float4 cascade2Scale = gfsdk_make_float4(m_params.cascades[0].fft_period/m_params.cascades[2].fft_period,0,0,0);
	const gfsdk_float4 cascade2TexelScale = gfsdk_make_float4((m_params.cascades[0].fft_period * m_params.cascades[2].fft_resolution) / (m_params.cascades[2].fft_period * m_params.cascades[0].fft_resolution),0,0,0);
	const gfsdk_float4 cascade2UVOffset = gfsdk_make_float4(0,0,0,0);
    const gfsdk_float4 cascade3Scale = gfsdk_make_float4(m_params.cascades[0].fft_period/m_params.cascades[3].fft_period,0,0,0);
	const gfsdk_float4 cascade3TexelScale = gfsdk_make_float4((m_params.cascades[0].fft_period * m_params.cascades[3].fft_resolution) / (m_params.cascades[3].fft_period * m_params.cascades[0].fft_resolution),0,0,0);
	const gfsdk_float4 cascade3UVOffset = gfsdk_make_float4(0,0,0,0);

    if(rm_g_WorldEye != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetVertexShaderConstantF(rm_g_WorldEye, (FLOAT*)&vGlobalEye, 1));
    if(rm_g_UVScaleCascade0123 != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetVertexShaderConstantF(rm_g_UVScaleCascade0123, (FLOAT*)&UVScaleCascade0123, 1));
	if(rm_g_TexelLength_x2_PS != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(rm_g_TexelLength_x2_PS, (FLOAT*)&texel_len, 1));
    //
    if(rm_g_Cascade1Scale_PS != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(rm_g_Cascade1Scale_PS, (FLOAT*)&cascade1Scale, 1));
    if(rm_g_Cascade1TexelScale_PS != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(rm_g_Cascade1TexelScale_PS, (FLOAT*)&cascade1TexelScale, 1));
    if(rm_g_Cascade1UVOffset_PS != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(rm_g_Cascade1UVOffset_PS, (FLOAT*)&cascade1UVOffset, 1));
    if(rm_g_Cascade2Scale_PS != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(rm_g_Cascade2Scale_PS, (FLOAT*)&cascade2Scale, 1));
    if(rm_g_Cascade2TexelScale_PS != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(rm_g_Cascade2TexelScale_PS, (FLOAT*)&cascade2TexelScale, 1));
    if(rm_g_Cascade2UVOffset_PS != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(rm_g_Cascade2UVOffset_PS, (FLOAT*)&cascade2UVOffset, 1));
    if(rm_g_Cascade3Scale_PS != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(rm_g_Cascade3Scale_PS, (FLOAT*)&cascade3Scale, 1));
    if(rm_g_Cascade3TexelScale_PS != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(rm_g_Cascade3TexelScale_PS, (FLOAT*)&cascade3TexelScale, 1));
    if(rm_g_Cascade3UVOffset_PS != nvrm_unused)
        V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(rm_g_Cascade3UVOffset_PS, (FLOAT*)&cascade3UVOffset, 1));

    return S_OK;
#else
return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::setRenderStateD3D10(	const gfsdk_float4x4& D3D10_ONLY(matView),
															const UINT* D3D10_ONLY(pShaderInputRegisterMappings),
															GFSDK_WaveWorks_Savestate* D3D10_ONLY(pSavestateImpl)
															)
{
#if WAVEWORKS_ENABLE_D3D10
    HRESULT hr;
    const UINT rm_vs_buffer = pShaderInputRegisterMappings[ShaderInputD3D10_vs_buffer];
    const UINT rm_g_samplerDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputD3D10_g_samplerDisplacementMap0];
    const UINT rm_g_samplerDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputD3D10_g_samplerDisplacementMap1];
	const UINT rm_g_samplerDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputD3D10_g_samplerDisplacementMap2];
	const UINT rm_g_samplerDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputD3D10_g_samplerDisplacementMap3];
    const UINT rm_g_textureDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputD3D10_g_textureDisplacementMap0];
    const UINT rm_g_textureDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputD3D10_g_textureDisplacementMap1];
	const UINT rm_g_textureDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputD3D10_g_textureDisplacementMap2];
	const UINT rm_g_textureDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputD3D10_g_textureDisplacementMap3];
    const UINT rm_ps_buffer = pShaderInputRegisterMappings[ShaderInputD3D10_ps_buffer];
    const UINT rm_g_samplerGradientMap0 = pShaderInputRegisterMappings[ShaderInputD3D10_g_samplerGradientMap0];
    const UINT rm_g_samplerGradientMap1 = pShaderInputRegisterMappings[ShaderInputD3D10_g_samplerGradientMap1];
	const UINT rm_g_samplerGradientMap2 = pShaderInputRegisterMappings[ShaderInputD3D10_g_samplerGradientMap2];
	const UINT rm_g_samplerGradientMap3 = pShaderInputRegisterMappings[ShaderInputD3D10_g_samplerGradientMap3];
    const UINT rm_g_textureGradientMap0 = pShaderInputRegisterMappings[ShaderInputD3D10_g_textureGradientMap0];
    const UINT rm_g_textureGradientMap1 = pShaderInputRegisterMappings[ShaderInputD3D10_g_textureGradientMap1];
	const UINT rm_g_textureGradientMap2 = pShaderInputRegisterMappings[ShaderInputD3D10_g_textureGradientMap2];
	const UINT rm_g_textureGradientMap3 = pShaderInputRegisterMappings[ShaderInputD3D10_g_textureGradientMap3];

    // Preserve state as necessary
    if(pSavestateImpl)
    {
        // Samplers/textures
		//VS
        if(rm_g_samplerDisplacementMap0 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10VertexShaderSampler(rm_g_samplerDisplacementMap0));
        if(rm_g_samplerDisplacementMap1 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10VertexShaderSampler(rm_g_samplerDisplacementMap1));
        if(rm_g_samplerDisplacementMap2 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10VertexShaderSampler(rm_g_samplerDisplacementMap2));
        if(rm_g_samplerDisplacementMap3 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10VertexShaderSampler(rm_g_samplerDisplacementMap3));

		if(rm_g_textureDisplacementMap0 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10VertexShaderResource(rm_g_textureDisplacementMap0));
        if(rm_g_textureDisplacementMap1 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10VertexShaderResource(rm_g_textureDisplacementMap1));
        if(rm_g_textureDisplacementMap2 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10VertexShaderResource(rm_g_textureDisplacementMap2));
        if(rm_g_textureDisplacementMap3 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10VertexShaderResource(rm_g_textureDisplacementMap3));

		// PS
        if(rm_g_samplerGradientMap0 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderSampler(rm_g_samplerGradientMap0));
        if(rm_g_samplerGradientMap1 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderSampler(rm_g_samplerGradientMap1));
        if(rm_g_samplerGradientMap2 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderSampler(rm_g_samplerGradientMap2));
        if(rm_g_samplerGradientMap3 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderSampler(rm_g_samplerGradientMap3));

        if(rm_g_textureGradientMap0 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderResource(rm_g_textureGradientMap0));
        if(rm_g_textureGradientMap1 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderResource(rm_g_textureGradientMap1));
        if(rm_g_textureGradientMap2 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderResource(rm_g_textureGradientMap2));
        if(rm_g_textureGradientMap3 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderResource(rm_g_textureGradientMap3));
 
        // Constants
        if(rm_vs_buffer != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10VertexShaderConstantBuffer(rm_vs_buffer));
        if(rm_ps_buffer != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderConstantBuffer(rm_ps_buffer));
    }

    // Vertex textures/samplers
    if(rm_g_samplerDisplacementMap0 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->VSSetSamplers(rm_g_samplerDisplacementMap0, 1, &m_d3d._10.m_pd3d10LinearNoMipSampler);
    if(rm_g_samplerDisplacementMap1 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->VSSetSamplers(rm_g_samplerDisplacementMap1, 1, &m_d3d._10.m_pd3d10LinearNoMipSampler);
    if(rm_g_samplerDisplacementMap2 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->VSSetSamplers(rm_g_samplerDisplacementMap2, 1, &m_d3d._10.m_pd3d10LinearNoMipSampler);
    if(rm_g_samplerDisplacementMap3 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->VSSetSamplers(rm_g_samplerDisplacementMap3, 1, &m_d3d._10.m_pd3d10LinearNoMipSampler);
    //
    if(rm_g_textureDisplacementMap0 != nvrm_unused)
		m_d3d._10.m_pd3d10Device->VSSetShaderResources(rm_g_textureDisplacementMap0, 1, cascade_states[0].m_pFFTSimulation->GetDisplacementMapD3D10());
    if(rm_g_textureDisplacementMap1 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->VSSetShaderResources(rm_g_textureDisplacementMap1, 1, cascade_states[1].m_pFFTSimulation->GetDisplacementMapD3D10());
    if(rm_g_textureDisplacementMap2 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->VSSetShaderResources(rm_g_textureDisplacementMap2, 1, cascade_states[2].m_pFFTSimulation->GetDisplacementMapD3D10());
    if(rm_g_textureDisplacementMap3 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->VSSetShaderResources(rm_g_textureDisplacementMap3, 1, cascade_states[3].m_pFFTSimulation->GetDisplacementMapD3D10());

	
	// Pixel textures/samplers
    if(rm_g_samplerGradientMap0 != nvrm_unused)
		m_d3d._10.m_pd3d10Device->PSSetSamplers(rm_g_samplerGradientMap0, 1, &m_d3d._10.m_pd3d10GradMapSampler);
    if(rm_g_samplerGradientMap1 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->PSSetSamplers(rm_g_samplerGradientMap1, 1, &m_d3d._10.m_pd3d10GradMapSampler);
    if(rm_g_samplerGradientMap2 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->PSSetSamplers(rm_g_samplerGradientMap2, 1, &m_d3d._10.m_pd3d10GradMapSampler);
    if(rm_g_samplerGradientMap3 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->PSSetSamplers(rm_g_samplerGradientMap3, 1, &m_d3d._10.m_pd3d10GradMapSampler);
	//
    if(rm_g_textureGradientMap0 != nvrm_unused)
		m_d3d._10.m_pd3d10Device->PSSetShaderResources(rm_g_textureGradientMap0, 1, &cascade_states[0].m_d3d._10.m_pd3d10GradientMap[m_active_GPU_slot]);
    if(rm_g_textureGradientMap1 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->PSSetShaderResources(rm_g_textureGradientMap1, 1, &cascade_states[1].m_d3d._10.m_pd3d10GradientMap[m_active_GPU_slot]);
    if(rm_g_textureGradientMap2 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->PSSetShaderResources(rm_g_textureGradientMap2, 1, &cascade_states[2].m_d3d._10.m_pd3d10GradientMap[m_active_GPU_slot]);
    if(rm_g_textureGradientMap3 != nvrm_unused)
        m_d3d._10.m_pd3d10Device->PSSetShaderResources(rm_g_textureGradientMap3, 1, &cascade_states[3].m_d3d._10.m_pd3d10GradientMap[m_active_GPU_slot]);
	
    // Constants
    vs_attr_cbuffer VSCB;
    vs_attr_cbuffer* pVSCB = NULL;
    if(rm_vs_buffer != nvrm_unused)
    {
        pVSCB = &VSCB;

        pVSCB->g_UVScaleCascade0123[0] = 1.0f / m_params.cascades[0].fft_period;
        pVSCB->g_UVScaleCascade0123[1] = 1.0f / m_params.cascades[1].fft_period;
        pVSCB->g_UVScaleCascade0123[2] = 1.0f / m_params.cascades[2].fft_period;
        pVSCB->g_UVScaleCascade0123[3] = 1.0f / m_params.cascades[3].fft_period;

		gfsdk_float4x4 inv_mat_view;
		gfsdk_float4 vec_original = {0,0,0,1};
		gfsdk_float4 vec_transformed;
		mat4Inverse(inv_mat_view,matView);
		vec4Mat4Mul(vec_transformed, vec_original, inv_mat_view);
		gfsdk_float4 vGlobalEye = vec_transformed;

        pVSCB->g_WorldEye[0] = vGlobalEye.x;
        pVSCB->g_WorldEye[1] = vGlobalEye.y;
        pVSCB->g_WorldEye[2] = vGlobalEye.z;
    }

	ps_attr_cbuffer PSCB;
    ps_attr_cbuffer* pPSCB = NULL;
	const FLOAT texel_len = m_params.cascades[0].fft_period / m_params.cascades[0].fft_resolution;
    const float cascade1Scale = m_params.cascades[0].fft_period/m_params.cascades[1].fft_period;
    const float cascade1UVOffset = 0.f; // half-pixel not required in D3D10
    const float cascade2Scale = m_params.cascades[0].fft_period/m_params.cascades[2].fft_period;
    const float cascade2UVOffset = 0.f; // half-pixel not required in D3D10
    const float cascade3Scale = m_params.cascades[0].fft_period/m_params.cascades[3].fft_period;
    const float cascade3UVOffset = 0.f; // half-pixel not required in D3D10

    if(rm_ps_buffer != nvrm_unused)
    {
        pPSCB = &PSCB;
		pPSCB->g_TexelLength_x2_PS = texel_len;
    }

    if(NULL != pPSCB)
    {
        pPSCB->g_Cascade1Scale_PS = cascade1Scale;
        pPSCB->g_Cascade1UVOffset_PS = cascade1UVOffset;
        pPSCB->g_Cascade2Scale_PS = cascade2Scale;
        pPSCB->g_Cascade2UVOffset_PS = cascade2UVOffset;
        pPSCB->g_Cascade3Scale_PS = cascade3Scale;
        pPSCB->g_Cascade3UVOffset_PS = cascade3UVOffset;
        pPSCB->g_Cascade1TexelScale_PS = (m_params.cascades[0].fft_period * m_params.cascades[1].fft_resolution) / (m_params.cascades[1].fft_period * m_params.cascades[0].fft_resolution);
		pPSCB->g_Cascade2TexelScale_PS = (m_params.cascades[0].fft_period * m_params.cascades[2].fft_resolution) / (m_params.cascades[2].fft_period * m_params.cascades[0].fft_resolution);
		pPSCB->g_Cascade3TexelScale_PS = (m_params.cascades[0].fft_period * m_params.cascades[3].fft_resolution) / (m_params.cascades[3].fft_period * m_params.cascades[0].fft_resolution);
    }

    if(pVSCB)
    {
        m_d3d._10.m_pd3d10Device->UpdateSubresource(m_d3d._10.m_pd3d10VertexShaderCB, 0, NULL, pVSCB, 0, 0);
        m_d3d._10.m_pd3d10Device->VSSetConstantBuffers(rm_vs_buffer, 1, &m_d3d._10.m_pd3d10VertexShaderCB);
    }

    if(pPSCB)
    {
        m_d3d._10.m_pd3d10Device->UpdateSubresource(m_d3d._10.m_pd3d10PixelShaderCB, 0, NULL, pPSCB, 0, 0);
        m_d3d._10.m_pd3d10Device->PSSetConstantBuffers(rm_ps_buffer, 1, &m_d3d._10.m_pd3d10PixelShaderCB);
    }

    return S_OK;
#else
return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::setRenderStateD3D11(	ID3D11DeviceContext* D3D11_ONLY(pDC),
															const gfsdk_float4x4& D3D11_ONLY(matView),
															const UINT* D3D11_ONLY(pShaderInputRegisterMappings),
															GFSDK_WaveWorks_Savestate* D3D11_ONLY(pSavestateImpl)
															)
{
#if WAVEWORKS_ENABLE_D3D11
	HRESULT hr;

	const UINT rm_vs_buffer = pShaderInputRegisterMappings[ShaderInputD3D11_vs_buffer];
	const UINT rm_vs_g_samplerDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputD3D11_vs_g_samplerDisplacementMap0];
	const UINT rm_vs_g_samplerDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputD3D11_vs_g_samplerDisplacementMap1];
	const UINT rm_vs_g_samplerDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputD3D11_vs_g_samplerDisplacementMap2];
	const UINT rm_vs_g_samplerDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputD3D11_vs_g_samplerDisplacementMap3];
	const UINT rm_vs_g_textureDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputD3D11_vs_g_textureDisplacementMap0];
	const UINT rm_vs_g_textureDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputD3D11_vs_g_textureDisplacementMap1];
	const UINT rm_vs_g_textureDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputD3D11_vs_g_textureDisplacementMap2];
	const UINT rm_vs_g_textureDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputD3D11_vs_g_textureDisplacementMap3];
	const UINT rm_ds_buffer = pShaderInputRegisterMappings[ShaderInputD3D11_ds_buffer];
	const UINT rm_ds_g_samplerDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputD3D11_ds_g_samplerDisplacementMap0];
	const UINT rm_ds_g_samplerDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputD3D11_ds_g_samplerDisplacementMap1];
	const UINT rm_ds_g_samplerDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputD3D11_ds_g_samplerDisplacementMap2];
	const UINT rm_ds_g_samplerDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputD3D11_ds_g_samplerDisplacementMap3];
	const UINT rm_ds_g_textureDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputD3D11_ds_g_textureDisplacementMap0];
	const UINT rm_ds_g_textureDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputD3D11_ds_g_textureDisplacementMap1];
	const UINT rm_ds_g_textureDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputD3D11_ds_g_textureDisplacementMap2];
	const UINT rm_ds_g_textureDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputD3D11_ds_g_textureDisplacementMap3];
	const UINT rm_ps_buffer = pShaderInputRegisterMappings[ShaderInputD3D11_ps_buffer];
	const UINT rm_g_samplerGradientMap0 = pShaderInputRegisterMappings[ShaderInputD3D11_g_samplerGradientMap0];
	const UINT rm_g_samplerGradientMap1 = pShaderInputRegisterMappings[ShaderInputD3D11_g_samplerGradientMap1];
	const UINT rm_g_samplerGradientMap2 = pShaderInputRegisterMappings[ShaderInputD3D11_g_samplerGradientMap2];
	const UINT rm_g_samplerGradientMap3 = pShaderInputRegisterMappings[ShaderInputD3D11_g_samplerGradientMap3];
	const UINT rm_g_textureGradientMap0 = pShaderInputRegisterMappings[ShaderInputD3D11_g_textureGradientMap0];
	const UINT rm_g_textureGradientMap1 = pShaderInputRegisterMappings[ShaderInputD3D11_g_textureGradientMap1];
	const UINT rm_g_textureGradientMap2 = pShaderInputRegisterMappings[ShaderInputD3D11_g_textureGradientMap2];
	const UINT rm_g_textureGradientMap3 = pShaderInputRegisterMappings[ShaderInputD3D11_g_textureGradientMap3];

	// Preserve state as necessary
	if(pSavestateImpl)
	{
		// Samplers/textures

		if(rm_vs_g_samplerDisplacementMap0 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderSampler(pDC, rm_vs_g_samplerDisplacementMap0));
		if(rm_vs_g_samplerDisplacementMap1 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderSampler(pDC, rm_vs_g_samplerDisplacementMap1));
		if(rm_vs_g_samplerDisplacementMap2 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderSampler(pDC, rm_vs_g_samplerDisplacementMap2));
		if(rm_vs_g_samplerDisplacementMap3 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderSampler(pDC, rm_vs_g_samplerDisplacementMap3));

		if(rm_vs_g_textureDisplacementMap0 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderResource(pDC, rm_vs_g_textureDisplacementMap0));
		if(rm_vs_g_textureDisplacementMap1 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderResource(pDC, rm_vs_g_textureDisplacementMap1));
		if(rm_vs_g_textureDisplacementMap2 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderResource(pDC, rm_vs_g_textureDisplacementMap2));
		if(rm_vs_g_textureDisplacementMap3 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderResource(pDC, rm_vs_g_textureDisplacementMap3));

		if(rm_ds_g_samplerDisplacementMap0 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderSampler(pDC, rm_ds_g_samplerDisplacementMap0));
		if(rm_ds_g_samplerDisplacementMap1 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderSampler(pDC, rm_ds_g_samplerDisplacementMap1));
		if(rm_ds_g_samplerDisplacementMap2 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderSampler(pDC, rm_ds_g_samplerDisplacementMap2));
		if(rm_ds_g_samplerDisplacementMap3 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderSampler(pDC, rm_ds_g_samplerDisplacementMap3));

		if(rm_ds_g_textureDisplacementMap0 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderResource(pDC, rm_ds_g_textureDisplacementMap0));
		if(rm_ds_g_textureDisplacementMap1 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderResource(pDC, rm_ds_g_textureDisplacementMap1));
		if(rm_ds_g_textureDisplacementMap2 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderResource(pDC, rm_ds_g_textureDisplacementMap2));
		if(rm_ds_g_textureDisplacementMap3 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderResource(pDC, rm_ds_g_textureDisplacementMap3));

		if(rm_g_samplerGradientMap0 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderSampler(pDC, rm_g_samplerGradientMap0));
		if(rm_g_samplerGradientMap1 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderSampler(pDC, rm_g_samplerGradientMap1));
		if(rm_g_samplerGradientMap2 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderSampler(pDC, rm_g_samplerGradientMap2));
		if(rm_g_samplerGradientMap3 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderSampler(pDC, rm_g_samplerGradientMap3));

		if(rm_g_textureGradientMap0 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderResource(pDC, rm_g_textureGradientMap0));
		if(rm_g_textureGradientMap1 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderResource(pDC, rm_g_textureGradientMap1));
		if(rm_g_textureGradientMap2 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderResource(pDC, rm_g_textureGradientMap2));
		if(rm_g_textureGradientMap3 != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderResource(pDC, rm_g_textureGradientMap3));

		// Constants
		if(rm_vs_buffer != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderConstantBuffer(pDC, rm_vs_buffer));
		if(rm_ds_buffer != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderConstantBuffer(pDC, rm_ds_buffer));
		if(rm_ps_buffer != nvrm_unused)
			V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderConstantBuffer(pDC, rm_ps_buffer));
	}

	// Vertex textures/samplers
	if(rm_vs_g_samplerDisplacementMap0 != nvrm_unused)
		pDC->VSSetSamplers(rm_vs_g_samplerDisplacementMap0, 1, &m_d3d._11.m_pd3d11LinearNoMipSampler);
	if(rm_vs_g_samplerDisplacementMap1 != nvrm_unused)
		pDC->VSSetSamplers(rm_vs_g_samplerDisplacementMap1, 1, &m_d3d._11.m_pd3d11LinearNoMipSampler);
	if(rm_vs_g_samplerDisplacementMap2 != nvrm_unused)
		pDC->VSSetSamplers(rm_vs_g_samplerDisplacementMap2, 1, &m_d3d._11.m_pd3d11LinearNoMipSampler);
	if(rm_vs_g_samplerDisplacementMap3 != nvrm_unused)
		pDC->VSSetSamplers(rm_vs_g_samplerDisplacementMap3, 1, &m_d3d._11.m_pd3d11LinearNoMipSampler);
	//
	if(rm_vs_g_textureDisplacementMap0 != nvrm_unused)
		pDC->VSSetShaderResources(rm_vs_g_textureDisplacementMap0, 1, cascade_states[0].m_pFFTSimulation->GetDisplacementMapD3D11());
	if(rm_vs_g_textureDisplacementMap1 != nvrm_unused)
		pDC->VSSetShaderResources(rm_vs_g_textureDisplacementMap1, 1, cascade_states[1].m_pFFTSimulation->GetDisplacementMapD3D11());
	if(rm_vs_g_textureDisplacementMap2 != nvrm_unused)
		pDC->VSSetShaderResources(rm_vs_g_textureDisplacementMap2, 1, cascade_states[2].m_pFFTSimulation->GetDisplacementMapD3D11());
	if(rm_vs_g_textureDisplacementMap3 != nvrm_unused)
		pDC->VSSetShaderResources(rm_vs_g_textureDisplacementMap3, 1, cascade_states[3].m_pFFTSimulation->GetDisplacementMapD3D11());

	// Domain textures/samplers
	if(rm_ds_g_samplerDisplacementMap0 != nvrm_unused)
		pDC->DSSetSamplers(rm_ds_g_samplerDisplacementMap0, 1, &m_d3d._11.m_pd3d11LinearNoMipSampler);
	if(rm_ds_g_samplerDisplacementMap1 != nvrm_unused)
		pDC->DSSetSamplers(rm_ds_g_samplerDisplacementMap1, 1, &m_d3d._11.m_pd3d11LinearNoMipSampler);
	if(rm_ds_g_samplerDisplacementMap2 != nvrm_unused)
		pDC->DSSetSamplers(rm_ds_g_samplerDisplacementMap2, 1, &m_d3d._11.m_pd3d11LinearNoMipSampler);
	if(rm_ds_g_samplerDisplacementMap3 != nvrm_unused)
		pDC->DSSetSamplers(rm_ds_g_samplerDisplacementMap3, 1, &m_d3d._11.m_pd3d11LinearNoMipSampler);
	//
	if(rm_ds_g_textureDisplacementMap0 != nvrm_unused)
		pDC->DSSetShaderResources(rm_ds_g_textureDisplacementMap0, 1, cascade_states[0].m_pFFTSimulation->GetDisplacementMapD3D11());
	if(rm_ds_g_textureDisplacementMap1 != nvrm_unused)
		pDC->DSSetShaderResources(rm_ds_g_textureDisplacementMap1, 1, cascade_states[1].m_pFFTSimulation->GetDisplacementMapD3D11());
	if(rm_ds_g_textureDisplacementMap2 != nvrm_unused)
		pDC->DSSetShaderResources(rm_ds_g_textureDisplacementMap2, 1, cascade_states[2].m_pFFTSimulation->GetDisplacementMapD3D11());
	if(rm_ds_g_textureDisplacementMap3 != nvrm_unused)
		pDC->DSSetShaderResources(rm_ds_g_textureDisplacementMap3, 1, cascade_states[3].m_pFFTSimulation->GetDisplacementMapD3D11());

	// Pixel textures/samplers
	if(rm_g_samplerGradientMap0 != nvrm_unused)
		pDC->PSSetSamplers(rm_g_samplerGradientMap0, 1, &m_d3d._11.m_pd3d11GradMapSampler);
	if(rm_g_samplerGradientMap1 != nvrm_unused)
		pDC->PSSetSamplers(rm_g_samplerGradientMap1, 1, &m_d3d._11.m_pd3d11GradMapSampler);
	if(rm_g_samplerGradientMap2 != nvrm_unused)
		pDC->PSSetSamplers(rm_g_samplerGradientMap2, 1, &m_d3d._11.m_pd3d11GradMapSampler);
	if(rm_g_samplerGradientMap3 != nvrm_unused)
		pDC->PSSetSamplers(rm_g_samplerGradientMap3, 1, &m_d3d._11.m_pd3d11GradMapSampler);
	//
	if(rm_g_textureGradientMap0 != nvrm_unused)
		pDC->PSSetShaderResources(rm_g_textureGradientMap0, 1, &cascade_states[0].m_d3d._11.m_pd3d11GradientMap[m_active_GPU_slot]);
	if(rm_g_textureGradientMap1 != nvrm_unused)
		pDC->PSSetShaderResources(rm_g_textureGradientMap1, 1, &cascade_states[1].m_d3d._11.m_pd3d11GradientMap[m_active_GPU_slot]);
	if(rm_g_textureGradientMap2 != nvrm_unused)
		pDC->PSSetShaderResources(rm_g_textureGradientMap2, 1, &cascade_states[2].m_d3d._11.m_pd3d11GradientMap[m_active_GPU_slot]);
	if(rm_g_textureGradientMap3 != nvrm_unused)
		pDC->PSSetShaderResources(rm_g_textureGradientMap3, 1, &cascade_states[3].m_d3d._11.m_pd3d11GradientMap[m_active_GPU_slot]);

	// Constants
	vs_ds_attr_cbuffer VSDSCB;
	vs_ds_attr_cbuffer* pVSDSCB = NULL;
	if(rm_ds_buffer != nvrm_unused || rm_vs_buffer != nvrm_unused)
	{
		pVSDSCB = &VSDSCB;

		pVSDSCB->g_UVScaleCascade0123[0] = 1.0f / m_params.cascades[0].fft_period;
		pVSDSCB->g_UVScaleCascade0123[1] = 1.0f / m_params.cascades[1].fft_period;
		pVSDSCB->g_UVScaleCascade0123[2] = 1.0f / m_params.cascades[2].fft_period;
		pVSDSCB->g_UVScaleCascade0123[3] = 1.0f / m_params.cascades[3].fft_period;

		gfsdk_float4x4 inv_mat_view;
		gfsdk_float4 vec_original = {0,0,0,1};
		gfsdk_float4 vec_transformed;
		mat4Inverse(inv_mat_view,matView);
		vec4Mat4Mul(vec_transformed, vec_original, inv_mat_view);
		gfsdk_float4 vGlobalEye = vec_transformed;

		pVSDSCB->g_WorldEye[0] = vGlobalEye.x;
		pVSDSCB->g_WorldEye[1] = vGlobalEye.y;
		pVSDSCB->g_WorldEye[2] = vGlobalEye.z;
	}

	ps_attr_cbuffer PSCB;
	ps_attr_cbuffer* pPSCB = NULL;
	const FLOAT texel_len = m_params.cascades[0].fft_period / m_params.cascades[0].fft_resolution;
	const float cascade1Scale = m_params.cascades[0].fft_period/m_params.cascades[1].fft_period;
	const float cascade1UVOffset = 0.f; // half-pixel not required in D3D11
	const float cascade2Scale = m_params.cascades[0].fft_period/m_params.cascades[2].fft_period;
	const float cascade2UVOffset = 0.f; // half-pixel not required in D3D11
	const float cascade3Scale = m_params.cascades[0].fft_period/m_params.cascades[3].fft_period;
	const float cascade3UVOffset = 0.f; // half-pixel not required in D3D11

	if(rm_ps_buffer != nvrm_unused)
	{
		pPSCB = &PSCB;
		pPSCB->g_TexelLength_x2_PS = texel_len;
	}

	if(NULL != pPSCB)
	{
		pPSCB->g_Cascade1Scale_PS = cascade1Scale;
		pPSCB->g_Cascade1UVOffset_PS = cascade1UVOffset;
		pPSCB->g_Cascade2Scale_PS = cascade2Scale;
		pPSCB->g_Cascade2UVOffset_PS = cascade2UVOffset;
		pPSCB->g_Cascade3Scale_PS = cascade3Scale;
		pPSCB->g_Cascade3UVOffset_PS = cascade3UVOffset;
		pPSCB->g_Cascade1TexelScale_PS = (m_params.cascades[0].fft_period * m_params.cascades[1].fft_resolution) / (m_params.cascades[1].fft_period * m_params.cascades[0].fft_resolution);
		pPSCB->g_Cascade2TexelScale_PS = (m_params.cascades[0].fft_period * m_params.cascades[2].fft_resolution) / (m_params.cascades[2].fft_period * m_params.cascades[0].fft_resolution);
		pPSCB->g_Cascade3TexelScale_PS = (m_params.cascades[0].fft_period * m_params.cascades[3].fft_resolution) / (m_params.cascades[3].fft_period * m_params.cascades[0].fft_resolution);
	}

	if(pVSDSCB)
	{
		{
			D3D11_CB_Updater<vs_ds_attr_cbuffer> cb(pDC,m_d3d._11.m_pd3d11VertexDomainShaderCB);
			cb.cb() = *pVSDSCB;
		}
		if(rm_vs_buffer != nvrm_unused)
			pDC->VSSetConstantBuffers(rm_vs_buffer, 1, &m_d3d._11.m_pd3d11VertexDomainShaderCB);
		if(rm_ds_buffer != nvrm_unused)
			pDC->DSSetConstantBuffers(rm_ds_buffer, 1, &m_d3d._11.m_pd3d11VertexDomainShaderCB);
	}
	if(pPSCB)
	{
		{
			D3D11_CB_Updater<ps_attr_cbuffer> cb(pDC,m_d3d._11.m_pd3d11PixelShaderCB);
			cb.cb() = *pPSCB;
		}
		pDC->PSSetConstantBuffers(rm_ps_buffer, 1, &m_d3d._11.m_pd3d11PixelShaderCB);
	}
	return S_OK;
#else
	return S_FALSE;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::setRenderStateGnm(	sce::Gnmx::LightweightGfxContext* GNM_ONLY(gfxContext),
														const gfsdk_float4x4& GNM_ONLY(matView),
														const UINT* GNM_ONLY(pShaderInputRegisterMappings),
														GFSDK_WaveWorks_Savestate* GNM_ONLY(pSavestateImpl)
														)
{
#if WAVEWORKS_ENABLE_GNM
    const UINT rm_vs_buffer = pShaderInputRegisterMappings[ShaderInputGnm_vs_buffer];
    const UINT rm_vs_g_samplerDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputGnm_vs_g_samplerDisplacementMap0];
    const UINT rm_vs_g_samplerDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputGnm_vs_g_samplerDisplacementMap1];
	const UINT rm_vs_g_samplerDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputGnm_vs_g_samplerDisplacementMap2];
	const UINT rm_vs_g_samplerDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputGnm_vs_g_samplerDisplacementMap3];
    const UINT rm_vs_g_textureDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputGnm_vs_g_textureDisplacementMap0];
    const UINT rm_vs_g_textureDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputGnm_vs_g_textureDisplacementMap1];
	const UINT rm_vs_g_textureDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputGnm_vs_g_textureDisplacementMap2];
	const UINT rm_vs_g_textureDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputGnm_vs_g_textureDisplacementMap3];
    const UINT rm_ds_buffer = pShaderInputRegisterMappings[ShaderInputGnm_ds_buffer];
    const UINT rm_ds_g_samplerDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputGnm_ds_g_samplerDisplacementMap0];
    const UINT rm_ds_g_samplerDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputGnm_ds_g_samplerDisplacementMap1];
	const UINT rm_ds_g_samplerDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputGnm_ds_g_samplerDisplacementMap2];
	const UINT rm_ds_g_samplerDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputGnm_ds_g_samplerDisplacementMap3];
    const UINT rm_ds_g_textureDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputGnm_ds_g_textureDisplacementMap0];
    const UINT rm_ds_g_textureDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputGnm_ds_g_textureDisplacementMap1];
	const UINT rm_ds_g_textureDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputGnm_ds_g_textureDisplacementMap2];
	const UINT rm_ds_g_textureDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputGnm_ds_g_textureDisplacementMap3];
    const UINT rm_ps_buffer = pShaderInputRegisterMappings[ShaderInputGnm_ps_buffer];
    const UINT rm_g_samplerGradientMap0 = pShaderInputRegisterMappings[ShaderInputGnm_g_samplerGradientMap0];
    const UINT rm_g_samplerGradientMap1 = pShaderInputRegisterMappings[ShaderInputGnm_g_samplerGradientMap1];
	const UINT rm_g_samplerGradientMap2 = pShaderInputRegisterMappings[ShaderInputGnm_g_samplerGradientMap2];
	const UINT rm_g_samplerGradientMap3 = pShaderInputRegisterMappings[ShaderInputGnm_g_samplerGradientMap3];
    const UINT rm_g_textureGradientMap0 = pShaderInputRegisterMappings[ShaderInputGnm_g_textureGradientMap0];
    const UINT rm_g_textureGradientMap1 = pShaderInputRegisterMappings[ShaderInputGnm_g_textureGradientMap1];
	const UINT rm_g_textureGradientMap2 = pShaderInputRegisterMappings[ShaderInputGnm_g_textureGradientMap2];
	const UINT rm_g_textureGradientMap3 = pShaderInputRegisterMappings[ShaderInputGnm_g_textureGradientMap3];

	GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();
	gnmxWrap->pushMarker(*gfxContext, "GFSDK_WaveWorks_Simulation::setRenderStateGnm");

    // Preserve state as necessary
    if(pSavestateImpl)
    {
		/*
        // Samplers/textures
        if(rm_vs_g_samplerDisplacementMap0 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderSampler(pDC, rm_vs_g_samplerDisplacementMap0));
        if(rm_vs_g_samplerDisplacementMap1 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderSampler(pDC, rm_vs_g_samplerDisplacementMap1));
        if(rm_vs_g_samplerDisplacementMap2 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderSampler(pDC, rm_vs_g_samplerDisplacementMap2));
        if(rm_vs_g_samplerDisplacementMap3 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderSampler(pDC, rm_vs_g_samplerDisplacementMap3));

		if(rm_vs_g_textureDisplacementMap0 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderResource(pDC, rm_vs_g_textureDisplacementMap0));
        if(rm_vs_g_textureDisplacementMap1 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderResource(pDC, rm_vs_g_textureDisplacementMap1));
        if(rm_vs_g_textureDisplacementMap2 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderResource(pDC, rm_vs_g_textureDisplacementMap2));
        if(rm_vs_g_textureDisplacementMap3 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderResource(pDC, rm_vs_g_textureDisplacementMap3));

        if(rm_ds_g_samplerDisplacementMap0 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderSampler(pDC, rm_ds_g_samplerDisplacementMap0));
        if(rm_ds_g_samplerDisplacementMap1 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderSampler(pDC, rm_ds_g_samplerDisplacementMap1));
        if(rm_ds_g_samplerDisplacementMap2 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderSampler(pDC, rm_ds_g_samplerDisplacementMap2));
        if(rm_ds_g_samplerDisplacementMap3 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderSampler(pDC, rm_ds_g_samplerDisplacementMap3));

		if(rm_ds_g_textureDisplacementMap0 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderResource(pDC, rm_ds_g_textureDisplacementMap0));
        if(rm_ds_g_textureDisplacementMap1 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderResource(pDC, rm_ds_g_textureDisplacementMap1));
        if(rm_ds_g_textureDisplacementMap2 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderResource(pDC, rm_ds_g_textureDisplacementMap2));
        if(rm_ds_g_textureDisplacementMap3 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderResource(pDC, rm_ds_g_textureDisplacementMap3));

        if(rm_g_samplerGradientMap0 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderSampler(pDC, rm_g_samplerGradientMap0));
        if(rm_g_samplerGradientMap1 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderSampler(pDC, rm_g_samplerGradientMap1));
        if(rm_g_samplerGradientMap2 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderSampler(pDC, rm_g_samplerGradientMap2));
        if(rm_g_samplerGradientMap3 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderSampler(pDC, rm_g_samplerGradientMap3));

        if(rm_g_textureGradientMap0 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderResource(pDC, rm_g_textureGradientMap0));
        if(rm_g_textureGradientMap1 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderResource(pDC, rm_g_textureGradientMap1));
        if(rm_g_textureGradientMap2 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderResource(pDC, rm_g_textureGradientMap2));
        if(rm_g_textureGradientMap3 != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderResource(pDC, rm_g_textureGradientMap3));
 
        // Constants
        if(rm_vs_buffer != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11VertexShaderConstantBuffer(pDC, rm_vs_buffer));
        if(rm_ds_buffer != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11DomainShaderConstantBuffer(pDC, rm_ds_buffer));
        if(rm_ps_buffer != nvrm_unused)
            V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderConstantBuffer(pDC, rm_ps_buffer));
		*/
    }

	const bool usingTessellation =	rm_ds_g_samplerDisplacementMap0 != nvrm_unused || 
									rm_ds_g_samplerDisplacementMap1 != nvrm_unused || 
									rm_ds_g_samplerDisplacementMap2 != nvrm_unused || 
									rm_ds_g_samplerDisplacementMap3 != nvrm_unused;

	Gnm::ShaderStage vsStage = usingTessellation ? Gnm::kShaderStageLs : Gnm::kShaderStageVs;

    // Vertex textures/samplers
    if(rm_vs_g_samplerDisplacementMap0 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, vsStage, rm_vs_g_samplerDisplacementMap0, 1, &m_d3d._gnm.m_pGnmLinearNoMipSampler);
    if(rm_vs_g_samplerDisplacementMap1 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, vsStage, rm_vs_g_samplerDisplacementMap1, 1, &m_d3d._gnm.m_pGnmLinearNoMipSampler);
    if(rm_vs_g_samplerDisplacementMap2 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, vsStage, rm_vs_g_samplerDisplacementMap2, 1, &m_d3d._gnm.m_pGnmLinearNoMipSampler);
    if(rm_vs_g_samplerDisplacementMap3 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, vsStage, rm_vs_g_samplerDisplacementMap3, 1, &m_d3d._gnm.m_pGnmLinearNoMipSampler);
    //
    if(rm_vs_g_textureDisplacementMap0 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, vsStage, rm_vs_g_textureDisplacementMap0, 1, cascade_states[0].m_pFFTSimulation->GetDisplacementMapGnm());
    if(rm_vs_g_textureDisplacementMap1 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, vsStage, rm_vs_g_textureDisplacementMap1, 1, cascade_states[1].m_pFFTSimulation->GetDisplacementMapGnm());
    if(rm_vs_g_textureDisplacementMap2 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, vsStage, rm_vs_g_textureDisplacementMap2, 1, cascade_states[2].m_pFFTSimulation->GetDisplacementMapGnm());
    if(rm_vs_g_textureDisplacementMap3 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, vsStage, rm_vs_g_textureDisplacementMap3, 1, cascade_states[3].m_pFFTSimulation->GetDisplacementMapGnm());

    // Domain textures/samplers
    if(rm_ds_g_samplerDisplacementMap0 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, Gnm::kShaderStageVs, rm_ds_g_samplerDisplacementMap0, 1, &m_d3d._gnm.m_pGnmLinearNoMipSampler);
    if(rm_ds_g_samplerDisplacementMap1 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, Gnm::kShaderStageVs, rm_ds_g_samplerDisplacementMap1, 1, &m_d3d._gnm.m_pGnmLinearNoMipSampler);
    if(rm_ds_g_samplerDisplacementMap2 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, Gnm::kShaderStageVs, rm_ds_g_samplerDisplacementMap2, 1, &m_d3d._gnm.m_pGnmLinearNoMipSampler);
    if(rm_ds_g_samplerDisplacementMap3 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, Gnm::kShaderStageVs, rm_ds_g_samplerDisplacementMap3, 1, &m_d3d._gnm.m_pGnmLinearNoMipSampler);
    //
    if(rm_ds_g_textureDisplacementMap0 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStageVs, rm_ds_g_textureDisplacementMap0, 1, cascade_states[0].m_pFFTSimulation->GetDisplacementMapGnm());
    if(rm_ds_g_textureDisplacementMap1 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStageVs, rm_ds_g_textureDisplacementMap1, 1, cascade_states[1].m_pFFTSimulation->GetDisplacementMapGnm());
    if(rm_ds_g_textureDisplacementMap2 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStageVs, rm_ds_g_textureDisplacementMap2, 1, cascade_states[2].m_pFFTSimulation->GetDisplacementMapGnm());
    if(rm_ds_g_textureDisplacementMap3 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStageVs, rm_ds_g_textureDisplacementMap3, 1, cascade_states[3].m_pFFTSimulation->GetDisplacementMapGnm());

	// Pixel textures/samplers
    if(rm_g_samplerGradientMap0 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, Gnm::kShaderStagePs, rm_g_samplerGradientMap0, 1, &m_d3d._gnm.m_pGnmGradMapSampler);
    if(rm_g_samplerGradientMap1 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, Gnm::kShaderStagePs, rm_g_samplerGradientMap1, 1, &m_d3d._gnm.m_pGnmGradMapSampler);
    if(rm_g_samplerGradientMap2 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, Gnm::kShaderStagePs, rm_g_samplerGradientMap2, 1, &m_d3d._gnm.m_pGnmGradMapSampler);
    if(rm_g_samplerGradientMap3 != nvrm_unused)
		gnmxWrap->setSamplers(*gfxContext, Gnm::kShaderStagePs, rm_g_samplerGradientMap3, 1, &m_d3d._gnm.m_pGnmGradMapSampler);
	//
    if(rm_g_textureGradientMap0 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStagePs, rm_g_textureGradientMap0, 1, &cascade_states[0].m_d3d._gnm.m_gnmGradientMap[m_active_GPU_slot]);
    if(rm_g_textureGradientMap1 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStagePs, rm_g_textureGradientMap1, 1, &cascade_states[1].m_d3d._gnm.m_gnmGradientMap[m_active_GPU_slot]);
    if(rm_g_textureGradientMap2 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStagePs, rm_g_textureGradientMap2, 1, &cascade_states[2].m_d3d._gnm.m_gnmGradientMap[m_active_GPU_slot]);
    if(rm_g_textureGradientMap3 != nvrm_unused)
		gnmxWrap->setTextures(*gfxContext, Gnm::kShaderStagePs, rm_g_textureGradientMap3, 1, &cascade_states[3].m_d3d._gnm.m_gnmGradientMap[m_active_GPU_slot]);

    // Constants
    vs_ds_attr_cbuffer VSDSCB;
    vs_ds_attr_cbuffer* pVSDSCB = NULL;
    if(rm_ds_buffer != nvrm_unused || rm_vs_buffer != nvrm_unused)
    {
        pVSDSCB = &VSDSCB;

        pVSDSCB->g_UVScaleCascade0123[0] = 1.0f / m_params.cascades[0].fft_period;
        pVSDSCB->g_UVScaleCascade0123[1] = 1.0f / m_params.cascades[1].fft_period;
        pVSDSCB->g_UVScaleCascade0123[2] = 1.0f / m_params.cascades[2].fft_period;
        pVSDSCB->g_UVScaleCascade0123[3] = 1.0f / m_params.cascades[3].fft_period;

		gfsdk_float4x4 inv_mat_view;
		gfsdk_float4 vec_original = {0,0,0,1};
		gfsdk_float4 vec_transformed;
		mat4Inverse(inv_mat_view,matView);
		vec4Mat4Mul(vec_transformed, vec_original, inv_mat_view);
		gfsdk_float4 vGlobalEye = vec_transformed;

        pVSDSCB->g_WorldEye[0] = vGlobalEye.x;
        pVSDSCB->g_WorldEye[1] = vGlobalEye.y;
        pVSDSCB->g_WorldEye[2] = vGlobalEye.z;
    }

	ps_attr_cbuffer PSCB;
    ps_attr_cbuffer* pPSCB = NULL;
	const FLOAT texel_len = m_params.cascades[0].fft_period / m_params.cascades[0].fft_resolution;
    const float cascade1Scale = m_params.cascades[0].fft_period/m_params.cascades[1].fft_period;
    const float cascade1UVOffset = 0.f; // half-pixel not required in D3D11
    const float cascade2Scale = m_params.cascades[0].fft_period/m_params.cascades[2].fft_period;
    const float cascade2UVOffset = 0.f; // half-pixel not required in D3D11
    const float cascade3Scale = m_params.cascades[0].fft_period/m_params.cascades[3].fft_period;
    const float cascade3UVOffset = 0.f; // half-pixel not required in D3D11

    if(rm_ps_buffer != nvrm_unused)
    {
        pPSCB = &PSCB;
		pPSCB->g_TexelLength_x2_PS = texel_len;
    }

    if(NULL != pPSCB)
    {
        pPSCB->g_Cascade1Scale_PS = cascade1Scale;
        pPSCB->g_Cascade1UVOffset_PS = cascade1UVOffset;
        pPSCB->g_Cascade2Scale_PS = cascade2Scale;
        pPSCB->g_Cascade2UVOffset_PS = cascade2UVOffset;
        pPSCB->g_Cascade3Scale_PS = cascade3Scale;
        pPSCB->g_Cascade3UVOffset_PS = cascade3UVOffset;
        pPSCB->g_Cascade1TexelScale_PS = (m_params.cascades[0].fft_period * m_params.cascades[1].fft_resolution) / (m_params.cascades[1].fft_period * m_params.cascades[0].fft_resolution);
		pPSCB->g_Cascade2TexelScale_PS = (m_params.cascades[0].fft_period * m_params.cascades[2].fft_resolution) / (m_params.cascades[2].fft_period * m_params.cascades[0].fft_resolution);
		pPSCB->g_Cascade3TexelScale_PS = (m_params.cascades[0].fft_period * m_params.cascades[3].fft_resolution) / (m_params.cascades[3].fft_period * m_params.cascades[0].fft_resolution);
    }

    if(pVSDSCB)
    {
		memcpy(m_d3d._gnm.m_pGnmVertexDomainShaderCB.getBaseAddress(), pVSDSCB, sizeof(VSDSCB));
		if(rm_vs_buffer != nvrm_unused)
			gnmxWrap->setConstantBuffers(*gfxContext, vsStage, rm_vs_buffer, 1, &m_d3d._gnm.m_pGnmVertexDomainShaderCB);
		if(rm_ds_buffer != nvrm_unused)
			gnmxWrap->setConstantBuffers(*gfxContext, Gnm::kShaderStageVs, rm_ds_buffer, 1, &m_d3d._gnm.m_pGnmVertexDomainShaderCB);
    }
    if(pPSCB)
    {
		memcpy(m_d3d._gnm.m_pGnmPixelShaderCB.getBaseAddress(), pPSCB, sizeof(PSCB));
		gnmxWrap->setConstantBuffers(*gfxContext, Gnm::kShaderStagePs, rm_ps_buffer, 1, &m_d3d._gnm.m_pGnmPixelShaderCB);
    }

	gnmxWrap->popMarker(*gfxContext);

    return S_OK;
#else
return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::setRenderStateGL2(		const gfsdk_float4x4& GL_ONLY(matView),
															const UINT* GL_ONLY(pShaderInputRegisterMappings),
															const GFSDK_WaveWorks_Simulation_GL_Pool& GL_ONLY(glPool)
															)
{
#if WAVEWORKS_ENABLE_GL
	const GLuint rm_g_textureBindLocationDisplacementMap0 = pShaderInputRegisterMappings[ShaderInputGL2_g_textureBindLocationDisplacementMap0];
    const GLuint rm_g_textureBindLocationDisplacementMap1 = pShaderInputRegisterMappings[ShaderInputGL2_g_textureBindLocationDisplacementMap1];
	const GLuint rm_g_textureBindLocationDisplacementMap2 = pShaderInputRegisterMappings[ShaderInputGL2_g_textureBindLocationDisplacementMap2];
    const GLuint rm_g_textureBindLocationDisplacementMap3 = pShaderInputRegisterMappings[ShaderInputGL2_g_textureBindLocationDisplacementMap3];
    const GLuint rm_g_textureBindLocationGradientMap0 = pShaderInputRegisterMappings[ShaderInputGL2_g_textureBindLocationGradientMap0];
    const GLuint rm_g_textureBindLocationGradientMap1 = pShaderInputRegisterMappings[ShaderInputGL2_g_textureBindLocationGradientMap1];
    const GLuint rm_g_textureBindLocationGradientMap2 = pShaderInputRegisterMappings[ShaderInputGL2_g_textureBindLocationGradientMap2];
    const GLuint rm_g_textureBindLocationGradientMap3 = pShaderInputRegisterMappings[ShaderInputGL2_g_textureBindLocationGradientMap3];
	const GLuint rm_g_textureBindLocationDisplacementMapArray = pShaderInputRegisterMappings[ShaderInputGL2_g_textureBindLocationDisplacementMapArray];
	const GLuint rm_g_textureBindLocationGradientMapArray = pShaderInputRegisterMappings[ShaderInputGL2_g_textureBindLocationGradientMapArray];
    const GLuint rm_g_WorldEye = pShaderInputRegisterMappings[ShaderInputGL2_g_WorldEye];
	const GLuint rm_g_UseTextureArrays = pShaderInputRegisterMappings[ShaderInputGL2_g_UseTextureArrays];
    const GLuint rm_g_UVScaleCascade0123 = pShaderInputRegisterMappings[ShaderInputGL2_g_UVScaleCascade0123];
    const GLuint rm_g_TexelLength_x2_PS = pShaderInputRegisterMappings[ShaderInputGL2_g_TexelLength_x2_PS];
    const GLuint rm_g_Cascade1Scale_PS = pShaderInputRegisterMappings[ShaderInputGL2_g_Cascade1Scale_PS];
    const GLuint rm_g_Cascade1TexelScale_PS = pShaderInputRegisterMappings[ShaderInputGL2_g_Cascade1TexelScale_PS];
    const GLuint rm_g_Cascade1UVOffset_PS = pShaderInputRegisterMappings[ShaderInputGL2_g_Cascade1UVOffset_PS];
    const GLuint rm_g_Cascade2Scale_PS = pShaderInputRegisterMappings[ShaderInputGL2_g_Cascade2Scale_PS];
    const GLuint rm_g_Cascade2TexelScale_PS = pShaderInputRegisterMappings[ShaderInputGL2_g_Cascade2TexelScale_PS];
    const GLuint rm_g_Cascade2UVOffset_PS = pShaderInputRegisterMappings[ShaderInputGL2_g_Cascade2UVOffset_PS];
    const GLuint rm_g_Cascade3Scale_PS = pShaderInputRegisterMappings[ShaderInputGL2_g_Cascade3Scale_PS];
    const GLuint rm_g_Cascade3TexelScale_PS = pShaderInputRegisterMappings[ShaderInputGL2_g_Cascade3TexelScale_PS];
    const GLuint rm_g_Cascade3UVOffset_PS = pShaderInputRegisterMappings[ShaderInputGL2_g_Cascade3UVOffset_PS];

	GLuint tu_DisplacementMap0 = 0;
	GLuint tu_DisplacementMap1 = 0;
	GLuint tu_DisplacementMap2 = 0;
	GLuint tu_DisplacementMap3 = 0;
	GLuint tu_GradientMap0 = 0;
	GLuint tu_GradientMap1 = 0;
	GLuint tu_GradientMap2 = 0;
	GLuint tu_GradientMap3 = 0;
	GLuint tu_DisplacementMapTextureArray = 0;
	GLuint tu_GradientMapTextureArray = 0;

	if(m_params.use_texture_arrays)
	{
		tu_DisplacementMapTextureArray = glPool.Reserved_Texture_Units[0];
		tu_GradientMapTextureArray = glPool.Reserved_Texture_Units[1];

	}
	else
	{
		tu_DisplacementMap0 = glPool.Reserved_Texture_Units[0];
		tu_DisplacementMap1 = glPool.Reserved_Texture_Units[1];
		tu_DisplacementMap2 = glPool.Reserved_Texture_Units[2];
		tu_DisplacementMap3 = glPool.Reserved_Texture_Units[3];
		tu_GradientMap0 = glPool.Reserved_Texture_Units[4];
		tu_GradientMap1 = glPool.Reserved_Texture_Units[5];
		tu_GradientMap2 = glPool.Reserved_Texture_Units[6];
		tu_GradientMap3 = glPool.Reserved_Texture_Units[7];	
	}
	
	if(m_params.use_texture_arrays)
	{
		UINT N = m_params.cascades[GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades-1].fft_resolution;

		// assembling the displacement textures to texture array
		// glBlitFramebuffer does upscale for cascades with smaller fft_resolution
		NVSDK_GLFunctions.glBindFramebuffer(GL_READ_FRAMEBUFFER, m_d3d._GL2.m_TextureArraysBlittingReadFBO); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_d3d._GL2.m_TextureArraysBlittingDrawFBO); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glReadBuffer(GL_COLOR_ATTACHMENT0); CHECK_GL_ERRORS;
		const GLenum bufs = GL_COLOR_ATTACHMENT0;
		NVSDK_GLFunctions.glDrawBuffers(1, &bufs); CHECK_GL_ERRORS;
		for(int i = 0; i < GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades; i++)
		{

			NVSDK_GLFunctions.glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cascade_states[i].m_pFFTSimulation->GetDisplacementMapGL2(), 0); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_d3d._GL2.m_DisplacementsTextureArray, 0, i); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBlitFramebuffer(0, 0, m_params.cascades[i].fft_resolution, m_params.cascades[i].fft_resolution, 0, 0, N, N, GL_COLOR_BUFFER_BIT, GL_LINEAR); CHECK_GL_ERRORS;
		}
	
		// assembling the gradient textures to texture array
		for(int i = 0; i < GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades; i++)
		{

			NVSDK_GLFunctions.glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cascade_states[i].m_d3d._GL2.m_GL2GradientMap[m_active_GPU_slot], 0); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_d3d._GL2.m_GradientsTextureArray, 0, i); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBlitFramebuffer(0, 0, m_params.cascades[i].fft_resolution, m_params.cascades[i].fft_resolution, 0, 0, N, N, GL_COLOR_BUFFER_BIT, GL_LINEAR); CHECK_GL_ERRORS;
		}
		NVSDK_GLFunctions.glBindFramebuffer(GL_FRAMEBUFFER, 0); CHECK_GL_ERRORS;

		// generating mipmaps for gradient texture array, using gradient texture array texture unit
		NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + tu_GradientMapTextureArray); CHECK_GL_ERRORS;
		for(int i=0; i<GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades;i++)
		{
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D_ARRAY, m_d3d._GL2.m_GradientsTextureArray); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glGenerateMipmap(GL_TEXTURE_2D_ARRAY); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D_ARRAY,0);
		}
	}	

	// Textures
	
	if(m_params.use_texture_arrays)
	{
		if(rm_g_textureBindLocationDisplacementMapArray != nvrm_unused)
		{
			NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + tu_DisplacementMapTextureArray); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D_ARRAY, m_d3d._GL2.m_DisplacementsTextureArray); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glUniform1i(rm_g_textureBindLocationDisplacementMapArray, tu_DisplacementMapTextureArray);
		}
		if(rm_g_textureBindLocationGradientMapArray != nvrm_unused)
		{
			NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + tu_GradientMapTextureArray); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D_ARRAY, m_d3d._GL2.m_GradientsTextureArray); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat)m_params.aniso_level); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glUniform1i(rm_g_textureBindLocationGradientMapArray, tu_GradientMapTextureArray);
		}	
	}
	else
	
	{
		if(rm_g_textureBindLocationDisplacementMap0 != nvrm_unused)
		{
			NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + tu_DisplacementMap0); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[0].m_pFFTSimulation->GetDisplacementMapGL2()); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glUniform1i(rm_g_textureBindLocationDisplacementMap0, tu_DisplacementMap0);
		}
		if(rm_g_textureBindLocationDisplacementMap1 != nvrm_unused)
		{
			NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + tu_DisplacementMap1); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[1].m_pFFTSimulation->GetDisplacementMapGL2()); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glUniform1i(rm_g_textureBindLocationDisplacementMap1, tu_DisplacementMap1);
		}
		if(rm_g_textureBindLocationDisplacementMap2 != nvrm_unused)
		{
			NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + tu_DisplacementMap2); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[2].m_pFFTSimulation->GetDisplacementMapGL2()); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glUniform1i(rm_g_textureBindLocationDisplacementMap2, tu_DisplacementMap2);
		}
		if(rm_g_textureBindLocationDisplacementMap3 != nvrm_unused)
		{
			NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + tu_DisplacementMap3); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[3].m_pFFTSimulation->GetDisplacementMapGL2()); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glUniform1i(rm_g_textureBindLocationDisplacementMap3, tu_DisplacementMap3);
		}
		//
		if(rm_g_textureBindLocationGradientMap0 != nvrm_unused)
		{
			NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + tu_GradientMap0); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[0].m_d3d._GL2.m_GL2GradientMap[m_active_GPU_slot]); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat)m_params.aniso_level); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glUniform1i(rm_g_textureBindLocationGradientMap0, tu_GradientMap0);
		}
		if(rm_g_textureBindLocationGradientMap1 != nvrm_unused)
		{
			NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + tu_GradientMap1); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[1].m_d3d._GL2.m_GL2GradientMap[m_active_GPU_slot]); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat)m_params.aniso_level); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glUniform1i(rm_g_textureBindLocationGradientMap1, tu_GradientMap1);
		}
		if(rm_g_textureBindLocationGradientMap2 != nvrm_unused)
		{
			NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + tu_GradientMap2); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[2].m_d3d._GL2.m_GL2GradientMap[m_active_GPU_slot]); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat)m_params.aniso_level); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glUniform1i(rm_g_textureBindLocationGradientMap2, tu_GradientMap2);
		}
		if(rm_g_textureBindLocationGradientMap3 != nvrm_unused)
		{
			NVSDK_GLFunctions.glActiveTexture(GL_TEXTURE0 + tu_GradientMap3); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[3].m_d3d._GL2.m_GL2GradientMap[m_active_GPU_slot]); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat)m_params.aniso_level); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glUniform1i(rm_g_textureBindLocationGradientMap3, tu_GradientMap3);
		}
	}
     
    // Constants
	gfsdk_float4 UVScaleCascade0123;
	UVScaleCascade0123.x = 1.0f / m_params.cascades[0].fft_period;
	UVScaleCascade0123.y = 1.0f / m_params.cascades[1].fft_period;
	UVScaleCascade0123.z = 1.0f / m_params.cascades[2].fft_period;
	UVScaleCascade0123.w = 1.0f / m_params.cascades[3].fft_period;

	gfsdk_float4x4 inv_mat_view;
	gfsdk_float4 vec_original = {0,0,0,1};
	gfsdk_float4 vec_transformed;
	mat4Inverse(inv_mat_view,matView);
	vec4Mat4Mul(vec_transformed, vec_original, inv_mat_view);
	gfsdk_float4 vGlobalEye = vec_transformed;

	const float texel_len = m_params.cascades[0].fft_period / m_params.cascades[0].fft_resolution;
    const float cascade1Scale = m_params.cascades[0].fft_period/m_params.cascades[1].fft_period;
	const float cascade1TexelScale = (m_params.cascades[0].fft_period * m_params.cascades[1].fft_resolution) / (m_params.cascades[1].fft_period * m_params.cascades[0].fft_resolution);
	const float cascade1UVOffset = 0;
    const float cascade2Scale = m_params.cascades[0].fft_period/m_params.cascades[2].fft_period;
	const float cascade2TexelScale = (m_params.cascades[0].fft_period * m_params.cascades[2].fft_resolution) / (m_params.cascades[2].fft_period * m_params.cascades[0].fft_resolution);
	const float cascade2UVOffset = 0;
    const float cascade3Scale = m_params.cascades[0].fft_period/m_params.cascades[3].fft_period;
	const float cascade3TexelScale = (m_params.cascades[0].fft_period * m_params.cascades[3].fft_resolution) / (m_params.cascades[3].fft_period * m_params.cascades[0].fft_resolution);
	const float cascade3UVOffset = 0;

    if(rm_g_WorldEye != nvrm_unused)
	{
       NVSDK_GLFunctions.glUniform3fv(rm_g_WorldEye, 1, (GLfloat*)&vGlobalEye); CHECK_GL_ERRORS;
	}
    if(rm_g_UseTextureArrays != nvrm_unused)
	{
		NVSDK_GLFunctions.glUniform1f(rm_g_UseTextureArrays, m_params.use_texture_arrays ? 1.0f:0.0f); CHECK_GL_ERRORS;
	}	
    if(rm_g_UVScaleCascade0123 != nvrm_unused)
	{
       NVSDK_GLFunctions.glUniform4fv(rm_g_UVScaleCascade0123, 1, (GLfloat*)&UVScaleCascade0123); CHECK_GL_ERRORS;
	}
	if(rm_g_TexelLength_x2_PS != nvrm_unused)
	{
		NVSDK_GLFunctions.glUniform1f(rm_g_TexelLength_x2_PS, texel_len); CHECK_GL_ERRORS;
	}
    //
    if(rm_g_Cascade1Scale_PS != nvrm_unused)
	{
        NVSDK_GLFunctions.glUniform1f(rm_g_Cascade1Scale_PS, cascade1Scale); CHECK_GL_ERRORS;
	}
    if(rm_g_Cascade1TexelScale_PS != nvrm_unused)
	{
		NVSDK_GLFunctions.glUniform1f(rm_g_Cascade1TexelScale_PS, cascade1TexelScale); CHECK_GL_ERRORS;
	}
    if(rm_g_Cascade1UVOffset_PS != nvrm_unused)
	{
		NVSDK_GLFunctions.glUniform1f(rm_g_Cascade1UVOffset_PS, cascade1UVOffset); CHECK_GL_ERRORS;
	}

    if(rm_g_Cascade2Scale_PS != nvrm_unused)
	{
        NVSDK_GLFunctions.glUniform1f(rm_g_Cascade2Scale_PS, cascade2Scale); CHECK_GL_ERRORS;
	}
    if(rm_g_Cascade2TexelScale_PS != nvrm_unused)
	{
		NVSDK_GLFunctions.glUniform1f(rm_g_Cascade2TexelScale_PS, cascade2TexelScale); CHECK_GL_ERRORS;
	}
    if(rm_g_Cascade2UVOffset_PS != nvrm_unused)
	{
		NVSDK_GLFunctions.glUniform1f(rm_g_Cascade2UVOffset_PS, cascade2UVOffset); CHECK_GL_ERRORS;
	}

    if(rm_g_Cascade3Scale_PS != nvrm_unused)
	{
        NVSDK_GLFunctions.glUniform1f(rm_g_Cascade3Scale_PS, cascade3Scale); CHECK_GL_ERRORS;
	}
    if(rm_g_Cascade3TexelScale_PS != nvrm_unused)
	{
		NVSDK_GLFunctions.glUniform1f(rm_g_Cascade3TexelScale_PS, cascade3TexelScale); CHECK_GL_ERRORS;
	}
    if(rm_g_Cascade3UVOffset_PS != nvrm_unused)
	{
		NVSDK_GLFunctions.glUniform1f(rm_g_Cascade3UVOffset_PS, cascade3UVOffset); CHECK_GL_ERRORS;
	}
	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::kick(gfsdk_U64* pKickID, Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl)
{
    HRESULT hr;

#if WAVEWORKS_ENABLE_GNM
	sce::Gnmx::LightweightGfxContext* gfxContext_gnm = NULL;
	GFSDK_WaveWorks_GnmxWrap* gnmxWrap = NULL;
	if(nv_water_d3d_api_gnm == m_d3dAPI)
	{
		gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();
		gfxContext_gnm = pGC->gnm();
		gnmxWrap->pushMarker(*gfxContext_gnm, "GFSDK_WaveWorks_Simulation::kick");
	}
#endif

	// Activate GPU slot for current frame
	// TODO: this assumes that we experience one tick per frame - this is unlikely to hold true in general
	//       the difficulty here is how to reliably detect when work switches to a new GPU
	//       - relying on the Kick() will fail if the developer ever ticks twice in a frame (likely)
	//       - relying on SetRenderState() is even more fragile, because it will fail if the water is rendered twice in a frame (very likely)
	//       - we could probably rely on NVAPI on NVIDIA setups, but what about non-NVIDIA setups?
	//       - so seems like we need to support this in the API
	//
	consumeGPUSlot();

	TimerSlot* pTimerSlot = NULL;
	if(m_pGFXTimer)
	{
		V_RETURN(queryAllGfxTimers(pGC, m_pGFXTimer));

		// Bracket GPU work with a disjoint timer query
		V_RETURN(m_pGFXTimer->beginDisjoint(pGC));

		V_RETURN(consumeAvailableTimerSlot(pGC, m_pGFXTimer, m_gpu_kick_timers, &pTimerSlot));
		m_pGFXTimer->issueTimerQuery(pGC, pTimerSlot->m_StartQueryIndex);

		// This is ensures that wait-timers report zero when the wait API is unused
		// The converse is unnecessary, since the user cannot get useful work done without calling kick()
		if(!m_has_consumed_wait_timer_slot_since_last_kick)
		{
			TimerSlot* pWaitTimerSlot = NULL;
			V_RETURN(consumeAvailableTimerSlot(pGC, m_pGFXTimer, m_gpu_wait_timers, &pWaitTimerSlot));

			// Setting the djqi to an invalid index causes this slot to be handled as a 'dummy' query
			// i.e. no attempt will be made to retrieve real GPU timing data, and the timing values
			// already in the slot (i.e. zero) will be used as the timing data, which is what we want
			if(NVWaveWorks_GFX_Timer_Impl::InvalidQueryIndex != pWaitTimerSlot->m_DisjointQueryIndex)
			{
				m_pGFXTimer->releaseDisjointQuery(pWaitTimerSlot->m_DisjointQueryIndex);
				pWaitTimerSlot->m_DisjointQueryIndex = NVWaveWorks_GFX_Timer_Impl::InvalidQueryIndex;
			}
		}
	}

	// Reset for next kick-to-kick interval
	m_has_consumed_wait_timer_slot_since_last_kick = false;

	gfsdk_U64 kickID;
	V_RETURN(m_pSimulationManager->kick(pGC,m_dSimTime,kickID));

	if(m_pGFXTimer) {
		m_pGFXTimer->issueTimerQuery(pGC, pTimerSlot->m_StartGFXQueryIndex);
	}

    V_RETURN(updateGradientMaps(pGC,pSavestateImpl));

	if(m_pGFXTimer)
	{
		m_pGFXTimer->issueTimerQuery(pGC, pTimerSlot->m_StopGFXQueryIndex);

		m_pGFXTimer->issueTimerQuery(pGC, pTimerSlot->m_StopQueryIndex);

		V_RETURN(m_pGFXTimer->endDisjoint(pGC));
	}

#if WAVEWORKS_ENABLE_GNM
	if(nv_water_d3d_api_gnm == m_d3dAPI)
	{
		gnmxWrap->popMarker(*gfxContext_gnm);
	}
#endif

	if(pKickID)
	{
		*pKickID = kickID;
	}

    return S_OK;
}

HRESULT GFSDK_WaveWorks_Simulation::getShaderInputCountD3D9()
{
    return NumShaderInputsD3D9;
}

HRESULT GFSDK_WaveWorks_Simulation::getShaderInputCountD3D10()
{
    return NumShaderInputsD3D10;
}

HRESULT GFSDK_WaveWorks_Simulation::getShaderInputCountD3D11()
{
    return NumShaderInputsD3D11;
}

HRESULT GFSDK_WaveWorks_Simulation::getShaderInputCountGnm()
{
	return NumShaderInputsGnm;
}

HRESULT GFSDK_WaveWorks_Simulation::getShaderInputCountGL2()
{
	return NumShaderInputsGL2;
}

HRESULT GFSDK_WaveWorks_Simulation::getTextureUnitCountGL2(bool useTextureArrays)
{
	return useTextureArrays? 2:8;
}

HRESULT GFSDK_WaveWorks_Simulation::getShaderInputDescD3D9(UINT D3D9_ONLY(inputIndex), GFSDK_WaveWorks_ShaderInput_Desc* D3D9_ONLY(pDesc))
{
#if WAVEWORKS_ENABLE_D3D9
    if(inputIndex >= NumShaderInputsD3D9)
        return E_FAIL;

    *pDesc = ShaderInputDescsD3D9[inputIndex];

    return S_OK;
#else // WAVEWORKS_ENABLE_D3D9
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::getShaderInputDescD3D10(UINT D3D10_ONLY(inputIndex), GFSDK_WaveWorks_ShaderInput_Desc* D3D10_ONLY(pDesc))
{
#if WAVEWORKS_ENABLE_D3D10
    if(inputIndex >= NumShaderInputsD3D10)
        return E_FAIL;

    *pDesc = ShaderInputDescsD3D10[inputIndex];

    return S_OK;
#else // WAVEWORKS_ENABLE_D3D10
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::getShaderInputDescD3D11(UINT D3D11_ONLY(inputIndex), GFSDK_WaveWorks_ShaderInput_Desc* D3D11_ONLY(pDesc))
{
#if WAVEWORKS_ENABLE_D3D11
    if(inputIndex >= NumShaderInputsD3D11)
        return E_FAIL;

    *pDesc = ShaderInputDescsD3D11[inputIndex];

    return S_OK;
#else // WAVEWORKS_ENABLE_D3D11
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::getShaderInputDescGnm(UINT GNM_ONLY(inputIndex), GFSDK_WaveWorks_ShaderInput_Desc* GNM_ONLY(pDesc))
{
#if WAVEWORKS_ENABLE_GNM
	if(inputIndex >= NumShaderInputsGnm)
		return E_FAIL;

	*pDesc = ShaderInputDescsGnm[inputIndex];

	return S_OK;
#else // WAVEWORKS_ENABLE_GNM
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::getShaderInputDescGL2(UINT GL_ONLY(inputIndex), GFSDK_WaveWorks_ShaderInput_Desc* GL_ONLY(pDesc))
{
#if WAVEWORKS_ENABLE_GL
    if(inputIndex >= NumShaderInputsGL2)
        return E_FAIL;

    *pDesc = ShaderInputDescsGL2[inputIndex];

    return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Simulation::getDisplacements(	const gfsdk_float2* inSamplePoints,
                                                        gfsdk_float4* outDisplacements,
                                                        UINT numSamples
                                                        )
{
    HRESULT hr;

    // Initialise displacements
    memset(outDisplacements, 0, numSamples * sizeof(*outDisplacements));

    for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
    {
		V_RETURN(cascade_states[cascade].m_pFFTSimulation->addDisplacements(inSamplePoints,outDisplacements,numSamples));
    }

    return S_OK;
}

HRESULT GFSDK_WaveWorks_Simulation::getArchivedDisplacements(	float coord,
																const gfsdk_float2* inSamplePoints,
																gfsdk_float4* outDisplacements,
																UINT numSamples
																)
{
    HRESULT hr;

    // Initialise displacements
    memset(outDisplacements, 0, numSamples * sizeof(*outDisplacements));

    for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
    {
		V_RETURN(cascade_states[cascade].m_pFFTSimulation->addArchivedDisplacements(coord,inSamplePoints,outDisplacements,numSamples));
    }

    return S_OK;
}

HRESULT GFSDK_WaveWorks_Simulation::allocateRenderingResources(int cascade)
{
	HRESULT hr;

	V_RETURN(initQuadMesh(cascade));

	m_num_GPU_slots = m_params.num_GPUs;
	m_active_GPU_slot = m_num_GPU_slots-1;	// First tick will tip back to zero
	m_numValidEntriesInSimTimeFIFO = 0;

#if WAVEWORKS_ENABLE_GRAPHICS
    int dmap_dim =m_params.cascades[cascade].fft_resolution;

	for(int gpu_slot = 0; gpu_slot != m_num_GPU_slots; ++gpu_slot)
	{
		switch(m_d3dAPI)
		{
#if WAVEWORKS_ENABLE_D3D9
		case nv_water_d3d_api_d3d9:
			{
				V_RETURN(m_d3d._9.m_pd3d9Device->CreateTexture(dmap_dim, dmap_dim, 0, D3DUSAGE_RENDERTARGET|D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &cascade_states[cascade].m_d3d._9.m_pd3d9GradientMap[gpu_slot], NULL));
			}
			break;
#endif
#if WAVEWORKS_ENABLE_D3D10
		case nv_water_d3d_api_d3d10:
			{
				D3D10_TEXTURE2D_DESC gradMapTD;
				gradMapTD.Width = dmap_dim;
				gradMapTD.Height = dmap_dim;
				gradMapTD.MipLevels = 0;
				gradMapTD.ArraySize = 1;
				gradMapTD.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				gradMapTD.SampleDesc = kNoSample;
				gradMapTD.Usage = D3D10_USAGE_DEFAULT;
				gradMapTD.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
				gradMapTD.CPUAccessFlags = 0;
				gradMapTD.MiscFlags = D3D10_RESOURCE_MISC_GENERATE_MIPS;

				ID3D10Texture2D* pD3D10Texture = NULL;
				V_RETURN(m_d3d._10.m_pd3d10Device->CreateTexture2D(&gradMapTD, NULL, &pD3D10Texture));
				V_RETURN(m_d3d._10.m_pd3d10Device->CreateShaderResourceView(pD3D10Texture, NULL, &cascade_states[cascade].m_d3d._10.m_pd3d10GradientMap[gpu_slot]));
				V_RETURN(m_d3d._10.m_pd3d10Device->CreateRenderTargetView(pD3D10Texture, NULL, &cascade_states[cascade].m_d3d._10.m_pd3d10GradientRenderTarget[gpu_slot]));
				SAFE_RELEASE(pD3D10Texture);
			}
			break;
#endif
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			{
				D3D11_TEXTURE2D_DESC gradMapTD;
				gradMapTD.Width = dmap_dim;
				gradMapTD.Height = dmap_dim;
				gradMapTD.MipLevels = 0;
				gradMapTD.ArraySize = 1;
				gradMapTD.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				gradMapTD.SampleDesc = kNoSample;
				gradMapTD.Usage = D3D11_USAGE_DEFAULT;
				gradMapTD.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
				gradMapTD.CPUAccessFlags = 0;
				gradMapTD.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

				ID3D11Texture2D* pD3D11Texture = NULL;
				V_RETURN(m_d3d._11.m_pd3d11Device->CreateTexture2D(&gradMapTD, NULL, &pD3D11Texture));
				V_RETURN(m_d3d._11.m_pd3d11Device->CreateShaderResourceView(pD3D11Texture, NULL, &cascade_states[cascade].m_d3d._11.m_pd3d11GradientMap[gpu_slot]));
				V_RETURN(m_d3d._11.m_pd3d11Device->CreateRenderTargetView(pD3D11Texture, NULL, &cascade_states[cascade].m_d3d._11.m_pd3d11GradientRenderTarget[gpu_slot]));
				SAFE_RELEASE(pD3D11Texture);
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			{
				int mips = 1;
				for(int pixels = dmap_dim; pixels >>= 1; ++mips)
					;

				Gnm::DataFormat dataFormat = Gnm::kDataFormatR16G16B16A16Float;
				Gnm::TileMode tileMode;
				GpuAddress::computeSurfaceTileMode(&tileMode, GpuAddress::kSurfaceTypeRwTextureFlat, dataFormat, 1);
#if 1
				Gnm::SizeAlign sizeAlign = cascade_states[cascade].m_d3d._gnm.m_gnmGradientMap[gpu_slot].initAs2d(dmap_dim, dmap_dim, mips, dataFormat, tileMode, SAMPLE_1);
				cascade_states[cascade].m_d3d._gnm.m_gnmGradientMap[gpu_slot].setBaseAddress(NVSDK_garlic_malloc(sizeAlign.m_size, sizeAlign.m_align));
				cascade_states[cascade].m_d3d._gnm.m_gnmGradientMap[gpu_slot].setResourceMemoryType(Gnm::kResourceMemoryTypeGC);
				cascade_states[cascade].m_d3d._gnm.m_gnmGradientRenderTarget[gpu_slot].initFromTexture(&cascade_states[cascade].m_d3d._gnm.m_gnmGradientMap[gpu_slot], 0);

				/* testing...
				struct rgba { uint16_t r, g, b, a; };
				rgba* tmp = (rgba*)NVSDK_aligned_malloc(dmap_dim * dmap_dim * sizeof(rgba), 16);
				Gnm::Texture texture = cascade_states[cascade].m_d3d._gnm.m_gnmGradientMap[gpu_slot];
				for(uint32_t level=0, width = dmap_dim; width > 0; ++level, width >>= 1)
				{
					for(uint32_t j=0; j<width; ++j)
					{
						for(uint32_t i=0; i<width; ++i)
						{
							rgba color = { 
								Gnmx::convertF32ToF16(i / (width - 1.0f)), 
								Gnmx::convertF32ToF16(j / (width - 1.0f)), 
								Gnmx::convertF32ToF16(level * 32.0f), 
								Gnmx::convertF32ToF16(1.0f) };
							tmp[j*width + i] = color;
						}
					}
					GpuAddress::TilingParameters tp;
					tp.initFromTexture(&texture, level, 0);
					uint64_t base;
					GpuAddress::computeTextureSurfaceOffsetAndSize(&base, (uint64_t*)0, &texture, level, 0);
					GpuAddress::tileSurface((rgba*)texture.getBaseAddress() + base / sizeof(rgba), tmp, &tp);
				}
				NVSDK_aligned_free(tmp);
				*/

#else // try the other way around....
				Gnm::SizeAlign sizeAlign = cascade_states[cascade].m_d3d._gnm.m_gnmGradientRenderTarget[gpu_slot].init(dmap_dim, dmap_dim, 1, dataFormat, tileMode, Gnm::kNumSamples1, Gnm::kNumFragments1, NULL, NULL);
				cascade_states[cascade].m_d3d._gnm.m_gnmGradientRenderTarget[gpu_slot].setAddresses(NVSDK_garlic_malloc(sizeAlign.m_size, sizeAlign.m_align), NULL, NULL);
				cascade_states[cascade].m_d3d._gnm.m_gnmGradientMap[gpu_slot].initFromRenderTarget(&cascade_states[cascade].m_d3d._gnm.m_gnmGradientRenderTarget[gpu_slot], false);
#endif
				// cascade_states[cascade].m_d3d._gnm.m_gnmGradientMap[gpu_slot].setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // we never write to this texture from a shader, so it's OK to mark the texture as read-only.
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				GLuint framebuffer_binding_result = 0; 
				NVSDK_GLFunctions.glGenTextures(1, &cascade_states[cascade].m_d3d._GL2.m_GL2GradientMap[gpu_slot]); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[cascade].m_d3d._GL2.m_GL2GradientMap[gpu_slot]); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, dmap_dim, dmap_dim, 0, GL_RGBA, GL_FLOAT, NULL); CHECK_GL_ERRORS;
				// do not allocate memory for gradient maps' mipmaps if texture arrays for gradient maps are used
				if(m_params.use_texture_arrays == false)
				{
					NVSDK_GLFunctions.glGenerateMipmap(GL_TEXTURE_2D); CHECK_GL_ERRORS;
				}
				NVSDK_GLFunctions.glGenFramebuffers(1, &cascade_states[cascade].m_d3d._GL2.m_GL2GradientFBO[gpu_slot]); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBindFramebuffer(GL_FRAMEBUFFER, cascade_states[cascade].m_d3d._GL2.m_GL2GradientFBO[gpu_slot]); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cascade_states[cascade].m_d3d._GL2.m_GL2GradientMap[gpu_slot], 0); CHECK_GL_ERRORS;
				framebuffer_binding_result = NVSDK_GLFunctions.glCheckFramebufferStatus(GL_FRAMEBUFFER); CHECK_GL_ERRORS;
				if(framebuffer_binding_result != GL_FRAMEBUFFER_COMPLETE) return E_FAIL;
				NVSDK_GLFunctions.glBindFramebuffer(GL_FRAMEBUFFER, 0); CHECK_GL_ERRORS;
			}
			break;
#endif
		case nv_water_d3d_api_none:
			break;
		default:
			// Unexpected API
			return E_FAIL;
		}
		cascade_states[cascade].m_gradient_map_needs_clear[gpu_slot] = true;
	}

    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        {
			V_RETURN(m_d3d._9.m_pd3d9Device->CreateTexture(dmap_dim, dmap_dim, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R16F, D3DPOOL_DEFAULT, &cascade_states[cascade].m_d3d._9.m_pd3d9FoamEnergyMap, NULL));
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
            D3D10_TEXTURE2D_DESC foamenergyTD;
            foamenergyTD.Width = dmap_dim;
            foamenergyTD.Height = dmap_dim;
            foamenergyTD.MipLevels = 1;
            foamenergyTD.ArraySize = 1;
			foamenergyTD.Format = DXGI_FORMAT_R16_FLOAT;
            foamenergyTD.SampleDesc = kNoSample;
            foamenergyTD.Usage = D3D10_USAGE_DEFAULT;
            foamenergyTD.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
            foamenergyTD.CPUAccessFlags = 0;
            foamenergyTD.MiscFlags = 0;

            ID3D10Texture2D* pD3D10FoamEnergyTexture = NULL;
            V_RETURN(m_d3d._10.m_pd3d10Device->CreateTexture2D(&foamenergyTD, NULL, &pD3D10FoamEnergyTexture));
			V_RETURN(m_d3d._10.m_pd3d10Device->CreateShaderResourceView(pD3D10FoamEnergyTexture, NULL, &cascade_states[cascade].m_d3d._10.m_pd3d10FoamEnergyMap));
			V_RETURN(m_d3d._10.m_pd3d10Device->CreateRenderTargetView(pD3D10FoamEnergyTexture, NULL, &cascade_states[cascade].m_d3d._10.m_pd3d10FoamEnergyRenderTarget));
            SAFE_RELEASE(pD3D10FoamEnergyTexture);
		
		}
		break;
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
            D3D11_TEXTURE2D_DESC foamenergyTD;
            foamenergyTD.Width = dmap_dim;
            foamenergyTD.Height = dmap_dim;
            foamenergyTD.MipLevels = 1;
            foamenergyTD.ArraySize = 1;
            foamenergyTD.Format = DXGI_FORMAT_R16_FLOAT;
            foamenergyTD.SampleDesc = kNoSample;
            foamenergyTD.Usage = D3D11_USAGE_DEFAULT;
            foamenergyTD.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            foamenergyTD.CPUAccessFlags = 0;
            foamenergyTD.MiscFlags = 0;

            ID3D11Texture2D* pD3D11FoamEnergyTexture = NULL;
            V_RETURN(m_d3d._11.m_pd3d11Device->CreateTexture2D(&foamenergyTD, NULL, &pD3D11FoamEnergyTexture));
			V_RETURN(m_d3d._11.m_pd3d11Device->CreateShaderResourceView(pD3D11FoamEnergyTexture, NULL, &cascade_states[cascade].m_d3d._11.m_pd3d11FoamEnergyMap));
			V_RETURN(m_d3d._11.m_pd3d11Device->CreateRenderTargetView(pD3D11FoamEnergyTexture, NULL, &cascade_states[cascade].m_d3d._11.m_pd3d11FoamEnergyRenderTarget));
            SAFE_RELEASE(pD3D11FoamEnergyTexture);
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			Gnm::DataFormat dataFormat = Gnm::kDataFormatR16Float;
			Gnm::TileMode tileMode;
			GpuAddress::computeSurfaceTileMode(&tileMode, GpuAddress::kSurfaceTypeColorTarget, dataFormat, 1);
#if 1
			Gnm::SizeAlign sizeAlign = cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyMap.initAs2d(dmap_dim, dmap_dim, 1, dataFormat, tileMode, SAMPLE_1);
			cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyMap.setBaseAddress(NVSDK_garlic_malloc(sizeAlign.m_size, sizeAlign.m_align));
			cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyMap.setResourceMemoryType(Gnm::kResourceMemoryTypeGC);
			cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyRenderTarget.initFromTexture(&cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyMap, SAMPLE_1);
#else // try the other way around....
			Gnm::SizeAlign sizeAlign = cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyRenderTarget.init(dmap_dim, dmap_dim, 1, dataFormat, tileMode, Gnm::kNumSamples1, Gnm::kNumFragments1, NULL, NULL);
			cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyRenderTarget.setAddresses(NVSDK_garlic_malloc(sizeAlign.m_size, sizeAlign.m_align), NULL, NULL);
			cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyMap.initFromRenderTarget(&cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyRenderTarget, false);
#endif
			cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyMap.setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // we never write to this texture from a shader, so it's OK to mark the texture as read-only.
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				GLuint framebuffer_binding_result = 0; 
				NVSDK_GLFunctions.glGenTextures(1, &cascade_states[cascade].m_d3d._GL2.m_GL2FoamEnergyMap); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, cascade_states[cascade].m_d3d._GL2.m_GL2FoamEnergyMap); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, dmap_dim, dmap_dim, 0, GL_RED, GL_FLOAT, NULL); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glGenFramebuffers(1, &cascade_states[cascade].m_d3d._GL2.m_GL2FoamEnergyFBO); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBindFramebuffer(GL_FRAMEBUFFER, cascade_states[cascade].m_d3d._GL2.m_GL2FoamEnergyFBO); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cascade_states[cascade].m_d3d._GL2.m_GL2FoamEnergyMap, 0); CHECK_GL_ERRORS;
				framebuffer_binding_result = NVSDK_GLFunctions.glCheckFramebufferStatus(GL_FRAMEBUFFER); CHECK_GL_ERRORS;
				if(framebuffer_binding_result != GL_FRAMEBUFFER_COMPLETE) return E_FAIL;
				NVSDK_GLFunctions.glBindFramebuffer(GL_FRAMEBUFFER, 0); CHECK_GL_ERRORS;
			}
			break;
#endif
	case nv_water_d3d_api_none:
		break;
	default:
		// Unexpected API
		return E_FAIL;
    }

#endif // WAVEWORKS_ENABLE_GRAPHICS
	cascade_states[cascade].m_gradient_map_version = GFSDK_WaveWorks_InvalidKickID;

	return S_OK;
}

void GFSDK_WaveWorks_Simulation::releaseRenderingResources(int cascade)
{
	SAFE_DELETE(cascade_states[cascade].m_pQuadMesh);

#if WAVEWORKS_ENABLE_GRAPHICS
	for(int gpu_slot = 0; gpu_slot != m_num_GPU_slots; ++gpu_slot)
	{
		switch(m_d3dAPI)
		{
#if WAVEWORKS_ENABLE_D3D9
		case nv_water_d3d_api_d3d9:
			{
				SAFE_RELEASE(cascade_states[cascade].m_d3d._9.m_pd3d9GradientMap[gpu_slot]);
			}
			break;
#endif
#if WAVEWORKS_ENABLE_D3D10
		case nv_water_d3d_api_d3d10:
			{
				SAFE_RELEASE(cascade_states[cascade].m_d3d._10.m_pd3d10GradientMap[gpu_slot]);
				SAFE_RELEASE(cascade_states[cascade].m_d3d._10.m_pd3d10GradientRenderTarget[gpu_slot]);
			}
			break;
#endif
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			{
				SAFE_RELEASE(cascade_states[cascade].m_d3d._11.m_pd3d11GradientMap[gpu_slot]);
				SAFE_RELEASE(cascade_states[cascade].m_d3d._11.m_pd3d11GradientRenderTarget[gpu_slot]);
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			{
				NVSDK_garlic_free(cascade_states[cascade].m_d3d._gnm.m_gnmGradientMap[gpu_slot].getBaseAddress());
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				if(cascade_states[cascade].m_d3d._GL2.m_GL2GradientMap[gpu_slot]) NVSDK_GLFunctions.glDeleteTextures(1, &cascade_states[cascade].m_d3d._GL2.m_GL2GradientMap[gpu_slot]); CHECK_GL_ERRORS;
				if(cascade_states[cascade].m_d3d._GL2.m_GL2GradientFBO[gpu_slot]) NVSDK_GLFunctions.glDeleteFramebuffers(1, &cascade_states[cascade].m_d3d._GL2.m_GL2GradientFBO[gpu_slot]); CHECK_GL_ERRORS;
			}
			break;
#endif
		default:
			break;
		}
	}

    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        {
            SAFE_RELEASE(cascade_states[cascade].m_d3d._9.m_pd3d9FoamEnergyMap);
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
			SAFE_RELEASE(cascade_states[cascade].m_d3d._10.m_pd3d10FoamEnergyMap);
			SAFE_RELEASE(cascade_states[cascade].m_d3d._10.m_pd3d10FoamEnergyRenderTarget);
		}
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			SAFE_RELEASE(cascade_states[cascade].m_d3d._11.m_pd3d11FoamEnergyMap);
			SAFE_RELEASE(cascade_states[cascade].m_d3d._11.m_pd3d11FoamEnergyRenderTarget);
        }
        break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			NVSDK_garlic_free(cascade_states[cascade].m_d3d._gnm.m_gnmFoamEnergyMap.getBaseAddress());
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				if(cascade_states[cascade].m_d3d._GL2.m_GL2FoamEnergyMap) NVSDK_GLFunctions.glDeleteTextures(1, &cascade_states[cascade].m_d3d._GL2.m_GL2FoamEnergyMap); CHECK_GL_ERRORS;
				if(cascade_states[cascade].m_d3d._GL2.m_GL2FoamEnergyFBO) NVSDK_GLFunctions.glDeleteFramebuffers(1, &cascade_states[cascade].m_d3d._GL2.m_GL2FoamEnergyFBO); CHECK_GL_ERRORS;
			}
			break;
#endif
	default:
		break;
    }
#endif // WAVEWORKS_ENABLE_GRAPHICS
}

HRESULT GFSDK_WaveWorks_Simulation::initQuadMesh(int GFX_ONLY(cascade))
{
	if(nv_water_d3d_api_none == m_d3dAPI)
		return S_OK;						// No GFX, no timers

#if WAVEWORKS_ENABLE_GRAPHICS
    SAFE_DELETE(cascade_states[cascade].m_pQuadMesh);

    // Vertices
    float tex_adjust = 0.f;
    if(nv_water_d3d_api_d3d9 == m_d3dAPI)
    {
        // Half-texel offset required in D3D9
        tex_adjust = 0.5f / m_params.cascades[cascade].fft_resolution;
    }

    float vertices[] = {-1.0f,  1.0f, 0,	tex_adjust,      tex_adjust,     
                        -1.0f, -1.0f, 0,	tex_adjust,      tex_adjust+1.0f,
                         1.0f,  1.0f, 0,	tex_adjust+1.0f, tex_adjust,	 
                         1.0f, -1.0f, 0,	tex_adjust+1.0f, tex_adjust+1.0f};

#if WAVEWORKS_ENABLE_GL
	// GL has different viewport origin(0,0) compared to DX, so flipping texcoords
    float verticesGL[]= {-1.0f,  1.0f, 0,	tex_adjust,      tex_adjust+1.0f,     
                        -1.0f, -1.0f, 0,	tex_adjust,      tex_adjust,
                         1.0f,  1.0f, 0,	tex_adjust+1.0f, tex_adjust+1.0f,	 
                         1.0f, -1.0f, 0,	tex_adjust+1.0f, tex_adjust};
#endif // WAVEWORKS_ENABLE_GL

    const UINT VertexStride = 20;

    // Indices
    const DWORD indices[] = {0, 1, 2, 3};

    // Init mesh
    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D9
    case nv_water_d3d_api_d3d9:
        {
			HRESULT hr;

            const D3DVERTEXELEMENT9 quad_decl[] =
            {
                {0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
                {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
                D3DDECL_END()
            };

            V_RETURN(NVWaveWorks_Mesh::CreateD3D9(m_d3d._9.m_pd3d9Device, quad_decl, VertexStride, vertices, 4, indices, 4, &cascade_states[cascade].m_pQuadMesh));
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D10
    case nv_water_d3d_api_d3d10:
        {
			HRESULT hr;

            const D3D10_INPUT_ELEMENT_DESC quad_layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
            };
            const UINT num_layout_elements = sizeof(quad_layout)/sizeof(quad_layout[0]);

            V_RETURN(NVWaveWorks_Mesh::CreateD3D10(	m_d3d._10.m_pd3d10Device,
													quad_layout, num_layout_elements,
													SM4::CalcGradient::g_vs, sizeof(SM4::CalcGradient::g_vs),
													VertexStride, vertices, 4, indices, 4,
													&cascade_states[cascade].m_pQuadMesh
													));
        }
        break;
#endif
#if WAVEWORKS_ENABLE_D3D11
    case nv_water_d3d_api_d3d11:
        {
			HRESULT hr;

            const D3D11_INPUT_ELEMENT_DESC quad_layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            const UINT num_layout_elements = sizeof(quad_layout)/sizeof(quad_layout[0]);

            V_RETURN(NVWaveWorks_Mesh::CreateD3D11(	m_d3d._11.m_pd3d11Device,
													quad_layout, num_layout_elements,
													SM4::CalcGradient::g_vs, sizeof(SM4::CalcGradient::g_vs),
													VertexStride, vertices, 4, indices, 4,
													&cascade_states[cascade].m_pQuadMesh
													));
        }
        break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			NVWaveWorks_Mesh::CreateGnm(VertexStride, vertices, 4, indices, 4,
				&cascade_states[cascade].m_pQuadMesh
				);
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GL
    case nv_water_d3d_api_gl2:
		{
			HRESULT hr;

			const NVWaveWorks_Mesh::GL_VERTEX_ATTRIBUTE_DESC attribute_descs[] = 
				{
					{3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), 0},							// Pos
					{2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), 3*sizeof(GLfloat)},			// TexCoord
				};

			V_RETURN(NVWaveWorks_Mesh::CreateGL2(	attribute_descs,
													sizeof(attribute_descs)/sizeof(attribute_descs[0]),
													VertexStride, verticesGL, 4,
													indices, 4,
													&cascade_states[cascade].m_pQuadMesh
													));
		}
        break;
#endif
	case nv_water_d3d_api_none:
		break;
	default:
		// Unexpected API
		return E_FAIL;
    }
#endif // WAVEWORKS_ENABLE_GRAPHICS

	return S_OK;
}


void GFSDK_WaveWorks_Simulation::updateRMS(const GFSDK_WaveWorks_Detailed_Simulation_Params& params)
{
	m_total_rms = 0.f;
	for(int i=0; i<GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades;i++)
	{
		m_total_rms += GFSDK_WaveWorks_Simulation_Util::get_spectrum_rms_sqr(params.cascades[i]);
	}
	m_total_rms = sqrtf(m_total_rms);
}

float GFSDK_WaveWorks_Simulation::getConservativeMaxDisplacementEstimate()
{
	// Based on significant wave height: http://en.wikipedia.org/wiki/Significant_wave_height
	//
	// Significant wave height is said to be 1.4x rms and represents a 1 in 3 event
	// Then, given that wave heights follow a Rayleigh distribution, and based on the form of the CDF,
	// we observe that a wave height of 4x significant should be *very* infrequent (1 in 3^16, approx)
	//
	// Hence, we use 4 x 1.4 x rms, or 6x with rounding up!
	//
	return 6.f * m_total_rms;
}
HRESULT GFSDK_WaveWorks_Simulation::getStats(GFSDK_WaveWorks_Simulation_Stats& stats)
{
	GFSDK_WaveWorks_Simulation_Manager_Timings timings;
	
	// getting the simulation implementation dependent timings
	m_pSimulationManager->getTimings(timings);

	// putting these to stats
	stats.CPU_main_thread_wait_time = timings.time_wait_for_completion;
	stats.CPU_threads_start_to_finish_time = timings.time_start_to_stop;
	stats.CPU_threads_total_time = timings.time_total;

	// collect GPU times individually from cascade members
	stats.GPU_simulation_time = 0.f;
	stats.GPU_FFT_simulation_time = 0.f;
    for(int cascade = 0; cascade != m_params.num_cascades; ++cascade)
	{
		NVWaveWorks_FFT_Simulation_Timings cascade_member_timing;
		cascade_states[cascade].m_pFFTSimulation->getTimings(cascade_member_timing);
		stats.GPU_simulation_time += cascade_member_timing.GPU_simulation_time;
		stats.GPU_FFT_simulation_time += cascade_member_timing.GPU_FFT_simulation_time;
	}

	// we collect GFX GPU time ourself during gradient map calcs
	stats.GPU_gfx_time = m_gpu_kick_timers.m_timer_slots[m_gpu_kick_timers.m_active_timer_slot].m_elapsed_gfx_time + m_gpu_wait_timers.m_timer_slots[m_gpu_wait_timers.m_active_timer_slot].m_elapsed_gfx_time;
	stats.GPU_update_time = m_gpu_kick_timers.m_timer_slots[m_gpu_kick_timers.m_active_timer_slot].m_elapsed_time + m_gpu_wait_timers.m_timer_slots[m_gpu_wait_timers.m_active_timer_slot].m_elapsed_time;

	return S_OK;
}

HRESULT GFSDK_WaveWorks_Simulation::consumeAvailableTimerSlot(Graphics_Context* pGC, NVWaveWorks_GFX_Timer_Impl* pGFXTimer, TimerPool& pool, TimerSlot** ppSlot)
{
    if(pool.m_active_timer_slot == pool.m_end_inflight_timer_slots)
    {
        // No slots available - we must wait for the oldest in-flight timer to complete
        int wait_slot = (pool.m_active_timer_slot + 1) % TimerPool::NumTimerSlots;
		TimerSlot* pWaitSlot = pool.m_timer_slots + wait_slot;

		if(NVWaveWorks_GFX_Timer_Impl::InvalidQueryIndex != pWaitSlot->m_DisjointQueryIndex)
		{
			UINT64 t_gfx;
			pGFXTimer->waitTimerQueries(pGC, pWaitSlot->m_StartGFXQueryIndex, pWaitSlot->m_StopGFXQueryIndex, t_gfx);

			UINT64 t_update;
			pGFXTimer->waitTimerQueries(pGC, pWaitSlot->m_StartQueryIndex, pWaitSlot->m_StopQueryIndex, t_update);

			UINT64 f;
			pGFXTimer->waitDisjointQuery(pGC, pWaitSlot->m_DisjointQueryIndex, f);

			if(f > 0)
			{
				pWaitSlot->m_elapsed_gfx_time = 1000.f * FLOAT(t_gfx)/FLOAT(f);
				pWaitSlot->m_elapsed_time = 1000.f * FLOAT(t_update)/FLOAT(f);
			}

			pGFXTimer->releaseDisjointQuery(pWaitSlot->m_DisjointQueryIndex);
			pGFXTimer->releaseTimerQuery(pWaitSlot->m_StartGFXQueryIndex);
			pGFXTimer->releaseTimerQuery(pWaitSlot->m_StopGFXQueryIndex);
			pGFXTimer->releaseTimerQuery(pWaitSlot->m_StartQueryIndex);
			pGFXTimer->releaseTimerQuery(pWaitSlot->m_StopQueryIndex);
		}

        pool.m_active_timer_slot = wait_slot;
    }

    // Consume a slot!
    *ppSlot = &pool.m_timer_slots[pool.m_end_inflight_timer_slots];
	(*ppSlot)->m_elapsed_gfx_time = 0.f;
	(*ppSlot)->m_elapsed_time = 0.f;
	(*ppSlot)->m_DisjointQueryIndex = pGFXTimer->getCurrentDisjointQuery();
    pool.m_end_inflight_timer_slots = (pool.m_end_inflight_timer_slots + 1) % TimerPool::NumTimerSlots;

    return S_OK;
}

HRESULT GFSDK_WaveWorks_Simulation::queryAllGfxTimers(Graphics_Context* pGC, NVWaveWorks_GFX_Timer_Impl* pGFXTimer)
{
	HRESULT hr;

	V_RETURN(queryTimers(pGC, pGFXTimer, m_gpu_kick_timers));
	V_RETURN(queryTimers(pGC, pGFXTimer, m_gpu_wait_timers));

	return S_OK;
}

HRESULT GFSDK_WaveWorks_Simulation::queryTimers(Graphics_Context* pGC, NVWaveWorks_GFX_Timer_Impl* pGFXTimer, TimerPool& pool)
{
	HRESULT hr;

    const int wait_slot = (pool.m_active_timer_slot + 1) % TimerPool::NumTimerSlots;

    // Just consume one timer result per check
    if(wait_slot != pool.m_end_inflight_timer_slots)
    {
		TimerSlot* pWaitSlot = pool.m_timer_slots + wait_slot;
		if(NVWaveWorks_GFX_Timer_Impl::InvalidQueryIndex != pWaitSlot->m_DisjointQueryIndex)
		{
			UINT64 t_gfx;
			hr = pGFXTimer->getTimerQueries(pGC, pWaitSlot->m_StartGFXQueryIndex, pWaitSlot->m_StopGFXQueryIndex, t_gfx);
			if(hr == S_FALSE)
				return S_OK;

			UINT64 t_update;
			hr = pGFXTimer->getTimerQueries(pGC, pWaitSlot->m_StartQueryIndex, pWaitSlot->m_StopQueryIndex, t_update);
			if(hr == S_FALSE)
				return S_OK;

			UINT64 f;
			hr = pGFXTimer->getDisjointQuery(pGC, pWaitSlot->m_DisjointQueryIndex, f);
			if(hr == S_FALSE)
				return S_OK;

			if(f > 0)
			{
				pWaitSlot->m_elapsed_gfx_time = 1000.f * FLOAT(t_gfx)/FLOAT(f);
				pWaitSlot->m_elapsed_time = 1000.f * FLOAT(t_update)/FLOAT(f);
			}

			pGFXTimer->releaseDisjointQuery(pWaitSlot->m_DisjointQueryIndex);
			pGFXTimer->releaseTimerQuery(pWaitSlot->m_StartGFXQueryIndex);
			pGFXTimer->releaseTimerQuery(pWaitSlot->m_StopGFXQueryIndex);
			pGFXTimer->releaseTimerQuery(pWaitSlot->m_StartQueryIndex);
			pGFXTimer->releaseTimerQuery(pWaitSlot->m_StopQueryIndex);
		}

		pool.m_active_timer_slot = wait_slot;
    }

    return S_OK;
}


void GFSDK_WaveWorks_Simulation::consumeGPUSlot()
{
	m_active_GPU_slot = (m_active_GPU_slot+1)%m_num_GPU_slots;
}


GLuint GFSDK_WaveWorks_Simulation::compileGLShader(const char* GL_ONLY(text), GLenum GL_ONLY(type)) //returns shader handle or 0 if error
{
#if WAVEWORKS_ENABLE_GL
	GLint compiled;
	GLuint shader;

	shader = NVSDK_GLFunctions.glCreateShader(type); CHECK_GL_ERRORS;
	NVSDK_GLFunctions.glShaderSource(shader, 1, (const GLchar **)&text, NULL); CHECK_GL_ERRORS;
	NVSDK_GLFunctions.glCompileShader(shader); CHECK_GL_ERRORS;
	NVSDK_GLFunctions.glGetShaderiv(shader, GL_COMPILE_STATUS,  &compiled); CHECK_GL_ERRORS;
	if (!compiled) {
		GLsizei logSize;
		NVSDK_GLFunctions.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize); CHECK_GL_ERRORS;
		char* pLog = new char[logSize];
		diagnostic_message(TEXT("\nGL shader [%i] compilation error"),type);
		diagnostic_message(TEXT("\n...\n") ASCII_STR_FMT TEXT("\n...\n"),text);
		NVSDK_GLFunctions.glGetShaderInfoLog(shader, logSize, NULL, pLog); CHECK_GL_ERRORS;
		diagnostic_message(TEXT("\ninfolog: ") ASCII_STR_FMT, pLog);
		NVSDK_GLFunctions.glDeleteShader(shader); CHECK_GL_ERRORS;
		return 0;
	}
	return shader;
#else
	return 0;
#endif
}

//returns program object handle or 0 if error
GLuint GFSDK_WaveWorks_Simulation::loadGLProgram(const char* GL_ONLY(vstext), const char* GL_ONLY(tetext), const char* GL_ONLY(tctext),  const char* GL_ONLY(gstext), const char* GL_ONLY(fstext))
{
#if WAVEWORKS_ENABLE_GL

	GLuint result = 0;
	GLenum program;
	GLint compiled;

	program = NVSDK_GLFunctions.glCreateProgram(); CHECK_GL_ERRORS;

	// vs
	if(vstext)  {
		result = compileGLShader(vstext,GL_VERTEX_SHADER);
		if(result)
		{
			NVSDK_GLFunctions.glAttachShader(program,result); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glDeleteShader(result); CHECK_GL_ERRORS;
		}
	}

	// tc
	if(tctext)  {
		result = compileGLShader(tctext,GL_TESS_CONTROL_SHADER);
		if(result)
		{
			NVSDK_GLFunctions.glAttachShader(program,result); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glDeleteShader(result); CHECK_GL_ERRORS;
		}
	}

	// te
	if(tetext)  {
		result = compileGLShader(tetext,GL_TESS_EVALUATION_SHADER);
		if(result)
		{
			NVSDK_GLFunctions.glAttachShader(program,result); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glDeleteShader(result); CHECK_GL_ERRORS;
		}
	}

	// gs
	if(gstext)  {
		result = compileGLShader(gstext,GL_GEOMETRY_SHADER);
		if(result)
		{
			NVSDK_GLFunctions.glAttachShader(program,result); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glDeleteShader(result); CHECK_GL_ERRORS;
		}
	}

	// ps
	if(fstext)  {
		result = compileGLShader(fstext,GL_FRAGMENT_SHADER);
		if(result)
		{
			NVSDK_GLFunctions.glAttachShader(program,result); CHECK_GL_ERRORS;
			NVSDK_GLFunctions.glDeleteShader(result); CHECK_GL_ERRORS;
		}
	}

	NVSDK_GLFunctions.glLinkProgram(program); CHECK_GL_ERRORS;

	NVSDK_GLFunctions.glGetProgramiv(program, GL_LINK_STATUS, &compiled); CHECK_GL_ERRORS;
	if (!compiled) {
		GLsizei logSize;
		NVSDK_GLFunctions.glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize); CHECK_GL_ERRORS;
		char* pLog = new char[logSize];
		diagnostic_message(TEXT("gl program link error\n"));
		NVSDK_GLFunctions.glGetProgramInfoLog(program, logSize, NULL, pLog); CHECK_GL_ERRORS;
		diagnostic_message(TEXT("\ninfolog: ") ASCII_STR_FMT TEXT("\n"),pLog);
		return 0;
	}
	return program;
#else
	return 0;
#endif
}
