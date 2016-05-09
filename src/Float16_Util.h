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

#ifndef _NVWAVEWORKS_FLOAT16_UTIL_H
#define _NVWAVEWORKS_FLOAT16_UTIL_H

#include "simd/Simd4f.h"
#include "simd/Simd4i.h"

namespace GFSDK_WaveWorks_Float16_Util
{
	inline void float16(gfsdk_U16* __restrict out, const float in)
	{
		// Non-SIMD implementation
		gfsdk_U32 fltInt32 = *((gfsdk_U32*)&in);
		gfsdk_U16 fltInt16 = (fltInt32 >> 31) << 5;
		gfsdk_U16 tmp = (fltInt32 >> 23) & 0xff;
		tmp = (tmp - 0x70) & (gfsdk_U32((int)(0x70 - tmp) >> 4) >> 27);
		fltInt16 = (fltInt16 | tmp) << 10;
		fltInt16 |= (fltInt32 >> 13) & 0x3ff;
		*((gfsdk_U16*)out) = (gfsdk_U16)fltInt16;
	};

	inline void float16x4(gfsdk_U16* __restrict out, const Simd4f in)
	{
		// SIMD implementation
		Simd4i fltInt32 = *((Simd4i*)&in);
		Simd4i fltInt16 = (fltInt32 >> 31) << 5;
		Simd4i tmp = (fltInt32 >> 23) & simd4i(0xff);
		Simd4i p = simd4i(0x70);
		Simd4i signmask_5bits = ((simdi::operator-(p,tmp)) >> 16) & simd4i(0x0000001f);
		tmp = (simdi::operator-(tmp,p)) & signmask_5bits;
		fltInt16 = (fltInt16 | tmp) << 10;
		fltInt16 = fltInt16 | ((fltInt32 >> 13) &  simd4i(0x3ff));
		gfsdk_U32* result = (gfsdk_U32*)&fltInt16;
		*((gfsdk_U16*)out + 0) = (gfsdk_U16)(*(result+0));
		*((gfsdk_U16*)out + 1) = (gfsdk_U16)(*(result+1));
		*((gfsdk_U16*)out + 2) = (gfsdk_U16)(*(result+2));
		*((gfsdk_U16*)out + 3) = (gfsdk_U16)(*(result+3));
	};

	inline float float32(const gfsdk_U16 in)
	{
		gfsdk_U32 fltInt16 = in;
		gfsdk_U32 fltInt32 = gfsdk_U32(fltInt16 >> 15) << 8;
		gfsdk_U32 tmp = (fltInt16 >> 10) & 0x1f;
		tmp = (tmp + 0x70);								// TODO: doesn't handle specials...
		fltInt32 = (fltInt32 | tmp) << 23;
		fltInt32 |= (fltInt16 << 13) & 0x7fffff;	

		float result;
		*((gfsdk_U32*)&result) = fltInt32;
		return result;
	}

	inline gfsdk_float4 float32x4(const gfsdk_U16* __restrict in)
	{
		gfsdk_float4 result;
		result.x = float32(in[0]);
		result.y = float32(in[1]);
		result.z = float32(in[2]);
		result.w = float32(in[3]);
		return result;
	}
};

#endif	// _NVWAVEWORKS_SIMULATION_UTIL_H
