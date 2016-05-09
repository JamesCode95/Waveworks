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
inline Simd4fFactory<const float&>::operator Simd4f() const
{
	return vec_splat(vec_lvlx(0, const_cast<float*>(&v)), 0);
}

inline Simd4fFactory<detail::FourTuple>::operator Simd4f() const
{
	return (const vec_float4&)v;
}

template <>
inline Simd4fFactory<detail::IntType<0> >::operator Simd4f() const
{
	return (vec_float4)vec_splat_s32(0);
}

template <>
inline Simd4fFactory<detail::IntType<1> >::operator Simd4f() const
{
	return vec_splats(1.0f);
}

template <>
inline Simd4fFactory<detail::IntType<0x80000000> >::operator Simd4f() const
{
	vec_uint4 mask = (vec_uint4)vec_splat_s32(-1);
	return (vec_float4)vec_sl(mask, mask);
}

template <>
inline Simd4fFactory<detail::IntType<0xffffffff> >::operator Simd4f() const
{
	return (vec_float4)vec_splat_s32(-1);
}

template <>
inline Simd4fFactory<const float*>::operator Simd4f() const
{
	return (vec_float4)vec_or(vec_lvlx(0, const_cast<float*>(v)), vec_lvrx(16, const_cast<float*>(v)));
}

template <>
inline Simd4fFactory<detail::AlignedPointer<float> >::operator Simd4f() const
{
	return vec_ld(0, const_cast<float*>(v.ptr));
}

template <>
inline Simd4fFactory<detail::OffsetPointer<float> >::operator Simd4f() const
{
	return vec_ld(v.offset, const_cast<float*>(v.ptr));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression templates
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <>
inline ComplementExpr<Simd4f>::operator Simd4f() const
{
	return vec_nor(v.f4, v.f4);
}

Simd4f operator&(const ComplementExpr<Simd4f>& complement, const Simd4f& v)
{
	return vec_andc(v.f4, complement.v.f4);
}

Simd4f operator&(const Simd4f& v, const ComplementExpr<Simd4f>& complement)
{
	return vec_andc(v.f4, complement.v.f4);
}

ProductExpr::operator Simd4f() const
{
	return vec_madd(v0.f4, v1.f4, (vec_float4)vec_splat_s32(0));
}

Simd4f operator+(const ProductExpr& p, const Simd4f& v)
{
	return vec_madd(p.v0.f4, p.v1.f4, v.f4);
}

Simd4f operator+(const Simd4f& v, const ProductExpr& p)
{
	return vec_madd(p.v0.f4, p.v1.f4, v.f4);
}

Simd4f operator+(const ProductExpr& p0, const ProductExpr& p1)
{
	// cast calls operator Simd4f() which evaluates the other ProductExpr
	return vec_madd(p1.v0.f4, p1.v1.f4, static_cast<Simd4f>(p0).f4);
}

Simd4f operator-(const Simd4f& v, const ProductExpr& p)
{
	return vec_nmsub(p.v0.f4, p.v1.f4, v.f4);
}

Simd4f operator-(const ProductExpr& p0, const ProductExpr& p1)
{
	// cast calls operator Simd4f() which evaluates the other ProductExpr
	return vec_nmsub(p1.v0.f4, p1.v1.f4, static_cast<Simd4f>(p0).f4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operator implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4f operator==(const Simd4f& v0, const Simd4f& v1)
{
	return (vec_float4)vec_cmpeq(v0.f4, v1.f4);
}

Simd4f operator<(const Simd4f& v0, const Simd4f& v1)
{
	return (vec_float4)vec_cmplt(v0.f4, v1.f4);
}

Simd4f operator<=(const Simd4f& v0, const Simd4f& v1)
{
	return (vec_float4)vec_cmple(v0.f4, v1.f4);
}

Simd4f operator>(const Simd4f& v0, const Simd4f& v1)
{
	return (vec_float4)vec_cmpgt(v0.f4, v1.f4);
}

Simd4f operator>=(const Simd4f& v0, const Simd4f& v1)
{
	return (vec_float4)vec_cmpge(v0.f4, v1.f4);
}

ComplementExpr<Simd4f> operator~(const Simd4f& v)
{
	return ComplementExpr<Simd4f>(v);
}

Simd4f operator&(const Simd4f& v0, const Simd4f& v1)
{
	return vec_and(v0.f4, v1.f4);
}

Simd4f operator|(const Simd4f& v0, const Simd4f& v1)
{
	return vec_or(v0.f4, v1.f4);
}

Simd4f operator^(const Simd4f& v0, const Simd4f& v1)
{
	return vec_xor(v0.f4, v1.f4);
}

Simd4f operator<<(const Simd4f& v, int shift)
{
	return (vec_float4)vec_sl((vec_uint4)v.f4, vec_splat((vec_uint4)vec_lvlx(0, &shift), 0));
}

Simd4f operator>>(const Simd4f& v, int shift)
{
	return (vec_float4)vec_sr((vec_uint4)v.f4, vec_splat((vec_uint4)vec_lvlx(0, &shift), 0));
}

Simd4f operator<<(const Simd4f& v, const Simd4f& shift)
{
	return (vec_float4)vec_sl((vec_uint4)v.f4, (vec_uint4)shift.f4);
}

Simd4f operator>>(const Simd4f& v, const Simd4f& shift)
{
	return (vec_float4)vec_sr((vec_uint4)v.f4, (vec_uint4)shift.f4);
}

Simd4f operator+(const Simd4f& v)
{
	return v;
}

Simd4f operator+(const Simd4f& v0, const Simd4f& v1)
{
	return vec_add(v0.f4, v1.f4);
}

Simd4f operator-(const Simd4f& v)
{
	vec_uint4 mask = (vec_uint4)vec_splat_s32(-1);
	return vec_xor(v.f4, (vec_float4)vec_sl(mask, mask));
}

Simd4f operator-(const Simd4f& v0, const Simd4f& v1)
{
	return vec_sub(v0.f4, v1.f4);
}

ProductExpr operator*(const Simd4f& v0, const Simd4f& v1)
{
	return ProductExpr(v0, v1);
}

Simd4f operator/(const Simd4f& v0, const Simd4f& v1)
{
	return v0 * vec_re(v1.f4); // reciprocal estimate
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// function implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4f simd4f(const Simd4i& v)
{
	return (vec_float4)v.u4;
}

Simd4f convert(const Simd4i& v)
{
	return vec_ctf(v.i4, 0);
}

float (&array(Simd4f& v))[4]
{
	return (float(&)[4])v;
}

const float (&array(const Simd4f& v))[4]
{
	return (const float(&)[4])v;
}

void store(float* ptr, Simd4f const& v)
{
	vec_stvlx(v.f4, 0, ptr);
	vec_stvrx(v.f4, 16, ptr);
}

void storeAligned(float* ptr, Simd4f const& v)
{
	vec_stvlx(v.f4, 0, ptr);
}

void storeAligned(float* ptr, unsigned int offset, Simd4f const& v)
{
	vec_stvlx(v.f4, offset, ptr);
}

template <size_t i>
Simd4f splat(Simd4f const& v)
{
	return vec_splat(v.f4, i);
}

Simd4f select(Simd4f const& mask, Simd4f const& v0, Simd4f const& v1)
{
	return vec_sel(v1.f4, v0.f4, (vec_uint4)mask.f4);
}

Simd4f abs(const Simd4f& v)
{
	vec_uint4 mask = (vec_uint4)vec_splat_s32(-1);
	return (vec_float4)vec_andc((vec_uint4)v.f4, vec_sl(mask, mask));
}

Simd4f floor(const Simd4f& v)
{
	return vec_floor(v.f4);
}

Simd4f max(const Simd4f& v0, const Simd4f& v1)
{
	return vec_max(v0.f4, v1.f4);
}

Simd4f min(const Simd4f& v0, const Simd4f& v1)
{
	return vec_min(v0.f4, v1.f4);
}

Simd4f recip(const Simd4f& v)
{
	return vec_re(v.f4);
}

template <int n>
Simd4f recip(const Simd4f& v)
{
	Simd4f two = simd4f(2.0f);
	Simd4f recipV = recip(v);
	for(int i = 0; i < n; ++i)
		recipV = recipV * (two - v * recipV);
	return recipV;
}

Simd4f sqrt(const Simd4f& v)
{
	return v * vec_rsqrte(v.f4);
}

Simd4f rsqrt(const Simd4f& v)
{
	return vec_rsqrte(v.f4);
}

template <int n>
Simd4f rsqrt(const Simd4f& v)
{
	Simd4f halfV = v * simd4f(0.5f);
	Simd4f threeHalf = simd4f(1.5f);
	Simd4f rsqrtV = rsqrt(v);
	for(int i = 0; i < n; ++i)
		rsqrtV = rsqrtV * (threeHalf - halfV * rsqrtV * rsqrtV);
	return rsqrtV;
}

Simd4f exp2(const Simd4f& v)
{
	// vec_expte approximation only valid for domain [-127, 127]
	Simd4f limit = simd4f(127.0f);
	Simd4f x = min(max(v, -limit), limit);

	return vec_expte(x.f4);
}

Simd4f log2(const Simd4f& v)
{
	return vec_loge(v.f4);
}

Simd4f dot3(const Simd4f& v0, const Simd4f& v1)
{
	Simd4f tmp = v0 * v1;
	return splat<0>(tmp) + splat<1>(tmp) + splat<2>(tmp);
}

Simd4f cross3(const Simd4f& v0, const Simd4f& v1)
{
	// w z y x -> w x z y
	uint32_t data[] __attribute__((aligned(16))) = { 0x04050607, 0x08090a0b, 0x00010203, 0x0c0d0e0f };
	vec_uchar16 perm = vec_ld(0, (unsigned char*)data);

	Simd4f t0 = vec_perm(v0.f4, v0.f4, perm);
	Simd4f t1 = vec_perm(v1.f4, v1.f4, perm);
	Simd4f tmp = v0 * t1 - t0 * v1;
	return vec_perm(tmp.f4, tmp.f4, perm);
}

void transpose(Simd4f& x, Simd4f& y, Simd4f& z, Simd4f& w)
{
	Simd4f v0 = vec_mergel(x.f4, z.f4);
	Simd4f v1 = vec_mergeh(x.f4, z.f4);
	Simd4f v2 = vec_mergel(y.f4, w.f4);
	Simd4f v3 = vec_mergeh(y.f4, w.f4);
	x = vec_mergeh(v1.f4, v3.f4);
	y = vec_mergel(v1.f4, v3.f4);
	z = vec_mergeh(v0.f4, v2.f4);
	w = vec_mergel(v0.f4, v2.f4);
}

void zip(Simd4f& v0, Simd4f& v1)
{
	Simd4f t0 = v0;
	v0 = vec_mergel(v0, v1);
	v1 = vec_mergeh(t0, v1);
}

void unzip(Simd4f& v0, Simd4f& v1)
{
	Simd4f t0 = vec_mergel(v0, v1); // v0.x, v1.x, v0.y, v1.y
	Simd4f t1 = vec_mergeh(v0, v1); // v0.z, v1.z, v0.w, v1.w
	v0 = vec_mergel(t0, t1); // v0.x, v0.z, v1.x, v1.z
	v1 = vec_mergeh(t0, t1); // v0.y, v0.w, v1.y, v1.w
}

Simd4f swaphilo(const Simd4f& v)
{
	uint32_t data[] __attribute__((aligned(16))) = { 0x08090a0b, 0x0c0d0e0f, 0x00010203, 0x04050607 };
	vec_uchar16 perm = vec_ld(0, (unsigned char*)data);

	return vec_perm(v0.f4, v0.f4, perm);
}

int allEqual(const Simd4f& v0, const Simd4f& v1)
{
	return vec_all_eq(v0.f4, v1.f4);
}

int allEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	int r = allEqual(v0, v1);
	outMask = v0 == v1;
	return r;
}

int anyEqual(const Simd4f& v0, const Simd4f& v1)
{
	return vec_any_eq(v0.f4, v1.f4);
}

int anyEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	int r = anyEqual(v0, v1);
	outMask = v0 == v1;
	return r;
}

int allGreater(const Simd4f& v0, const Simd4f& v1)
{
	return vec_all_gt(v0.f4, v1.f4);
}

int allGreater(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	int r = allGreater(v0, v1);
	outMask = v0 > v1;
	return r;
}

int anyGreater(const Simd4f& v0, const Simd4f& v1)
{
	return vec_any_gt(v0.f4, v1.f4);
}

int anyGreater(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	int r = anyGreater(v0, v1);
	outMask = v0 > v1;
	return r;
}

int allGreaterEqual(const Simd4f& v0, const Simd4f& v1)
{
	return vec_all_ge(v0.f4, v1.f4);
}

int allGreaterEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	int r = allGreaterEqual(v0, v1);
	outMask = v0 >= v1;
	return r;
}

int anyGreaterEqual(const Simd4f& v0, const Simd4f& v1)
{
	return vec_any_ge(v0.f4, v1.f4);
}

int anyGreaterEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	int r = anyGreaterEqual(v0, v1);
	outMask = v0 >= v1;
	return r;
}

int allTrue(const Simd4f& v)
{
	return !vec_any_ge(v.f4, (vec_float4)vec_splat_s32(0));
}

int anyTrue(const Simd4f& v)
{
	return !vec_all_ge(v.f4, (vec_float4)vec_splat_s32(0));
}
