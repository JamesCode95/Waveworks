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

#ifndef _NVWAVEWORKS_SIMULATION_UTIL_H
#define _NVWAVEWORKS_SIMULATION_UTIL_H

namespace GFSDK_WaveWorks_Simulation_Util
{
	void init_omega(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params, float* pOutOmega);
	void init_gauss(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params, float2* pOutGauss);
	float get_spectrum_rms_sqr(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params);
	void add_displacements_float16(	const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params,
									const BYTE* pReadbackData,
									UINT rowPitch, 
									const gfsdk_float2* inSamplePoints,
									gfsdk_float4* outDisplacements,
									UINT numSamples,
									float multiplier = 1.f
									);
	void add_displacements_float32(	const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params,
									const BYTE* pReadbackData,
									UINT rowPitch, 
									const gfsdk_float2* inSamplePoints,
									gfsdk_float4* outDisplacements,
									UINT numSamples,
									float multiplier = 1.f
									);
	TickType getTicks();
	float	getMilliseconds(const TickType& start, const TickType& stop);
	void    tieThreadToCore(unsigned char core);
};

#endif	// _NVWAVEWORKS_SIMULATION_UTIL_H
