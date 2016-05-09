// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2014 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#pragma once

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// factory implementation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <>
inline Simd4iFactory<const int&>::operator Simd4i() const
{
	return (vec_uint4)vec_splat(vec_lvlx(0, (int*)&v), 0);
}

inline Simd4iFactory<detail::FourTuple>::operator Simd4i() const
{
	return (const vec_uint4&)v;
}

template <int i>
inline Simd4iFactory<detail::IntType<i> >::operator Simd4i() const
{
	return (vec_uint4)vec_splat_s32(i);
}

template <>
inline Simd4iFactory<detail::IntType<0x80000000> >::operator Simd4i() const
{
	vec_uint4 mask = (vec_uint4)vec_splat_s32(-1);
	return vec_sl(mask, mask);
}

template <>
inline Simd4iFactory<const int*>::operator Simd4i() const
{
	return (vec_uint4)vec_or(vec_lvlx(0, const_cast<int*>(v)), vec_lvrx(16, const_cast<int*>(v)));
}

template <>
inline Simd4iFactory<detail::AlignedPointer<int> >::operator Simd4i() const
{
	return (vec_uint4)vec_ld(0, const_cast<int*>(v.ptr));
}

template <>
inline Simd4iFactory<detail::OffsetPointer<int> >::operator Simd4i() const
{
	return (vec_uint4)vec_ld(v.offset, const_cast<int*>(v.ptr));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression template
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <>
inline ComplementExpr<Simd4i>::operator Simd4i() const
{
	return vec_nor(v.u4, v.u4);
}

Simd4i operator&(const ComplementExpr<Simd4i>& complement, const Simd4i& v)
{
	return vec_andc(v.u4, complement.v.u4);
}

Simd4i operator&(const Simd4i& v, const ComplementExpr<Simd4i>& complement)
{
	return vec_andc(v.u4, complement.v.u4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operator implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4i simdi::operator==(const Simd4i& v0, const Simd4i& v1)
{
	return (vec_uint4)vec_cmpeq(v0.u4, v1.u4);
}

Simd4i simdi::operator<(const Simd4i& v0, const Simd4i& v1)
{
	return (vec_uint4)vec_cmplt((vec_int4)v0.u4, (vec_int4)v1.u4);
}

Simd4i simdi::operator>(const Simd4i& v0, const Simd4i& v1)
{
	return (vec_uint4)vec_cmpgt((vec_int4)v0.u4, (vec_int4)v1.u4);
}

ComplementExpr<Simd4i> operator~(const Simd4i& v)
{
	return ComplementExpr<Simd4i>(v);
}

Simd4i operator&(const Simd4i& v0, const Simd4i& v1)
{
	return vec_and(v0.u4, v1.u4);
}

Simd4i operator|(const Simd4i& v0, const Simd4i& v1)
{
	return vec_or(v0.u4, v1.u4);
}

Simd4i operator^(const Simd4i& v0, const Simd4i& v1)
{
	return vec_xor(v0.u4, v1.u4);
}

Simd4i operator<<(const Simd4i& v, int shift)
{
	return vec_sl(v.u4, vec_splat((vec_uint4)vec_lvlx(0, &shift), 0));
}

Simd4i operator>>(const Simd4i& v, int shift)
{
	return vec_sr(v.u4, vec_splat((vec_uint4)vec_lvlx(0, &shift), 0));
}

Simd4i operator<<(const Simd4i& v, const Simd4i& shift)
{
	return vec_sl(v.u4, shift.u4);
}

Simd4i operator>>(const Simd4i& v, const Simd4i& shift)
{
	return vec_sr(v.u4, shift.u4);
}

Simd4i simdi::operator+(const Simd4i& v0, const Simd4i& v1)
{
	return vec_add(v0.u4, v1.u4);
}

Simd4i simdi::operator-(const Simd4i& v)
{
	return vec_sub((vec_uint4)vec_splat_s32(0), v.u4);
}

Simd4i simdi::operator-(const Simd4i& v0, const Simd4i& v1)
{
	return vec_sub(v0.u4, v1.u4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// function implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4i simd4i(const Simd4f& v)
{
	return (vec_uint4)v.f4;
}

Simd4i truncate(const Simd4f& v)
{
	return vec_cts(v.f4, 0);	
}

int (&simdi::array(Simd4i& v))[4]
{
	return (int(&)[4])v;
}

const int (&simdi::array(const Simd4i& v))[4]
{
	return (const int(&)[4])v;
}

void store(int* ptr, const Simd4i& v)
{
	vec_stvlx((vec_int4)v.u4, 0, ptr);
	vec_stvrx((vec_int4)v.u4, 16, ptr);
}

void storeAligned(int* ptr, const Simd4i& v)
{
	vec_stvlx((vec_int4)v.u4, 0, ptr);
}

void storeAligned(int* ptr, unsigned int offset, const Simd4i& v)
{
	vec_stvlx((vec_int4)v.u4, offset, ptr);
}

template <size_t i>
Simd4i splat(Simd4i const& v)
{
	return vec_splat(v.u4, i);
}

Simd4i select(Simd4i const& mask, Simd4i const& v0, Simd4i const& v1)
{
	return vec_sel(v1.u4, v0.u4, mask.u4);
}

int simdi::allEqual(const Simd4i& v0, const Simd4i& v1)
{
	return vec_all_eq(v0.u4, v1.u4);
}

int simdi::allEqual(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	int r = simdi::allEqual(v0, v1);
	outMask = simdi::operator==(v0, v1);
	return r;
}

int simdi::anyEqual(const Simd4i& v0, const Simd4i& v1)
{
	return vec_any_eq(v0.u4, v1.u4);
}

int simdi::anyEqual(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	int r = simdi::anyEqual(v0, v1);
	outMask = simdi::operator==(v0, v1);
	return r;
}

int simdi::allGreater(const Simd4i& v0, const Simd4i& v1)
{
	return vec_all_gt(v0.u4, v1.u4);
}

int simdi::allGreater(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	int r = simdi::allGreater(v0, v1);
	outMask = simdi::operator>(v0, v1);
	return r;
}

int simdi::anyGreater(const Simd4i& v0, const Simd4i& v1)
{
	return vec_any_gt(v0.u4, v1.u4);
}

int simdi::anyGreater(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	int r = simdi::anyGreater(v0, v1);
	outMask = simdi::operator>(v0, v1);
	return r;
}

int allTrue(const Simd4i& v)
{
	return vec_all_lt((vec_int4)v.u4, vec_splat_s32(0));
}

int anyTrue(const Simd4i& v)
{
	return vec_any_lt((vec_int4)v.u4, vec_splat_s32(0));
}
