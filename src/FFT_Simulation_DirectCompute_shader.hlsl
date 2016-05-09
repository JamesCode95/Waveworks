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

#define MAX_FFT_RESOLUTION 512
#define WARP_WIDTH 8 // minimum number of threads which execute in lockstep

#ifdef GFSDK_WAVEWORKS_GNM
#define cbuffer ConstantBuffer
#define StructuredBuffer DataBuffer
#define RWStructuredBuffer RW_DataBuffer
#define numthreads NUM_THREADS
#define SV_DispatchThreadID S_DISPATCH_THREAD_ID
#define groupshared thread_group_memory
#define GroupMemoryBarrierWithGroupSync ThreadGroupMemoryBarrierSync
#define reversebits ReverseBits
#define RWTexture2D RW_Texture2D
#endif

// constants, needs to match struct in FFT_Simulation_DirectCompute.cpp
cbuffer MyConstantBuffer : register(b0) 
{
	uint m_resolution;
	uint m_resolution_plus_one;
	uint m_half_resolution;
	uint m_half_resolution_plus_one;
	uint m_resolution_plus_one_squared_minus_one;
	uint m_32_minus_log2_resolution;

	float m_window_in;
	float m_window_out;

	float2 m_wind_dir;
	float m_frequency_scale;
	float m_linear_scale;
	float m_wind_scale;
	float m_root_scale;
	float m_power_scale;

	double m_time;

	float m_choppy_scale;
};

StructuredBuffer<float2> g_gauss_input : register(t0);
RWStructuredBuffer<float2> g_h0_output : register(u0);

// update H0 from Gauss (one CTA per row)
[numthreads(MAX_FFT_RESOLUTION, 1, 1)]
void ComputeH0( uint3 dispatchThreadId : SV_DispatchThreadID )
{
	uint columnIdx = dispatchThreadId.x;
	uint rowIdx = dispatchThreadId.y;

	if(columnIdx < m_resolution) 
	{
		int nx = columnIdx - m_half_resolution;
		int ny = rowIdx - m_half_resolution;
		float nr = sqrt(float(nx*nx + ny*ny));

		float amplitude = 0.0f;
		if((nx || ny) && nr >= m_window_in && nr < m_window_out)
		{
			float2 k = float2(nx * m_frequency_scale, ny * m_frequency_scale);

			float kSqr = k.x * k.x + k.y * k.y;
			float kCos = k.x * m_wind_dir.x + k.y * m_wind_dir.y;

			float scale = m_linear_scale * kCos * rsqrt(kSqr * kSqr * kSqr);

			if (kCos < 0)
				scale *= m_wind_scale;

			amplitude = scale * exp(m_power_scale * kSqr + m_root_scale / kSqr);
		}

		int index = rowIdx * m_resolution_plus_one + columnIdx;
		float2 h0 = amplitude * g_gauss_input[index - rowIdx];
		g_h0_output[index] = h0;

		// mirror first row/column, CPU and CUDA paths don't do that
		// however, we need to initialize the N+1'th row/column to zero 
		if(!rowIdx || !columnIdx)
			g_h0_output[m_resolution_plus_one_squared_minus_one - index] = 0; //h0;
	}
}

groupshared float2 uData[MAX_FFT_RESOLUTION/2];
groupshared float2 vData[MAX_FFT_RESOLUTION/2];
groupshared float2 wData[MAX_FFT_RESOLUTION/2];

// input is bit-reversed threadIdx and threadIdx+1
// output is threadIdx and threadIdx + resolution/2
void fft(inout float2 u[2], inout float2 v[2], inout float2 w[2], uint threadIdx)
{
	bool flag = false;
	float scale = 3.14159265359f * 0.5f; // Pi

	if(threadIdx < m_half_resolution)
	{
		{
			uint i = threadIdx;

			float2 du = u[1]; 
			float2 dv = v[1]; 
			float2 dw = w[1]; 
				
			u[1] = u[0] - du; 
			u[0] = u[0] + du;
			v[1] = v[0] - dv; 
			v[0] = v[0] + dv;
			w[1] = w[0] - dw; 
			w[0] = w[0] + dw;
			
			flag = threadIdx & 1;

			// much slower: vData[i] = v[!flag];
			if(flag) 
			{
				uData[i] = u[0]; 
				vData[i] = v[0]; 
				wData[i] = w[0]; 
			} else {
				uData[i] = u[1]; 
				vData[i] = v[1]; 
				wData[i] = w[1]; 
			}

			GroupMemoryBarrier();
		}

		[unroll(2)] // log2(WARP_WIDTH) - 1
		for(uint stride = 2; stride < WARP_WIDTH; stride <<= 1, scale *= 0.5f)
		{
			uint i = threadIdx ^ (stride-1);
			uint j = threadIdx & (stride-1);

			// much slower: v[!flag] = vData[i];
			if(flag) 
			{
				u[0] = uData[i]; 
				v[0] = vData[i]; 
				w[0] = wData[i]; 
			} else { 
				u[1] = uData[i];
				v[1] = vData[i];
				w[1] = wData[i];
			}

			float sin, cos;
			sincos(j * scale, sin, cos);

			float2 du = float2(
				cos * u[1].x - sin * u[1].y, 
				sin * u[1].x + cos * u[1].y);
			float2 dv = float2(
				cos * v[1].x - sin * v[1].y, 
				sin * v[1].x + cos * v[1].y);
			float2 dw = float2(
				cos * w[1].x - sin * w[1].y, 
				sin * w[1].x + cos * w[1].y);

			u[1] = u[0] - du;
			u[0] = u[0] + du;
			v[1] = v[0] - dv;
			v[0] = v[0] + dv;
			w[1] = w[0] - dw;
			w[0] = w[0] + dw;

			flag = threadIdx & stride;

			// much slower: vData[i] = v[!flag];
			if(flag) 
			{
				uData[i] = u[0]; 
				vData[i] = v[0]; 
				wData[i] = w[0]; 
			} else { 
				uData[i] = u[1];
				vData[i] = v[1];
				wData[i] = w[1];
			}

			GroupMemoryBarrier();
		}
	}

	[unroll(6)] // log2(MAX_FFT_RESOLUTION) - log2(WARP_WIDTH)
	for(uint stride = WARP_WIDTH; stride < m_resolution; stride <<= 1, scale *= 0.5f)
	{
		if(threadIdx < m_half_resolution)
		{
			uint i = threadIdx ^ (stride-1);
			uint j = threadIdx & (stride-1);

			// much slower: v[!flag] = vData[i];
			if(flag) 
			{
				u[0] = uData[i]; 
				v[0] = vData[i]; 
				w[0] = wData[i]; 
			} else { 
				u[1] = uData[i];
				v[1] = vData[i];
				w[1] = wData[i];
			}

			float sin, cos;
			sincos(j * scale, sin, cos);

			float2 du = float2(
				cos * u[1].x - sin * u[1].y, 
				sin * u[1].x + cos * u[1].y);
			float2 dv = float2(
				cos * v[1].x - sin * v[1].y, 
				sin * v[1].x + cos * v[1].y);
			float2 dw = float2(
				cos * w[1].x - sin * w[1].y, 
				sin * w[1].x + cos * w[1].y);

			u[1] = u[0] - du;
			u[0] = u[0] + du;
			v[1] = v[0] - dv;
			v[0] = v[0] + dv;
			w[1] = w[0] - dw;
			w[0] = w[0] + dw;

			flag = threadIdx & stride;

			// much slower: vData[i] = v[!flag];
			if(flag) 
			{
				uData[i] = u[0]; 
				vData[i] = v[0]; 
				wData[i] = w[0]; 
			} else { 
				uData[i] = u[1];
				vData[i] = v[1];
				wData[i] = w[1];
			}
		}
		
		GroupMemoryBarrierWithGroupSync();
	}
}

StructuredBuffer<float2> g_h0_input : register(t0);
StructuredBuffer<float> g_omega_input : register(t1);

RWStructuredBuffer<float2> g_ht_output : register(u0);
RWStructuredBuffer<float4> g_dt_output : register(u1);

// update Ht, Dt_x, Dt_y from H0 and Omega, fourier transform per row (one CTA per row)
[numthreads(MAX_FFT_RESOLUTION/2, 1, 1)]
void ComputeRows( uint3 dispatchThreadId : SV_DispatchThreadID )
{
	uint columnIdx = dispatchThreadId.x * 2;
	uint rowIdx = dispatchThreadId.y;
	uint reverseColumnIdx = reversebits(columnIdx) >> m_32_minus_log2_resolution;
	int3 n = int3(reverseColumnIdx - m_half_resolution, reverseColumnIdx, rowIdx - m_half_resolution);

	float2 ht[2], dx[2], dy[2];
	if(columnIdx < m_resolution) 
	{
		float4 h0i, h0j;
		double2 omega;

		uint h0_index = rowIdx * m_resolution_plus_one + reverseColumnIdx;
		uint h0_jndex = h0_index + m_half_resolution;
		uint omega_index = rowIdx * m_half_resolution_plus_one;
		uint omega_jndex = omega_index + m_half_resolution;

		h0i.xy = g_h0_input[h0_index];
		h0j.xy = g_h0_input[m_resolution_plus_one_squared_minus_one - h0_index]; 
		omega.x = g_omega_input[omega_index + reverseColumnIdx] * m_time;

		h0i.zw = g_h0_input[h0_jndex];
		h0j.zw = g_h0_input[m_resolution_plus_one_squared_minus_one - h0_jndex]; 
		omega.y = g_omega_input[omega_jndex - reverseColumnIdx] * m_time;

		// modulo 2 * Pi
		const double oneOverTwoPi = 0.15915494309189533576888376337251;
		const double twoPi = 6.283185307179586476925286766559;
		omega -= floor(float2(omega * oneOverTwoPi)) * twoPi;

		float2 sinOmega, cosOmega;
		sincos(float2(omega), sinOmega, cosOmega);

		// H(0) -> H(t)
		ht[0].x = (h0i.x + h0j.x) * cosOmega.x - (h0i.y + h0j.y) * sinOmega.x;
		ht[1].x = (h0i.z + h0j.z) * cosOmega.y - (h0i.w + h0j.w) * sinOmega.y;
		ht[0].y = (h0i.x - h0j.x) * sinOmega.x + (h0i.y - h0j.y) * cosOmega.x;
		ht[1].y = (h0i.z - h0j.z) * sinOmega.y + (h0i.w - h0j.w) * cosOmega.y;

		float2 nr = n.xy || n.z ? rsqrt(float2(n.xy*n.xy + n.z*n.z)) : 0;
		float2 dt0 = float2(-ht[0].y, ht[0].x) * nr.x;
		float2 dt1 = float2(-ht[1].y, ht[1].x) * nr.y;

		dx[0] = n.x * dt0;
		dx[1] = n.y * dt1;
		dy[0] = n.z * dt0;
		dy[1] = n.z * dt1;
	}

	fft(ht, dx, dy, dispatchThreadId.x);

	if(columnIdx < m_resolution)
	{
		uint index = rowIdx * m_resolution + dispatchThreadId.x;

		g_ht_output[index] = ht[0];
		g_ht_output[index+m_half_resolution] = ht[1];

		g_dt_output[index] = float4(dx[0], dy[0]);
		g_dt_output[index+m_half_resolution] = float4(dx[1], dy[1]);
	}
}

StructuredBuffer<float2> g_ht_input : register(t0);
StructuredBuffer<float4> g_dt_input : register(t1);

RWTexture2D<float4> g_displacement_output : register(u0);

// do fourier transform per row of Ht, Dt_x, Dt_y, write displacement texture (one CTA per column)
[numthreads(MAX_FFT_RESOLUTION/2, 1, 1)]
void ComputeColumns( uint3 dispatchThreadId : SV_DispatchThreadID )
{
	uint rowIdx = dispatchThreadId.x * 2;
	uint columnIdx = dispatchThreadId.y;
	uint reverseRowIdx = reversebits(rowIdx) >> m_32_minus_log2_resolution;

	int index = reverseRowIdx * m_resolution + columnIdx;
	int jndex = (m_half_resolution - reverseRowIdx) * m_resolution + columnIdx;

	float2 ht[2], dx[2], dy[2];
	if(rowIdx < m_resolution)
	{
		ht[0] = g_ht_input[index];
		ht[1] = g_ht_input[jndex];
		ht[1].y = -ht[1].y;

		float4 dti = g_dt_input[index];
		float4 dtj = g_dt_input[jndex];

		dx[0] = dti.xy;
		dx[1] = float2(dtj.x, -dtj.y);
		dy[0] = dti.zw;
		dy[1] = float2(dtj.z, -dtj.w);
	}

	fft(ht, dx, dy, dispatchThreadId.x);

	if(rowIdx < m_resolution)
	{
		float sgn = (dispatchThreadId.x + columnIdx) & 0x1 ? -1.0f : +1.0f;
		float scale = m_choppy_scale * sgn;

		g_displacement_output[uint2(columnIdx, dispatchThreadId.x)] = 
			float4(dx[0].x * scale, dy[0].x * scale, ht[0].x * sgn, 0);
		g_displacement_output[uint2(columnIdx, dispatchThreadId.x+m_half_resolution)] = 
			float4(dx[1].x * scale, dy[1].x * scale, ht[1].x * sgn, 0);
	}
}
