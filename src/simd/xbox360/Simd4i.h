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
	return __vspltw(__lvlx(&v, 0), 0);
}

inline Simd4iFactory<detail::FourTuple>::operator Simd4i() const
{
	return reinterpret_cast<const Simd4i&>(v);
}

template <int i>
inline Simd4iFactory<detail::IntType<i> >::operator Simd4i() const
{
	return __vspltisw(i);
}

template <>
inline Simd4iFactory<detail::IntType<0x80000000> >::operator Simd4i() const
{
	Simd4f mask = __vspltisw(-1);
	return __vslw(mask, mask);
}

template <>
inline Simd4iFactory<const int*>::operator Simd4i() const
{
	return __vor(__lvlx(v, 0), __lvrx(v, 16));
}

template <>
inline Simd4iFactory<detail::AlignedPointer<int> >::operator Simd4i() const
{
	return __lvx(v.ptr, 0);
}

template <>
inline Simd4iFactory<detail::OffsetPointer<int> >::operator Simd4i() const
{
	return __lvx(v.ptr, int(v.offset));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operator implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4i simdi::operator==(const Simd4i& v0, const Simd4i& v1)
{
	return __vcmpequw(v0, v1);
}

Simd4i simdi::operator<(const Simd4i& v0, const Simd4i& v1)
{
	return __vcmpgtsw(v1, v0);
}

Simd4i simdi::operator>(const Simd4i& v0, const Simd4i& v1)
{
	return __vcmpgtsw(v0, v1);
}

Simd4i simdi::operator+(const Simd4i& v0, const Simd4i& v1)
{
	return __vadduwm(v0, v1);
}

Simd4i simdi::operator-(const Simd4i& v)
{
	return __vsubuwm(__vspltisw(0), v);
}

Simd4i simdi::operator-(const Simd4i& v0, const Simd4i& v1)
{
	return __vsubuwm(v0, v1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// function implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4i simd4i(const Simd4f& v)
{
	return v;
}

Simd4i truncate(const Simd4f& v)
{
	return __vrfiz(v);	
}

int (&simdi::array(Simd4i& v))[4]
{
	return reinterpret_cast<int(&)[4]>(v.vector4_u32);
}

const int (&simdi::array(const Simd4i& v))[4]
{
	return reinterpret_cast<const int(&)[4]>(v.vector4_u32);
}

void store(int* ptr, const Simd4i& v)
{
	__stvlx(v, ptr, 0);
	__stvrx(v, ptr, 16);
}

void storeAligned(int* ptr, const Simd4i& v)
{
	__stvlx(v, ptr, 0);
}

void storeAligned(int* ptr, unsigned int offset, const Simd4i& v)
{
	__stvlx(v, ptr, int(offset));
}

int simdi::allEqual(const Simd4i& v0, const Simd4i& v1)
{
	unsigned int control;
	__vcmpequwR(v0, v1, &control);
	return int(0x80 & control); // all true
}

int simdi::allEqual(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	unsigned int control;
	outMask = __vcmpequwR(v0, v1, &control);
	return int(0x80 & control); // all true
}

int simdi::anyEqual(const Simd4i& v0, const Simd4i& v1)
{
	unsigned int control;
	__vcmpequwR(v0, v1, &control);
	return int(0x20 & ~control); // not all false
}

int simdi::anyEqual(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	unsigned int control;
	outMask = __vcmpequwR(v0, v1, &control);
	return int(0x20 & ~control); // not all false
}

int simdi::allGreater(const Simd4i& v0, const Simd4i& v1)
{
	unsigned int control;
	__vcmpgtswR(v0, v1, &control);
	return int(0x80 & control); // all true
}

int simdi::allGreater(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	unsigned int control;
	outMask = __vcmpgtswR(v0, v1, &control);
	return int(0x80 & control); // all true
}

int simdi::anyGreater(const Simd4i& v0, const Simd4i& v1)
{
	unsigned int control;
	__vcmpgtswR(v0, v1, &control);
	return int(0x20 & ~control); // not all false
}

int simdi::anyGreater(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	unsigned int control;
	outMask = __vcmpgtswR(v0, v1, &control);
	return int(0x20 & ~control); // not all false
}
