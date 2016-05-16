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
// Copyright � 2008- 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#ifndef _GFSDK_WAVEWORKS_D3D_UTIL_H
#define _GFSDK_WAVEWORKS_D3D_UTIL_H

#include <GFSDK_WaveWorks_Types.h>

using namespace DirectX;

// Convenience functions for converting between DX and NV types when using the WaveWorks API
// This header should be #included *AFTER* d3d and d3dx headers

////////////////////////////////////////////////////////////////////////////////
// Conversion helpers
////////////////////////////////////////////////////////////////////////////////
__GFSDK_INLINE__ XMFLOAT2 NvToDX(const gfsdk_float2& rhs) {
	XMFLOAT2 result;
	result.x = rhs.x;
	result.y = rhs.y;
	return result;
}

__GFSDK_INLINE__ gfsdk_float2 NvFromDX(const XMFLOAT2& rhs) {
	gfsdk_float2 result;
	result.x = rhs.x;
	result.y = rhs.y;
	return result;
}

__GFSDK_INLINE__ XMFLOAT4 NvToDX(const gfsdk_float4& rhs) {
	XMFLOAT4 result;
	result.x = rhs.x;
	result.y = rhs.y;
	result.z = rhs.z;
	result.w = rhs.w;
	return result;
}

__GFSDK_INLINE__ gfsdk_float4 NvFromDX(const XMFLOAT4& rhs) {
	gfsdk_float4 result;
	result.x = rhs.x;
	result.y = rhs.y;
	result.z = rhs.z;
	result.w = rhs.w;
	return result;
}

__GFSDK_INLINE__ XMMATRIX NvToDX(const gfsdk_float4x4& rhs) {

	return XMMATRIX(rhs._11, rhs._12, rhs._13, rhs._14,
					rhs._21, rhs._22, rhs._23, rhs._24,
					rhs._31, rhs._32, rhs._33, rhs._34,
					rhs._41, rhs._42, rhs._43, rhs._44);
}

__GFSDK_INLINE__ gfsdk_float4x4 NvFromDX(const XMMATRIX& rhs) {

	gfsdk_float4x4 result;

	//NOTE: Not happy with using a reinterpret_cast here.
	XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&result), rhs);

	return result;
}

__GFSDK_INLINE__ gfsdk_float4x4 NvFromGL(const float* rhs) {

	gfsdk_float4x4 result;

	result._11 = rhs[0];
	result._12 = rhs[1];
	result._13 = rhs[2];
	result._14 = rhs[3];

	result._21 = rhs[4];
	result._22 = rhs[5];
	result._23 = rhs[6];
	result._24 = rhs[7];

	result._31 = rhs[8];
	result._32 = rhs[9];
	result._33 = rhs[10];
	result._34 = rhs[11];

	result._41 = rhs[12];
	result._42 = rhs[13];
	result._43 = rhs[14];
	result._44 = rhs[15];

	return result;
}

#endif	// _GFSDK_WAVEWORKS_D3D_UTIL_H
