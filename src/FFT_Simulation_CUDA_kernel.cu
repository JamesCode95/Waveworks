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

#include "Shared_Globals.h"
#include <cassert>

template <typename T> T sqr(const T& t) { return t*t; }
__device__ float2 operator+(const float2& v0, const float2& v1) { return make_float2(v0.x + v1.x, v0.y + v1.y); }
__device__ float2 operator-(const float2& v0, const float2& v1) { return make_float2(v0.x - v1.x, v0.y - v1.y); }
__device__ float2 operator*(const float2& v, const float& s) { return make_float2(v.x * s, v.y * s); }
__device__ float2 make_float2(const float& s) { return make_float2(s, s); }

struct Constants
{
	float2* m_Gauss;
	float2* m_H0;
	float2* m_Ht;
	float4* m_Dt;
	float* m_Omega;

	int m_resolution;
	int m_resolution_plus_one;
	int m_half_resolution;
	int m_half_resolution_plus_one;
	int m_half_of_resolution_squared;
	int m_resolution_plus_one_squared_minus_one;
	int m_32_minus_log2_resolution;

	float m_window_in;
	float m_window_out;

	float m_frequency_scale;
	float m_linear_scale;
	float m_wind_scale;
	float m_root_scale;
	float m_power_scale;
	float2 m_wind_dir;
	float m_choppy_scale;
};

static __constant__ Constants gConstants[MAX_NUM_CASCADES];

extern "C" 
cudaError cuda_GetConstantsSize(size_t* size)
{
	return cudaGetSymbolSize(size, gConstants);
}

extern "C" 
cudaError cuda_GetConstantsAddress(void** ptr)
{
	return cudaGetSymbolAddress(ptr, gConstants);
}

extern "C" 
cudaError cuda_SetConstants (void* dst,
						float2* Gauss, 
						float2* H0, 
						float2* Ht, 
						float4* Dt, 
						float* Omega,
						int resolution,
						float fft_period,
						float window_in,
						float window_out,
						float2 wind_dir,
						float wind_speed,
						float wind_dependency,
						float wave_amplitude,
						float small_wave_fraction,
						float choppy_scale,
						cudaStream_t cu_stream)
{
	const float twoPi = 6.28318530718f;
	const float gravity = 9.810f;
	const float sqrtHalf = 0.707106781186f;
	const float euler = 2.71828182846f;

	float fftNorm = powf(float(resolution), -0.25f);
	float philNorm = euler / fft_period;
	float gravityScale = sqr(gravity / sqr(wind_speed));

	static Constants constants;
	constants.m_Gauss = Gauss;
	constants.m_H0 = H0;
	constants.m_Ht = Ht;
	constants.m_Dt = Dt;
	constants.m_Omega = Omega;
	constants.m_resolution = resolution;
	constants.m_resolution_plus_one = resolution+1;
	constants.m_half_resolution = resolution/2;
	constants.m_half_resolution_plus_one = resolution/2+1;
	constants.m_half_of_resolution_squared = sqr(resolution)/2;
	constants.m_resolution_plus_one_squared_minus_one = sqr(resolution+1)-1;
	for(int i = 0; (1 << i) <= resolution; ++i)
		constants.m_32_minus_log2_resolution = 32 - i;
	constants.m_window_in = window_in;
	constants.m_window_out = window_out;
	constants.m_wind_dir = wind_dir;
	constants.m_frequency_scale = twoPi / fft_period;
	constants.m_linear_scale = fftNorm * philNorm * sqrtHalf * wave_amplitude;
	constants.m_wind_scale = -sqrtf(1 - wind_dependency);
	constants.m_root_scale = -0.5f * gravityScale;
	constants.m_power_scale = -0.5f / gravityScale * sqr(small_wave_fraction);
	constants.m_choppy_scale = choppy_scale;

	return cudaMemcpyAsync(dst, &constants, 
		sizeof(constants), cudaMemcpyHostToDevice, cu_stream);
}

template <int N>
__global__ void kernel_ComputeH0()
{
	float2* __restrict__ h0_output = gConstants[N].m_H0;
	const float2* __restrict__ gauss_input = gConstants[N].m_Gauss;

	int columnIdx = blockIdx.x * blockDim.x + threadIdx.x;
	int rowIdx = blockIdx.y * blockDim.y + threadIdx.y;

	int nx = columnIdx - gConstants[N].m_half_resolution;
	int ny = rowIdx - gConstants[N].m_half_resolution;
	float nr = sqrtf(nx*nx + ny*ny);

	float amplitude = 0.0f;
	if((nx || ny) && nr >= gConstants[N].m_window_in && nr < gConstants[N].m_window_out)
	{
		float2 k = make_float2(nx, ny) * gConstants[N].m_frequency_scale;

		float kSqr = k.x * k.x + k.y * k.y;
		float kCos = k.x * gConstants[N].m_wind_dir.x + k.y * gConstants[N].m_wind_dir.y;

		float scale = gConstants[N].m_linear_scale * kCos * rsqrtf(kSqr * kSqr * kSqr);

		if (kCos < 0)
			scale *= gConstants[N].m_wind_scale;

		amplitude = scale * expf(gConstants[N].m_power_scale * kSqr + fdividef(gConstants[N].m_root_scale, kSqr));
	}

	int index = rowIdx * gConstants[N].m_resolution_plus_one + columnIdx;
	float2 h0 = gauss_input[index - rowIdx] * amplitude;
	h0_output[index] = h0;

	// mirror first row/column, CPU and CUDA paths don't do that
	// however, we need to initialize the N+1'th row/column to zero 
	if(!rowIdx || !columnIdx)
		h0_output[gConstants[N].m_resolution_plus_one_squared_minus_one - index] = make_float2(0); //h0;
}

extern "C" 
cudaError cuda_ComputeH0(int resolution, int constantsIndex, cudaStream_t cu_stream)
{
	dim3 block = dim3(8, 8); // block dimensions are fixed to be 64 threads
    dim3 grid = dim3(resolution / block.x, resolution / block.y);
	assert(grid.x * block.x == unsigned(resolution) && grid.y * block.y == unsigned(resolution));

	switch(constantsIndex)
	{
		case 0: kernel_ComputeH0<0><<<grid, block, 0, cu_stream>>>(); break;
		case 1: kernel_ComputeH0<1><<<grid, block, 0, cu_stream>>>(); break;
		case 2: kernel_ComputeH0<2><<<grid, block, 0, cu_stream>>>(); break;
		case 3: kernel_ComputeH0<3><<<grid, block, 0, cu_stream>>>(); break;
	}
	return cudaPeekAtLastError();
}

extern __shared__ float2 gData[];

template <int N>
__device__ void fft(float2 (&u)[2], float2 (&v)[2], float2 (&w)[2])
{
	float2 u0 = u[0] + u[1], u1 = u[0] - u[1];
	float2 v0 = v[0] + v[1], v1 = v[0] - v[1];
	float2 w0 = w[0] + w[1], w1 = w[0] - w[1];

	int stride = 1;
	float scale = 3.14159265359f; // Pi

	#pragma unroll
	while(stride < 32)
	{
		bool flag = threadIdx.x & stride;

		float2 tu = flag ? u0 : u1;
		float2 tv = flag ? v0 : v1;
		float2 tw = flag ? w0 : w1;
#if __CUDA_ARCH__ >= 300
		tu.x = __shfl_xor(tu.x, stride);
		tu.y = __shfl_xor(tu.y, stride);
		tv.x = __shfl_xor(tv.x, stride);
		tv.y = __shfl_xor(tv.y, stride);
		tw.x = __shfl_xor(tw.x, stride);
		tw.y = __shfl_xor(tw.y, stride);
#else
		float2* pDst = gData + threadIdx.x;
		pDst[0]                               = tu;
		pDst[gConstants[N].m_half_resolution] = tv;
		pDst[gConstants[N].m_resolution]      = tw;
		__threadfence_block();
		float2* pSrc = gData + (threadIdx.x ^ stride);
		tu = pSrc[0];
		tv = pSrc[gConstants[N].m_half_resolution];
		tw = pSrc[gConstants[N].m_resolution];
#endif
		(flag ? u0 : u1) = tu;
		(flag ? v0 : v1) = tv;
		(flag ? w0 : w1) = tw;

		stride <<= 1;
		scale *= 0.5f;

		float sin, cos;
		int j = threadIdx.x & (stride-1);
		sincosf(j * scale, &sin, &cos);

		float2 du = make_float2(
			cos * u1.x - sin * u1.y, 
			sin * u1.x + cos * u1.y);
		float2 dv = make_float2(
			cos * v1.x - sin * v1.y, 
			sin * v1.x + cos * v1.y);
		float2 dw = make_float2(
			cos * w1.x - sin * w1.y, 
			sin * w1.x + cos * w1.y);

		u1 = u0 - du;
		u0 = u0 + du;
		v1 = v0 - dv;
		v0 = v0 + dv;
		w1 = w0 - dw;
		w0 = w0 + dw;
	}

	int i = threadIdx.x;
	while(stride < gConstants[N].m_half_resolution)
	{
		bool flag = threadIdx.x & stride;

		float2* pDst = gData + i;
		stride <<= 1;
		scale *= 0.5f;
		i = threadIdx.x ^ (stride - 32);
		float2* pSrc = gData + i;

		if(flag)
		{
			pDst[0]                               = u0;
			pDst[gConstants[N].m_half_resolution] = v0;
			pDst[gConstants[N].m_resolution]      = w0;
			__syncthreads();
			u0 = pSrc[0];
			v0 = pSrc[gConstants[N].m_half_resolution];
			w0 = pSrc[gConstants[N].m_resolution];
		}
		else 
		{
			pDst[0]                               = u1;
			pDst[gConstants[N].m_half_resolution] = v1;
			pDst[gConstants[N].m_resolution]      = w1;
			__syncthreads();
			u1 = pSrc[0];
			v1 = pSrc[gConstants[N].m_half_resolution];
			w1 = pSrc[gConstants[N].m_resolution];
		}

		float sin, cos;
		int j = threadIdx.x & (stride-1);
		sincosf(j * scale, &sin, &cos);

		float2 du = make_float2(
			cos * u1.x - sin * u1.y, 
			sin * u1.x + cos * u1.y);
		float2 dv = make_float2(
			cos * v1.x - sin * v1.y, 
			sin * v1.x + cos * v1.y);
		float2 dw = make_float2(
			cos * w1.x - sin * w1.y, 
			sin * w1.x + cos * w1.y);

		u1 = u0 - du;
		u0 = u0 + du;
		v1 = v0 - dv;
		v0 = v0 + dv;
		w1 = w0 - dw;
		w0 = w0 + dw;
	}

	u[0] = u0;
	u[1] = u1;
	v[0] = v0;
	v[1] = v1;
	w[0] = w0;
	w[1] = w1;
}

// update Ht, Dt_x, Dt_y from H0 and Omega, fourier transform per row (one CTA per row)
template <int N>
__launch_bounds__(MAX_FFT_RESOLUTION/2)
__global__ void kernel_ComputeRows(double timeOverTwoPi)
{
	float2* __restrict__ ht_output = gConstants[N].m_Ht;
	float4* __restrict__ dt_output = gConstants[N].m_Dt;
	const float2* __restrict__ h0_input = gConstants[N].m_H0;
	const float* __restrict__ omega_input = gConstants[N].m_Omega;

	int columnIdx = threadIdx.x * 2;
	int rowIdx = blockIdx.x;

	int reverseColumnIdx = __brev(columnIdx) >> gConstants[N].m_32_minus_log2_resolution;

	int nx = reverseColumnIdx - gConstants[N].m_half_resolution;
	int ny = reverseColumnIdx;
	int nz = rowIdx - gConstants[N].m_half_resolution;

	float2 h0i[2], h0j[2];
	double omega[2];

	int h0_index = rowIdx * gConstants[N].m_resolution_plus_one + reverseColumnIdx;
	int h0_jndex = h0_index + gConstants[N].m_half_resolution;
	int omega_index = rowIdx * gConstants[N].m_half_resolution_plus_one;
	int omega_jndex = omega_index + gConstants[N].m_half_resolution;

	h0i[0] = h0_input[h0_index];
	h0j[0] = h0_input[gConstants[N].m_resolution_plus_one_squared_minus_one - h0_index]; 
	omega[0] = omega_input[omega_index + reverseColumnIdx] * timeOverTwoPi;

	h0i[1] = h0_input[h0_jndex];
	h0j[1] = h0_input[gConstants[N].m_resolution_plus_one_squared_minus_one - h0_jndex]; 
	omega[1] = omega_input[omega_jndex - reverseColumnIdx] * timeOverTwoPi;

	float sinOmega[2], cosOmega[2];
	const float twoPi = 6.283185307179586476925286766559f;
	sincosf(float(omega[0] - floor(omega[0])) * twoPi, sinOmega + 0, cosOmega + 0);
	sincosf(float(omega[1] - floor(omega[1])) * twoPi, sinOmega + 1, cosOmega + 1);

	// H(0) -> H(t)
	float2 ht[2];
	ht[0].x = (h0i[0].x + h0j[0].x) * cosOmega[0] - (h0i[0].y + h0j[0].y) * sinOmega[0];
	ht[1].x = (h0i[1].x + h0j[1].x) * cosOmega[1] - (h0i[1].y + h0j[1].y) * sinOmega[1];
	ht[0].y = (h0i[0].x - h0j[0].x) * sinOmega[0] + (h0i[0].y - h0j[0].y) * cosOmega[0];
	ht[1].y = (h0i[1].x - h0j[1].x) * sinOmega[1] + (h0i[1].y - h0j[1].y) * cosOmega[1];

	float nrx = nx || nz ? rsqrtf(nx*nx + nz*nz) : 0;
	float nry = ny || nz ? rsqrtf(ny*ny + nz*nz) : 0;

	float2 dt0 = make_float2(-ht[0].y, ht[0].x) * nrx;
	float2 dt1 = make_float2(-ht[1].y, ht[1].x) * nry;

	float2 dx[2] = { dt0 * nx, dt1 * ny };
	float2 dy[2] = { dt0 * nz, dt1 * nz };

	fft<N>(ht, dx, dy);

	int index = rowIdx * gConstants[N].m_resolution + threadIdx.x;

	ht_output[index] = ht[0];
	ht_output[index+gConstants[N].m_half_resolution] = ht[1];

	dt_output[index] = make_float4(dx[0].x, dx[0].y, dy[0].x, dy[0].y);
	dt_output[index+gConstants[N].m_half_resolution] = make_float4(dx[1].x, dx[1].y, dy[1].x, dy[1].y);
}

extern "C" 
cudaError cuda_ComputeRows(int resolution, double time, int constantsIndex, cudaStream_t cu_stream)
{
	dim3 block = dim3(resolution/2);
    dim3 grid = dim3(resolution/2+1);
	int sharedMemory = 3 * sizeof(float) * resolution;

	const double oneOverTwoPi = 0.15915494309189533576888376337251;
	time *= oneOverTwoPi;

	switch(constantsIndex)
	{
		case 0: kernel_ComputeRows<0><<<grid, block, sharedMemory, cu_stream>>>(time); break;
		case 1: kernel_ComputeRows<1><<<grid, block, sharedMemory, cu_stream>>>(time); break;
		case 2: kernel_ComputeRows<2><<<grid, block, sharedMemory, cu_stream>>>(time); break;
		case 3: kernel_ComputeRows<3><<<grid, block, sharedMemory, cu_stream>>>(time); break;
	}
	return cudaPeekAtLastError();
}

template <int N>
__device__ void computeColumns (float4 (&displacement_output)[2])
{
	const float2* __restrict__ ht_input = gConstants[N].m_Ht;
	const float4* __restrict__ dt_input = gConstants[N].m_Dt;

	int rowIdx = threadIdx.x * 2;
	int columnIdx = blockIdx.x;

	int reverseRowIdx = __brev(rowIdx) >> gConstants[N].m_32_minus_log2_resolution;

	int index = reverseRowIdx * gConstants[N].m_resolution + columnIdx;
	int jndex = (gConstants[N].m_half_resolution - reverseRowIdx) * gConstants[N].m_resolution + columnIdx;

	float2 ht[2];
	ht[0] = ht_input[index];
	ht[1] = ht_input[jndex];
	ht[1].y = -ht[1].y;

	float4 dti = dt_input[index];
	float4 dtj = dt_input[jndex];

	float2 dx[2] = { make_float2(dti.x, dti.y), make_float2(dtj.x, -dtj.y) };
	float2 dy[2] = { make_float2(dti.z, dti.w), make_float2(dtj.z, -dtj.w) };

	fft<N>(ht, dx, dy);

	float sgn = (threadIdx.x + columnIdx) & 0x1 ? -1.0f : +1.0f;
	float scale = gConstants[N].m_choppy_scale * sgn;

	displacement_output[0] = make_float4(dx[0].x * scale, dy[0].x * scale, ht[0].x * sgn, 0);
	displacement_output[1] = make_float4(dx[1].x * scale, dy[1].x * scale, ht[1].x * sgn, 0);
}

// do fourier transform per row of Ht, Dt_x, Dt_y, write displacement texture (one CTA per column)
template <int N>
__launch_bounds__(MAX_FFT_RESOLUTION/2)
__global__ void kernel_ComputeColumns  (float4* __restrict__ displacement_output)
{
	float4 displacement[2];
	computeColumns<N>(displacement);

	displacement_output += blockIdx.x + gConstants[N].m_resolution * threadIdx.x;
	displacement_output[0] = displacement[0];
	displacement_output[gConstants[N].m_half_of_resolution_squared] = displacement[1];
}

extern "C" 
cudaError cuda_ComputeColumns(float4* displacement, int resolution, int constantsIndex, cudaStream_t cu_stream)
{
	dim3 block = dim3(resolution/2);
    dim3 grid = dim3(resolution);
	int sharedMemory = 3 * sizeof(float) * resolution;

	switch(constantsIndex)
	{
		case 0: kernel_ComputeColumns<0><<<grid, block, sharedMemory, cu_stream>>>(displacement); break;
		case 1: kernel_ComputeColumns<1><<<grid, block, sharedMemory, cu_stream>>>(displacement); break;
		case 2: kernel_ComputeColumns<2><<<grid, block, sharedMemory, cu_stream>>>(displacement); break;
		case 3: kernel_ComputeColumns<3><<<grid, block, sharedMemory, cu_stream>>>(displacement); break;
	}
	return cudaPeekAtLastError();
}

#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 200

surface<void, cudaSurfaceType2D> gDisplacement;

template <int N>
__launch_bounds__(MAX_FFT_RESOLUTION/2)
__global__ void kernel_ComputeColumns_array()
{
	float4 displacement[2];
	computeColumns<N>(displacement);

	ushort4 displacement0 = make_ushort4(
		__float2half_rn(displacement[0].x),
		__float2half_rn(displacement[0].y),
		__float2half_rn(displacement[0].z),
		0);

	ushort4 displacement1 = make_ushort4(
		__float2half_rn(displacement[1].x),
		__float2half_rn(displacement[1].y),
		__float2half_rn(displacement[1].z),
		0);

	int rowAddr = blockIdx.x * sizeof(ushort4);
	surf2Dwrite(displacement0, gDisplacement, rowAddr, threadIdx.x);
	surf2Dwrite(displacement1, gDisplacement, rowAddr, threadIdx.x + gConstants[N].m_half_resolution);
}

extern "C" 
cudaError cuda_ComputeColumns_array(cudaArray* displacement, int resolution, int constantsIndex, cudaStream_t cu_stream)
{
	cudaBindSurfaceToArray(gDisplacement, displacement);
    dim3 block = dim3(resolution/2);
    dim3 grid = dim3(resolution);
	int sharedMemory = 3 * sizeof(float) * resolution;

	switch(constantsIndex)
	{
		case 0: kernel_ComputeColumns_array<0><<<grid, block, sharedMemory, cu_stream>>>(); break;
		case 1: kernel_ComputeColumns_array<1><<<grid, block, sharedMemory, cu_stream>>>(); break;
		case 2: kernel_ComputeColumns_array<2><<<grid, block, sharedMemory, cu_stream>>>(); break;
		case 3: kernel_ComputeColumns_array<3><<<grid, block, sharedMemory, cu_stream>>>(); break;
	}
	return cudaPeekAtLastError();
}

#endif