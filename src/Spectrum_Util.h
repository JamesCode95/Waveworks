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

#ifndef _NVWAVEWORKS_SPECTRUM_UTIL_H
#define _NVWAVEWORKS_SPECTRUM_UTIL_H

#ifndef GRAV_ACCEL
#define GRAV_ACCEL	9.810f	// The acceleration of gravity, m/s^2
#endif

#ifndef PI
#define PI 3.1415926536f
#endif

// Phillips Spectrum
// K: normalized wave vector, W: wind direction, v: wind velocity, a: constant affecting factor
FN_QUALIFIER float FN_NAME(Phillips)(float2 K, float2 W, float v, float a, float dir_depend, float small_wave_fraction)
{
	// largest possible wave from constant wind of velocity v
	float l = v * v / GRAV_ACCEL;
	// damp out waves with very small length w << l
	float w = small_wave_fraction * l;

	float Ksqr = K.x * K.x + K.y * K.y;
	float Kcos = K.x * W.x + K.y * W.y;
	float phillips = a * expf(-1 / (l * l * Ksqr)) / (Ksqr * Ksqr * Ksqr) * (Kcos * Kcos);

	// filter out waves moving opposite to wind
	if (Kcos < 0)
		phillips *= (1.0f-dir_depend);

	// damp out waves with very small length w << l
	return phillips * expf(-Ksqr * w * w);
}

// Upper-bound estimate of integral of Phillips Spectrum power over disc-shaped 2D wave vector space of radius k centred on K = {0,0}
// There is no wind velocity parameter, since the integral is rotationally invariant
//
FN_QUALIFIER float FN_NAME(UpperBoundPhillipsIntegral)(float k, float v, float a, float dir_depend, float /*small_wave_fraction*/)
{
	if(k <= 0.f) return 0.f;

	// largest possible wave from constant wind of velocity v
	float l = v * v / GRAV_ACCEL;

	// integral has analytic form, yay!
	float phillips_integ = 0.5f * PI * a * l * l * expf(-1.f/(k*k*l*l));

	// dir_depend affects half the domain
	phillips_integ *= (1.0f-0.5f*dir_depend);

	// we may safely ignore 'small_wave_fraction' for an upper-bound estimate
	return phillips_integ;
}

// Rectangular window, parameterised by in (i) and out (o) thresholds
FN_QUALIFIER float FN_NAME(RectWindow)(float r, float i, float o)
{
	if(r < i)
	{
		return 0.f;
	}
	else if(r < o)
	{
		return 1.f;
	}
	else
	{
		return 0.f;
	}
}

FN_QUALIFIER float FN_NAME(CalcH0)(	int nx, int ny,
									float2 K,
									float window_in, float window_out,
									float2 wind_dir, float v, float dir_depend,
									float a,
									float norm,
									float small_wave_fraction
									)
{
	// distance from DC, in wave-numbers
	float nr = sqrtf(float(nx*nx)+float(ny*ny));
	float window = sqrtf(FN_NAME(RectWindow)(nr, window_in, window_out));

	float amplitude = (K.x == 0 && K.y == 0) ? 0 : sqrtf(FN_NAME(Phillips)(K, wind_dir, v, a, dir_depend, small_wave_fraction));
	amplitude *= norm;
	amplitude *= window;
	amplitude *= 0.7071068f;

	return amplitude;
}

#endif //_NVWAVEWORKS_SPECTRUM_UTIL_H
