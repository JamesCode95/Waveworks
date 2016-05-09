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
	return __vspltw(__lvlx(&v, 0), 0);
}

inline Simd4fFactory<detail::FourTuple>::operator Simd4f() const
{
	return reinterpret_cast<const Simd4f&>(v);
}

template <>
inline Simd4fFactory<detail::IntType<0> >::operator Simd4f() const
{
	return __vspltisw(0);
}

template <>
inline Simd4fFactory<detail::IntType<1> >::operator Simd4f() const
{
	return __vupkd3d(__vspltisw(0), VPACK_D3DCOLOR);
}

template <>
inline Simd4fFactory<detail::IntType<0x80000000> >::operator Simd4f() const
{
	Simd4f mask = __vspltisw(-1);
	return __vslw(mask, mask);
}

template <>
inline Simd4fFactory<detail::IntType<0xffffffff> >::operator Simd4f() const
{
	return __vspltisw(-1);
}

template <>
inline Simd4fFactory<const float*>::operator Simd4f() const
{
	return __vor(__lvlx(v, 0), __lvrx(v, 16));
}

template <>
inline Simd4fFactory<detail::AlignedPointer<float> >::operator Simd4f() const
{
	return __lvx(v.ptr, 0);
}

template <>
inline Simd4fFactory<detail::OffsetPointer<float> >::operator Simd4f() const
{
	return __lvx(v.ptr, int(v.offset));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression templates
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <>
ComplementExpr<Simd4f>::operator Simd4f() const
{
	return __vnor(v, v);
}

Simd4f operator&(const ComplementExpr<Simd4f>& complement, const Simd4f& v)
{
	return __vandc(v, complement.v);
}

Simd4f operator&(const Simd4f& v, const ComplementExpr<Simd4f>& complement)
{
	return __vandc(v, complement.v);
}

ProductExpr::operator Simd4f() const
{
	return __vmulfp(v0, v1);
}

Simd4f operator+(const ProductExpr& p, const Simd4f& v)
{
	return __vmaddfp(p.v0, p.v1, v);
}

Simd4f operator+(const Simd4f& v, const ProductExpr& p)
{
	return __vmaddfp(p.v0, p.v1, v);
}

Simd4f operator+(const ProductExpr& p0, const ProductExpr& p1)
{
	return __vmaddfp(p1.v0, p1.v1, p0);
}

Simd4f operator-(const Simd4f& v, const ProductExpr& p)
{
	return __vnmsubfp(p.v0, p.v1, v);
}

Simd4f operator-(const ProductExpr& p0, const ProductExpr& p1)
{
	return __vnmsubfp(p1.v0, p1.v1, p0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operator implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4f operator==(const Simd4f& v0, const Simd4f& v1)
{
	return __vcmpeqfp(v0, v1);
}

Simd4f operator<(const Simd4f& v0, const Simd4f& v1)
{
	return __vcmpgtfp(v1, v0);
}

Simd4f operator<=(const Simd4f& v0, const Simd4f& v1)
{
	return __vcmpgefp(v1, v0);
}

Simd4f operator>(const Simd4f& v0, const Simd4f& v1)
{
	return __vcmpgtfp(v0, v1);
}

Simd4f operator>=(const Simd4f& v0, const Simd4f& v1)
{
	return __vcmpgefp(v0, v1);
}

ComplementExpr<Simd4f> operator~(const Simd4f& v)
{
	return ComplementExpr<Simd4f>(v);
}

Simd4f operator&(const Simd4f& v0, const Simd4f& v1)
{
	return __vand(v0, v1);
}

Simd4f operator|(const Simd4f& v0, const Simd4f& v1)
{
	return __vor(v0, v1);
}

Simd4f operator^(const Simd4f& v0, const Simd4f& v1)
{
	return __vxor(v0, v1);
}

Simd4f operator<<(const Simd4f& v, int shift)
{
	return __vslw(v, __vspltw(__lvlx(&shift, 0), 0));
}

Simd4f operator>>(const Simd4f& v, int shift)
{
	return __vsrw(v, __vspltw(__lvlx(&shift, 0), 0));
}

Simd4f operator<<(const Simd4f& v, const Simd4f& shift)
{
	return __vslw(v, shift);
}

Simd4f operator>>(const Simd4f& v, const Simd4f& shift)
{
	return __vsrw(v, shift);
}

Simd4f operator+(const Simd4f& v)
{
	return v;
}

Simd4f operator+(const Simd4f& v0, const Simd4f& v1)
{
	return __vaddfp(v0, v1);
}

Simd4f operator-(const Simd4f& v)
{
	return __vxor(v, simd4f(_sign));
}

Simd4f operator-(const Simd4f& v0, const Simd4f& v1)
{
	return __vsubfp(v0, v1);
}

ProductExpr operator*(const Simd4f& v0, const Simd4f& v1)
{
	return ProductExpr(v0, v1);
}

Simd4f operator/(const Simd4f& v0, const Simd4f& v1)
{
	return __vmulfp(v0, __vrefp(v1)); // reciprocal estimate
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// function implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4f simd4f(const Simd4i& v)
{
	return v;
}

Simd4f convert(const Simd4i& v)
{
	return __vcfsx(v, 0);
}

float (&array(Simd4f& v))[4]
{
	return v.vector4_f32;
}

const float (&array(const Simd4f& v))[4]
{
	return v.vector4_f32;
}

void store(float* ptr, Simd4f const& v)
{
	__stvlx(v, ptr, 0);
	__stvrx(v, ptr, 16);
}

void storeAligned(float* ptr, Simd4f const& v)
{
	__stvlx(v, ptr, 0);
}

void storeAligned(float* ptr, unsigned int offset, Simd4f const& v)
{
	__stvlx(v, ptr, int(offset));
}

template <size_t i>
Simd4f splat(Simd4f const& v)
{
	return __vspltw(v, i);
}

Simd4f select(Simd4f const& mask, Simd4f const& v0, Simd4f const& v1)
{
	return __vsel(v1, v0, mask);
}

Simd4f abs(const Simd4f& v)
{
	return __vandc(v, simd4f(_sign));
}

Simd4f floor(const Simd4f& v)
{
	return __vrfim(v);
}

Simd4f max(const Simd4f& v0, const Simd4f& v1)
{
	return __vmaxfp(v0, v1);
}

Simd4f min(const Simd4f& v0, const Simd4f& v1)
{
	return __vminfp(v0, v1);
}

Simd4f recip(const Simd4f& v)
{
	return __vrefp(v);
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
	return __vmulfp(v, __vrsqrtefp(v));
}

Simd4f rsqrt(const Simd4f& v)
{
	return __vrsqrtefp(v);
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
	return __vexptefp(v);
}

Simd4f log2(const Simd4f& v)
{
	return __vlogefp(v);
}

Simd4f dot3(const Simd4f& v0, const Simd4f& v1)
{
	return __vmsum3fp(v0, v1);
}

Simd4f cross3(const Simd4f& v0, const Simd4f& v1)
{
	Simd4f t0 = __vpermwi(v0, 0x63); // x y z w -> y z x w
	Simd4f t1 = __vpermwi(v1, 0x63);
	Simd4f tmp = __vnmsubfp(t0, v1, __vmulfp(v0, t1));
	return __vpermwi(tmp, 0x63);
}

void transpose(Simd4f& x, Simd4f& y, Simd4f& z, Simd4f& w)
{
	Simd4f v0 = __vmrglw(x, z);
	Simd4f v1 = __vmrghw(x, z);
	Simd4f v2 = __vmrglw(y, w);
	Simd4f v3 = __vmrghw(y, w);
	x = __vmrghw(v1, v3);
	y = __vmrglw(v1, v3);
	z = __vmrghw(v0, v2);
	w = __vmrglw(v0, v2);
}

void zip(Simd4f& v0, Simd4f& v1)
{
	Simd4f t0 = v0;
	v0 = __vmrglw(v0, v1);
	v1 = __vmrghw(t0, v1);
}

void unzip(Simd4f& v0, Simd4f& v1)
{
	Simd4f t0 = __vmrglw(v0, v1); // v0.x, v1.x, v0.y, v1.y
	Simd4f t1 = __vmrghw(v0, v1); // v0.z, v1.z, v0.w, v1.w
	v0 = __vmrglw(t0, t1); // v0.x, v0.z, v1.x, v1.z
	v1 = __vmrghw(t0, t1); // v0.y, v0.w, v1.y, v1.w
}

Simd4f swaphilo(const Simd4f& v)
{
	return __vpermwi(v, 0xa1); // x y z w -> z w x y
}

int allEqual(const Simd4f& v0, const Simd4f& v1)
{
	unsigned int control;
	__vcmpeqfpR(v0, v1, &control);
	return int(0x80 & control); // all true
}

int allEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	unsigned int control;
	outMask = __vcmpeqfpR(v0, v1, &control);
	return int(0x80 & control); // all true
}

int anyEqual(const Simd4f& v0, const Simd4f& v1)
{
	unsigned int control;
	__vcmpeqfpR(v0, v1, &control);
	return int(0x20 & ~control); // not all false
}

int anyEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	unsigned int control;
	outMask = __vcmpeqfpR(v0, v1, &control);
	return int(0x20 & ~control); // not all false
}

int allGreater(const Simd4f& v0, const Simd4f& v1)
{
	unsigned int control;
	__vcmpgtfpR(v0, v1, &control);
	return int(0x80 & control); // all true
}

int allGreater(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	unsigned int control;
	outMask = __vcmpgtfpR(v0, v1, &control);
	return int(0x80 & control); // all true
}

int anyGreater(const Simd4f& v0, const Simd4f& v1)
{
	unsigned int control;
	__vcmpgtfpR(v0, v1, &control);
	return int(0x20 & ~control); // not all false
}

int anyGreater(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	unsigned int control;
	outMask = __vcmpgtfpR(v0, v1, &control);
	return int(0x20 & ~control); // not all false
}

int allGreaterEqual(const Simd4f& v0, const Simd4f& v1)
{
	unsigned int control;
	__vcmpgefpR(v0, v1, &control);
	return int(0x80 & control); // all true
}

int allGreaterEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	unsigned int control;
	outMask = __vcmpgefpR(v0, v1, &control);
	return int(0x80 & control); // all true
}

int anyGreaterEqual(const Simd4f& v0, const Simd4f& v1)
{
	unsigned int control;
	__vcmpgefpR(v0, v1, &control);
	return int(0x20 & ~control); // not all false
}

int anyGreaterEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	unsigned int control;
	outMask = __vcmpgefpR(v0, v1, &control);
	return int(0x20 & ~control); // not all false
}

int allTrue(const Simd4f& v)
{
	unsigned int control;
	__vcmpgefpR(v, simd4f(_0), &control);
	return int(0x20 & control); // all false
}

int anyTrue(const Simd4f& v)
{
	unsigned int control;
	__vcmpgefpR(v, simd4f(_0), &control);
	return int(0x80 & ~control); // not all true
}
