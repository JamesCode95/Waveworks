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
 
#include "DXUT.h"
#include "ocean_surface_heights.h"
#include "GFSDK_WaveWorks.h"
#include "GFSDK_WaveWorks_D3D_Util.h"
#include <math.h>

extern HRESULT LoadFile(LPCTSTR FileName, ID3DXBuffer** ppBuffer);

namespace {

	gfsdk_float2 do_rot(const gfsdk_float2& coord, const gfsdk_float2& rot) {
		gfsdk_float2 result;
		result.x =  coord.x * rot.x + coord.y * rot.y;
		result.y = -coord.x * rot.y + coord.y * rot.x;
		return result;
	}

	gfsdk_float2 do_scale_and_rot(const gfsdk_float2& coord, const gfsdk_float2& scale, const gfsdk_float2& rot) {

		gfsdk_float2 scaled;
		scaled.x = coord.x * scale.x;
		scaled.y = coord.y * scale.y;

		return do_rot(scaled,rot);
	}

	gfsdk_float2 do_inv_scale_and_rot(const gfsdk_float2& coord, const gfsdk_float2& scale, const gfsdk_float2& rot) {

		gfsdk_float2 inv_rot;
		inv_rot.x =  rot.x;
		inv_rot.y = -rot.y;

		gfsdk_float2 rotated = do_rot(coord,inv_rot);

		gfsdk_float2 result;
		result.x = rotated.x * 1.f/scale.x;
		result.y = rotated.y * 1.f/scale.y;

		return result;
	}

	const int kGPUSampleDensityMultiplier = 4;

	const float kMaxError = 0.01f;

	const int kMaxConvergenceIterations = 20;				// Empirically determined
	const float kConvergenceMultiplier = 0.975f;			// Empirically determined
	const float kProgressiveConvergenceMultiplier = 0.99f;	// Empirically determined

	UINT GetShaderInputRegisterMapping(	ID3D11ShaderReflection* pReflectionVS,
										ID3D11ShaderReflection* pReflectionHS,
										ID3D11ShaderReflection* pReflectionDS,
										ID3D11ShaderReflection* pReflectionPS,
										const GFSDK_WaveWorks_ShaderInput_Desc& inputDesc
										)
	{
		ID3D11ShaderReflection* pReflection = NULL;
		switch(inputDesc.Type)
		{
		case GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_FloatConstant:
		case GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Sampler:
		case GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_Texture:
		case GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_ConstantBuffer:
			pReflection = pReflectionVS;
			break;
		case GFSDK_WaveWorks_ShaderInput_Desc::HullShader_FloatConstant:
		case GFSDK_WaveWorks_ShaderInput_Desc::HullShader_Sampler:
		case GFSDK_WaveWorks_ShaderInput_Desc::HullShader_Texture:
		case GFSDK_WaveWorks_ShaderInput_Desc::HullShader_ConstantBuffer:
			pReflection = pReflectionHS;
			break;
		case GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_FloatConstant:
		case GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Sampler:
		case GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_Texture:
		case GFSDK_WaveWorks_ShaderInput_Desc::DomainShader_ConstantBuffer:
			pReflection = pReflectionDS;
			break;
		case GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_FloatConstant:
		case GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Sampler:
		case GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_Texture:
		case GFSDK_WaveWorks_ShaderInput_Desc::PixelShader_ConstantBuffer:
			pReflection = pReflectionPS;
			break;
		default:
			pReflection = NULL;
			break;
		}

		assert(pReflection);
		D3D11_SHADER_INPUT_BIND_DESC desc;
		HRESULT hr = pReflection->GetResourceBindingDescByName(inputDesc.Name, &desc);
		if(FAILED(hr))
		{
			// Shader doesn't use this input, mark it as unused
			return 0xFFFFFFFF;
		}

		return desc.BindPoint;
	}

	ID3D11ShaderReflection* GetReflection(const D3DX11_PASS_SHADER_DESC& passShader)
	{
		D3DX11_EFFECT_SHADER_DESC shaderDesc;
		passShader.pShaderVariable->GetShaderDesc(passShader.ShaderIndex, &shaderDesc);
		ID3D11ShaderReflection* pResult = NULL;
		D3DReflect(shaderDesc.pBytecode, shaderDesc.BytecodeLength, IID_ID3D11ShaderReflection, (void**)&pResult);

		return pResult;
	}
}

OceanSurfaceHeights::OceanSurfaceHeights(int max_num_samples, const gfsdk_float2& UVToWorldScale) :
	m_UVToWorldScale(UVToWorldScale)
{
	// Figure out how to allocate samples to area
	int num_intervals_l = 1;
	int num_intervals_w = 1;
	int num_intervals_stride = 1;
	if(m_UVToWorldScale.x > m_UVToWorldScale.y) {
		num_intervals_w = (int)floorf(0.5f + m_UVToWorldScale.x/m_UVToWorldScale.y);
	} else {
		num_intervals_l = (int)floorf(0.5f + m_UVToWorldScale.y/m_UVToWorldScale.x);
	}

	while(((2*num_intervals_l+1)*(2*num_intervals_w+1)) <= max_num_samples) {
		num_intervals_l *= 2;
		num_intervals_w *= 2;
		num_intervals_stride *= 2;
	}

	m_num_samples_l = num_intervals_l + 1;
	m_num_samples_w = num_intervals_w + 1;
	m_max_num_intervals_stride = num_intervals_stride;

	m_num_gpu_samples_l = m_num_samples_l*kGPUSampleDensityMultiplier;
	m_num_gpu_samples_w = m_num_samples_w*kGPUSampleDensityMultiplier;

	int num_samples = m_num_samples_l * m_num_samples_w;

	m_pTargetSamplePositions = new gfsdk_float2[num_samples];
	m_pAdjustedSamplePositions = new gfsdk_float2[num_samples];
	m_pSampleDisplacements = new gfsdk_float4[num_samples];
	m_pSampleGradients = new gfsdk_float2[num_samples];

	m_pScratchSampleDisplacements = new gfsdk_float4[num_samples];
	m_pScratchTargetSampleInputs = new gfsdk_float2[num_samples];
	m_pScratchAdjustedSampleInputs = new gfsdk_float2[num_samples];

	m_pFX = NULL;
	m_pRenderSurfaceToReverseLookupTechnique = NULL;
    m_pRenderQuadToUITechnique = NULL;
	m_pRenderMarkerTechnique = NULL;

	m_pNumQuadsWVariable = NULL;
	m_pNumQuadsHVariable = NULL;
	m_pQuadUVDimsVariable = NULL;
	m_pSrcUVToWorldScaleVariable = NULL;
	m_pSrcUVToWorldRotationVariable = NULL;
	m_pSrcUVToWorldOffsetVariable = NULL;
	m_pWorldToClipScaleVariable = NULL;
	m_pClipToWorldRotationVariable = NULL;
	m_pClipToWorldOffsetVariable = NULL;
	m_pTexDiffuseVariable = NULL;
	m_pTexLookupVariable = NULL;

	m_pMatViewProjVariable = NULL;
	m_pMatWorldVariable = NULL;
	m_pWorldToUVScaleVariable = NULL;
	m_pWorldToUVRotationVariable = NULL;
	m_pWorldToUVOffsetVariable = NULL;

	m_pWaterSimulationShaderInputMappings = NULL;

	m_pd3dDevice = DXUTGetD3D11Device();
	m_pLookupSRV = 0;
	m_pLookupRTV = 0;
}

OceanSurfaceHeights::~OceanSurfaceHeights()
{
	SAFE_RELEASE(m_pLookupSRV);
	SAFE_RELEASE(m_pLookupRTV);

	SAFE_RELEASE(m_pFX);

	SAFE_DELETE(m_pTargetSamplePositions);
	SAFE_DELETE(m_pAdjustedSamplePositions);
	SAFE_DELETE(m_pSampleDisplacements);
	SAFE_DELETE(m_pSampleGradients);

	SAFE_DELETE(m_pScratchSampleDisplacements);
	SAFE_DELETE(m_pScratchTargetSampleInputs);
	SAFE_DELETE(m_pScratchAdjustedSampleInputs);

	SAFE_DELETE_ARRAY(m_pWaterSimulationShaderInputMappings);
}

void OceanSurfaceHeights::updateHeights(ID3D11DeviceContext* pDC, GFSDK_WaveWorks_SimulationHandle hSim, const gfsdk_float2& UVToWorldRotation, const gfsdk_float2& worldCentroid)
{
	m_UVToWorldRotation = UVToWorldRotation;

	// To get the offset, add rotated and scaled (-0.5,-0.5) to worldCentroid
	gfsdk_float2 minus_half = {-0.5f,-0.5f};
	minus_half = do_scale_and_rot(minus_half,m_UVToWorldScale,m_UVToWorldRotation);
	m_UVToWorldOffset = worldCentroid;
	m_UVToWorldOffset.x += minus_half.x;
	m_UVToWorldOffset.y += minus_half.y;

	const int num_intervals_w = m_num_samples_w-1;
	const int num_intervals_l = m_num_samples_l-1;

	// Initialise sample inputs - note that we slop one sample over in each direction in order
	// to calculate gradients
	for(int vi = 0; vi != m_num_samples_l; ++vi) {
		float v = float(vi-1)/float(num_intervals_l-2);
		for(int ui = 0; ui != m_num_samples_w; ++ui) {
			float u = float(ui-1)/float(num_intervals_w-2);
			gfsdk_float2 coord;
			coord.x = u;
			coord.y = v;
			coord = do_scale_and_rot(coord,m_UVToWorldScale,m_UVToWorldRotation);
			coord.x += m_UVToWorldOffset.x;
			coord.y += m_UVToWorldOffset.y;
			m_pTargetSamplePositions[ui + vi * m_num_samples_w] = coord;
		}
	}

	// Initialise coarse samples
	int num_samples = 0;
	int num_intervals_stride = m_max_num_intervals_stride;
	for(int vi = 0; vi < m_num_samples_l; vi += num_intervals_stride) {
		for(int ui = 0; ui < m_num_samples_w; ui += num_intervals_stride) {
			gfsdk_float2 coord = m_pTargetSamplePositions[ui + vi * m_num_samples_w];
			m_pScratchTargetSampleInputs[num_samples] = coord;
			m_pScratchAdjustedSampleInputs[num_samples] = coord;
			++num_samples;
		}
	}

	// Do the coarse converge
	float initial_conv_amt = 1.f;
	int num_sample_queries = converge_scratch(hSim,num_samples,initial_conv_amt);

	// Copy out into the results area
	num_samples = 0;
	for(int vi = 0; vi < m_num_samples_l; vi += num_intervals_stride) {
		for(int ui = 0; ui < m_num_samples_w; ui += num_intervals_stride) {
			const int results_ix = ui + vi * m_num_samples_w;
			m_pAdjustedSamplePositions[results_ix] = m_pScratchAdjustedSampleInputs[num_samples];
			m_pSampleDisplacements[results_ix] = m_pScratchSampleDisplacements[num_samples];
			++num_samples;
		}
	}

	// Progressive refinement
	while(num_intervals_stride > 1) {

		const int half_stride = num_intervals_stride >> 1;

		// Initialise samples
		num_samples = 0;
		for(int vi = 0; vi != num_intervals_l; vi += num_intervals_stride) {
			for(int ui = 0; ui != num_intervals_w; ui += num_intervals_stride) {

				gfsdk_float2 adj_corner_0 = m_pAdjustedSamplePositions[(ui+0)                    + (vi+0)                    * m_num_samples_w];
				gfsdk_float2 adj_corner_1 = m_pAdjustedSamplePositions[(ui+num_intervals_stride) + (vi+0)                    * m_num_samples_w];
				gfsdk_float2 adj_corner_2 = m_pAdjustedSamplePositions[(ui+0)                    + (vi+num_intervals_stride) * m_num_samples_w];
				gfsdk_float2 adj_corner_3 = m_pAdjustedSamplePositions[(ui+num_intervals_stride) + (vi+num_intervals_stride) * m_num_samples_w];

				gfsdk_float2 e01, e23, e02, e13, c;
				e01.x = 0.5f * adj_corner_0.x + 0.5f * adj_corner_1.x;
				e01.y = 0.5f * adj_corner_0.y + 0.5f * adj_corner_1.y;
				e23.x = 0.5f * adj_corner_2.x + 0.5f * adj_corner_3.x;
				e23.y = 0.5f * adj_corner_2.y + 0.5f * adj_corner_3.y;
				e02.x = 0.5f * adj_corner_0.x + 0.5f * adj_corner_2.x;
				e02.y = 0.5f * adj_corner_0.y + 0.5f * adj_corner_2.y;
				e13.x = 0.5f * adj_corner_1.x + 0.5f * adj_corner_3.x;
				e13.y = 0.5f * adj_corner_1.y + 0.5f * adj_corner_3.y;
				c.x = 0.5f * e01.x + 0.5f * e23.x;
				c.y = 0.5f * e01.y + 0.5f * e23.y;

				if(0 == vi) {
					m_pScratchTargetSampleInputs[num_samples] = m_pTargetSamplePositions[(ui+half_stride) + (vi+0) * m_num_samples_w];
					m_pScratchAdjustedSampleInputs[num_samples] = e01;
					++num_samples;
				}

				if(0 == ui) {
					m_pScratchTargetSampleInputs[num_samples] = m_pTargetSamplePositions[(ui+0) + (vi+half_stride) * m_num_samples_w];
					m_pScratchAdjustedSampleInputs[num_samples] = e02;
					++num_samples;
				}

				m_pScratchTargetSampleInputs[num_samples] = m_pTargetSamplePositions[(ui+num_intervals_stride) + (vi+half_stride) * m_num_samples_w];
				m_pScratchAdjustedSampleInputs[num_samples] = e13;
				++num_samples;

				m_pScratchTargetSampleInputs[num_samples] = m_pTargetSamplePositions[(ui+half_stride) + (vi+num_intervals_stride) * m_num_samples_w];
				m_pScratchAdjustedSampleInputs[num_samples] = e23;
				++num_samples;

				m_pScratchTargetSampleInputs[num_samples] = m_pTargetSamplePositions[(ui+half_stride) + (vi+half_stride) * m_num_samples_w];
				m_pScratchAdjustedSampleInputs[num_samples] = c;
				++num_samples;
			}
		}

		// Converge
		num_sample_queries += converge_scratch(hSim,num_samples,initial_conv_amt);

		// Copy out results
		num_samples = 0;
		for(int vi = 0; vi != num_intervals_l; vi += num_intervals_stride) {
			for(int ui = 0; ui != num_intervals_w; ui += num_intervals_stride) {

				int results_ix;

				if(0 == vi) {
					results_ix = (ui+half_stride) + (vi+0) * m_num_samples_w;
					m_pAdjustedSamplePositions[results_ix] = m_pScratchAdjustedSampleInputs[num_samples];
					m_pSampleDisplacements[results_ix] = m_pScratchSampleDisplacements[num_samples];
					++num_samples;
				}

				if(0 == ui) {
					results_ix = (ui+0) + (vi+half_stride) * m_num_samples_w;
					m_pAdjustedSamplePositions[results_ix] = m_pScratchAdjustedSampleInputs[num_samples];
					m_pSampleDisplacements[results_ix] = m_pScratchSampleDisplacements[num_samples];
					++num_samples;
				}

				results_ix = (ui+num_intervals_stride) + (vi+half_stride) * m_num_samples_w;
				m_pAdjustedSamplePositions[results_ix] = m_pScratchAdjustedSampleInputs[num_samples];
				m_pSampleDisplacements[results_ix] = m_pScratchSampleDisplacements[num_samples];
				++num_samples;

				results_ix = (ui+half_stride) + (vi+num_intervals_stride) * m_num_samples_w;
				m_pAdjustedSamplePositions[results_ix] = m_pScratchAdjustedSampleInputs[num_samples];
				m_pSampleDisplacements[results_ix] = m_pScratchSampleDisplacements[num_samples];
				++num_samples;

				results_ix = (ui+half_stride) + (vi+half_stride) * m_num_samples_w;
				m_pAdjustedSamplePositions[results_ix] = m_pScratchAdjustedSampleInputs[num_samples];
				m_pSampleDisplacements[results_ix] = m_pScratchSampleDisplacements[num_samples];
				++num_samples;
			}
		}

		num_intervals_stride = half_stride;
		initial_conv_amt *= kProgressiveConvergenceMultiplier;
	}

	/*
	static int max_num_sample_queries = 0;
	if(num_sample_queries > max_num_sample_queries) {
		max_num_sample_queries = num_sample_queries;
		char buff[256];
		sprintf(buff, "Max queries: %d\n", max_num_sample_queries);
		OutputDebugStringA(buff);
	}
	*/

	updateGradients();
}

int OceanSurfaceHeights::converge_scratch(GFSDK_WaveWorks_SimulationHandle hSim, int num_samples, float initial_conv_amt)
{
	int num_sample_queries = 0;

	// Converge the scratch samples
	float rms_error;
	float convergence_amount = initial_conv_amt;
	int num_iterations = 0;
	do {

		GFSDK_WaveWorks_Simulation_GetDisplacements(hSim, m_pScratchAdjustedSampleInputs, m_pScratchSampleDisplacements, num_samples);
		num_sample_queries += num_samples;

		rms_error = 0.f;
		for(int sample = 0; sample != num_samples; ++sample) {
			gfsdk_float2 coord;
			coord.x = m_pScratchAdjustedSampleInputs[sample].x + m_pScratchSampleDisplacements[sample].x;
			coord.y = m_pScratchAdjustedSampleInputs[sample].y + m_pScratchSampleDisplacements[sample].y;
			gfsdk_float2 error;
			error.x = coord.x - m_pScratchTargetSampleInputs[sample].x;
			error.y = coord.y - m_pScratchTargetSampleInputs[sample].y;
			float sqr_error = error.x*error.x + error.y*error.y;
			rms_error += sqr_error;
		}

		rms_error = sqrtf(rms_error/float(num_samples));

		if(rms_error > kMaxError) {
			for(int sample = 0; sample != num_samples; ++sample) {
				gfsdk_float2 coord;
				coord.x = m_pScratchAdjustedSampleInputs[sample].x + m_pScratchSampleDisplacements[sample].x;
				coord.y = m_pScratchAdjustedSampleInputs[sample].y + m_pScratchSampleDisplacements[sample].y;
				gfsdk_float2 error;
				error.x = coord.x - m_pScratchTargetSampleInputs[sample].x;
				error.y = coord.y - m_pScratchTargetSampleInputs[sample].y;
				m_pScratchAdjustedSampleInputs[sample].x -= convergence_amount * error.x;
				m_pScratchAdjustedSampleInputs[sample].y -= convergence_amount * error.y;
			}
		}

		convergence_amount *= kConvergenceMultiplier;	// Converge a bit less each time round
		++num_iterations;

	} while (rms_error > kMaxError && num_iterations != kMaxConvergenceIterations);

	// assert(num_iterations != kMaxConvergenceIterations);	// Useful to know if we ran out of road!

	/*
	static int max_num_iterations = 0;
	if(num_iterations > max_num_iterations) {
		max_num_iterations = num_iterations;
		char buff[256];
		sprintf(buff, "Max iterations: %d\n", max_num_iterations);
		OutputDebugStringA(buff);
	}
	*/

	return num_sample_queries;
}

void OceanSurfaceHeights::getDisplacements(const gfsdk_float2* inSamplePoints, gfsdk_float4* outDisplacements, gfsdk_U32 numSamples) const
{
	const float num_intervals_w = float(m_num_samples_w-3);	// Not including the 'slop' samples
	const float num_intervals_l = float(m_num_samples_l-3); // Not including the 'slop' samples
	for(gfsdk_U32 sample = 0; sample != numSamples; ++sample) {

		// Transform the sample point to UV
		gfsdk_float2 coord;
		coord.x = inSamplePoints[sample].x - m_UVToWorldOffset.x;
		coord.y = inSamplePoints[sample].y - m_UVToWorldOffset.y;
		coord = do_inv_scale_and_rot(coord,m_UVToWorldScale,m_UVToWorldRotation);

		// Transform UV to non-slop samples
		coord.x *= num_intervals_w;
		coord.y *= num_intervals_l;
		assert(coord.x >= 0.f);
		assert(coord.x <= num_intervals_w);
		assert(coord.y >= 0.f);
		assert(coord.y <= num_intervals_l);

		// Then allow for the slop
		coord.x += 1.f;
		coord.y += 1.f;

		float flower_x = floorf(coord.x);
		float flower_y = floorf(coord.y);
		int lower_x = int(flower_x);
		int lower_y = int(flower_y);

		gfsdk_float4* p00 = &m_pSampleDisplacements[(lower_x+0) + (lower_y+0)*m_num_samples_w];
		gfsdk_float4* p01 = &m_pSampleDisplacements[(lower_x+1) + (lower_y+0)*m_num_samples_w];
		gfsdk_float4* p10 = &m_pSampleDisplacements[(lower_x+0) + (lower_y+1)*m_num_samples_w];
		gfsdk_float4* p11 = &m_pSampleDisplacements[(lower_x+1) + (lower_y+1)*m_num_samples_w];

		float frac_x = coord.x - float(lower_x);
		float frac_y = coord.y - float(lower_y);

		outDisplacements[sample].x = (1.f-frac_x)*(1.f-frac_y)*p00->x + frac_x*(1.f-frac_y)*p01->x + (1.f-frac_x)*frac_y*p10->x + frac_x*frac_y*p11->x;
		outDisplacements[sample].y = (1.f-frac_x)*(1.f-frac_y)*p00->y + frac_x*(1.f-frac_y)*p01->y + (1.f-frac_x)*frac_y*p10->y + frac_x*frac_y*p11->y;
		outDisplacements[sample].z = (1.f-frac_x)*(1.f-frac_y)*p00->z + frac_x*(1.f-frac_y)*p01->z + (1.f-frac_x)*frac_y*p10->z + frac_x*frac_y*p11->z;
		outDisplacements[sample].w = (1.f-frac_x)*(1.f-frac_y)*p00->w + frac_x*(1.f-frac_y)*p01->w + (1.f-frac_x)*frac_y*p10->w + frac_x*frac_y*p11->w;
	}
}

void OceanSurfaceHeights::getGradients(const gfsdk_float2* inSamplePoints, gfsdk_float2* outGradients, gfsdk_U32 numSamples) const
{
	const float num_intervals_w = float(m_num_samples_w-3);	// Not including the 'slop' samples
	const float num_intervals_l = float(m_num_samples_l-3); // Not including the 'slop' samples
	for(gfsdk_U32 sample = 0; sample != numSamples; ++sample) {

		// Transform the sample point to UV
		gfsdk_float2 coord;
		coord.x = inSamplePoints[sample].x - m_UVToWorldOffset.x;
		coord.y = inSamplePoints[sample].y - m_UVToWorldOffset.y;
		coord = do_inv_scale_and_rot(coord,m_UVToWorldScale,m_UVToWorldRotation);

		// Transform UV to non-slop samples
		coord.x *= num_intervals_w;
		coord.y *= num_intervals_l;
		assert(coord.x >= 0.f);
		assert(coord.x <= num_intervals_w);
		assert(coord.y >= 0.f);
		assert(coord.y <= num_intervals_l);

		// Then allow for the slop
		coord.x += 1.f;
		coord.y += 1.f;

		float flower_x = floorf(coord.x);
		float flower_y = floorf(coord.y);
		int lower_x = int(flower_x);
		int lower_y = int(flower_y);

		gfsdk_float2* p00 = &m_pSampleGradients[(lower_x+0) + (lower_y+0)*m_num_samples_w];
		gfsdk_float2* p01 = &m_pSampleGradients[(lower_x+1) + (lower_y+0)*m_num_samples_w];
		gfsdk_float2* p10 = &m_pSampleGradients[(lower_x+0) + (lower_y+1)*m_num_samples_w];
		gfsdk_float2* p11 = &m_pSampleGradients[(lower_x+1) + (lower_y+1)*m_num_samples_w];

		float frac_x = coord.x - float(lower_x);
		float frac_y = coord.y - float(lower_y);

		outGradients[sample].x = (1.f-frac_x)*(1.f-frac_y)*p00->x + frac_x*(1.f-frac_y)*p01->x + (1.f-frac_x)*frac_y*p10->x + frac_x*frac_y*p11->x;
		outGradients[sample].y = (1.f-frac_x)*(1.f-frac_y)*p00->y + frac_x*(1.f-frac_y)*p01->y + (1.f-frac_x)*frac_y*p10->y + frac_x*frac_y*p11->y;
	}
}

void OceanSurfaceHeights::updateGradients()
{
	const float u_scale = 0.5f*float(m_num_samples_w-3)/m_UVToWorldScale.x;
	const float v_scale = 0.5f*float(m_num_samples_l-3)/m_UVToWorldScale.y;
	for(int vi = 1; vi != m_num_samples_l-1; ++vi) {
		for(int ui = 1; ui != m_num_samples_w-1; ++ui) {
			float u_neg = m_pSampleDisplacements[(ui-1) + (vi+0)*m_num_samples_w].z;
			float u_pos = m_pSampleDisplacements[(ui+1) + (vi+0)*m_num_samples_w].z;
			float v_neg = m_pSampleDisplacements[(ui+0) + (vi-1)*m_num_samples_w].z;
			float v_pos = m_pSampleDisplacements[(ui+0) + (vi+1)*m_num_samples_w].z;

			gfsdk_float2 grad;
			grad.x = u_scale*(u_pos-u_neg);
			grad.y = v_scale*(v_pos-v_neg);

			m_pSampleGradients[ui + vi*m_num_samples_w] = do_rot(grad,m_UVToWorldRotation);
		}
	} 
}

HRESULT OceanSurfaceHeights::init()
{
	HRESULT hr;

	SAFE_RELEASE(m_pFX);
    ID3DXBuffer* pEffectBuffer = NULL;
    V_RETURN(LoadFile(TEXT(".\\Media\\ocean_surface_heights_d3d11.fxo"), &pEffectBuffer));
    V_RETURN(D3DX11CreateEffectFromMemory(pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, m_pd3dDevice, &m_pFX));
    pEffectBuffer->Release();

	m_pRenderSurfaceToReverseLookupTechnique = m_pFX->GetTechniqueByName("RenderSurfaceToReverseLookupTech");
	m_pRenderQuadToUITechnique = m_pFX->GetTechniqueByName("RenderQuadToUITech");
	m_pRenderMarkerTechnique = m_pFX->GetTechniqueByName("RenderMarkerTech");

	m_pNumQuadsWVariable = m_pFX->GetVariableByName("g_numQuadsW")->AsScalar();
	m_pNumQuadsHVariable = m_pFX->GetVariableByName("g_numQuadsH")->AsScalar();
	m_pQuadUVDimsVariable = m_pFX->GetVariableByName("g_quadUVDims")->AsVector();
	m_pSrcUVToWorldScaleVariable = m_pFX->GetVariableByName("g_srcUVToWorldScale")->AsVector();
	m_pSrcUVToWorldRotationVariable = m_pFX->GetVariableByName("g_srcUVToWorldRot")->AsVector();
	m_pSrcUVToWorldOffsetVariable = m_pFX->GetVariableByName("g_srcUVToWorldOffset")->AsVector();
	m_pWorldToClipScaleVariable = m_pFX->GetVariableByName("g_worldToClipScale")->AsVector();
	m_pClipToWorldRotationVariable = m_pFX->GetVariableByName("g_clipToWorldRot")->AsVector();
	m_pClipToWorldOffsetVariable = m_pFX->GetVariableByName("g_clipToWorldOffset")->AsVector();
	m_pTexDiffuseVariable = m_pFX->GetVariableByName("g_texDiffuse")->AsShaderResource();
	m_pTexLookupVariable = m_pFX->GetVariableByName("g_texLookup")->AsShaderResource();

	m_pMatViewProjVariable = m_pFX->GetVariableByName("g_matViewProj")->AsMatrix();
	m_pMatWorldVariable = m_pFX->GetVariableByName("g_matWorld")->AsMatrix();
	m_pWorldToUVScaleVariable =m_pFX->GetVariableByName("g_worldToUVScale")->AsVector();
	m_pWorldToUVRotationVariable =m_pFX->GetVariableByName("g_worldToUVRot")->AsVector();
	m_pWorldToUVOffsetVariable =m_pFX->GetVariableByName("g_worldToUVOffset")->AsVector();

	UINT NumSimulationShaderInputs = GFSDK_WaveWorks_Simulation_GetShaderInputCountD3D11();
	m_pWaterSimulationShaderInputMappings = new UINT[NumSimulationShaderInputs];

	D3DX11_PASS_SHADER_DESC passShaderDesc;
	ID3DX11EffectPass* pRenderSurfaceToReverseLookupPass = m_pRenderSurfaceToReverseLookupTechnique->GetPassByIndex(0);

	V_RETURN(pRenderSurfaceToReverseLookupPass->GetVertexShaderDesc(&passShaderDesc));
	ID3D11ShaderReflection* pShadedReflectionVS = GetReflection(passShaderDesc);

	V_RETURN(pRenderSurfaceToReverseLookupPass->GetHullShaderDesc(&passShaderDesc));
	ID3D11ShaderReflection* pShadedReflectionHS = GetReflection(passShaderDesc);

	V_RETURN(pRenderSurfaceToReverseLookupPass->GetDomainShaderDesc(&passShaderDesc));
	ID3D11ShaderReflection* pShadedReflectionDS = GetReflection(passShaderDesc);

	V_RETURN(pRenderSurfaceToReverseLookupPass->GetPixelShaderDesc(&passShaderDesc));
	ID3D11ShaderReflection* pShadedReflectionPS = GetReflection(passShaderDesc);

	for(UINT i = 0; i != NumSimulationShaderInputs; ++i)
	{
		GFSDK_WaveWorks_ShaderInput_Desc inputDesc;
		GFSDK_WaveWorks_Simulation_GetShaderInputDescD3D11(i, &inputDesc);

		m_pWaterSimulationShaderInputMappings[i] = GetShaderInputRegisterMapping(pShadedReflectionVS, pShadedReflectionHS, pShadedReflectionDS, pShadedReflectionPS, inputDesc);
	}

	{
		SAFE_RELEASE(m_pLookupSRV);
		SAFE_RELEASE(m_pLookupRTV);
		ID3D11Texture2D* pTexture = NULL;

		// Set up textures for rendering hull profile
		D3D11_TEXTURE2D_DESC tex_desc;
		tex_desc.Width = (m_num_gpu_samples_w-2);
		tex_desc.Height = (m_num_gpu_samples_l-2);
		tex_desc.ArraySize = 1;
		tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		tex_desc.CPUAccessFlags = 0;
		tex_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		tex_desc.MipLevels = 1;
		tex_desc.MiscFlags = 0;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.SampleDesc.Quality = 0;
		tex_desc.Usage = D3D11_USAGE_DEFAULT;

		V_RETURN(m_pd3dDevice->CreateTexture2D(&tex_desc,NULL,&pTexture));
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(pTexture,NULL,&m_pLookupSRV));
		V_RETURN(m_pd3dDevice->CreateRenderTargetView(pTexture,NULL,&m_pLookupRTV));
		SAFE_RELEASE(pTexture);
	}

	return S_OK;
}

void OceanSurfaceHeights::renderTextureToUI(ID3D11DeviceContext* pDC) const
{
	m_pTexDiffuseVariable->SetResource(getGPULookupSRV());
	m_pRenderQuadToUITechnique->GetPassByIndex(0)->Apply(0, pDC);

	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pDC->IASetInputLayout(NULL);
	pDC->Draw(4,0);

	m_pTexDiffuseVariable->SetResource(NULL);
	m_pRenderQuadToUITechnique->GetPassByIndex(0)->Apply(0, pDC);
}

void OceanSurfaceHeights::getGPUWorldToUVTransform(gfsdk_float2& offset, gfsdk_float2& rot, gfsdk_float2& scale) const
{
	// To match CPU sampling, a sample at the min corner of the footprint (worldToUVOffset) should map to (0,0) whereas a sample
	// at the max corner should map to (m_num_gpu_samples_w-3,m_num_gpu_samples_l-3), hence...
	const float world_quad_size_w = m_UVToWorldScale.x/float(m_num_gpu_samples_w-3);
	const float world_quad_size_h = m_UVToWorldScale.y/float(m_num_gpu_samples_l-3);
	scale.x = 1.f/(m_UVToWorldScale.x+world_quad_size_w);
	scale.y = 1.f/(m_UVToWorldScale.y+world_quad_size_h);
	offset.x = -m_UVToWorldOffset.x;
	offset.y = -m_UVToWorldOffset.y;
	rot.x = m_UVToWorldRotation.x;
	rot.y = -m_UVToWorldRotation.y;
}

void OceanSurfaceHeights::renderMarkerArray(ID3D11DeviceContext* pDC, const D3DXMATRIX& matViewProj, const D3DXMATRIX& matWorld) const
{
	m_pTexLookupVariable->SetResource(getGPULookupSRV());
	m_pMatViewProjVariable->SetMatrix((float*)&matViewProj);
	m_pMatWorldVariable->SetMatrix((float*)&matWorld);

	gfsdk_float2 worldToUVScale;
	gfsdk_float2 worldToUVOffset;
	gfsdk_float2 worldToUVRot;
	getGPUWorldToUVTransform(worldToUVOffset, worldToUVRot, worldToUVScale);

	m_pWorldToUVScaleVariable->SetFloatVector((float*)&worldToUVScale);
	m_pWorldToUVRotationVariable->SetFloatVector((float*)&worldToUVRot);
	m_pWorldToUVOffsetVariable->SetFloatVector((float*)&worldToUVOffset);

	m_pRenderMarkerTechnique->GetPassByIndex(0)->Apply(0, pDC);

	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pDC->IASetInputLayout(NULL);
	pDC->Draw(300,0);

	m_pTexLookupVariable->SetResource(NULL);
	m_pRenderMarkerTechnique->GetPassByIndex(0)->Apply(0, pDC);
}

void OceanSurfaceHeights::updateGPUHeights(ID3D11DeviceContext* pDC, GFSDK_WaveWorks_SimulationHandle hSim, const D3DXMATRIX& matView)
{
	// Not strictly necessary, but good for debugging and SLI
	FLOAT clearColor[] = {1.f,0.f,1.f,1.f};
	pDC->ClearRenderTargetView(m_pLookupRTV, clearColor);

	// We inflate the rendered area by the conservative displacement amount to make sure we overlap the target
	// with enough slop to absorb displacements
	const float min_inflate_amount = GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(hSim);

	// Expand number of quads to match slop
	const float world_quad_size_w = m_UVToWorldScale.x/float(m_num_gpu_samples_w-3);
	const float world_quad_size_h = m_UVToWorldScale.y/float(m_num_gpu_samples_l-3);
	const int extra_quads_w = (int)ceilf(min_inflate_amount/world_quad_size_w);
	const int extra_quads_h = (int)ceilf(min_inflate_amount/world_quad_size_h);
	const float inflate_w = float(extra_quads_w)*world_quad_size_w;
	const float inflate_h = float(extra_quads_h)*world_quad_size_h;
	const int quads_w = (m_num_gpu_samples_w-2) + 2 * extra_quads_w;
	const int quads_h = (m_num_gpu_samples_l-2) + 2 * extra_quads_h;

	// Note that our CPU sampling scheme is somewhat different from conventional GPU sampling,
	// hence we have an extra quad's worth of world space in play to ensure coverage of all the
	// texels in the lookup
	gfsdk_float2 srcUVToWorld_additional_offset = {-inflate_w-0.5f*world_quad_size_w,-inflate_h-0.5f*world_quad_size_h};
	srcUVToWorld_additional_offset = do_rot(srcUVToWorld_additional_offset,m_UVToWorldRotation);

	gfsdk_float2 srcUVToWorldOffset = m_UVToWorldOffset;
	srcUVToWorldOffset.x += srcUVToWorld_additional_offset.x;
	srcUVToWorldOffset.y += srcUVToWorld_additional_offset.y;

	gfsdk_float2 srcUVToWorldScale = m_UVToWorldScale;
	srcUVToWorldScale.x += 2.f * inflate_w + world_quad_size_w;
	srcUVToWorldScale.y += 2.f * inflate_h + world_quad_size_h;

	m_pSrcUVToWorldScaleVariable->SetFloatVector((FLOAT*)&srcUVToWorldScale);
	m_pSrcUVToWorldRotationVariable->SetFloatVector((FLOAT*)&m_UVToWorldRotation);
	m_pSrcUVToWorldOffsetVariable->SetFloatVector((FLOAT*)&srcUVToWorldOffset);

	// World to clip are inverse of vanilla forwards, but with the additional half-quad offset to
	// match CPU sampling scheme
	gfsdk_float2 worldToClipScale;
	worldToClipScale.x = 1.f/(m_UVToWorldScale.x+world_quad_size_w);
	worldToClipScale.y = 1.f/(m_UVToWorldScale.y+world_quad_size_h);

	gfsdk_float2 clipToWorld_additional_offset = {-0.5f*world_quad_size_w,-0.5f*world_quad_size_h};
	clipToWorld_additional_offset = do_rot(clipToWorld_additional_offset,m_UVToWorldRotation);

	gfsdk_float2 clipToWorldOffset = m_UVToWorldOffset;
	clipToWorldOffset.x += clipToWorld_additional_offset.x;
	clipToWorldOffset.y += clipToWorld_additional_offset.y;

	m_pWorldToClipScaleVariable->SetFloatVector((FLOAT*)&worldToClipScale);
	m_pClipToWorldRotationVariable->SetFloatVector((FLOAT*)&m_UVToWorldRotation);
	m_pClipToWorldOffsetVariable->SetFloatVector((FLOAT*)&clipToWorldOffset);

	// Quads setup
	float quadsWH[] = {float(quads_w),float(quads_h)};
	m_pNumQuadsWVariable->SetFloat(quadsWH[0]);
	m_pNumQuadsHVariable->SetFloat(quadsWH[1]);
	float quadUVdims[] = {1.f/float(quads_w),1.f/float(quads_h)};
	m_pQuadUVDimsVariable->SetFloatVector(quadUVdims);

	// Preserve original viewports
	D3D11_VIEWPORT original_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	UINT num_original_viewports = sizeof(original_viewports)/sizeof(original_viewports[0]);
	pDC->RSGetViewports( &num_original_viewports, original_viewports);

	// RTV setup
	pDC->OMSetRenderTargets(1,&m_pLookupRTV,NULL);
	const D3D11_VIEWPORT viewport = {0.f, 0.f, FLOAT(m_num_gpu_samples_w-2), FLOAT(m_num_gpu_samples_l-2), 0.f, 1.f };
	pDC->RSSetViewports(1,&viewport);

	// Render
	m_pRenderSurfaceToReverseLookupTechnique->GetPassByIndex(0)->Apply(0, pDC);
	GFSDK_WaveWorks_Simulation_SetRenderStateD3D11(hSim, pDC, NvFromDX(matView), m_pWaterSimulationShaderInputMappings, NULL);
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	pDC->IASetInputLayout(NULL);
	pDC->Draw(4*quads_w*quads_h,0);

	// Restore viewports
	pDC->RSSetViewports(num_original_viewports,original_viewports);
}
