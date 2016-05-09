#ifndef MATH_CODE_H_
#define MATH_CODE_H_

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

#include "Internal.h"

// vector and matrix math functions
void mat4Mat4Mul(gfsdk_float4x4& result, const gfsdk_float4x4& a, const gfsdk_float4x4& b);
void vec4Mat4Mul(gfsdk_float4& result, const gfsdk_float4& a, const gfsdk_float4x4& b);
void mat4Inverse(gfsdk_float4x4& result, const gfsdk_float4x4& source);

inline gfsdk_float2 gfsdk_make_float2(float x, float y) { gfsdk_float2 result = {x, y}; return result; }
inline gfsdk_float3 gfsdk_make_float3(float x, float y, float z) { gfsdk_float3 result = {x, y, z}; return result; }
inline gfsdk_float4 gfsdk_make_float4(float x, float y, float z, float w) { gfsdk_float4 result = {x, y, z, w}; return result; }

inline gfsdk_float2 operator+(const gfsdk_float2& a, const gfsdk_float2& b) { gfsdk_float2 result = {a.x+b.x, a.y+b.y }; return result; }
inline gfsdk_float3 operator+(const gfsdk_float3& a, const gfsdk_float3& b) { gfsdk_float3 result = {a.x+b.x, a.y+b.y, a.z+b.z }; return result; }
inline gfsdk_float4 operator+(const gfsdk_float4& a, const gfsdk_float4& b) { gfsdk_float4 result = {a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w }; return result; }

inline gfsdk_float2 operator-(const gfsdk_float2& a, const gfsdk_float2& b) { gfsdk_float2 result = {a.x-b.x, a.y-b.y }; return result; }
inline gfsdk_float3 operator-(const gfsdk_float3& a, const gfsdk_float3& b) { gfsdk_float3 result = {a.x-b.x, a.y-b.y, a.z-b.z }; return result; }
inline gfsdk_float4 operator-(const gfsdk_float4& a, const gfsdk_float4& b) { gfsdk_float4 result = {a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w }; return result; }

inline gfsdk_float2 operator*(const gfsdk_float2& b, float s) { gfsdk_float2 result = {s*b.x, s*b.y}; return result; }
inline gfsdk_float4 operator*(float s, const gfsdk_float4& b) { gfsdk_float4 result = {s*b.x, s*b.y, s*b.z, s*b.w }; return result; }
inline gfsdk_float4& operator+=(gfsdk_float4& a, const gfsdk_float4& b) { a.x += b.x; a.y += b.y; a.z += b.z; a.w += b.w; return a; }

inline float length(const gfsdk_float3& a) { return sqrtf(a.x*a.x+a.y*a.y+a.z*a.z); }
inline void setIdentity(gfsdk_float4x4& m) { for(int j=0; j<4; ++j) for(int i=0; i<4; ++i) (&m._11)[j*4+i] = float(i == j); }

#endif /* MATH_CODE_H_ */
