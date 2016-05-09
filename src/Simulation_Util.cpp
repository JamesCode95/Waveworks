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
 
#include "Internal.h"
#include "Simulation_Util.h"
#include "D3DX_replacement_code.h"

#define FN_QUALIFIER inline
#define FN_NAME(x) x
#include "Spectrum_Util.h"
#include "Float16_Util.h"

#ifndef __ANDROID__ 
#include <random>
#endif

#ifdef __ANDROID__ 
#include "math.h"
#endif

#if !defined(__GNUC__) && !defined(__ANDROID__)
namespace std { using namespace tr1; }
#define USE_MERSENNE_TWISTER_RNG
#else
#undef USE_MERSENNE_TWISTER_RNG
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
	// Template algo for initializaing various aspects of simulation
	template<class Functor> void for_each_wavevector(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params, const Functor& functor)
	{
		const int dmap_dim = params.fft_resolution;
		const float fft_period = params.fft_period;

		for (int i = 0; i <= dmap_dim; i++)
		{
			// ny is y-coord wave number
			const int ny = (-dmap_dim/2 + i);

			// K is wave-vector, range [-|DX/W, |DX/W], [-|DY/H, |DY/H]
			float2 K;
			K.y = float(ny) * (2 * float(M_PI) / fft_period);

			for (int j = 0; j <= dmap_dim; j++)
			{
				// nx is x-coord wave number
				int nx = (-dmap_dim/2 + j);

				K.x = float(nx) * (2 * float(M_PI) / fft_period);

				functor(i,j,nx,ny,K);
			}
		}
	}

	struct init_omega_functor {

		int dmap_dim;
		float* pOutOmega;

		void operator()(int i, int j, int /* not used nx*/, int /* not used ny*/, float2 K) const {
			// The angular frequency is following the dispersion relation:
			//            omega^2 = g*k
			// So the equation of Gerstner wave is:
			//            x = x0 - K/k * A * sin(dot(K, x0) - sqrt(g * k) * t), x is a 2D vector.
			//            z = A * cos(dot(K, x0) - sqrt(g * k) * t)
			// Gerstner wave means: a point on a simple sinusoid wave is doing a uniform circular motion. The
			// center is (x0, y0, z0), the radius is A, and the circle plane is parallel to K.
			pOutOmega[i * (dmap_dim + 4) + j] = sqrtf(GRAV_ACCEL * sqrtf(K.x * K.x + K.y * K.y));
		}
	};

#ifdef USE_MERSENNE_TWISTER_RNG
	static std::mt19937 g_random_number_generation;

	template <class _Engine>
	float generate_uniform_01(_Engine& _Eng)
	{
		return ((_Eng() - (_Eng.min)()) / ((float)(_Eng.max)() - (float)(_Eng.min)() + 1.f));
	}

#else // using simple and fast XOR SHIFT RNG
	
	unsigned long xor_shift_rand128()
	{
		static uint32_t x = 123456789; // time(0);  
		static uint32_t y = 362436069;
		static uint32_t z = 521288629;
		static uint32_t w = 88675123;
		uint32_t t;
  
		t = x ^ (x << 11);
		x = y; y = z; z = w;
		return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
	}

	struct xorshift_engine 
	{
		unsigned long xor_shift_rand128();
		unsigned long min() { return 0;};
		unsigned long max() { return 4294967295ul;};
		void seed() { return;};
	};

	xorshift_engine g_random_number_generation;

	float generate_uniform_01(xorshift_engine& _Eng)
	{
		return ((float)(xor_shift_rand128() - (_Eng.min)()) / ((float)(_Eng.max)() - (float)(_Eng.min)() + 1.f));
	}

#endif

	// Generating gaussian random number with mean 0 and standard deviation 1.
	float Gauss()
	{
		float u1 = generate_uniform_01(g_random_number_generation);
		float u2 = generate_uniform_01(g_random_number_generation);
		if (u1 < 1e-6f)
			u1 = 1e-6f;
		return sqrtf(-2 * logf(u1)) * cosf(2 * float(M_PI) * u2);
	}

}
namespace GFSDK_WaveWorks_Simulation_Util
{
	void init_omega(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params, float* pOutOmega)
	{
		init_omega_functor f;
		f.dmap_dim = params.fft_resolution;
		f.pOutOmega = pOutOmega;

		for_each_wavevector(params, f);
	}

	void init_gauss( const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& /*params*/, float2* pOutGauss)
	{
		g_random_number_generation.seed();
		for (int i = 0; i <= gauss_map_resolution; i++)
		{
			for (int j = 0; j <= gauss_map_resolution; j++)
			{
				const int ix = i * (gauss_map_resolution + 4) + j;
				pOutGauss[ix].x = Gauss();
				pOutGauss[ix].y = Gauss();
			}
		}
	}

	float get_spectrum_rms_sqr(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params)
	{
		float a = params.wave_amplitude * params.wave_amplitude;
		float v = params.wind_speed;
		float dir_depend = params.wind_dependency;
		float fft_period = params.fft_period;

		float phil_norm = expf(1)/fft_period;	// This normalization ensures that the simulation is invariant w.r.t. units and/or fft_period
		phil_norm *= phil_norm;					// Use the square as we are accumulating RMS

		// We can compute the integral of Phillips over a disc in wave vector space analytically, and by subtracting one
		// disc from the other we can compute the integral for the ring defined by {params.window_in,params.window_out}
		const float lower_k = params.window_in * 2.f * float(M_PI) / fft_period;
		const float upper_k = params.window_out * 2.f * float(M_PI) / fft_period;
		float rms_est = UpperBoundPhillipsIntegral(upper_k, v, a, dir_depend, params.small_wave_fraction) - UpperBoundPhillipsIntegral(lower_k, v, a, dir_depend, params.small_wave_fraction);

		// Normalize to wave number space
		rms_est *= 0.25f*(fft_period*fft_period)/(float(M_PI) * float(M_PI));
		rms_est *= phil_norm;

		return rms_est;
	}

	template<class InputPolicy, class MultiplierPolicy>
	void add_displacements(	const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params,
							const BYTE* pReadbackData,
							UINT rowPitch, 
							const gfsdk_float2* inSamplePoints,
							gfsdk_float4* outDisplacements,
							UINT numSamples,
							const MultiplierPolicy& multiplier
							)
	{
		const int dmap_dim = params.fft_resolution;
		const FLOAT f_dmap_dim = FLOAT(dmap_dim);
		const FLOAT uv_scale = f_dmap_dim / params.fft_period;

		const gfsdk_float2* currSrc = inSamplePoints;
		gfsdk_float4* currDst = outDisplacements;
		for(UINT sample = 0; sample != numSamples; ++sample, ++currSrc, ++currDst)
		{
			// Calculate the UV coords, in texels
			const gfsdk_float2 uv = *currSrc * uv_scale - gfsdk_make_float2(0.5f, 0.5f);
			gfsdk_float2 uv_wrap = gfsdk_make_float2(fmodf(uv.x,f_dmap_dim),fmodf(uv.y,f_dmap_dim));
			if(uv_wrap.x < 0.f)
				uv_wrap.x += f_dmap_dim;
			else if(uv_wrap.x >= f_dmap_dim)
				uv_wrap.x -= f_dmap_dim;
			if(uv_wrap.y < 0.f)
				uv_wrap.y += f_dmap_dim;
			else if(uv_wrap.y >= f_dmap_dim)
				uv_wrap.y -= f_dmap_dim;
			const gfsdk_float2 uv_round = gfsdk_make_float2(floorf(uv_wrap.x),floorf(uv_wrap.y));
			const gfsdk_float2 uv_frac = uv_wrap - uv_round;

			const int uv_x = ((int)uv_round.x) % dmap_dim;
			const int uv_y = ((int)uv_round.y) % dmap_dim;
			const int uv_x_1 = (uv_x + 1) % dmap_dim;
			const int uv_y_1 = (uv_y + 1) % dmap_dim;

			// Ensure we wrap round during the lerp too
			const typename InputPolicy::InputType* pTL = reinterpret_cast<const typename InputPolicy::InputType*>(pReadbackData + uv_y * rowPitch);
			const typename InputPolicy::InputType* pTR = pTL + uv_x_1;
			pTL += uv_x;
			const typename InputPolicy::InputType* pBL = reinterpret_cast<const typename InputPolicy::InputType*>(pReadbackData + uv_y_1 * rowPitch);
			const typename InputPolicy::InputType* pBR = pBL + uv_x_1;
			pBL += uv_x;

			gfsdk_float4 toadd = (1.f - uv_frac.x) * (1.f - uv_frac.y) * InputPolicy::get_float4(pTL);
			toadd             +=        uv_frac.x  * (1.f - uv_frac.y) * InputPolicy::get_float4(pTR);
			toadd             += (1.f - uv_frac.x) *        uv_frac.y  * InputPolicy::get_float4(pBL);
			toadd             +=        uv_frac.x  *        uv_frac.y  * InputPolicy::get_float4(pBR);
			*currDst += multiplier.mult(toadd);
		}
	}

	struct NoMultiplierPolicy
	{
		inline const gfsdk_float4& mult(const gfsdk_float4& val) const { return val; }
	};

	struct ParameterizedMultiplierPolicy
	{
		ParameterizedMultiplierPolicy(float m) :
			m_multiplier(m)
		{
		}

		inline gfsdk_float4 mult(const gfsdk_float4& val) const { return m_multiplier*val; }

		float m_multiplier;
	};

	template<class InputPolicy>
	void add_displacements(	const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params,
							const BYTE* pReadbackData,
							UINT rowPitch, 
							const gfsdk_float2* inSamplePoints,
							gfsdk_float4* outDisplacements,
							UINT numSamples,
							float multiplier
							)
	{
		if(1.f == multiplier)
		{
			// No multiplier required
			add_displacements<InputPolicy,NoMultiplierPolicy>(params,pReadbackData,rowPitch,inSamplePoints,outDisplacements,numSamples,NoMultiplierPolicy());
		}
		else if(0.f != multiplier)
		{
			add_displacements<InputPolicy,ParameterizedMultiplierPolicy>(params,pReadbackData,rowPitch,inSamplePoints,outDisplacements,numSamples,ParameterizedMultiplierPolicy(multiplier));
		}
		else
		{
			// Nothin to add, do nothin
		}
	}

	struct Float16InputPolicy
	{
		struct half4
		{
			gfsdk_U16 _components[4];
		};
		typedef half4 InputType;
		static inline gfsdk_float4 get_float4(const half4* pIn)
		{
			return GFSDK_WaveWorks_Float16_Util::float32x4((gfsdk_U16*)pIn);
		}
	};

	void add_displacements_float16(	const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params,
									const BYTE* pReadbackData,
									UINT rowPitch, 
									const gfsdk_float2* inSamplePoints,
									gfsdk_float4* outDisplacements,
									UINT numSamples,
									float multiplier
									)
	{
		add_displacements<Float16InputPolicy>(params,pReadbackData,rowPitch,inSamplePoints,outDisplacements,numSamples,multiplier);
	}

	struct Float32InputPolicy
	{
		typedef gfsdk_float4 InputType;
		static inline const gfsdk_float4& get_float4(const gfsdk_float4* pIn) { return *pIn; }
	};

	void add_displacements_float32(	const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params,
									const BYTE* pReadbackData,
									UINT rowPitch, 
									const gfsdk_float2* inSamplePoints,
									gfsdk_float4* outDisplacements,
									UINT numSamples,
									float multiplier
									)
	{
		add_displacements<Float32InputPolicy>(params,pReadbackData,rowPitch,inSamplePoints,outDisplacements,numSamples,multiplier);
	}

#if !defined(WAVEWORKS_ENABLE_PROFILING)	// defined in FFT_API_support.h
	void tieThreadToCore(unsigned char core)
	{
        // do nothing
	}
	TickType getTicks()
	{
#if defined(TARGET_PLATFORM_PS4)
		return 0;
#elif defined(TARGET_PLATFORM_NIXLIKE)
        timespec retval = {0,0};
        return retval;
#else
		return 0;
#endif
	}
	float getMilliseconds(const TickType& start, const TickType& stop)
	{
		return 0;
	}
#elif defined(TARGET_PLATFORM_PS4)
	void tieThreadToCore(unsigned char core)
	{
        // do nothing
	}

	TickType getTicks()
	{
		return __builtin_readcyclecounter();
	}

	float	getMilliseconds(const TickType& start, const TickType& stop)
	{
		return float(stop - start) * 1000.f/1600000000.f;	// Based on 1.6GHz clocks
	}

#elif defined(TARGET_PLATFORM_NIXLIKE)
	void tieThreadToCore(unsigned char /*core*/)
	{
		// Enable this to WAR on systems that have core-sensitive QueryPerformanceFrequency
        // SetThreadAffinityMask( GetCurrentThread(), 1<<core );
	}

    // A somewhat vile WAR to make sure we can compile on older Linuxes, however in our defense
    // we do check for _RAW support by relaxing down to no-_RAW if a call to clock_gettime() fails
    #ifndef CLOCK_MONOTONIC_RAW
    #define CLOCK_MONOTONIC_RAW 4
    #endif

#if defined(TARGET_PLATFORM_MACOSX)
    #include <mach/mach_time.h>
	TickType getTicks()
	{
        mach_timebase_info_data_t timebase;
        timespec currTime;
        uint64_t clock;
        uint64_t nano;
        clock = mach_absolute_time();
        mach_timebase_info(&timebase); // should better use it once to get numer/denom
        nano = clock * (uint64_t)timebase.numer / (uint64_t)timebase.denom;
        currTime.tv_sec = nano / 1000000000L;
        currTime.tv_nsec = nano % 1000000000L;
        return currTime;
	}
#else
 	TickType getTicks()
	{
        static int clk_id = CLOCK_MONOTONIC_RAW;
        timespec currTime;
        if(clock_gettime(clk_id, &currTime))
        {
            clk_id = CLOCK_MONOTONIC;
            clock_gettime(clk_id, &currTime);
        }
        return currTime;
	}
#endif
	
	float	getMilliseconds(const TickType& start, const TickType& stop)
	{
        timespec x = stop;
        timespec y = start;

        /* Perform the carry for the later subtraction by updating y. */
        if (x.tv_nsec < y.tv_nsec) {
         long numsec = (y.tv_nsec - x.tv_nsec) / 1000000000 + 1;
         y.tv_nsec -= 1000000000 * numsec;
         y.tv_sec += numsec;
        }
        if (x.tv_nsec - y.tv_nsec > 1000000000) {
         long numsec = (x.tv_nsec - y.tv_nsec) / 1000000000;
         y.tv_nsec += 1000000000 * numsec;
         y.tv_sec -= numsec;
        }

        /* Compute the time remaining to wait.
          tv_nsec is certainly positive. */
        timespec diff;
        diff.tv_sec = x.tv_sec - y.tv_sec;
        diff.tv_nsec = x.tv_nsec - y.tv_nsec;

        return float(1000. * ((double) diff.tv_sec + 0.000000001 * (double) diff.tv_nsec));
	}
#else // !WAVEWORKS_ENABLE_PROFILING && !TARGET_PLATFORM_NIXLIKE
	void tieThreadToCore(unsigned char /*core*/)
	{
		// Enable this to WAR on systems that have core-sensitive QueryPerformanceFrequency
        // SetThreadAffinityMask( GetCurrentThread(), 1<<core );
	}

	TickType getTicks()
	{
		LARGE_INTEGER c;
		QueryPerformanceCounter(&c);
		return c.QuadPart;
	}
	
	float	getMilliseconds(const TickType& start, const TickType& stop)
	{
		static bool first_time=true;
		static LARGE_INTEGER f;
		if(first_time)
		{
			QueryPerformanceFrequency(&f);
			first_time=false;
		}
		// clamping timestamp 
		TickType clampedStop = stop < start ? start : stop;
		return (float)((double)(clampedStop - start)/(double)(f.QuadPart/1000.0));
	}
#endif
}
