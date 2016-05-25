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
#ifdef SUPPORT_DIRECTCOMPUTE
#include "FFT_Simulation_DirectCompute_impl.h"
#include "FFT_Simulation_Manager_DirectCompute_impl.h"
#include "Simulation_Util.h"
#include "CircularFIFO.h"

#include <malloc.h>

#include "generated/ComputeH0_cs_5_0.h"
#include "generated/ComputeColumns_cs_5_0.h"
#include "generated/ComputeRows_cs_5_0.h"

namespace
{
	const DXGI_SAMPLE_DESC kNoSample = {1, 0};
}

NVWaveWorks_FFT_Simulation_DirectCompute_Impl::NVWaveWorks_FFT_Simulation_DirectCompute_Impl(	NVWaveWorks_FFT_Simulation_Manager_DirectCompute_Impl* pManager,
																								const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params) :
	m_pManager(pManager),
	m_params(params),
	m_avoid_frame_depedencies(true),
	m_ReadbackInitialised(false),
	m_DisplacementMapVersion(GFSDK_WaveWorks_InvalidKickID),
	m_d3dAPI(nv_water_d3d_api_undefined)
{
    for(int slot = 0; slot != NumReadbackSlots; ++slot)
    {
		m_readback_kickIDs[slot] = GFSDK_WaveWorks_InvalidKickID;
    }
    m_active_readback_slot = 0;
    m_end_inflight_readback_slots = 1;

	for(int slot = 0; slot != NumTimerSlots; ++slot)
	{
		m_timer_kickIDs[slot] = GFSDK_WaveWorks_InvalidKickID;
		m_timer_results[slot] = 0.f;
	}
	m_active_timer_slot = 0;
	m_end_inflight_timer_slots = 1;
}

NVWaveWorks_FFT_Simulation_DirectCompute_Impl::~NVWaveWorks_FFT_Simulation_DirectCompute_Impl()
{
    releaseAll();
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::initD3D11(ID3D11Device* pD3DDevice)
{
    HRESULT hr;

    if(nv_water_d3d_api_d3d11 != m_d3dAPI)
    {
        releaseAll();
    }
    else if(m_d3d._11.m_device != pD3DDevice)
    {
        releaseAll();
    }

    if(nv_water_d3d_api_undefined == m_d3dAPI)
    {
        m_d3dAPI = nv_water_d3d_api_d3d11;
		memset(&m_d3d._11, 0, sizeof(m_d3d._11));

		m_d3d._11.m_device = pD3DDevice;
        m_d3d._11.m_device->AddRef();
		m_d3d._11.m_device->GetImmediateContext(&m_d3d._11.m_context);

        V_RETURN(allocateAllResources());
    }

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::reinit(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params)
{
	HRESULT hr;

    bool reallocate = false;

    if(params.fft_resolution != m_params.fft_resolution ||
        params.readback_displacements != m_params.readback_displacements)
    {
        reallocate = true;

		// We're reallocating, which breaks various lockstep/synchronization assumptions...
		V_RETURN(m_pManager->beforeReallocateSimulation());
    }

    if(	params.fft_period != m_params.fft_period )
    {
		m_GaussAndOmegaInitialised = false;
    }

    if(	params.wave_amplitude != m_params.wave_amplitude ||
        params.wind_speed != m_params.wind_speed ||
        params.wind_dir.x != m_params.wind_dir.y ||
		params.wind_dir.x != m_params.wind_dir.y ||
        params.wind_dependency != m_params.wind_dependency ||
        params.small_wave_fraction != m_params.small_wave_fraction ||
        params.window_in != m_params.window_in ||
        params.window_out != m_params.window_out )
    {
		m_H0Dirty = true;
    }

	m_params = params;

    if(reallocate)
    {
        releaseAllResources();
        V_RETURN(allocateAllResources());
    }

	return S_OK;
}

namespace 
{
	template <typename T> 
	T sqr(T const& x) 
	{ 
		return x * x; 
	}

	float2 normalize(gfsdk_float2 v)
	{
		float scale = 1.0f / sqrtf(v.x*v.x + v.y*v.y);
		float2 result = {v.x * scale, v.y * scale};
		return result;
	}
}

void NVWaveWorks_FFT_Simulation_DirectCompute_Impl::updateConstantBuffer(double simTime) const
{
	// constants, needs to match cbuffer in FFT_Simulation_DirectCompute_shader.hlsl
	struct __declspec(align(16)) ConstantBuffer
	{
		typedef unsigned __int32 uint;

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
	} constant_buffer;

	assert(sizeof(constant_buffer) < 128); // make sure allocated buffer is big enough

	const float twoPi = 6.28318530718f;
	const float gravity = 9.810f;
	const float sqrtHalf = 0.707106781186f;
	const float euler = 2.71828182846f;

	float fftNorm = powf(float(m_resolution), -0.25f);
	float philNorm = euler / m_params.fft_period;
	float gravityScale = sqr(gravity / sqr(m_params.wind_speed));

	constant_buffer.m_resolution = m_resolution;
	constant_buffer.m_resolution_plus_one = m_resolution + 1;
	constant_buffer.m_half_resolution = m_resolution / 2;
	constant_buffer.m_half_resolution_plus_one = m_resolution / 2 + 1;
	constant_buffer.m_resolution_plus_one_squared_minus_one = sqr(m_resolution + 1) - 1;
	for(unsigned int i=0; (1u << i) <= m_resolution; ++i)
		constant_buffer.m_32_minus_log2_resolution = 32 - i;
	constant_buffer.m_window_in = m_params.window_in;
	constant_buffer.m_window_out = m_params.window_out;
	constant_buffer.m_wind_dir = normalize(m_params.wind_dir);
	constant_buffer.m_frequency_scale = twoPi / m_params.fft_period;
	constant_buffer.m_linear_scale = fftNorm * philNorm * sqrtHalf * m_params.wave_amplitude;
	constant_buffer.m_wind_scale = -sqrt(1 - m_params.wind_dependency);
	constant_buffer.m_root_scale = -0.5f * gravityScale;
	constant_buffer.m_power_scale = -0.5f / gravityScale * sqr(m_params.small_wave_fraction);
	constant_buffer.m_time = simTime;
	constant_buffer.m_choppy_scale = m_params.choppy_scale;

	switch(m_d3dAPI)
	{
	case nv_water_d3d_api_d3d11:
		{
			D3D11_MAPPED_SUBRESOURCE map;
			m_d3d._11.m_context->Map(m_d3d._11.m_buffer_constants, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
			memcpy(map.pData, &constant_buffer, sizeof(constant_buffer));
			m_d3d._11.m_context->Unmap(m_d3d._11.m_buffer_constants, 0);
		}
		break;
	}
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::kick(Graphics_Context* /*pGC*/, double dSimTime, gfsdk_U64 kickID)
{
    HRESULT hr;

    if(!m_GaussAndOmegaInitialised)
    {
        V_RETURN(initGaussAndOmega());
    }

    const double fModeSimTime = dSimTime * (double)m_params.time_scale;

	int timerSlot;
	V_RETURN(consumeAvailableTimerSlot(timerSlot,kickID));

	int readbackSlot;
	V_RETURN(consumeAvailableReadbackSlot(readbackSlot,kickID));

	switch(m_d3dAPI)
	{
	case nv_water_d3d_api_d3d11:
		{
			ID3D11DeviceContext* context = m_d3d._11.m_context;

			context->Begin(m_d3d._11.m_frequency_queries[timerSlot]);
			context->End(m_d3d._11.m_start_queries[timerSlot]);

			updateConstantBuffer(fModeSimTime);
			context->CSSetConstantBuffers(0, 1, &m_d3d._11.m_buffer_constants);

			if(m_avoid_frame_depedencies)
			{
				float zeros[4] = {};
				/* todo: structured buffers have unknown format, therefore can't be cleared
				if(m_H0Dirty)
					context->ClearUnorderedAccessViewFloat(m_d3d._11.m_uav_H0, zeros);
				context->ClearUnorderedAccessViewFloat(m_d3d._11.m_uav_Ht, zeros);
				context->ClearUnorderedAccessViewFloat(m_d3d._11.m_uav_Dt, zeros);
				*/
				context->ClearUnorderedAccessViewFloat(m_d3d._11.m_uav_Displacement, zeros);
			}

			if(m_H0Dirty) 
			{
				context->CSSetShader(m_d3d._11.m_update_h0_shader, NULL, 0);
				context->CSSetUnorderedAccessViews(0, 1, &m_d3d._11.m_uav_H0, NULL);
				context->CSSetShaderResources(0, 1, &m_d3d._11.m_srv_Gauss);
				context->Dispatch(1, m_resolution, 1);
				m_H0Dirty = false;

				#if 0 // read back result for debugging purposes
				{
					D3D11_BUFFER_DESC buffer_desc;
					memset(&buffer_desc, 0, sizeof(buffer_desc));
					buffer_desc.Usage = D3D11_USAGE_STAGING;
					buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
					buffer_desc.ByteWidth = (m_resolution+1)*(m_resolution+1) * sizeof(float2);
					buffer_desc.StructureByteStride = sizeof(float2);
					buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
					ID3D11Buffer* buffer;
					V_RETURN(m_d3d._11.m_device->CreateBuffer(&buffer_desc, nullptr, &buffer));
					context->CopyResource(buffer, m_d3d._11.m_buffer_H0);
					D3D11_MAPPED_SUBRESOURCE mapped;
					context->Map(buffer, 0, D3D11_MAP_READ, 0, &mapped);
					context->Unmap(buffer, 0);
					buffer->Release();
				}
				#endif
			}

			context->CSSetShader(m_d3d._11.m_row_shader, NULL, 0);
			ID3D11UnorderedAccessView* row_uavs[] = { m_d3d._11.m_uav_Ht, m_d3d._11.m_uav_Dt };
			context->CSSetUnorderedAccessViews(0, 2, row_uavs, NULL);
			ID3D11ShaderResourceView* row_srvs[] = { m_d3d._11.m_srv_H0, m_d3d._11.m_srv_Omega };
			context->CSSetShaderResources(0, 2, row_srvs);
			context->Dispatch(1, m_half_resolution_plus_one, 1);

			#if 0 // read back result for debugging purposes
			{
				D3D11_BUFFER_DESC buffer_desc;
				memset(&buffer_desc, 0, sizeof(buffer_desc));
				buffer_desc.Usage = D3D11_USAGE_STAGING;
				buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				buffer_desc.ByteWidth = m_half_resolution_plus_one*m_resolution * sizeof(float2);
				buffer_desc.StructureByteStride = sizeof(float2);
				buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				ID3D11Buffer* buffer;
				V_RETURN(m_d3d._11.m_device->CreateBuffer(&buffer_desc, nullptr, &buffer));
				context->CopyResource(buffer, m_d3d._11.m_buffer_Ht);
				D3D11_MAPPED_SUBRESOURCE mapped;
				context->Map(buffer, 0, D3D11_MAP_READ, 0, &mapped);
				context->Unmap(buffer, 0);
				buffer->Release();
			}
			#endif

			context->CSSetShader(m_d3d._11.m_column_shader, NULL, 0);
			ID3D11UnorderedAccessView* column_uavs[] = { m_d3d._11.m_uav_Displacement, NULL };
			context->CSSetUnorderedAccessViews(0, 2, column_uavs, NULL);
			ID3D11ShaderResourceView* column_srvs[] = { m_d3d._11.m_srv_Ht, m_d3d._11.m_srv_Dt };
			context->CSSetShaderResources(0, 2, column_srvs);
			context->Dispatch(1, m_resolution, 1);

			#if 0 // read back result for debugging purposes
			{
				D3D11_TEXTURE2D_DESC texture_desc;
				texture_desc.Width = m_resolution;
				texture_desc.Height = m_resolution;
				texture_desc.MipLevels = 1;
				texture_desc.ArraySize = 1;
				texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				texture_desc.SampleDesc = kNoSample;
				texture_desc.Usage = D3D11_USAGE_STAGING;
				texture_desc.BindFlags = 0;
				texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				texture_desc.MiscFlags = 0;
				ID3D11Texture2D* texture;
				V_RETURN(m_d3d._11.m_device->CreateTexture2D(&texture_desc, nullptr, &texture));
				context->CopyResource(texture, m_d3d._11.m_texture_Displacement);
				D3D11_MAPPED_SUBRESOURCE mapped;
				context->Map(texture, 0, D3D11_MAP_READ, 0, &mapped);
				context->Unmap(texture, 0);
				texture->Release();
			}
			#endif

			// unbind 
			ID3D11ShaderResourceView* null_srvs[2] = {};
			context->CSSetShaderResources(0, 2, null_srvs);
			ID3D11UnorderedAccessView* null_uavs[2] = {};
			context->CSSetUnorderedAccessViews(0, 2, null_uavs, NULL);
			context->CSSetShader(NULL, NULL, 0);

			if(m_ReadbackInitialised)
			{
				context->CopyResource(m_d3d._11.m_readback_buffers[readbackSlot], m_d3d._11.m_texture_Displacement);
				context->End(m_d3d._11.m_readback_queries[readbackSlot]);
			}

			context->End(m_d3d._11.m_end_queries[timerSlot]);
			context->End(m_d3d._11.m_frequency_queries[timerSlot]);
		}
		break;
	}

	// Update displacement map version
	m_DisplacementMapVersion = kickID;

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::allocateAllResources()
{
    HRESULT hr;

	m_resolution = m_params.fft_resolution;
	m_half_resolution_plus_one = m_resolution / 2 + 1;

	int gauss_size = m_resolution * m_resolution;
	int h0_size = (m_resolution + 1) * (m_resolution + 1);
	int omega_size = m_half_resolution_plus_one * m_half_resolution_plus_one;
	int htdt_size = m_half_resolution_plus_one * m_resolution;

    switch(m_d3dAPI)
    {
    case nv_water_d3d_api_d3d11:
        {
			ID3D11Device* device = m_d3d._11.m_device;

			D3D11_BUFFER_DESC buffer_desc;
			memset(&buffer_desc, 0, sizeof(buffer_desc));
			buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			buffer_desc.Usage = D3D11_USAGE_DEFAULT;
			buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

			// Gauss
			buffer_desc.ByteWidth = gauss_size * sizeof(float2);
			buffer_desc.StructureByteStride = sizeof(float2);
			V_RETURN(device->CreateBuffer(&buffer_desc, nullptr, &m_d3d._11.m_buffer_Gauss));

			// omega
			buffer_desc.ByteWidth = omega_size * sizeof(float);
			buffer_desc.StructureByteStride = sizeof(float);
			V_RETURN(device->CreateBuffer(&buffer_desc, nullptr, &m_d3d._11.m_buffer_Omega));

			buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

			// H(0)
			buffer_desc.ByteWidth = h0_size * sizeof(float2);
			buffer_desc.StructureByteStride = sizeof(float2);
			V_RETURN(device->CreateBuffer(&buffer_desc, nullptr, &m_d3d._11.m_buffer_H0));

			// H(t), D(t)
			buffer_desc.ByteWidth = htdt_size * sizeof(float2);
			buffer_desc.StructureByteStride = sizeof(float2);
			V_RETURN(device->CreateBuffer(&buffer_desc, nullptr, &m_d3d._11.m_buffer_Ht));
			buffer_desc.ByteWidth = htdt_size * sizeof(float4);
			buffer_desc.StructureByteStride = sizeof(float4);
			V_RETURN(device->CreateBuffer(&buffer_desc, nullptr, &m_d3d._11.m_buffer_Dt));

			// Create displacement maps
			D3D11_TEXTURE2D_DESC texture_desc;
			texture_desc.Width = m_resolution;
			texture_desc.Height = m_resolution;
			texture_desc.MipLevels = 1;
			texture_desc.ArraySize = 1;
			texture_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			texture_desc.SampleDesc = kNoSample;
			texture_desc.Usage = D3D11_USAGE_DEFAULT;
			texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			texture_desc.CPUAccessFlags = 0;
			texture_desc.MiscFlags = 0;

			V_RETURN(device->CreateTexture2D(&texture_desc, NULL, &m_d3d._11.m_texture_Displacement));

			// constant buffer
			buffer_desc.ByteWidth = 128;
			buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
			buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			buffer_desc.MiscFlags = 0;
			buffer_desc.StructureByteStride = 0;

			V_RETURN(device->CreateBuffer(&buffer_desc, NULL, &m_d3d._11.m_buffer_constants));

			if(m_params.readback_displacements)
			{
				texture_desc.Usage = D3D11_USAGE_STAGING;
				texture_desc.BindFlags = 0;
				texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

				D3D11_QUERY_DESC event_query_desc = {D3D11_QUERY_EVENT, 0};

				for(int slot = 0; slot != NumReadbackSlots; ++slot)
				{
					V_RETURN(device->CreateTexture2D(&texture_desc, nullptr, m_d3d._11.m_readback_buffers + slot));
					V_RETURN(device->CreateQuery(&event_query_desc, m_d3d._11.m_readback_queries + slot));
					m_readback_kickIDs[slot] = GFSDK_WaveWorks_InvalidKickID;
				}
				m_active_readback_slot = 0;
				m_end_inflight_readback_slots = 1;
				m_d3d._11.m_active_readback_buffer = NULL;

				const int num_readback_FIFO_entries = m_params.num_readback_FIFO_entries;
				if(num_readback_FIFO_entries)
				{
					m_d3d._11.m_pReadbackFIFO = new CircularFIFO<D3D11Objects::ReadbackFIFOSlot>(num_readback_FIFO_entries);
					for(int i = 0; i != m_d3d._11.m_pReadbackFIFO->capacity(); ++i)
					{
						D3D11Objects::ReadbackFIFOSlot& slot = m_d3d._11.m_pReadbackFIFO->raw_at(i);
						V_RETURN(device->CreateTexture2D(&texture_desc, nullptr, &slot.buffer));
						slot.kickID = GFSDK_WaveWorks_InvalidKickID;
					}
				}

				m_ReadbackInitialised = true;
			}

			// timers
			D3D11_QUERY_DESC disjoint_query_desc = {D3D11_QUERY_TIMESTAMP_DISJOINT, 0};
			D3D11_QUERY_DESC timestamp_query_desc = {D3D11_QUERY_TIMESTAMP, 0};
			for(int slot = 0; slot != NumTimerSlots; ++slot)
			{
				device->CreateQuery(&disjoint_query_desc, m_d3d._11.m_frequency_queries + slot);
				device->CreateQuery(&timestamp_query_desc, m_d3d._11.m_start_queries + slot);
				device->CreateQuery(&timestamp_query_desc, m_d3d._11.m_end_queries + slot);
				m_timer_kickIDs[slot] = GFSDK_WaveWorks_InvalidKickID;
				m_timer_results[slot] = 0.f;
			}
			m_active_timer_slot = 0;
			m_end_inflight_timer_slots = 1;

			// shader resource views
			D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
			srv_desc.Format = DXGI_FORMAT_UNKNOWN;
			srv_desc.ViewDimension = D3D_SRV_DIMENSION_BUFFER;
			srv_desc.Buffer.FirstElement = 0;

			srv_desc.Buffer.NumElements = gauss_size;
			V_RETURN(device->CreateShaderResourceView(m_d3d._11.m_buffer_Gauss, &srv_desc, &m_d3d._11.m_srv_Gauss));
			srv_desc.Buffer.NumElements = omega_size;
			V_RETURN(device->CreateShaderResourceView(m_d3d._11.m_buffer_Omega, &srv_desc, &m_d3d._11.m_srv_Omega));
			srv_desc.Buffer.NumElements = h0_size;
			V_RETURN(device->CreateShaderResourceView(m_d3d._11.m_buffer_H0, &srv_desc, &m_d3d._11.m_srv_H0));
			srv_desc.Buffer.NumElements = htdt_size;
			V_RETURN(device->CreateShaderResourceView(m_d3d._11.m_buffer_Ht, &srv_desc, &m_d3d._11.m_srv_Ht));
			V_RETURN(device->CreateShaderResourceView(m_d3d._11.m_buffer_Dt, &srv_desc, &m_d3d._11.m_srv_Dt));
			V_RETURN(device->CreateShaderResourceView(m_d3d._11.m_texture_Displacement, NULL, &m_d3d._11.m_srv_Displacement));

			// unordered access view
			D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
			uav_desc.Format = DXGI_FORMAT_UNKNOWN;
			uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;
			uav_desc.Buffer.Flags = 0;

			uav_desc.Buffer.NumElements = h0_size;
			V_RETURN(device->CreateUnorderedAccessView(m_d3d._11.m_buffer_H0, &uav_desc, &m_d3d._11.m_uav_H0));
			uav_desc.Buffer.NumElements = htdt_size;
			V_RETURN(device->CreateUnorderedAccessView(m_d3d._11.m_buffer_Ht, &uav_desc, &m_d3d._11.m_uav_Ht));
			V_RETURN(device->CreateUnorderedAccessView(m_d3d._11.m_buffer_Dt, &uav_desc, &m_d3d._11.m_uav_Dt));
			V_RETURN(device->CreateUnorderedAccessView(m_d3d._11.m_texture_Displacement, NULL, &m_d3d._11.m_uav_Displacement));

			// shaders
			V_RETURN(device->CreateComputeShader(g_ComputeH0, sizeof(g_ComputeH0), NULL, &m_d3d._11.m_update_h0_shader));
			V_RETURN(device->CreateComputeShader(g_ComputeRows, sizeof(g_ComputeRows), NULL, &m_d3d._11.m_row_shader));
			V_RETURN(device->CreateComputeShader(g_ComputeColumns, sizeof(g_ComputeColumns), NULL, &m_d3d._11.m_column_shader));
        }
		break;
    }

	// Remaining allocations are deferred, in order to ensure that they occur on the host's simulation thread
	m_GaussAndOmegaInitialised = false;
	m_H0Dirty = true;

	m_DisplacementMapVersion = GFSDK_WaveWorks_InvalidKickID;

    return S_OK;
}

void NVWaveWorks_FFT_Simulation_DirectCompute_Impl::releaseAll()
{
	releaseAllResources();

    switch(m_d3dAPI)
    {
    case nv_water_d3d_api_d3d11:
        {
			SAFE_RELEASE(m_d3d._11.m_device);
			SAFE_RELEASE(m_d3d._11.m_context);
        }
        break;
    }

	m_d3dAPI = nv_water_d3d_api_undefined;
}

void NVWaveWorks_FFT_Simulation_DirectCompute_Impl::releaseAllResources()
{
	waitForAllInFlightReadbacks();
	waitForAllInFlightTimers();

    switch(m_d3dAPI)
    {
    case nv_water_d3d_api_d3d11:
        {
			SAFE_RELEASE(m_d3d._11.m_buffer_Gauss);
			SAFE_RELEASE(m_d3d._11.m_buffer_Omega);
			SAFE_RELEASE(m_d3d._11.m_buffer_H0);
			SAFE_RELEASE(m_d3d._11.m_buffer_Ht);
			SAFE_RELEASE(m_d3d._11.m_buffer_Dt);
			SAFE_RELEASE(m_d3d._11.m_texture_Displacement);
			SAFE_RELEASE(m_d3d._11.m_buffer_constants);

			SAFE_RELEASE(m_d3d._11.m_srv_Gauss);
			SAFE_RELEASE(m_d3d._11.m_srv_Omega);
			SAFE_RELEASE(m_d3d._11.m_srv_H0);
			SAFE_RELEASE(m_d3d._11.m_srv_Ht);
			SAFE_RELEASE(m_d3d._11.m_srv_Dt);
			SAFE_RELEASE(m_d3d._11.m_srv_Displacement);

			SAFE_RELEASE(m_d3d._11.m_uav_H0);
			SAFE_RELEASE(m_d3d._11.m_uav_Ht);
			SAFE_RELEASE(m_d3d._11.m_uav_Dt);
			SAFE_RELEASE(m_d3d._11.m_uav_Displacement);

			for(int slot = 0; slot != NumReadbackSlots; ++slot)
			{
				SAFE_RELEASE(m_d3d._11.m_readback_buffers[slot]);
				SAFE_RELEASE(m_d3d._11.m_readback_queries[slot]);
			}

			if(m_d3d._11.m_pReadbackFIFO)
			{
				for(int i = 0; i != m_d3d._11.m_pReadbackFIFO->capacity(); ++i)
				{
					SAFE_RELEASE(m_d3d._11.m_pReadbackFIFO->raw_at(i).buffer);
				}
				SAFE_DELETE(m_d3d._11.m_pReadbackFIFO);
			}

			for(int slot = 0; slot != NumTimerSlots; ++slot)
			{
				SAFE_RELEASE(m_d3d._11.m_frequency_queries[slot]);
				SAFE_RELEASE(m_d3d._11.m_start_queries[slot]);
				SAFE_RELEASE(m_d3d._11.m_end_queries[slot]);
			}

			SAFE_RELEASE(m_d3d._11.m_update_h0_shader);
			SAFE_RELEASE(m_d3d._11.m_row_shader);
			SAFE_RELEASE(m_d3d._11.m_column_shader);
        }
        break;
    }

	m_ReadbackInitialised = false;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::initGaussAndOmega()
{
	int omega_width = m_resolution + 4;
	int gauss_width = gauss_map_resolution + 4;

	float2* gauss = new float2[gauss_map_size];
	float* omega = new float[omega_width * (m_resolution + 1)];

	GFSDK_WaveWorks_Simulation_Util::init_gauss(m_params, gauss);
	GFSDK_WaveWorks_Simulation_Util::init_omega(m_params, omega);

	// copy actually used gauss window around center of max resolution buffer
	// note that we need to generate full resolution to maintain pseudo-randomness
	float2* gauss_src = gauss + (gauss_map_resolution - m_resolution) / 2 * (1 + gauss_width);
	for(unsigned int i=0; i<m_resolution; ++i)
		memmove(gauss + i * m_resolution, gauss_src + i * gauss_width, m_resolution * sizeof(float2));

	// strip unneeded padding
	for(unsigned int i=0; i<m_half_resolution_plus_one; ++i)
		memmove(omega + i * m_half_resolution_plus_one, omega + i * omega_width, m_half_resolution_plus_one * sizeof(float));

	int gauss_size = m_resolution * m_resolution;
	int omega_size = m_half_resolution_plus_one * m_half_resolution_plus_one;

	switch(m_d3dAPI)
	{
	case nv_water_d3d_api_d3d11:
		{
			CD3D11_BOX gauss_box = CD3D11_BOX(0, 0, 0, gauss_size * sizeof(float2), 1, 1);
			m_d3d._11.m_context->UpdateSubresource(m_d3d._11.m_buffer_Gauss, 0, &gauss_box, gauss, 0, 0);
			CD3D11_BOX omega_box = CD3D11_BOX(0, 0, 0, omega_size * sizeof(float), 1, 1);
			m_d3d._11.m_context->UpdateSubresource(m_d3d._11.m_buffer_Omega, 0, &omega_box, omega, 0, 0);
		}
		break;
	}

    SAFE_DELETE_ARRAY(gauss);
    SAFE_DELETE_ARRAY(omega);

    m_GaussAndOmegaInitialised = true;
	m_H0Dirty = true;

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::waitForAllInFlightReadbacks()
{
	HRESULT hr;

	// Consume the readbacks
	int wait_slot = (m_active_readback_slot + 1) % NumReadbackSlots;
	while(wait_slot != m_end_inflight_readback_slots)
	{
		V_RETURN(collectSingleReadbackResult(true));
		wait_slot = (m_active_readback_slot + 1) % NumReadbackSlots;
	}

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::waitForAllInFlightTimers()
{
	switch(m_d3dAPI)
	{
	case nv_water_d3d_api_d3d11:
		{
			// The slot after the active slot is always the first in-flight slot
			for (int slot = m_active_timer_slot; m_end_inflight_timer_slots != (++slot %= NumTimerSlots);)
			{
				while(m_d3d._11.m_context->GetData(m_d3d._11.m_frequency_queries[slot], nullptr, 0, 0))
					;
			}
		}
		break;
	}

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::consumeAvailableReadbackSlot(int& slot, gfsdk_U64 kickID)
{
	if(!m_ReadbackInitialised)
		return S_OK;

	if(m_active_readback_slot == m_end_inflight_readback_slots)
	{
		switch(m_d3dAPI)
		{
		case nv_water_d3d_api_d3d11:
			{
				HRESULT hr = S_FALSE;

				// No slots available - we must wait for the oldest in-flight readback to complete
				int wait_slot = (m_active_readback_slot + 1) % NumReadbackSlots;
				int flag = 0;
				do 
				{
					hr = m_d3d._11.m_context->GetData(m_d3d._11.m_readback_queries[wait_slot], nullptr, 0, flag);
				} while(S_FALSE == hr);

				if(hr == S_OK)
				{
					m_active_readback_slot = wait_slot;
					m_d3d._11.m_active_readback_buffer = m_d3d._11.m_readback_buffers[m_active_readback_slot];
				}
				else
				{
					return hr;
				}
			}
			break;
		}
	}

	slot = m_end_inflight_readback_slots;
	++m_end_inflight_readback_slots %= NumReadbackSlots;
	m_readback_kickIDs[slot] = kickID;

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::consumeAvailableTimerSlot(int& slot, gfsdk_U64 kickID)
{
	if(m_active_timer_slot == m_end_inflight_timer_slots)
	{
		switch(m_d3dAPI)
		{
		case nv_water_d3d_api_d3d11:
			{
				HRESULT hr = S_FALSE;

				// No slots available - we must wait for the oldest in-flight timer to complete
				int wait_slot = (m_active_timer_slot + 1) % NumTimerSlots;
				int flag = 0;

				D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;
				UINT64 start, end;
				do 
				{
					hr =  m_d3d._11.m_context->GetData(m_d3d._11.m_frequency_queries[wait_slot], &disjoint, sizeof(disjoint), flag)
						| m_d3d._11.m_context->GetData(m_d3d._11.m_start_queries[wait_slot], &start, sizeof(start), flag)
						| m_d3d._11.m_context->GetData(m_d3d._11.m_end_queries[wait_slot], &end, sizeof(end), flag);
				} while(S_FALSE == hr);

				if(hr == S_OK)
				{
					m_timer_results[wait_slot] = disjoint.Disjoint ? 0.0f : (end - start) * 1000.0f / disjoint.Frequency;
					m_active_timer_slot = wait_slot;
					m_timer_kickIDs[wait_slot] = kickID;
				}
				else
				{
					return hr;
				}
			}
			break;
		}
	}

	slot = m_end_inflight_timer_slots;
    ++m_end_inflight_timer_slots %= NumTimerSlots;

    return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::resetReadbacks()
{
	HRESULT hr;

	if(!m_ReadbackInitialised)
	{
		// Nothing to reset
		return S_OK;
	}

	V_RETURN(waitForAllInFlightReadbacks());

    m_active_readback_slot = 0;
    m_end_inflight_readback_slots = 1;
	m_readback_kickIDs[m_active_readback_slot] = GFSDK_WaveWorks_InvalidKickID;

	switch(m_d3dAPI)
	{
	case nv_water_d3d_api_d3d11:
		{
			m_d3d._11.m_active_readback_buffer = NULL;
		}
		break;
	}

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::canCollectSingleReadbackResultWithoutBlocking()
{
	if(!m_ReadbackInitialised)
	{
		return S_FALSE;
	}

    const int wait_slot = (m_active_readback_slot + 1) % NumReadbackSlots;
	if(wait_slot == m_end_inflight_readback_slots)
	{
		// Nothing in-flight...
		return S_FALSE;
	}

	// Do the query
	HRESULT query_result = m_d3d._11.m_context->GetData(m_d3d._11.m_readback_queries[wait_slot], nullptr, 0, 0);
	if(S_OK == query_result)
	{
		// Whaddyaknow, it's ready!
		return S_OK;
	}
	else if(S_FALSE == query_result)
	{
		// Not ready
		return S_FALSE;
	}
	else
	{
		// Fail
		return E_FAIL;
	}
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::collectSingleReadbackResult(bool blocking)
{
	if(!m_ReadbackInitialised)
	{
		return S_FALSE;
	}

    const int wait_slot = (m_active_readback_slot + 1) % NumReadbackSlots;

    // Just consume one readback result per check (per function name!)
    if(wait_slot != m_end_inflight_readback_slots)
    {
		if(blocking)
		{
			while(m_d3d._11.m_context->GetData(m_d3d._11.m_readback_queries[wait_slot], nullptr, 0, 0))
				;
            m_active_readback_slot = wait_slot;
			m_d3d._11.m_active_readback_buffer = m_d3d._11.m_readback_buffers[m_active_readback_slot];
			return S_OK;
		}
		else
		{
			const HRESULT query_result = m_d3d._11.m_context->GetData(m_d3d._11.m_readback_queries[wait_slot], nullptr, 0, 0);
			if(S_OK == query_result)
			{
				m_active_readback_slot = wait_slot;
				m_d3d._11.m_active_readback_buffer = m_d3d._11.m_readback_buffers[m_active_readback_slot];
				return S_OK;
			}
			else if(FAILED(query_result))
			{
				return E_FAIL;
			}
		}
    }

	// Nothing in-flight, or else not ready yet
    return S_FALSE;
}

void NVWaveWorks_FFT_Simulation_DirectCompute_Impl::add_displacements_float16_d3d11(	ID3D11Texture2D* buffer,
																						const gfsdk_float2* inSamplePoints,
																						gfsdk_float4* outDisplacements,
																						UINT numSamples,
																						float multiplier
																						)
{
	assert(nv_water_d3d_api_d3d11 == m_d3dAPI);

	D3D11_MAPPED_SUBRESOURCE msr;
	m_d3d._11.m_context->Map(buffer, 0, D3D11_MAP_READ, 0, &msr);
	const BYTE* pRB = reinterpret_cast<BYTE*>(msr.pData);
	GFSDK_WaveWorks_Simulation_Util::add_displacements_float16(m_params, pRB, msr.RowPitch, inSamplePoints, outDisplacements, numSamples, multiplier);
	m_d3d._11.m_context->Unmap(buffer, 0);
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::addDisplacements(	const gfsdk_float2* inSamplePoints,
																			gfsdk_float4* outDisplacements,
																			UINT numSamples
																			)
{
	if(!getReadbackCursor(NULL))
	{
		return S_OK;
	}

	switch(m_d3dAPI)
	{
	case nv_water_d3d_api_d3d11:
		add_displacements_float16_d3d11(m_d3d._11.m_active_readback_buffer, inSamplePoints, outDisplacements, numSamples, 1.f);
		break;
	}

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::addArchivedDisplacements(	float coord,
																					const gfsdk_float2* inSamplePoints,
																					gfsdk_float4* outDisplacements,
																					UINT numSamples
																					)
{
	switch(m_d3dAPI)
	{
	case nv_water_d3d_api_d3d11:
		return addArchivedDisplacementsD3D11(coord, inSamplePoints, outDisplacements, numSamples);
		break;
	default:
		return E_FAIL;
	}
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::addArchivedDisplacementsD3D11(	float coord,
																						const gfsdk_float2* inSamplePoints,
																						gfsdk_float4* outDisplacements,
																						UINT numSamples
																						)
{
	assert(nv_water_d3d_api_d3d11 == m_d3dAPI);

	if(NULL == m_d3d._11.m_pReadbackFIFO)
	{
		// No FIFO, nothing to add
		return S_OK;
	}
	else if(0 == m_d3d._11.m_pReadbackFIFO->range_count())
	{
		// No entries, nothing to add
		return S_OK;
	}

	const float coordMax = float(m_d3d._11.m_pReadbackFIFO->range_count()-1);

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

		switch(m_d3dAPI)
		{
		case nv_water_d3d_api_d3d11:
			add_displacements_float16_d3d11(m_d3d._11.m_pReadbackFIFO->range_at(coord_lower).buffer, inSamplePoints, outDisplacements, numSamples, 1.f-coord_frac);
			add_displacements_float16_d3d11(m_d3d._11.m_pReadbackFIFO->range_at(coord_upper).buffer, inSamplePoints, outDisplacements, numSamples, coord_frac);
			break;
		}
	}
	else
	{
		switch(m_d3dAPI)
		{
		case nv_water_d3d_api_d3d11:
			add_displacements_float16_d3d11(m_d3d._11.m_pReadbackFIFO->range_at(coord_lower).buffer, inSamplePoints, outDisplacements, numSamples, 1.f);
			break;
		}
	}

    return S_OK;
}

bool NVWaveWorks_FFT_Simulation_DirectCompute_Impl::getReadbackCursor(gfsdk_U64* pKickID)
{
	if(!m_params.readback_displacements || !m_ReadbackInitialised)
	{
		return false;
	}

	if(GFSDK_WaveWorks_InvalidKickID == m_readback_kickIDs[m_active_readback_slot])
	{
		// No results yet
		return false;
	}

	if(pKickID)
	{
		*pKickID = m_readback_kickIDs[m_active_readback_slot];
	}

	return true;
}

bool NVWaveWorks_FFT_Simulation_DirectCompute_Impl::hasReadbacksInFlight() const
{
	if(!m_params.readback_displacements || !m_ReadbackInitialised)
	{
		return false;
	}

	int begin_inflight_readback_slots = (m_active_readback_slot + 1) % NumReadbackSlots;
	return begin_inflight_readback_slots != m_end_inflight_readback_slots;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::getTimings(NVWaveWorks_FFT_Simulation_Timings& timings) const
{
	timings.GPU_simulation_time = m_timer_results[m_active_timer_slot];
	timings.GPU_FFT_simulation_time = 0.0f;
	return S_OK;
}


ID3D11ShaderResourceView** NVWaveWorks_FFT_Simulation_DirectCompute_Impl::GetDisplacementMapD3D11()
{
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);
	return &m_d3d._11.m_srv_Displacement;
}

GLuint NVWaveWorks_FFT_Simulation_DirectCompute_Impl::GetDisplacementMapGL2()
{
	return 0;
}

HRESULT NVWaveWorks_FFT_Simulation_DirectCompute_Impl::archiveDisplacements()
{
	gfsdk_U64 kickID = GFSDK_WaveWorks_InvalidKickID;
	if(getReadbackCursor(&kickID) && m_d3d._11.m_pReadbackFIFO)
	{
		// We avoid big memcpys by swapping pointers, specifically we will either evict a FIFO entry or else use a free one and
		// swap it with one of the slots used for in-flight readbacks
		//
		// First job is to check whether the FIFO already contains this result. We know that if it does contain this result,
		// it will be the last one pushed on...
		if(m_d3d._11.m_pReadbackFIFO->range_count())
		{
			if(kickID == m_d3d._11.m_pReadbackFIFO->range_at(0).kickID)
			{
				// It is an error to archive the same results twice...
				return E_FAIL;
			}
		}

		// Assuming the current results have not been archived, the next-up readback buffer should match the one we are serving up
		// for addDisplacements...
		assert(m_d3d._11.m_active_readback_buffer == m_d3d._11.m_readback_buffers[m_active_readback_slot]);

		D3D11Objects::ReadbackFIFOSlot& slot = m_d3d._11.m_pReadbackFIFO->consume_one();
		m_d3d._11.m_readback_buffers[m_active_readback_slot] = slot.buffer;
		slot.buffer = m_d3d._11.m_active_readback_buffer;
		slot.kickID = kickID;
	}

	return S_OK;
}

#endif //SUPPORT_DIRECTCOMPUTE
