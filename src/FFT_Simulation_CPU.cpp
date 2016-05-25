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

/* 
 * CPU simulations performs Update Philips spectrum, computes three backward FFT
 * and combines result into one 2D texture with choppy and height.
 * All cascades simulations are performed as bunch of simple tasks in working threads 
 * that are parallel to user thread(rendering thread). The last call to updateNonCompute
 * waits to completion of all tasks and pauses working threads. Then unmaps textures for
 * all cascades and flips textures with followed locking of next textures. Then main thread
 * starts working threads and returns to the user. So user code is executed in parallel to
 * working threads that are filling mapped textures while unmapped textures can be retrived
 * by user and can be rendered safely.
 * All working threads pull tasks from queue and executes task. There 3 types of tasks:
 * 1) Update spectrum takes one scan-line of a spectrum and fills 3 scan-lines for three FFTs
 * 2) Backward FFT is performed by using Cooley-Tuckey FFT algorithm
 * 3) Update texture is done by merge three results of FFT into one texture
 * No device or context methods are called from threads - safe solution
 * Tasks is very small (except FFT) so load balancing is nice as well as scalability
 */

#include "Internal.h"

#ifdef SUPPORT_FFTCPU
#include "FFT_Simulation_CPU_impl.h"
#include "Simulation_Util.h"
#include "Graphics_Context.h"

#define FN_QUALIFIER inline
#define FN_NAME(x) x
#include "Spectrum_Util.h"
#include "Float16_Util.h"
#include "CircularFIFO.h"

#include <string.h>

#include "simd/Simd4f.h"
#include "simd/Simd4i.h"

using namespace sce;

#ifndef SAFE_ALIGNED_FREE
	#define SAFE_ALIGNED_FREE(p) { if(p) { NVSDK_aligned_free(p);   (p)=NULL; } }
#endif    

//------------------------------------------------------------------------------------
//Fast sincos from AMath library: Approximated Math from Intel. License rules allow to use this code for our purposes

#ifndef PI
#define PI			(3.14159265358979323846f)
#endif

namespace
{
	typedef Simd4fFactory<detail::FourTuple> Simd4fConstant;

	const Simd4fConstant DP1_PS = simd4f(-0.78515625);
	const Simd4fConstant DP2_PS = simd4f(-2.4187564849853515625e-4);
	const Simd4fConstant DP3_PS = simd4f(-3.77489497744594108e-8);
	const Simd4fConstant COSCOF_P0_PS = simd4f(2.443315711809948E-005);
	const Simd4fConstant COSCOF_P1_PS = simd4f(-1.388731625493765E-003);
	const Simd4fConstant COSCOF_P2_PS = simd4f(4.166664568298827E-002);
	const Simd4fConstant SINCOF_P0_PS = simd4f(-1.9515295891E-4);
	const Simd4fConstant SINCOF_P1_PS = simd4f(8.3321608736E-3);
	const Simd4fConstant SINCOF_P2_PS = simd4f(-1.6666654611E-1);

	const Simd4fConstant ONE_PS = simd4f(1.0f);
	const Simd4fConstant HALF_PS = simd4f(0.5f);
	const Simd4fConstant FOUR_OVER_PI_PS = simd4f(4 / PI);
	const Simd4fConstant TWO_PI_PS = simd4f(2 * PI);

	typedef Simd4iFactory<detail::FourTuple> Simd4iConstant;

	const Simd4iConstant ONE_PI32 = simd4i(1);
	const Simd4iConstant TWO_PI32 = simd4i(2);
	const Simd4iConstant FOUR_PI32 = simd4i(4);
	const Simd4iConstant INVONE_PI32 = simd4i(~1);
}

//4 components fast approximated sin and cos computation
inline void sincos_ps(Simd4f x, Simd4f* s, Simd4f* c)
{
	// extract the sign bit
    Simd4f sign_bit_x = x & simd4f(_sign);
    // take the absolute value
    x = x ^ sign_bit_x;
    Simd4f y = x * FOUR_OVER_PI_PS;
    // truncate to integer
    Simd4i emm2 = truncate(y);
    // j = (j+1) & ~1 (see the cephes sources)
    emm2 = simdi::operator+(emm2, ONE_PI32) & INVONE_PI32;
    y = convert(emm2);

	// get signs for sine and cosine
	Simd4f sign_bit_sin = simd4f((FOUR_PI32 & emm2) << 29);
	sign_bit_sin = sign_bit_sin ^ sign_bit_x;
	Simd4i emm4 = simdi::operator-(emm2, TWO_PI32);
	Simd4f sign_bit_cos = simd4f((FOUR_PI32 & ~emm4) << 29);

	// get the polynomial selection mask:
    // there is one polynomial for 0 <= x <= Pi/4 and another one for Pi/4<x<=Pi/2
    // both branches will be computed
    emm2 = simdi::operator==(emm2 & TWO_PI32, simd4i(_0));
    Simd4f poly_mask = simd4f(emm2);

    // the magic pass: "Extended precision modular arithmetic"
    // x = ((x - y * DP1) - y * DP2) - y * DP3
    x = x + y * DP1_PS + y * DP2_PS + y * DP3_PS;
	Simd4f z = x * x;

    // evaluate the first polynomial (0 <= x <= Pi/4)
    Simd4f y1 = COSCOF_P0_PS;
    y1 = y1 * z + COSCOF_P1_PS;
    y1 = y1 * z + COSCOF_P2_PS;
    y1 = y1 * z * z - z * HALF_PS + ONE_PS;

    // evaluate the second polynomial (Pi/4 <= x <= 0)
    Simd4f y2 = SINCOF_P0_PS;
    y2 = y2 * z + SINCOF_P1_PS;
	y2 = y2 * z + SINCOF_P2_PS;
    y2 = y2 * z * x + x;

	// select the correct result from the two polynomials
	Simd4f xmm1 = select(poly_mask, y2, y1);
	Simd4f xmm2 = y1 ^ y2 ^ xmm1; // select(poly_mask, y1, y2);


    // update the sign
    *s = xmm1 ^ sign_bit_sin;
    *c = xmm2 ^ sign_bit_cos;
}

//   Gets integer log2 of v and puts it to m, also sets twopm=2^m 
void Powerof2(int v, int *m, int *twopm)
{
	int nn = 1;
	int mm=0;
	while(nn<v)
	{
		nn<<=1;
		++mm;
	}
	*m = mm;
	*twopm = nn;
}


// Performs a 1D FFT inplace given x- interleaved real/imaginary array of data
// FFT2D (non-SIMD code) is left here in case we need compatibility with non-SIMD CPUs
void FFTc(unsigned int m, float *x)
{
   // Calculate the number of points 
   unsigned int nn = 1u << m;

   // Do the bit reversal 
   unsigned int i2 = nn >> 1;
   unsigned int j = 0; 
   for (unsigned int i=0; i<nn-1; ++i) 
   {
      if (i < j) 
	  {
         float tx = x[i*2];
         float ty = x[i*2+1];
         x[i*2] = x[j*2];
         x[i*2+1] = x[j*2+1];
         x[j*2] = tx;
         x[j*2+1] = ty;
      }
      unsigned int k = i2;
      while (k <= j) 
	  {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   // Compute the FFT 
   float c1 = -1.0f;
   float c2 = 0.0f;
   unsigned int l2 = 1;
   for (unsigned int l=0; l<m; ++l) 
   {
      unsigned int l1 = l2;
      l2 <<= 1;
      float u1 = 1.0f;
      float u2 = 0.0f;
      for (unsigned int j=0; j<l1; ++j) 
	  {
         for (unsigned int i=j; i<nn; i+=l2) 
		 {
            unsigned int i1 = i + l1;
            float t1 = u1 * x[i1*2] - u2 * x[i1*2+1];
            float t2 = u1 * x[i1*2+1] + u2 * x[i1*2];
            x[i1*2] = x[i*2] - t1;
            x[i1*2+1] = x[i*2+1] - t2;
            x[i*2] += t1;
            x[i*2+1] += t2;
         }
         float z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0f - c1) * 0.5f);
      c1 = sqrt((1.0f + c1) * 0.5f);
   }
}

//   Performs a 1D FFT inplace given x- interleaved real/imaginary array of data,
//   data is aligned to 16bytes, data is arranged the following way: 
//   real0,real1,real2,real3,imag0,imag1,imag2,imag3,real4,real5,real6,real7,imag4,imag5,imag6,imag7, etc

void FFTcSIMD(unsigned int m, float *x)
{
	// Calculate the number of points 
	unsigned int nn = 1u << m;

	// Do the bit reversal 
	unsigned int i2 = nn >> 1;
	unsigned int j = 0; 
	for (unsigned int i=0; i<nn-1; ++i) 
	{
		if (i < j) 
		{
			Simd4f tx = loadAligned(x, i*32);
			Simd4f ty = loadAligned(x, i*32+16);
			storeAligned(x, i*32, loadAligned(x, j*32));
			storeAligned(x, i*32+16, loadAligned(x, j*32+16));
			storeAligned(x, j*32, tx);
			storeAligned(x, j*32+16, ty);
		}
		unsigned int k = i2;
		while (k <= j) 
		{
			j -= k;
			k >>= 1;
		}
		j += k;
	}

   // Compute the FFT 
   Simd4f c1 = simd4f(-1.0f);									//c1= -1.0f;
   Simd4f c2 = simd4f(_0);									//c2 = 0.0f;
   unsigned int l2 = 1;
   for (unsigned int l=0; l<m; ++l) 
   {
      unsigned int l1 = l2;
      l2 <<= 1;
      Simd4f u1 = simd4f(_1);								//u1 = 1.0f;
      Simd4f u2 = simd4f(_0);								//u2 = 0.0f;
      for (unsigned int j=0; j<l1; ++j) 
	  {
         for (unsigned int i=j; i<nn; i+=l2) 
		 {
            unsigned int i1 = i + l1;

			Simd4f tmp1 = loadAligned(x, i1*32);
			Simd4f tmp2 = loadAligned(x, i1*32+16);					

			Simd4f t1 = u1 * tmp1 - u2 * tmp2;						//t1 = u1 * x[i1*2] - u2 * x[i1*2+1];
			Simd4f t2 = u1 * tmp2 + u2 * tmp1;						//t2 = u1 * x[i1*2+1] + u2 * x[i1*2];

			tmp1 = loadAligned(x, i*32);
			tmp2 = loadAligned(x, i*32+16);					

			storeAligned(x, i1*32, tmp1 - t1);						//x[i1*2] = x[i*2] - t1;
			storeAligned(x, i1*32+16, tmp2 - t2);					//x[i1*2+1] = x[i*2+1] - t2;
            storeAligned(x, i*32, tmp1 + t1);						//x[i*2] += t1;
            storeAligned(x, i*32+16, tmp2 + t2);					//x[i*2+1] += t2;
         }
		 Simd4f z = u1 * c1 - u2 * c2;								//z =  u1 * c1 - u2 * c2;
		 u2 = u1 * c2 + u2 * c1;									//u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
	  c2 = sqrt(HALF_PS - c1 * HALF_PS);							//c2 = sqrt((1.0f - c1) / 2.0f);
	  c1 = sqrt(HALF_PS + c1 * HALF_PS);							//c1 = sqrt((1.0f + c1) / 2.0f);
   }
}

void FFT1DSIMD_X_4wide(complex *c, int nx)
{
	NVMATH_ALIGN(16, float) iv_data[512 * 2 * 4];

	int m, twopm;
	Powerof2(nx,&m,&twopm);

	float* f0 = c[0*nx];
	float* f1 = c[1*nx];
	float* f2 = c[2*nx];
	float* f3 = c[3*nx];
	for(int i = 0; i < nx; ++i)
	{
		storeAligned(iv_data, i*32, simd4f(f0[0], f1[0], f2[0], f3[0]));
		storeAligned(iv_data, i*32+16, simd4f(f0[1], f1[1], f2[1], f3[1]));
		f0+=2;	
		f1+=2;
		f2+=2;
		f3+=2;
	}

	FFTcSIMD(m, iv_data);

	for(int i = 0; i < nx; ++i)
	{
		float* f0 = c[0*nx + i];
		float* f1 = c[1*nx + i];
		float* f2 = c[2*nx + i];
		float* f3 = c[3*nx + i];

		float* r = iv_data + i*8;
		f0[0] = r[0];
		f0[1] = r[4];
		f1[0] = r[1];
		f1[1] = r[5];
		f2[0] = r[2];
		f2[1] = r[6];
		f3[0] = r[3];
		f3[1] = r[7];	   
	}
}

void FFT1DSIMD_Y_4wide(complex *c, int nx)
{
	NVMATH_ALIGN(16, float) iv_data[512 * 2 * 4];

	int m, twopm;
	Powerof2(nx,&m,&twopm);

	for(int i = 0; i < nx; ++i)
	{
		Simd4f tmp0 = loadAligned(c[i*nx + 0]);
		Simd4f tmp1 = loadAligned(c[i*nx + 2]);
		unzip(tmp0, tmp1);
		storeAligned(iv_data, i*32, tmp0);
		storeAligned(iv_data, i*32+16, tmp1);
	}

	FFTcSIMD(m, iv_data);

	for(int i = 0; i < nx; i+=4)
	{
		float* f0 = c[(i+0)*nx];
		float* f1 = c[(i+1)*nx];
		float* f2 = c[(i+2)*nx];
		float* f3 = c[(i+3)*nx];

		float* r0 = iv_data + i*8 +  0;
		float* r1 = iv_data + i*8 +  8;
		float* r2 = iv_data + i*8 + 16;
		float* r3 = iv_data + i*8 + 24;
		   
		f0[0] = r0[0];
		f0[1] = r0[4];
		f0[2] = r0[1];
		f0[3] = r0[5];
		f0[4] = r0[2];
		f0[5] = r0[6];
		f0[6] = r0[3];
		f0[7] = r0[7];

		f1[0] = r1[0];
		f1[1] = r1[4];
		f1[2] = r1[1];
		f1[3] = r1[5];
		f1[4] = r1[2];
		f1[5] = r1[6];
		f1[6] = r1[3];
		f1[7] = r1[7];

   		f2[0] = r2[0];
		f2[1] = r2[4];
		f2[2] = r2[1];
		f2[3] = r2[5];
		f2[4] = r2[2];
		f2[5] = r2[6];
		f2[6] = r2[3];
		f2[7] = r2[7];

   		f3[0] = r3[0];
		f3[1] = r3[4];
		f3[2] = r3[1];
		f3[3] = r3[5];
		f3[4] = r3[2];
		f3[5] = r3[6];
		f3[6] = r3[3];
		f3[7] = r3[7];
	}
}

//   Perform a 2D FFT inplace given a complex 2D array
//   The size of the array (nx,nx)
void FFT2DSIMD(complex *c, int nx)
{
   for (int j=0; j<nx; j+=4) 
   {
	   FFT1DSIMD_X_4wide(c+j*nx, nx);
   }
   
   for (int j=0; j<nx; j+=4) 
   {
	   FFT1DSIMD_Y_4wide(c+j, nx);
   }
}

//   Perform a 2D FFT inplace given a complex 2D array
//   The size of the array (nx,nx)
//   FFT2D (non-SIMD code) is left here in case we need compatibility with non-SIMD CPUs
void FFT2D(complex *c,int nx)
{
   int i,j;
   int m, twopm;
   float tre, tim;

   Powerof2(nx,&m,&twopm);

   for (j=0;j<nx;j++) 
   {
	   FFTc(m,(float *)&c[j*nx]);
   }
   
   // 2D matrix transpose
   for (i=0;i<nx-1;i++) 
   {
      for (j=i+1;j<nx;j++) 
	  {
		  tre = c[(j*nx+i)][0];
		  tim = c[(j*nx+i)][1];
    	  c[(j*nx+i)][0] = c[(i*nx+j)][0];
    	  c[(j*nx+i)][1] = c[(i*nx+j)][1];
    	  c[(i*nx+j)][0] = tre;
    	  c[(i*nx+j)][1] = tim;
      }
   }
   // doing 1D FFT for rows
   for (j=0;j<nx;j++) 
   {
	   FFTc(m,(float *)&c[j*nx]);
   }

   // 2D matrix transpose
   for (i=0;i<nx-1;i++) 
   {
      for (j=i+1;j<nx;j++) 
	  {
		  tre = c[(j*nx+i)][0];
		  tim = c[(j*nx+i)][1];
    	  c[(j*nx+i)][0] = c[(i*nx+j)][0];
    	  c[(j*nx+i)][1] = c[(i*nx+j)][1];
    	  c[(i*nx+j)][0] = tre;
    	  c[(i*nx+j)][1] = tim;
      }
   }
}

//Updates Ht to desired time. Each call computes one scan line from source spectrum into 3 textures
bool NVWaveWorks_FFT_Simulation_CPU_Impl::UpdateHt(int row)
{
	// here is a port of ComputeShader version of update spectrum with various optimizations:
	// preprocessing of coefficients moved to m_sqrt_table that removes sqrt and some other math but introduces memory access
	// but this is faster
	int N = m_params.fft_resolution;
	int width = N + 4;
	int index = row * width;

	float* omega_ptr = m_omega_data + index;
	float2* h0i_ptr  = m_h0_data + index;
	float2* h0j_ptr = m_h0_data + N * (width + 1) - index - 1; // mirrored h0i, not aligned
	float* sqt = m_sqrt_table + row*N;
	float* out0 = m_fftCPU_io_buffer[N*row];
	float* out1 = m_fftCPU_io_buffer[N*(N+row)];
	float* out2 = m_fftCPU_io_buffer[N*(N+N+row)];

	//some iterated values
	float kx = -0.5f * N;
	float ky = kx + row;
	Simd4f ky01 = simd4f( -ky, ky, -ky, ky);
	Simd4f kx0 = simd4f( -(kx+0.0f), kx+0.0f, -(kx+1.0f), kx+1.0f );
	Simd4f kx1 = simd4f( -(kx+2.0f), kx+2.0f, -(kx+3.0f), kx+3.0f );
	Simd4f kxinc = simd4f( -4.0f, 4.0f, -4.0f, 4.0f );
	
	double dt = m_doubletime/6.28318530718;

	//perform 4 pixels simultaneously
	for(int i=0; i<int(N); i+=4)
	{
		double odt0 = omega_ptr[i+0]*dt;
		double odt1 = omega_ptr[i+1]*dt;
		double odt2 = omega_ptr[i+2]*dt;
		double odt3 = omega_ptr[i+3]*dt;

		odt0 -= int(odt0);
		odt1 -= int(odt1);
		odt2 -= int(odt2);
		odt3 -= int(odt3);

		Simd4f omega = simd4f(float(odt0), float(odt1), float(odt2), float(odt3));
		Simd4f sin, cos;
		sincos_ps(omega * TWO_PI_PS, &sin, &cos);

		Simd4f h01j = swaphilo(load(&h0j_ptr[-i-0].x));
		Simd4f h32j = swaphilo(load(&h0j_ptr[-i-2].x));

		Simd4f h01i = loadAligned(&h0i_ptr[i+0].x);
		Simd4f h23i = loadAligned(&h0i_ptr[i+2].x);

		Simd4f sx = h01i + h01j;
		Simd4f sy = h23i + h32j;
		unzip(sx, sy);
		Simd4f hx = sx * cos - sy * sin;

		Simd4f dx = h01i - h01j;
		Simd4f dy = h23i - h32j;
		unzip(dx, dy);
		Simd4f hy = dx * sin + dy * cos;

		// Ht
		Simd4f h01 = hx;
		Simd4f h23 = hy;
		zip(h01, h23);
		storeAligned(out0, i*8, h01);
		storeAligned(out0, i*8+16, h23);

		// Dt_x, Dt_y
		Simd4f ss = loadAligned(sqt, i*4);
		Simd4f d01 = hy * ss;
		Simd4f d23 = hx * ss; // hx and hy are reversed intentionally
		zip(d01, d23);
		storeAligned(out1, i*8, kx0 * d01);
		storeAligned(out1, i*8+16, kx1 * d23);
		storeAligned(out2, i*8, ky01 * d01);
		storeAligned(out2, i*8+16, ky01 * d23);

		kx0 = kx0 + kxinc;
		kx1 = kx1 + kxinc;
	}

	//did we finish all scan lines of this cascade?
	LONG remainingLines = InterlockedDecrement( &m_ref_count_update_ht );
	assert(remainingLines>=0);
	return remainingLines<=0;
}

// Update H0 to latest parameters
bool NVWaveWorks_FFT_Simulation_CPU_Impl::UpdateH0(int row)
{
	// TODO: SIMD please!

	int N = m_params.fft_resolution;

	const int ny = (-N/2 + row);
	const float ky = float(ny) * (2.f * PI / m_params.fft_period);

	float2 wind_dir;
	float wind_dir_len = sqrtf(m_params.wind_dir.x*m_params.wind_dir.x + m_params.wind_dir.y*m_params.wind_dir.y);
	wind_dir.x = m_params.wind_dir.x / wind_dir_len;
	wind_dir.y = m_params.wind_dir.y / wind_dir_len;
	float a = m_params.wave_amplitude * m_params.wave_amplitude;	// Use square of amplitude, because Phillips is an *energy* spectrum
	float v = m_params.wind_speed;
	float dir_depend = m_params.wind_dependency;

	int dmap_dim = m_params.fft_resolution;
	int inout_width = (dmap_dim + 4);
	float fft_period = m_params.fft_period;

	float fft_norm = 1.f/powf(float(dmap_dim),0.25f);	// TBD: I empirically determined that dim^0.25 is required to
														// make the results independent of dim, but why? (JJ)

	float phil_norm = expf(1)/fft_period;	// This normalization ensures that the simulation is invariant w.r.t. units and/or fft_period

	float norm = fft_norm * phil_norm;

	float2* outH0 = &m_h0_data[inout_width*row];

	// Generate an index into the linear gauss map, which has a fixed size of 512,
	//	using the X Y coordinate of the H0 map lookup.  We also need to apply an offset
	//	so that the lookup coordinate will be centred on the gauss map, of a size equal
	//	to that of the H0 map.
	int gauss_row_size = (gauss_map_resolution + 4);
	int gauss_offset = (gauss_row_size - inout_width)/2;
	int gauss_index = (gauss_offset+row) * gauss_row_size + gauss_offset;
	const float2* inGauss = &m_gauss_data[gauss_index];

	for(int i=0; i<=int(N); ++i)	// NB: <= because the h0 wave vector space needs to be inclusive for the ht calc
	{
		const int nx = (-N/2 + i);
		const float kx = float(nx) * (2.f * PI / m_params.fft_period);

		float2 K;
		K.x = kx;
		K.y = ky;

		float amplitude = FN_NAME(CalcH0)(	nx, ny,
											K,
											m_params.window_in, m_params.window_out,
											wind_dir, v, dir_depend,
											a, norm,
											m_params.small_wave_fraction
											);

		outH0[i].x = amplitude * inGauss[i].x;
		outH0[i].y = amplitude * inGauss[i].y;
	}

	//did we finish all scan lines of this cascade?
	LONG remainingLines = InterlockedDecrement( &m_ref_count_update_h0 );
	assert(remainingLines>=0);
	return remainingLines<=0;
}

enum { NumRowcolInFFTTask = 4 };

int NVWaveWorks_FFT_Simulation_CPU_Impl::GetNumRowsIn_FFT_X() const
{
	return m_params.fft_resolution/(4*NumRowcolInFFTTask);
}

int NVWaveWorks_FFT_Simulation_CPU_Impl::GetNumRowsIn_FFT_Y() const
{
	return m_params.fft_resolution/(4*NumRowcolInFFTTask);
}

bool NVWaveWorks_FFT_Simulation_CPU_Impl::ComputeFFT_XY_NxN(int index)
{
	int N = m_params.fft_resolution;
	//FFT2D (non-SIMD code) is left here in case we need compatibility with non-SIMD CPUs
	//FFT2D(&m_fftCPU_io_buffer[index*N*N],N);
	FFT2DSIMD(&m_fftCPU_io_buffer[index*N*N],N);

	//did we finish all 3 FFT tasks? Track via the x-count...
	LONG remainingFFTs_X = customInterlockedSubtract( &m_ref_count_FFT_X,N);
	if(0 == remainingFFTs_X)
	{
		// Ensure that the Y count and X count reach zero at the same time, for consistency
		m_ref_count_FFT_Y = 0;
	}
	assert(remainingFFTs_X>=0);
	return remainingFFTs_X<=0;
}

bool NVWaveWorks_FFT_Simulation_CPU_Impl::ComputeFFT_X(int XYZindex, int subIndex)
{
	int N = m_params.fft_resolution;

	for(int sub_row = 0; sub_row != NumRowcolInFFTTask; ++sub_row)
	{
		int row_index = (NumRowcolInFFTTask*subIndex)+sub_row;
		FFT1DSIMD_X_4wide(&m_fftCPU_io_buffer[XYZindex*N*N + 4*row_index*N],N);
	}

	//did we finish all 3*N FFT_X tasks?
	LONG remainingFFTs = customInterlockedSubtract(&m_ref_count_FFT_X,NumRowcolInFFTTask);
	assert(remainingFFTs>=0);
	return remainingFFTs<=0;
}

bool NVWaveWorks_FFT_Simulation_CPU_Impl::ComputeFFT_Y(int XYZindex, int subIndex)
{
	int N = m_params.fft_resolution;

	for(int sub_col = 0; sub_col != NumRowcolInFFTTask; ++sub_col)
	{
		int col_index = (NumRowcolInFFTTask*subIndex)+sub_col;
		FFT1DSIMD_Y_4wide(&m_fftCPU_io_buffer[XYZindex*N*N + 4*col_index],N);
	}

	//did we finish all 3*N FFT_Y tasks?
	LONG remainingFFTs = customInterlockedSubtract(&m_ref_count_FFT_Y,NumRowcolInFFTTask);
	assert(remainingFFTs>=0);
	return remainingFFTs<=0;
}


inline void float16x4(gfsdk_U16* __restrict out, const Simd4f in)
{
	GFSDK_WaveWorks_Float16_Util::float16x4(out,in);
}

//Merge all 3 results of FFT into one texture with Dx,Dz and height
bool NVWaveWorks_FFT_Simulation_CPU_Impl::UpdateTexture(int row)
{
	int N = m_params.fft_resolution;
	gfsdk_U16* pTex = reinterpret_cast<gfsdk_U16*>(m_mapped_texture_ptr + row * m_mapped_texture_row_pitch);
	gfsdk_float4* pRb = &m_readback_buffer[m_mapped_texture_index][row*N];
	complex* fftRes = & ((complex*)m_fftCPU_io_buffer) [row*N];
	Simd4f s[2];
	float choppy_scale = m_params.choppy_scale;
	s[   row&1 ] = simd4f(  choppy_scale,  choppy_scale,  1.0f, 1.0f);
	s[1-(row&1)] = simd4f( -choppy_scale, -choppy_scale, -1.0f, 1.0f);

	for(int x = 0; x<N; x+=4, pTex+=16, pRb+=4, fftRes+=4)
	{
		Simd4f h0 = loadAligned(fftRes[N*N*0]), h1 = loadAligned(fftRes[N*N*0], 16);
		Simd4f x0 = loadAligned(fftRes[N*N*1]), x1 = loadAligned(fftRes[N*N*1], 16);
		Simd4f y0 = loadAligned(fftRes[N*N*2]), y1 = loadAligned(fftRes[N*N*2], 16);
		Simd4f e0 = simd4f(_1), e1 = simd4f(_1);

		transpose(x0, y0, h0, e0);
		transpose(x1, y1, h1, e1);
		
		Simd4f a0 = x0 * s[0];
		Simd4f a1 = h0 * s[1];
		Simd4f a2 = x1 * s[0];
		Simd4f a3 = h1 * s[1];

		float16x4( pTex +  0, a0 );
		float16x4( pTex +  4, a1 );
		float16x4( pTex +  8, a2 );
		float16x4( pTex + 12, a3 );

		if(m_params.readback_displacements)
		{
			storeAligned( (float*)pRb    , a0 );
			storeAligned( (float*)pRb, 16, a1 );
			storeAligned( (float*)pRb, 32, a2 );
			storeAligned( (float*)pRb, 48, a3 );
		}
	}

	LONG refCountMerge = InterlockedDecrement( &m_ref_count_update_texture );
	assert(refCountMerge>=0);
	return refCountMerge<=0;
}

NVWaveWorks_FFT_Simulation_CPU_Impl::NVWaveWorks_FFT_Simulation_CPU_Impl(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params) :
	m_next_params(params),
	m_params(params)
{
	m_params_are_dirty = false;

    memset(&m_d3d, 0, sizeof(m_d3d));
    m_d3dAPI = nv_water_d3d_api_undefined;

	m_gauss_data = 0;
	m_h0_data = 0;
	m_omega_data = 0;
	m_fftCPU_io_buffer = 0;
	m_mapped_texture_index = 0;
	m_mapped_texture_ptr = 0;
	m_mapped_texture_row_pitch = 0;
	m_sqrt_table = 0;
	m_readback_buffer[0] = 0;
	m_readback_buffer[1] = 0;
	m_active_readback_buffer = 0;

	m_pReadbackFIFO = NULL;

	m_H0UpdateRequired = true;
	m_DisplacementMapVersion = GFSDK_WaveWorks_InvalidKickID;
	m_pipelineNextReinit = false;
}

NVWaveWorks_FFT_Simulation_CPU_Impl::~NVWaveWorks_FFT_Simulation_CPU_Impl()
{
    releaseAll();
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::initD3D11(ID3D11Device* D3D11_ONLY(pD3DDevice))
{
#if WAVEWORKS_ENABLE_D3D11
    HRESULT hr;

    if(nv_water_d3d_api_d3d11 != m_d3dAPI)
    {
        releaseAll();
    }
    else if(m_d3d._11.m_pd3d11Device != pD3DDevice)
    {
        releaseAll();
    }
    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_d3d11;
        m_d3d._11.m_pd3d11Device = pD3DDevice;
        m_d3d._11.m_pd3d11Device->AddRef();
        V_RETURN(allocateAllResources());
    }
    return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::initGnm()
{
#if WAVEWORKS_ENABLE_GNM
	HRESULT hr;

	if(nv_water_d3d_api_gnm != m_d3dAPI)
	{
		releaseAll();
	}
	if(nv_water_d3d_api_undefined == m_d3dAPI)
	{
		m_d3dAPI = nv_water_d3d_api_gnm;
		V_RETURN(allocateAllResources());
	}
	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::initGL2(void* GL_ONLY(pGLContext))
{
#if WAVEWORKS_ENABLE_GL
	HRESULT hr;

	if(nv_water_d3d_api_gl2 != m_d3dAPI)
	{
		releaseAll();
	}
	else if(m_d3d._GL2.m_pGLContext != pGLContext)
	{
		releaseAll();
	}
	if(nv_water_d3d_api_undefined == m_d3dAPI)
	{
		m_d3dAPI = nv_water_d3d_api_gl2;
		m_d3d._GL2.m_pGLContext = pGLContext;
		V_RETURN(allocateAllResources());
	}
	return S_OK;
#else
	return S_FALSE;
#endif
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::initNoGraphics()
{
    HRESULT hr;

    if(nv_water_d3d_api_none != m_d3dAPI)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_none;
        V_RETURN(allocateAllResources());
    }
    return S_OK;
}

void NVWaveWorks_FFT_Simulation_CPU_Impl::calcReinit(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params, bool& bRelease, bool& bAllocate, bool& bReinitH0, bool& bReinitGaussAndOmega)
{
    bRelease = false;
    bAllocate = false;
    bReinitH0 = false;
	bReinitGaussAndOmega = false;

	const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade* curr_params = m_params_are_dirty ? &m_next_params : &m_params;

    if(params.fft_resolution != curr_params->fft_resolution ||
        params.readback_displacements != curr_params->readback_displacements ||
		(params.readback_displacements && (params.num_readback_FIFO_entries != curr_params->num_readback_FIFO_entries)))
    {
        bRelease = true;
        bAllocate = true;
    }

    if(	params.fft_period != curr_params->fft_period || 
        params.fft_resolution != curr_params->fft_resolution
        )
    {
        bReinitGaussAndOmega = true;
    }

    if(	params.wave_amplitude != curr_params->wave_amplitude ||
        params.wind_speed != curr_params->wind_speed ||
        params.wind_dir.x != curr_params->wind_dir.x ||
		params.wind_dir.y != curr_params->wind_dir.y ||
        params.wind_dependency != curr_params->wind_dependency ||
        params.small_wave_fraction != curr_params->small_wave_fraction ||
        params.window_in != curr_params->window_in ||
        params.window_out != curr_params->window_out ||
		bReinitGaussAndOmega
        )
    {
        bReinitH0 = true;
    }
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::reinit(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params)
{
	HRESULT hr;

    bool bRelease = false;
    bool bAllocate = false;
    bool bReinitH0 = false;
	bool bReinitGaussAndOmega = false;
	calcReinit(params, bRelease, bAllocate, bReinitH0, bReinitGaussAndOmega);

	if(m_pipelineNextReinit)
	{
		m_next_params = params;
		m_params_are_dirty = true;
	}
	else
	{
		// Ensure any texture locks are relinquished
		OnCompleteSimulationStep(GFSDK_WaveWorks_InvalidKickID);

		m_params = params;
	}
	
    if(bRelease)
    {
		assert(!m_pipelineNextReinit);
        releaseAllResources();
    }

    if(bAllocate)
    {
		assert(!m_pipelineNextReinit);
        V_RETURN(allocateAllResources());
    }
	else
	{
		// allocateAllResources() does these inits anyway, so only do them forcibly
		// if we're not re-allocating...
		if(bReinitGaussAndOmega)
		{
			assert(!m_pipelineNextReinit);

			// Important to do this first, because H0 relies on an up-to-date Gaussian distribution
			V_RETURN(initGaussAndOmega());
		}

		if(bReinitH0)
		{
			m_H0UpdateRequired = true;
		}
	}

	// Reset the pipelining flag
	m_pipelineNextReinit = false;

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::initGaussAndOmega()
{
	GFSDK_WaveWorks_Simulation_Util::init_gauss(m_params, m_gauss_data);
	GFSDK_WaveWorks_Simulation_Util::init_omega(m_params, m_omega_data);
	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::allocateAllResources()
{
    HRESULT hr;

	int N = m_params.fft_resolution;
	int num_height_map_samples = (N + 4) * (N + 1);

	//reallocating buffer for readbacks
	SAFE_ALIGNED_FREE(m_readback_buffer[0]);
	SAFE_ALIGNED_FREE(m_readback_buffer[1]);
	if(m_params.readback_displacements)
	{
		m_readback_buffer[0] = (gfsdk_float4*)NVSDK_aligned_malloc( N*N*4*sizeof(float), sizeof(Simd4f));
		m_readback_buffer[1] = (gfsdk_float4*)NVSDK_aligned_malloc( N*N*4*sizeof(float), sizeof(Simd4f));
	}
	m_active_readback_buffer = 0;

	//reallocating readback FIFO buffers
	if(m_pReadbackFIFO)
	{
		for(int i = 0; i != m_pReadbackFIFO->capacity(); ++i)
		{
			SAFE_ALIGNED_FREE(m_pReadbackFIFO->raw_at(i).buffer);
		}
		SAFE_DELETE(m_pReadbackFIFO);
	}

	const int num_readback_FIFO_entries = m_params.readback_displacements ? m_params.num_readback_FIFO_entries : 0;
	if(num_readback_FIFO_entries)
	{
		m_pReadbackFIFO = new CircularFIFO<ReadbackFIFOSlot>(num_readback_FIFO_entries);
		for(int i = 0; i != m_pReadbackFIFO->capacity(); ++i)
		{
			ReadbackFIFOSlot& slot = m_pReadbackFIFO->raw_at(i);
			slot.buffer = (gfsdk_float4*)NVSDK_aligned_malloc( N*N*4*sizeof(float), sizeof(Simd4f));
			slot.kickID = GFSDK_WaveWorks_InvalidKickID;
		}
	}

	//initialize rarely-updated datas
	SAFE_ALIGNED_FREE(m_gauss_data);
	m_gauss_data = (float2*)NVSDK_aligned_malloc( gauss_map_size*sizeof(*m_gauss_data), sizeof(Simd4f));

	SAFE_ALIGNED_FREE(m_omega_data);
	m_omega_data = (float*)NVSDK_aligned_malloc( num_height_map_samples*sizeof(*m_omega_data), sizeof(Simd4f));

	V_RETURN(initGaussAndOmega());

	//initialize philips spectrum
	SAFE_ALIGNED_FREE(m_h0_data);
	m_h0_data = (float2*)NVSDK_aligned_malloc( num_height_map_samples*sizeof(*m_h0_data), sizeof(Simd4f));
	m_H0UpdateRequired = true;

	//reallocate fft in-out buffer
	SAFE_ALIGNED_FREE(m_fftCPU_io_buffer);
	m_fftCPU_io_buffer = (complex*)NVSDK_aligned_malloc( 3*N*N*sizeof(complex), sizeof(Simd4f));

	//precompute coefficients for faster update spectrum computation
	//this code was ported from hlsl
	SAFE_ALIGNED_FREE(m_sqrt_table);
	m_sqrt_table = (float*)NVSDK_aligned_malloc(N*N*sizeof(*m_sqrt_table), sizeof(Simd4f));
	for(int y=0; y<N; y++)
	{
		float ky = y - N * 0.5f;
		float ky2 = ky*ky;
		float kx = -0.5f*N;

		for(int x=0; x<N; x++, kx+=1.0f)
		{
			float sqr_k = kx * kx + ky2;
			float s = 0.0f;
			if (sqr_k > 1e-12f)
				s = 1.0f / sqrtf(sqr_k);
			m_sqrt_table[y*N+x] = s;
		}
	}

    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			SAFE_RELEASE(m_d3d._11.m_pDC);//release previous context
			SAFE_RELEASE(m_d3d._11.m_pd3d11DisplacementMapTexture[1]);
			SAFE_RELEASE(m_d3d._11.m_pd3d11DisplacementMapTexture[0]);
			SAFE_RELEASE(m_d3d._11.m_pd3d11DisplacementMapTextureSRV[0]);
			SAFE_RELEASE(m_d3d._11.m_pd3d11DisplacementMapTextureSRV[1]);
			for(int i=0; i<2; i++)
			{
				// Create 2D texture
				D3D11_TEXTURE2D_DESC tex_desc;
				tex_desc.Width = N;
				tex_desc.Height = N;
				tex_desc.MipLevels = 1;
				tex_desc.ArraySize = 1;
				tex_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				tex_desc.SampleDesc.Count = 1;
				tex_desc.SampleDesc.Quality = 0;
				tex_desc.Usage = D3D11_USAGE_DYNAMIC;
				tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				tex_desc.MiscFlags = 0;
				V_RETURN(m_d3d._11.m_pd3d11Device->CreateTexture2D(&tex_desc, NULL, &m_d3d._11.m_pd3d11DisplacementMapTexture[i]));

				// Create shader resource view
				D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
				srv_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
				srv_desc.Texture2D.MostDetailedMip = 0;
				V_RETURN(m_d3d._11.m_pd3d11Device->CreateShaderResourceView(m_d3d._11.m_pd3d11DisplacementMapTexture[i], &srv_desc, &m_d3d._11.m_pd3d11DisplacementMapTextureSRV[i]));
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			for(int i=0; i<GnmObjects::NumGnmTextures; i++)
			{
				if(void* ptr = m_d3d._gnm.m_pGnmDisplacementMapTexture[i].getBaseAddress())
					NVSDK_garlic_free(ptr);

				Gnm::SizeAlign sizeAlign = m_d3d._gnm.m_pGnmDisplacementMapTexture[i].initAs2d(N, N, 1, Gnm::kDataFormatR16G16B16A16Float, Gnm::kTileModeDisplay_LinearAligned, SAMPLE_1);
				m_d3d._gnm.m_pGnmDisplacementMapTexture[i].setBaseAddress(NVSDK_garlic_malloc(sizeAlign.m_size, sizeAlign.m_align));
				m_d3d._gnm.m_pGnmDisplacementMapTexture[i].setResourceMemoryType(Gnm::kResourceMemoryTypeRO);
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				if(m_d3d._GL2.m_GLDisplacementMapTexture[0] != 0) NVSDK_GLFunctions.glDeleteTextures(1,&m_d3d._GL2.m_GLDisplacementMapTexture[0]); CHECK_GL_ERRORS;
				if(m_d3d._GL2.m_GLDisplacementMapTexture[1] != 0) NVSDK_GLFunctions.glDeleteTextures(1,&m_d3d._GL2.m_GLDisplacementMapTexture[1]); CHECK_GL_ERRORS;
				if(m_d3d._GL2.m_GLDisplacementMapPBO[0] != 0) NVSDK_GLFunctions.glDeleteBuffers(1,&m_d3d._GL2.m_GLDisplacementMapTexture[0]); CHECK_GL_ERRORS;
				if(m_d3d._GL2.m_GLDisplacementMapPBO[1] != 0) NVSDK_GLFunctions.glDeleteBuffers(1,&m_d3d._GL2.m_GLDisplacementMapTexture[1]); CHECK_GL_ERRORS;
				// Create 2D textures 
				float* blank_data = (float*)NVSDK_malloc(N*N*4*sizeof(gfsdk_U16));
				memset(blank_data, 0, N*N*4*sizeof(gfsdk_U16));
				NVSDK_GLFunctions.glGenTextures(1,&m_d3d._GL2.m_GLDisplacementMapTexture[0]); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, m_d3d._GL2.m_GLDisplacementMapTexture[0]); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, N, N, 0, GL_RGBA, GL_HALF_FLOAT, blank_data); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glGenTextures(1,&m_d3d._GL2.m_GLDisplacementMapTexture[1]); CHECK_GL_ERRORS; 
				NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, m_d3d._GL2.m_GLDisplacementMapTexture[1]); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, N, N, 0, GL_RGBA, GL_HALF_FLOAT, blank_data); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERRORS;
				// Create PBOs
				NVSDK_GLFunctions.glGenBuffers(1,&m_d3d._GL2.m_GLDisplacementMapPBO[0]); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_d3d._GL2.m_GLDisplacementMapPBO[0]); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBufferData(GL_PIXEL_UNPACK_BUFFER, N*N*4*sizeof(gfsdk_U16), blank_data, GL_STREAM_DRAW); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glGenBuffers(1,&m_d3d._GL2.m_GLDisplacementMapPBO[1]); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_d3d._GL2.m_GLDisplacementMapPBO[1]); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBufferData(GL_PIXEL_UNPACK_BUFFER,  N*N*4*sizeof(gfsdk_U16), blank_data, GL_STREAM_DRAW); CHECK_GL_ERRORS;
				NVSDK_GLFunctions.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); CHECK_GL_ERRORS;
				free(blank_data);
			}
			break;
#endif
		case nv_water_d3d_api_none:
			{
				SAFE_ALIGNED_FREE(m_d3d._noGFX.m_pnogfxDisplacementMap[0]);
				SAFE_ALIGNED_FREE(m_d3d._noGFX.m_pnogfxDisplacementMap[1]);
				const size_t row_size = 4 * N;
				m_d3d._noGFX.m_pnogfxDisplacementMap[0] = NVSDK_aligned_malloc(row_size*N*sizeof(gfsdk_U16), sizeof(Simd4f));
				m_d3d._noGFX.m_pnogfxDisplacementMap[1] = NVSDK_aligned_malloc(row_size*N*sizeof(gfsdk_U16), sizeof(Simd4f));
				m_d3d._noGFX.m_nogfxDisplacementMapRowPitch = row_size * sizeof(gfsdk_U16);
			}
			break;

		default:
			// Unexpected API
			return E_FAIL;
    }

	// Displacement map contents are initially undefined
	m_DisplacementMapVersion = GFSDK_WaveWorks_InvalidKickID;

    return S_OK;
}

void NVWaveWorks_FFT_Simulation_CPU_Impl::releaseAll()
{
	releaseAllResources();

#if WAVEWORKS_ENABLE_GRAPHICS
    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			SAFE_RELEASE(m_d3d._11.m_pd3d11Device);
	        break;
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			//nothing to do
	        break;
#endif
		default:
			break;
    }
#endif // WAVEWORKS_ENABLE_GRAPHICS

	m_d3dAPI = nv_water_d3d_api_undefined;
}

void NVWaveWorks_FFT_Simulation_CPU_Impl::releaseAllResources()
{
	// Ensure any texture locks are relinquished
	OnCompleteSimulationStep(GFSDK_WaveWorks_InvalidKickID);

	SAFE_ALIGNED_FREE(m_sqrt_table);
	SAFE_ALIGNED_FREE(m_gauss_data);
	SAFE_ALIGNED_FREE(m_h0_data);
	SAFE_ALIGNED_FREE(m_omega_data);

	SAFE_ALIGNED_FREE(m_fftCPU_io_buffer);
	SAFE_ALIGNED_FREE(m_readback_buffer[0]);
	SAFE_ALIGNED_FREE(m_readback_buffer[1]);
	m_active_readback_buffer = 0;

	if(m_pReadbackFIFO)
	{
		for(int i = 0; i != m_pReadbackFIFO->capacity(); ++i)
		{
			SAFE_ALIGNED_FREE(m_pReadbackFIFO->raw_at(i).buffer);
		}
		SAFE_DELETE(m_pReadbackFIFO);
	}

    switch(m_d3dAPI)
    {
#if WAVEWORKS_ENABLE_D3D11
	    case nv_water_d3d_api_d3d11:
			assert(NULL == m_d3d._11.m_pDC); // should be done by OnCompleteSimulationStep()
			SAFE_RELEASE(m_d3d._11.m_pd3d11DisplacementMapTexture[0]);
			SAFE_RELEASE(m_d3d._11.m_pd3d11DisplacementMapTexture[1]);
			SAFE_RELEASE(m_d3d._11.m_pd3d11DisplacementMapTextureSRV[0]);
			SAFE_RELEASE(m_d3d._11.m_pd3d11DisplacementMapTextureSRV[1]);
	        break;

#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			for(int i=0; i<GnmObjects::NumGnmTextures; ++i)
			{
				if(void* ptr = m_d3d._gnm.m_pGnmDisplacementMapTexture[i].getBaseAddress())
					NVSDK_garlic_free(ptr);
				m_d3d._gnm.m_pGnmDisplacementMapTexture[i].setBaseAddress(NULL);
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GL
	    case nv_water_d3d_api_gl2:
			if(m_d3d._GL2.m_GLDisplacementMapTexture[0] != 0) NVSDK_GLFunctions.glDeleteTextures(1,&m_d3d._GL2.m_GLDisplacementMapTexture[0]); CHECK_GL_ERRORS;
			if(m_d3d._GL2.m_GLDisplacementMapTexture[1] != 0) NVSDK_GLFunctions.glDeleteTextures(1,&m_d3d._GL2.m_GLDisplacementMapTexture[1]); CHECK_GL_ERRORS;
			if(m_d3d._GL2.m_GLDisplacementMapPBO[0] != 0) NVSDK_GLFunctions.glDeleteBuffers(1,&m_d3d._GL2.m_GLDisplacementMapTexture[0]); CHECK_GL_ERRORS;
			if(m_d3d._GL2.m_GLDisplacementMapPBO[1] != 0) NVSDK_GLFunctions.glDeleteBuffers(1,&m_d3d._GL2.m_GLDisplacementMapTexture[1]); CHECK_GL_ERRORS;
	        break;
#endif

		case nv_water_d3d_api_none:
			SAFE_ALIGNED_FREE(m_d3d._noGFX.m_pnogfxDisplacementMap[0]);
			SAFE_ALIGNED_FREE(m_d3d._noGFX.m_pnogfxDisplacementMap[1]);
			break;

		default:
			break;

    }
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::addDisplacements(	const gfsdk_float2* inSamplePoints,
																gfsdk_float4* outDisplacements,
																UINT numSamples
																)
{
	if(m_active_readback_buffer) {
		GFSDK_WaveWorks_Simulation_Util::add_displacements_float32(
			m_params, (const BYTE*)m_active_readback_buffer,
			sizeof(gfsdk_float4) * m_params.fft_resolution,
			inSamplePoints, outDisplacements, numSamples);
	}
    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::addArchivedDisplacements(	float coord,
																		const gfsdk_float2* inSamplePoints,
																		gfsdk_float4* outDisplacements,
																		UINT numSamples
																		)
{
	if(NULL == m_pReadbackFIFO)
	{
		// No FIFO, nothing to add
		return S_OK;
	}
	else if(0 == m_pReadbackFIFO->range_count())
	{
		// No entries, nothing to add
		return S_OK;
	}

	const float coordMax = float(m_pReadbackFIFO->range_count()-1);

	// Clamp coord to archived range
	float coord_clamped = coord;
	if(coord_clamped < 0.f)
		coord_clamped = 0.f;
	else if(coord_clamped > coordMax)
		coord_clamped = coordMax;

	// Figure out what interp is required
	const float coord_round = floorf(coord_clamped);
	const float coord_frac = coord_clamped - coord_round;
	const int coord_lower = (int)coord_round;
	if(0.f != coord_frac)
	{
		const int coord_upper = coord_lower + 1;

		GFSDK_WaveWorks_Simulation_Util::add_displacements_float32(
			m_params, (const BYTE*)m_pReadbackFIFO->range_at(coord_lower).buffer,
			sizeof(gfsdk_float4) * m_params.fft_resolution,
			inSamplePoints, outDisplacements, numSamples,
			1.f-coord_frac);

		GFSDK_WaveWorks_Simulation_Util::add_displacements_float32(
			m_params, (const BYTE*)m_pReadbackFIFO->range_at(coord_upper).buffer,
			sizeof(gfsdk_float4) * m_params.fft_resolution,
			inSamplePoints, outDisplacements, numSamples,
			coord_frac);
	}
	else
	{
		GFSDK_WaveWorks_Simulation_Util::add_displacements_float32(
			m_params, (const BYTE*)m_pReadbackFIFO->range_at(coord_lower).buffer,
			sizeof(gfsdk_float4) * m_params.fft_resolution,
			inSamplePoints, outDisplacements, numSamples,
			1.f);
	}

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::getTimings(NVWaveWorks_FFT_Simulation_Timings& timings) const
{
	timings.GPU_simulation_time = 0.f;
	timings.GPU_FFT_simulation_time = 0.f;
	return S_OK;
}

ID3D11ShaderResourceView** NVWaveWorks_FFT_Simulation_CPU_Impl::GetDisplacementMapD3D11()
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);
	int ti = (m_mapped_texture_index+1)&1;
	return &m_d3d._11.m_pd3d11DisplacementMapTextureSRV[ti];
#else
	return NULL;
#endif
}

Gnm::Texture* NVWaveWorks_FFT_Simulation_CPU_Impl::GetDisplacementMapGnm()
{
#if WAVEWORKS_ENABLE_GNM
	assert(m_d3dAPI == nv_water_d3d_api_gnm);
	int ti = (m_d3d._gnm.m_mapped_gnm_texture_index+GnmObjects::NumGnmTextures-1) % GnmObjects::NumGnmTextures;
	return &m_d3d._gnm.m_pGnmDisplacementMapTexture[ti];
#else
	return NULL;
#endif
}

GLuint NVWaveWorks_FFT_Simulation_CPU_Impl::GetDisplacementMapGL2()
{
#if WAVEWORKS_ENABLE_GL
	assert(m_d3dAPI == nv_water_d3d_api_gl2);
	int ti = (m_mapped_texture_index+1)&1;
	return m_d3d._GL2.m_GLDisplacementMapTexture[ti];
#else
	return 0;
#endif
}

void NVWaveWorks_FFT_Simulation_CPU_Impl::OnCompleteSimulationStep(gfsdk_U64 kickID)
{
	if(m_mapped_texture_ptr) {
		switch(m_d3dAPI) {
#if WAVEWORKS_ENABLE_D3D11
			case nv_water_d3d_api_d3d11:
				assert(NULL != m_d3d._11.m_pDC);
				m_d3d._11.m_pDC->Unmap(m_d3d._11.m_pd3d11DisplacementMapTexture[m_mapped_texture_index], 0);
				SAFE_RELEASE(m_d3d._11.m_pDC);//release previous context
				break;
#endif
#if WAVEWORKS_ENABLE_GNM
			case nv_water_d3d_api_gnm:
				// nothing to do? synchronization?
				break;
#endif
#if WAVEWORKS_ENABLE_GL
			case nv_water_d3d_api_gl2:
				{
					UINT N = m_params.fft_resolution;

					// copy pixels from PBO to texture object
					NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, m_d3d._GL2.m_GLDisplacementMapTexture[m_mapped_texture_index]); CHECK_GL_ERRORS;
					NVSDK_GLFunctions.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_d3d._GL2.m_GLDisplacementMapPBO[m_mapped_texture_index]); CHECK_GL_ERRORS;
					NVSDK_GLFunctions.glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); CHECK_GL_ERRORS;
					NVSDK_GLFunctions.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, N, N, GL_RGBA, GL_HALF_FLOAT, 0); CHECK_GL_ERRORS;
					NVSDK_GLFunctions.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); CHECK_GL_ERRORS;
					NVSDK_GLFunctions.glBindTexture(GL_TEXTURE_2D, 0);
				}
				break;	
#endif
			case nv_water_d3d_api_none:
				break;	// no-op
			default:
				break;
		}
		m_active_readback_buffer = m_readback_buffer[m_mapped_texture_index];
		m_mapped_texture_index = (m_mapped_texture_index+1)&1; //flip to other texture
		m_mapped_texture_ptr = 0;
		m_mapped_texture_row_pitch = 0;

		switch(m_d3dAPI) {
#if WAVEWORKS_ENABLE_GNM
			case nv_water_d3d_api_gnm:
				// Special case: triple-buffer under GNM
				m_d3d._gnm.m_mapped_gnm_texture_index = (m_d3d._gnm.m_mapped_gnm_texture_index+1) % GnmObjects::NumGnmTextures;
				break;
#endif
			case nv_water_d3d_api_none:
				break;	// no-op
			default:
				break;
		}

		m_DisplacementMapVersion = kickID;
	}
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::OnInitiateSimulationStep(Graphics_Context* pGC, double dSimTime)
{
	// Roll new params into p
	if(m_params_are_dirty)
	{
		m_params = m_next_params;
		m_params_are_dirty = false;
	}

	UINT N = m_params.fft_resolution;
	switch(m_d3dAPI) {
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11: {
				HRESULT hr;
				assert(NULL == m_d3d._11.m_pDC);
				m_d3d._11.m_pDC = pGC->d3d11();
				m_d3d._11.m_pDC->AddRef();
				D3D11_MAPPED_SUBRESOURCE msr_d3d11;
				V_RETURN(m_d3d._11.m_pDC->Map( m_d3d._11.m_pd3d11DisplacementMapTexture[m_mapped_texture_index], 0, D3D11_MAP_WRITE_DISCARD, 0, &msr_d3d11));
				m_mapped_texture_ptr = static_cast<BYTE*>(msr_d3d11.pData);
				m_mapped_texture_row_pitch = msr_d3d11.RowPitch;
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm: {
				m_mapped_texture_ptr = static_cast<BYTE*>(m_d3d._gnm.m_pGnmDisplacementMapTexture[m_d3d._gnm.m_mapped_gnm_texture_index].getBaseAddress());
				m_mapped_texture_row_pitch = m_d3d._gnm.m_pGnmDisplacementMapTexture[m_d3d._gnm.m_mapped_gnm_texture_index].getPitch() * 
					m_d3d._gnm.m_pGnmDisplacementMapTexture[m_d3d._gnm.m_mapped_gnm_texture_index].getDataFormat().getBytesPerElement();
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2: 
			NVSDK_GLFunctions.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_d3d._GL2.m_GLDisplacementMapPBO[m_mapped_texture_index]); CHECK_GL_ERRORS;
			m_mapped_texture_ptr = static_cast<BYTE*>((GLubyte*)NVSDK_GLFunctions.glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, N*N*sizeof(gfsdk_U16)*4, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT)); CHECK_GL_ERRORS;
			m_mapped_texture_row_pitch = N*4*sizeof(gfsdk_U16);
			NVSDK_GLFunctions.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); CHECK_GL_ERRORS;
			break;
#endif
		case nv_water_d3d_api_none: 
			// This is a plain old system memory allocation masquerading as a texture lock - doing it this way means we can re-use all our
			// CPU simulation existing infrastucture
			m_mapped_texture_ptr = static_cast<BYTE*>(m_d3d._noGFX.m_pnogfxDisplacementMap[m_mapped_texture_index]);
			m_mapped_texture_row_pitch = m_d3d._noGFX.m_nogfxDisplacementMapRowPitch;
			break;
		default:
			break;
	}

	m_doubletime = dSimTime * (double)m_params.time_scale;

	m_ref_count_update_h0 = (LONG) N+1; //indicates that h0 is updated and we can push ht tasks when count becomes zero
	m_ref_count_update_ht = (LONG) N;   //indicates that ht is updated and we can push FFT tasks when count becomes zero
	m_ref_count_FFT_X = (LONG) (3*N)/4;	// One task per group of 4 rows per XYZ
	m_ref_count_FFT_Y = (LONG) (3*N)/4; // One task per group of 4 columns per XYZ
	m_ref_count_update_texture = (LONG)N;
	
	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_CPU_Impl::archiveDisplacements(gfsdk_U64 kickID)
{
	if(m_active_readback_buffer && m_pReadbackFIFO)
	{
		// We avoid big memcpys by swapping pointers, specifically we will either evict a FIFO entry or else use a free one and
		// swap it with one of the 'scratch' m_readback_buffers used for double-buffering
		//
		// First job is to check whether the FIFO already contains this result. We know that if it does contain this result,
		// it will be the last one pushed on...
		if(m_pReadbackFIFO->range_count())
		{
			if(kickID == m_pReadbackFIFO->range_at(0).kickID)
			{
				// It is an error to archive the same results twice...
				return E_FAIL;
			}
		}

		// Assuming the current results have not been archived, the next-up readback buffer should match the one we are serving up
		// for addDisplacements...
		const int ri = (m_mapped_texture_index+1)&1;
		assert(m_active_readback_buffer == m_readback_buffer[ri]);

		ReadbackFIFOSlot& slot = m_pReadbackFIFO->consume_one();
		m_readback_buffer[ri] = slot.buffer;
		slot.buffer = m_active_readback_buffer;
		slot.kickID = kickID;
	}

	return S_OK;
}

#endif //SUPPORT_FFTCPU

