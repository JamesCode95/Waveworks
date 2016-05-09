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
#include "Savestate_impl.h"
#include "Graphics_Context.h"

#include <string.h>

GFSDK_WaveWorks_Savestate::GFSDK_WaveWorks_Savestate(IDirect3DDevice9* D3D9_ONLY(pD3DDevice), GFSDK_WaveWorks_StatePreserveFlags PreserveFlags) :
	m_UserPreserveFlags(PreserveFlags)
{
	memset(&m_d3d, 0, sizeof(m_d3d));

	m_d3dAPI = nv_water_d3d_api_d3d9;

#if WAVEWORKS_ENABLE_D3D9
	m_d3d._9.m_pEndVertexShaderConstantF = m_d3d._9.m_VertexShaderConstantF;
	m_d3d._9.m_pEndPixelShaderConstantF = m_d3d._9.m_PixelShaderConstantF;
	m_d3d._9.m_pEndTexture = m_d3d._9.m_Texture;
	m_d3d._9.m_pEndSamplerState = m_d3d._9.m_SamplerState;
	m_d3d._9.m_pEndRenderState = m_d3d._9.m_RenderState;

	m_d3d._9.m_pd3d9Device = pD3DDevice;
	m_d3d._9.m_pd3d9Device->AddRef();
#endif
}

GFSDK_WaveWorks_Savestate::GFSDK_WaveWorks_Savestate(ID3D10Device* D3D10_ONLY(pD3DDevice), GFSDK_WaveWorks_StatePreserveFlags PreserveFlags) :
	m_UserPreserveFlags(PreserveFlags)
{
	memset(&m_d3d, 0, sizeof(m_d3d));

	m_d3dAPI = nv_water_d3d_api_d3d10;

#if WAVEWORKS_ENABLE_D3D10
	m_d3d._10.m_pEndVertexShaderConstantBuffer = m_d3d._10.m_VertexShaderConstantBuffer;
	m_d3d._10.m_pEndPixelShaderConstantBuffer = m_d3d._10.m_PixelShaderConstantBuffer;
	m_d3d._10.m_pEndVertexShaderSampler = m_d3d._10.m_VertexShaderSampler;
	m_d3d._10.m_pEndPixelShaderSampler = m_d3d._10.m_PixelShaderSampler;
	m_d3d._10.m_pEndVertexShaderResource = m_d3d._10.m_VertexShaderResource;
	m_d3d._10.m_pEndPixelShaderResource = m_d3d._10.m_PixelShaderResource;

	m_d3d._10.m_pd3d10Device = pD3DDevice;
	m_d3d._10.m_pd3d10Device->AddRef();
#endif
}

GFSDK_WaveWorks_Savestate::GFSDK_WaveWorks_Savestate(ID3D11Device* D3D11_ONLY(pD3DDevice), GFSDK_WaveWorks_StatePreserveFlags PreserveFlags) :
	m_UserPreserveFlags(PreserveFlags)
{
	memset(&m_d3d, 0, sizeof(m_d3d));

	m_d3dAPI = nv_water_d3d_api_d3d11;

#if WAVEWORKS_ENABLE_D3D11
	m_d3d._11.m_pEndVertexShaderConstantBuffer = m_d3d._11.m_VertexShaderConstantBuffer;
	m_d3d._11.m_pEndPixelShaderConstantBuffer = m_d3d._11.m_PixelShaderConstantBuffer;
	m_d3d._11.m_pEndHullShaderConstantBuffer = m_d3d._11.m_HullShaderConstantBuffer;
	m_d3d._11.m_pEndDomainShaderConstantBuffer = m_d3d._11.m_DomainShaderConstantBuffer;
	m_d3d._11.m_pEndComputeShaderConstantBuffer = m_d3d._11.m_ComputeShaderConstantBuffer;
	m_d3d._11.m_pEndVertexShaderSampler = m_d3d._11.m_VertexShaderSampler;
	m_d3d._11.m_pEndPixelShaderSampler = m_d3d._11.m_PixelShaderSampler;
	m_d3d._11.m_pEndHullShaderSampler = m_d3d._11.m_HullShaderSampler;
	m_d3d._11.m_pEndDomainShaderSampler = m_d3d._11.m_DomainShaderSampler;
	m_d3d._11.m_pEndComputeShaderSampler = m_d3d._11.m_ComputeShaderSampler;
	m_d3d._11.m_pEndVertexShaderResource = m_d3d._11.m_VertexShaderResource;
	m_d3d._11.m_pEndPixelShaderResource = m_d3d._11.m_PixelShaderResource;
	m_d3d._11.m_pEndHullShaderResource = m_d3d._11.m_HullShaderResource;
	m_d3d._11.m_pEndDomainShaderResource = m_d3d._11.m_DomainShaderResource;
	m_d3d._11.m_pEndComputeShaderResource = m_d3d._11.m_ComputeShaderResource;
	m_d3d._11.m_pEndComputeShaderUAV = m_d3d._11.m_ComputeShaderUAV;

	m_d3d._11.m_pd3d11Device = pD3DDevice;
	m_d3d._11.m_pd3d11Device->AddRef();
#endif
}

GFSDK_WaveWorks_Savestate::~GFSDK_WaveWorks_Savestate()
{
#if WAVEWORKS_ENABLE_GRAPHICS
	switch(m_d3dAPI)
	{
#if WAVEWORKS_ENABLE_D3D9
	case nv_water_d3d_api_d3d9:
		ReleaseD3D9Resources();
		m_d3d._9.m_pd3d9Device->Release();
		break;
#endif
#if WAVEWORKS_ENABLE_D3D10
	case nv_water_d3d_api_d3d10:
		ReleaseD3D10Resources();
		m_d3d._10.m_pd3d10Device->Release();
		break;
#endif
#if WAVEWORKS_ENABLE_D3D11
	case nv_water_d3d_api_d3d11:
		ReleaseD3D11Resources();
		m_d3d._11.m_pd3d11Device->Release();
		break;
#endif
	default:
		break;
	}
#endif // WAVEWORKS_ENABLE_GRAPHICS
}

HRESULT GFSDK_WaveWorks_Savestate::Restore(Graphics_Context* pGC)
{
#if WAVEWORKS_ENABLE_GRAPHICS
	switch(m_d3dAPI)
	{
#if WAVEWORKS_ENABLE_D3D9
	case nv_water_d3d_api_d3d9:
		return RestoreD3D9();
#endif
#if WAVEWORKS_ENABLE_D3D10
	case nv_water_d3d_api_d3d10:
		return RestoreD3D10();
#endif
#if WAVEWORKS_ENABLE_D3D11
	case nv_water_d3d_api_d3d11:
		{
			ID3D11DeviceContext* pDC_d3d11 = pGC->d3d11();
			return RestoreD3D11(pDC_d3d11);
		}
#endif
	default:
		break;
	}
#endif // WAVEWORKS_ENABLE_GRAPHICS

	return E_FAIL;
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D9Viewport()
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d9);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Viewports) && !(m_d3d._9.m_PreservedFlags & D3D9Objects::ViewportPreserved))
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->GetViewport(&m_d3d._9.m_Viewport));
		m_d3d._9.m_PreservedFlags |= D3D9Objects::ViewportPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D9RenderTargets()
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d9);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_RenderTargets) && !(m_d3d._9.m_PreservedFlags & D3D9Objects::RenderTargetsPreserved))
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->GetRenderTarget(0, &m_d3d._9.m_pRenderTarget));
		V_RETURN(m_d3d._9.m_pd3d9Device->GetDepthStencilSurface(&m_d3d._9.m_pDepthStencil));
		
		m_d3d._9.m_PreservedFlags |= D3D9Objects::RenderTargetsPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D9Shaders()
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d9);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Shaders) && !(m_d3d._9.m_PreservedFlags & D3D9Objects::ShadersPreserved))
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->GetVertexShader(&m_d3d._9.m_pVertexShader));
		V_RETURN(m_d3d._9.m_pd3d9Device->GetPixelShader(&m_d3d._9.m_pPixelShader));
		
		m_d3d._9.m_PreservedFlags |= D3D9Objects::ShadersPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D9PixelShaderConstantF(UINT D3D9_ONLY(ix), UINT D3D9_ONLY(count))
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d9);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_ShaderConstants)
	{
		for(; count; --count, ++ix)
		{
			if(!m_d3d._9.m_PixelShaderConstantF_Flags[ix])
			{
				m_d3d._9.m_pEndPixelShaderConstantF->regIndex = ix;
				V_RETURN(m_d3d._9.m_pd3d9Device->GetPixelShaderConstantF(ix, m_d3d._9.m_pEndPixelShaderConstantF->value, 1));
				++m_d3d._9.m_pEndPixelShaderConstantF;

				m_d3d._9.m_PixelShaderConstantF_Flags[ix] = 1;
			}
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D9VertexShaderConstantF(UINT D3D9_ONLY(ix), UINT D3D9_ONLY(count))
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d9);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_ShaderConstants)
	{
		for(; count; --count, ++ix)
		{
			if(!m_d3d._9.m_VertexShaderConstantF_Flags[ix])
			{
				m_d3d._9.m_pEndVertexShaderConstantF->regIndex = ix;
				V_RETURN(m_d3d._9.m_pd3d9Device->GetVertexShaderConstantF(ix, m_d3d._9.m_pEndVertexShaderConstantF->value, 1));
				++m_d3d._9.m_pEndVertexShaderConstantF;

				m_d3d._9.m_VertexShaderConstantF_Flags[ix] = 1;
			}
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D9Texture(UINT D3D9_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d9);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		if(!(m_d3d._9.m_Texture_Flags[ix]))
		{
			m_d3d._9.m_pEndTexture->regIndex = ix;
			V_RETURN(m_d3d._9.m_pd3d9Device->GetTexture(ix, &m_d3d._9.m_pEndTexture->pTexture));
			++m_d3d._9.m_pEndTexture;

			m_d3d._9.m_Texture_Flags[ix] = 1;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D9SamplerState(UINT D3D9_ONLY(ix), D3DSAMPLERSTATETYPE D3D9_ONLY(type))
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d9);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		if(!(m_d3d._9.m_SamplerState_Flags[ix][type]))
		{
			m_d3d._9.m_pEndSamplerState->regIndex = ix;
			m_d3d._9.m_pEndSamplerState->type = type;
			V_RETURN(m_d3d._9.m_pd3d9Device->GetSamplerState(ix, type, &m_d3d._9.m_pEndSamplerState->value));
			++m_d3d._9.m_pEndSamplerState;

			m_d3d._9.m_SamplerState_Flags[ix][type] = 0;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D9RenderState(D3DRENDERSTATETYPE D3D9_ONLY(rs))
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d9);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Other)
	{
		if(!(m_d3d._9.m_RenderState_Flags[rs]))
		{
			m_d3d._9.m_pEndRenderState->type = rs;
			V_RETURN(m_d3d._9.m_pd3d9Device->GetRenderState(rs, &m_d3d._9.m_pEndRenderState->value));
			++m_d3d._9.m_pEndRenderState;

			m_d3d._9.m_RenderState_Flags[rs] = 1;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D9Streams()
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d9);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Streams) && !(m_d3d._9.m_PreservedFlags & D3D9Objects::StreamsPreserved))
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->GetVertexDeclaration(&m_d3d._9.m_pDecl));
		V_RETURN(m_d3d._9.m_pd3d9Device->GetStreamSource(0, &m_d3d._9.m_pStream0VB, &m_d3d._9.m_Stream0Offset, &m_d3d._9.m_Stream0Stride));
		V_RETURN(m_d3d._9.m_pd3d9Device->GetIndices(&m_d3d._9.m_pIB));
		
		m_d3d._9.m_PreservedFlags |= D3D9Objects::StreamsPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::RestoreD3D9()
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d9);

	if(m_d3d._9.m_PreservedFlags & D3D9Objects::RenderTargetsPreserved)
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderTarget(0, m_d3d._9.m_pRenderTarget));
		V_RETURN(m_d3d._9.m_pd3d9Device->SetDepthStencilSurface(m_d3d._9.m_pDepthStencil));
	}

	if(m_d3d._9.m_PreservedFlags & D3D9Objects::ViewportPreserved)
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->SetViewport(&m_d3d._9.m_Viewport));
	}

	if(m_d3d._9.m_PreservedFlags & D3D9Objects::ShadersPreserved)
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->SetVertexShader(m_d3d._9.m_pVertexShader));
		V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShader(m_d3d._9.m_pPixelShader));
	}

	for(D3D9Objects::ShaderConstantF* it = m_d3d._9.m_VertexShaderConstantF; it != m_d3d._9.m_pEndVertexShaderConstantF; ++it)
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->SetVertexShaderConstantF(it->regIndex, it->value, 1));
		m_d3d._9.m_VertexShaderConstantF_Flags[it->regIndex] = 0;
	}

	for(D3D9Objects::ShaderConstantF* it = m_d3d._9.m_PixelShaderConstantF; it != m_d3d._9.m_pEndPixelShaderConstantF; ++it)
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->SetPixelShaderConstantF(it->regIndex, it->value, 1));
		m_d3d._9.m_PixelShaderConstantF_Flags[it->regIndex] = 0;
	}

	for(D3D9Objects::Texture* it = m_d3d._9.m_Texture; it != m_d3d._9.m_pEndTexture; ++it)
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->SetTexture(it->regIndex, it->pTexture));
		m_d3d._9.m_Texture_Flags[it->regIndex] = 0;
	}

	for(D3D9Objects::SamplerState* it = m_d3d._9.m_SamplerState; it != m_d3d._9.m_pEndSamplerState; ++it)
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->SetSamplerState(it->regIndex, it->type, it->value));
		m_d3d._9.m_SamplerState_Flags[it->regIndex][it->type] = 0;
	}

	for(D3D9Objects::RenderState* it = m_d3d._9.m_RenderState; it != m_d3d._9.m_pEndRenderState; ++it)
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->SetRenderState(it->type, it->value));
		m_d3d._9.m_RenderState_Flags[it->type] = 0;
	}

	if(m_d3d._9.m_PreservedFlags & D3D9Objects::StreamsPreserved)
	{
		V_RETURN(m_d3d._9.m_pd3d9Device->SetVertexDeclaration(m_d3d._9.m_pDecl));
		V_RETURN(m_d3d._9.m_pd3d9Device->SetStreamSource(0, m_d3d._9.m_pStream0VB, m_d3d._9.m_Stream0Offset, m_d3d._9.m_Stream0Stride));
		V_RETURN(m_d3d._9.m_pd3d9Device->SetIndices(m_d3d._9.m_pIB));
	}

	// Release ref-counts etc.
	V_RETURN(ReleaseD3D9Resources());

	// Reset remaining flags etc.
	m_d3d._9.m_PreservedFlags = 0;

	m_d3d._9.m_pEndVertexShaderConstantF = m_d3d._9.m_VertexShaderConstantF;
	m_d3d._9.m_pEndPixelShaderConstantF = m_d3d._9.m_PixelShaderConstantF;
	m_d3d._9.m_pEndTexture = m_d3d._9.m_Texture;
	m_d3d._9.m_pEndSamplerState = m_d3d._9.m_SamplerState;
	m_d3d._9.m_pEndRenderState = m_d3d._9.m_RenderState;

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::ReleaseD3D9Resources()
{
#if WAVEWORKS_ENABLE_D3D9
	SAFE_RELEASE(m_d3d._9.m_pRenderTarget);
	SAFE_RELEASE(m_d3d._9.m_pDepthStencil);

	SAFE_RELEASE(m_d3d._9.m_pVertexShader);
	SAFE_RELEASE(m_d3d._9.m_pPixelShader);

	for(D3D9Objects::Texture* it = m_d3d._9.m_Texture; it != m_d3d._9.m_pEndTexture; ++it)
	{
		SAFE_RELEASE(it->pTexture);
	}

	SAFE_RELEASE(m_d3d._9.m_pDecl);
	SAFE_RELEASE(m_d3d._9.m_pStream0VB);
	SAFE_RELEASE(m_d3d._9.m_pIB);

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10Viewport()
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Viewports) && !(m_d3d._10.m_PreservedFlags & D3D10Objects::ViewportPreserved))
	{
		UINT num_vp = 1;
		m_d3d._10.m_pd3d10Device->RSGetViewports(&num_vp, &m_d3d._10.m_Viewport);
		m_d3d._10.m_PreservedFlags |= D3D10Objects::ViewportPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10RenderTargets()
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_RenderTargets) && !(m_d3d._10.m_PreservedFlags & D3D10Objects::RenderTargetsPreserved))
	{
		m_d3d._10.m_pd3d10Device->OMGetRenderTargets(1, &m_d3d._10.m_pRenderTarget, &m_d3d._10.m_pDepthStencil);
		m_d3d._10.m_PreservedFlags |= D3D10Objects::RenderTargetsPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10Shaders()
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Shaders) && !(m_d3d._10.m_PreservedFlags & D3D10Objects::ShadersPreserved))
	{
		m_d3d._10.m_pd3d10Device->VSGetShader(&m_d3d._10.m_pVertexShader);
		m_d3d._10.m_pd3d10Device->GSGetShader(&m_d3d._10.m_pGeomShader);
		m_d3d._10.m_pd3d10Device->PSGetShader(&m_d3d._10.m_pPixelShader);

		m_d3d._10.m_PreservedFlags |= D3D10Objects::ShadersPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10Streams()
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Streams) && !(m_d3d._10.m_PreservedFlags & D3D10Objects::StreamsPreserved))
	{
		m_d3d._10.m_pd3d10Device->IAGetVertexBuffers(0, 1, &m_d3d._10.m_pSlot0VB, &m_d3d._10.m_Slot0VBOffset, &m_d3d._10.m_Slot0VBStride);
		m_d3d._10.m_pd3d10Device->IAGetIndexBuffer(&m_d3d._10.m_pIB, &m_d3d._10.m_IBFormat, &m_d3d._10.m_IBOffset);
		m_d3d._10.m_pd3d10Device->IAGetInputLayout(&m_d3d._10.m_pLayout);
		m_d3d._10.m_pd3d10Device->IAGetPrimitiveTopology(&m_d3d._10.m_Topology);

		m_d3d._10.m_PreservedFlags |= D3D10Objects::StreamsPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10DepthStencil()
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Other) && !(m_d3d._10.m_PreservedFlags & D3D10Objects::DepthStencilPreserved))
	{
		m_d3d._10.m_pd3d10Device->OMGetDepthStencilState(&m_d3d._10.m_pDepthStencilState, &m_d3d._10.m_StencilRef);
		m_d3d._10.m_PreservedFlags |= D3D10Objects::DepthStencilPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10Blend()
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Other) && !(m_d3d._10.m_PreservedFlags & D3D10Objects::BlendPreserved))
	{
		m_d3d._10.m_pd3d10Device->OMGetBlendState(&m_d3d._10.m_pBlendState, m_d3d._10.m_BlendFactors, &m_d3d._10.m_SampleMask);
		m_d3d._10.m_PreservedFlags |= D3D10Objects::BlendPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10Raster()
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Other) && !(m_d3d._10.m_PreservedFlags & D3D10Objects::RasterPreserved))
	{
		m_d3d._10.m_pd3d10Device->RSGetState(&m_d3d._10.m_pRSState);
		m_d3d._10.m_PreservedFlags |= D3D10Objects::RasterPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10PixelShaderConstantBuffer(UINT D3D10_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_ShaderConstants)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._10.m_PixelShaderConstantBuffer_Flags & ixBit))
		{
			m_d3d._10.m_pEndPixelShaderConstantBuffer->regIndex = ix;
			m_d3d._10.m_pd3d10Device->PSGetConstantBuffers(ix, 1, &m_d3d._10.m_pEndPixelShaderConstantBuffer->pBuffer);
			++m_d3d._10.m_pEndPixelShaderConstantBuffer;

			m_d3d._10.m_PixelShaderConstantBuffer_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10VertexShaderConstantBuffer(UINT D3D10_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_ShaderConstants)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._10.m_VertexShaderConstantBuffer_Flags & ixBit))
		{
			m_d3d._10.m_pEndVertexShaderConstantBuffer->regIndex = ix;
			m_d3d._10.m_pd3d10Device->VSGetConstantBuffers(ix, 1, &m_d3d._10.m_pEndVertexShaderConstantBuffer->pBuffer);
			++m_d3d._10.m_pEndVertexShaderConstantBuffer;

			m_d3d._10.m_VertexShaderConstantBuffer_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10PixelShaderSampler(UINT D3D10_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._10.m_PixelShaderSampler_Flags & ixBit))
		{
			m_d3d._10.m_pEndPixelShaderSampler->regIndex = ix;
			m_d3d._10.m_pd3d10Device->PSGetSamplers(ix, 1, &m_d3d._10.m_pEndPixelShaderSampler->pSampler);
			++m_d3d._10.m_pEndPixelShaderSampler;

			m_d3d._10.m_PixelShaderSampler_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10VertexShaderSampler(UINT D3D10_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._10.m_VertexShaderSampler_Flags & ixBit))
		{
			m_d3d._10.m_pEndVertexShaderSampler->regIndex = ix;
			m_d3d._10.m_pd3d10Device->VSGetSamplers(ix, 1, &m_d3d._10.m_pEndVertexShaderSampler->pSampler);
			++m_d3d._10.m_pEndVertexShaderSampler;

			m_d3d._10.m_VertexShaderSampler_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10PixelShaderResource(UINT D3D10_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		if(!m_d3d._10.m_PixelShaderResource_Flags[ix])
		{
			m_d3d._10.m_pEndPixelShaderResource->regIndex = ix;
			m_d3d._10.m_pd3d10Device->PSGetShaderResources(ix, 1, &m_d3d._10.m_pEndPixelShaderResource->pResource);
			++m_d3d._10.m_pEndPixelShaderResource;

			m_d3d._10.m_PixelShaderResource_Flags[ix] = 1;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D10VertexShaderResource(UINT D3D10_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		if(!m_d3d._10.m_VertexShaderResource_Flags[ix])
		{
			m_d3d._10.m_pEndVertexShaderResource->regIndex = ix;
			m_d3d._10.m_pd3d10Device->VSGetShaderResources(ix, 1, &m_d3d._10.m_pEndVertexShaderResource->pResource);
			++m_d3d._10.m_pEndVertexShaderResource;

			m_d3d._10.m_VertexShaderResource_Flags[ix] = 1;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::RestoreD3D10()
{
#if WAVEWORKS_ENABLE_D3D10
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	if(m_d3d._10.m_PreservedFlags & D3D10Objects::RenderTargetsPreserved)
	{
		m_d3d._10.m_pd3d10Device->OMSetRenderTargets(1, &m_d3d._10.m_pRenderTarget, m_d3d._10.m_pDepthStencil);
	}

	if(m_d3d._10.m_PreservedFlags & D3D10Objects::ViewportPreserved)
	{
		m_d3d._10.m_pd3d10Device->RSSetViewports(1, &m_d3d._10.m_Viewport);
	}

	if(m_d3d._10.m_PreservedFlags & D3D10Objects::ShadersPreserved)
	{
		m_d3d._10.m_pd3d10Device->VSSetShader(m_d3d._10.m_pVertexShader);
		m_d3d._10.m_pd3d10Device->GSSetShader(m_d3d._10.m_pGeomShader);
		m_d3d._10.m_pd3d10Device->PSSetShader(m_d3d._10.m_pPixelShader);
	}

	if(m_d3d._10.m_PreservedFlags & D3D10Objects::StreamsPreserved)
	{
		m_d3d._10.m_pd3d10Device->IASetVertexBuffers(0, 1, &m_d3d._10.m_pSlot0VB, &m_d3d._10.m_Slot0VBOffset, &m_d3d._10.m_Slot0VBStride);
		m_d3d._10.m_pd3d10Device->IASetIndexBuffer(m_d3d._10.m_pIB, m_d3d._10.m_IBFormat, m_d3d._10.m_IBOffset);
		m_d3d._10.m_pd3d10Device->IASetInputLayout(m_d3d._10.m_pLayout);
		m_d3d._10.m_pd3d10Device->IASetPrimitiveTopology(m_d3d._10.m_Topology);
	}

	if(m_d3d._10.m_PreservedFlags & D3D10Objects::DepthStencilPreserved)
	{
		m_d3d._10.m_pd3d10Device->OMSetDepthStencilState(m_d3d._10.m_pDepthStencilState, m_d3d._10.m_StencilRef);
	}

	if(m_d3d._10.m_PreservedFlags & D3D10Objects::BlendPreserved)
	{
		m_d3d._10.m_pd3d10Device->OMSetBlendState(m_d3d._10.m_pBlendState, m_d3d._10.m_BlendFactors, m_d3d._10.m_SampleMask);
	}

	if(m_d3d._10.m_PreservedFlags & D3D10Objects::RasterPreserved)
	{
		m_d3d._10.m_pd3d10Device->RSSetState(m_d3d._10.m_pRSState);
	}

	for(D3D10Objects::ShaderConstantBuffer* it = m_d3d._10.m_VertexShaderConstantBuffer; it != m_d3d._10.m_pEndVertexShaderConstantBuffer; ++it)
	{
		m_d3d._10.m_pd3d10Device->VSSetConstantBuffers(it->regIndex, 1, &it->pBuffer);
	}
	m_d3d._10.m_VertexShaderConstantBuffer_Flags = 0;

	for(D3D10Objects::ShaderConstantBuffer* it = m_d3d._10.m_PixelShaderConstantBuffer; it != m_d3d._10.m_pEndPixelShaderConstantBuffer; ++it)
	{
		m_d3d._10.m_pd3d10Device->PSSetConstantBuffers(it->regIndex, 1, &it->pBuffer);
	}
	m_d3d._10.m_PixelShaderConstantBuffer_Flags = 0;

	for(D3D10Objects::ShaderSampler* it = m_d3d._10.m_VertexShaderSampler; it != m_d3d._10.m_pEndVertexShaderSampler; ++it)
	{
		m_d3d._10.m_pd3d10Device->VSSetSamplers(it->regIndex, 1, &it->pSampler);
	}
	m_d3d._10.m_VertexShaderSampler_Flags = 0;

	for(D3D10Objects::ShaderSampler* it = m_d3d._10.m_PixelShaderSampler; it != m_d3d._10.m_pEndPixelShaderSampler; ++it)
	{
		m_d3d._10.m_pd3d10Device->PSSetSamplers(it->regIndex, 1, &it->pSampler);
	}
	m_d3d._10.m_PixelShaderSampler_Flags = 0;

	for(D3D10Objects::ShaderResource* it = m_d3d._10.m_VertexShaderResource; it != m_d3d._10.m_pEndVertexShaderResource; ++it)
	{
		m_d3d._10.m_pd3d10Device->VSSetShaderResources(it->regIndex, 1, &it->pResource);
		m_d3d._10.m_VertexShaderResource_Flags[it->regIndex] = 0;
	}

	for(D3D10Objects::ShaderResource* it = m_d3d._10.m_PixelShaderResource; it != m_d3d._10.m_pEndPixelShaderResource; ++it)
	{
		m_d3d._10.m_pd3d10Device->PSSetShaderResources(it->regIndex, 1, &it->pResource);
		m_d3d._10.m_PixelShaderResource_Flags[it->regIndex] = 0;
	}

	// Release ref-counts etc.
	V_RETURN(ReleaseD3D10Resources());

	// Reset remaining flags etc.
	m_d3d._10.m_PreservedFlags = 0;

	m_d3d._10.m_pEndVertexShaderConstantBuffer = m_d3d._10.m_VertexShaderConstantBuffer;
	m_d3d._10.m_pEndPixelShaderConstantBuffer = m_d3d._10.m_PixelShaderConstantBuffer;
	m_d3d._10.m_pEndVertexShaderSampler = m_d3d._10.m_VertexShaderSampler;
	m_d3d._10.m_pEndPixelShaderSampler = m_d3d._10.m_PixelShaderSampler;
	m_d3d._10.m_pEndVertexShaderResource = m_d3d._10.m_VertexShaderResource;
	m_d3d._10.m_pEndPixelShaderResource = m_d3d._10.m_PixelShaderResource;

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::ReleaseD3D10Resources()
{
#if WAVEWORKS_ENABLE_D3D10
	assert(m_d3dAPI == nv_water_d3d_api_d3d10);

	SAFE_RELEASE(m_d3d._10.m_pRenderTarget);
	SAFE_RELEASE(m_d3d._10.m_pDepthStencil);

	SAFE_RELEASE(m_d3d._10.m_pVertexShader);
	SAFE_RELEASE(m_d3d._10.m_pGeomShader);
	SAFE_RELEASE(m_d3d._10.m_pPixelShader);

	SAFE_RELEASE(m_d3d._10.m_pSlot0VB);
	SAFE_RELEASE(m_d3d._10.m_pIB);
	SAFE_RELEASE(m_d3d._10.m_pLayout);

	SAFE_RELEASE(m_d3d._10.m_pDepthStencilState);

	SAFE_RELEASE(m_d3d._10.m_pBlendState);

	SAFE_RELEASE(m_d3d._10.m_pRSState);

	for(D3D10Objects::ShaderConstantBuffer* it = m_d3d._10.m_VertexShaderConstantBuffer; it != m_d3d._10.m_pEndVertexShaderConstantBuffer; ++it)
	{
		SAFE_RELEASE(it->pBuffer);
	}

	for(D3D10Objects::ShaderConstantBuffer* it = m_d3d._10.m_PixelShaderConstantBuffer; it != m_d3d._10.m_pEndPixelShaderConstantBuffer; ++it)
	{
		SAFE_RELEASE(it->pBuffer);
	}

	for(D3D10Objects::ShaderSampler* it = m_d3d._10.m_VertexShaderSampler; it != m_d3d._10.m_pEndVertexShaderSampler; ++it)
	{
		SAFE_RELEASE(it->pSampler);
	}

	for(D3D10Objects::ShaderSampler* it = m_d3d._10.m_PixelShaderSampler; it != m_d3d._10.m_pEndPixelShaderSampler; ++it)
	{
		SAFE_RELEASE(it->pSampler);
	}

	for(D3D10Objects::ShaderResource* it = m_d3d._10.m_VertexShaderResource; it != m_d3d._10.m_pEndVertexShaderResource; ++it)
	{
		SAFE_RELEASE(it->pResource);
	}

	for(D3D10Objects::ShaderResource* it = m_d3d._10.m_PixelShaderResource; it != m_d3d._10.m_pEndPixelShaderResource; ++it)
	{
		SAFE_RELEASE(it->pResource);
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11Viewport(ID3D11DeviceContext* D3D11_ONLY(pDC))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Viewports) && !(m_d3d._11.m_PreservedFlags & D3D11Objects::ViewportPreserved))
	{
		UINT num_vp = 1;
		pDC->RSGetViewports(&num_vp, &m_d3d._11.m_Viewport);
		m_d3d._11.m_PreservedFlags |= D3D11Objects::ViewportPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11RenderTargets(ID3D11DeviceContext* D3D11_ONLY(pDC))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_RenderTargets) && !(m_d3d._11.m_PreservedFlags & D3D11Objects::RenderTargetsPreserved))
	{
		pDC->OMGetRenderTargets(1, &m_d3d._11.m_pRenderTarget, &m_d3d._11.m_pDepthStencil);
		m_d3d._11.m_PreservedFlags |= D3D11Objects::RenderTargetsPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11Shaders(ID3D11DeviceContext* D3D11_ONLY(pDC))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Shaders) && !(m_d3d._11.m_PreservedFlags & D3D11Objects::ShadersPreserved))
	{
		D3D11Objects& d3d11 = m_d3d._11;
		pDC->VSGetShader(&d3d11.m_VertexShaderState.pShader, d3d11.m_VertexShaderState.pClassInstances, &d3d11.m_VertexShaderState.NumClassInstances);
		pDC->HSGetShader(&d3d11.m_HullShaderState.pShader, d3d11.m_HullShaderState.pClassInstances, &d3d11.m_HullShaderState.NumClassInstances);
		pDC->DSGetShader(&d3d11.m_DomainShaderState.pShader, d3d11.m_DomainShaderState.pClassInstances, &d3d11.m_DomainShaderState.NumClassInstances);
		pDC->GSGetShader(&d3d11.m_GeomShaderState.pShader, d3d11.m_GeomShaderState.pClassInstances, &d3d11.m_GeomShaderState.NumClassInstances);
		pDC->PSGetShader(&d3d11.m_PixelShaderState.pShader, d3d11.m_PixelShaderState.pClassInstances, &d3d11.m_PixelShaderState.NumClassInstances);

		m_d3d._11.m_PreservedFlags |= D3D11Objects::ShadersPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11ComputeShader(ID3D11DeviceContext* D3D11_ONLY(pDC))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Shaders) && !(m_d3d._11.m_PreservedFlags & D3D11Objects::ComputeShaderPreserved))
	{
		D3D11Objects& d3d11 = m_d3d._11;
		pDC->CSGetShader(&d3d11.m_ComputeShaderState.pShader, d3d11.m_ComputeShaderState.pClassInstances, &d3d11.m_ComputeShaderState.NumClassInstances);

		m_d3d._11.m_PreservedFlags |= D3D11Objects::ComputeShaderPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11Streams(ID3D11DeviceContext* D3D11_ONLY(pDC))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Streams) && !(m_d3d._11.m_PreservedFlags & D3D11Objects::StreamsPreserved))
	{
		pDC->IAGetVertexBuffers(0, 1, &m_d3d._11.m_pSlot0VB, &m_d3d._11.m_Slot0VBOffset, &m_d3d._11.m_Slot0VBStride);
		pDC->IAGetIndexBuffer(&m_d3d._11.m_pIB, &m_d3d._11.m_IBFormat, &m_d3d._11.m_IBOffset);
		pDC->IAGetInputLayout(&m_d3d._11.m_pLayout);
		pDC->IAGetPrimitiveTopology(&m_d3d._11.m_Topology);

		m_d3d._11.m_PreservedFlags |= D3D11Objects::StreamsPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11DepthStencil(ID3D11DeviceContext* D3D11_ONLY(pDC))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Other) && !(m_d3d._11.m_PreservedFlags & D3D11Objects::DepthStencilPreserved))
	{
		pDC->OMGetDepthStencilState(&m_d3d._11.m_pDepthStencilState, &m_d3d._11.m_StencilRef);
		m_d3d._11.m_PreservedFlags |= D3D11Objects::DepthStencilPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11Blend(ID3D11DeviceContext* D3D11_ONLY(pDC))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Other) && !(m_d3d._11.m_PreservedFlags & D3D11Objects::BlendPreserved))
	{
		pDC->OMGetBlendState(&m_d3d._11.m_pBlendState, m_d3d._11.m_BlendFactors, &m_d3d._11.m_SampleMask);
		m_d3d._11.m_PreservedFlags |= D3D11Objects::BlendPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11Raster(ID3D11DeviceContext* D3D11_ONLY(pDC))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if((m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Other) && !(m_d3d._11.m_PreservedFlags & D3D11Objects::RasterPreserved))
	{
		pDC->RSGetState(&m_d3d._11.m_pRSState);
		m_d3d._11.m_PreservedFlags |= D3D11Objects::RasterPreserved;
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11PixelShaderConstantBuffer(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_ShaderConstants)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._11.m_PixelShaderConstantBuffer_Flags & ixBit))
		{
			m_d3d._11.m_pEndPixelShaderConstantBuffer->regIndex = ix;
			pDC->PSGetConstantBuffers(ix, 1, &m_d3d._11.m_pEndPixelShaderConstantBuffer->pBuffer);
			++m_d3d._11.m_pEndPixelShaderConstantBuffer;

			m_d3d._11.m_PixelShaderConstantBuffer_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11VertexShaderConstantBuffer(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_ShaderConstants)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._11.m_VertexShaderConstantBuffer_Flags & ixBit))
		{
			m_d3d._11.m_pEndVertexShaderConstantBuffer->regIndex = ix;
			pDC->VSGetConstantBuffers(ix, 1, &m_d3d._11.m_pEndVertexShaderConstantBuffer->pBuffer);
			++m_d3d._11.m_pEndVertexShaderConstantBuffer;

			m_d3d._11.m_VertexShaderConstantBuffer_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11HullShaderConstantBuffer(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_ShaderConstants)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._11.m_HullShaderConstantBuffer_Flags & ixBit))
		{
			m_d3d._11.m_pEndHullShaderConstantBuffer->regIndex = ix;
			pDC->HSGetConstantBuffers(ix, 1, &m_d3d._11.m_pEndHullShaderConstantBuffer->pBuffer);
			++m_d3d._11.m_pEndHullShaderConstantBuffer;

			m_d3d._11.m_HullShaderConstantBuffer_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11DomainShaderConstantBuffer(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_ShaderConstants)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._11.m_DomainShaderConstantBuffer_Flags & ixBit))
		{
			m_d3d._11.m_pEndDomainShaderConstantBuffer->regIndex = ix;
			pDC->DSGetConstantBuffers(ix, 1, &m_d3d._11.m_pEndDomainShaderConstantBuffer->pBuffer);
			++m_d3d._11.m_pEndDomainShaderConstantBuffer;

			m_d3d._11.m_DomainShaderConstantBuffer_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11ComputeShaderConstantBuffer(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_ShaderConstants)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._11.m_ComputeShaderConstantBuffer_Flags & ixBit))
		{
			m_d3d._11.m_pEndComputeShaderConstantBuffer->regIndex = ix;
			pDC->CSGetConstantBuffers(ix, 1, &m_d3d._11.m_pEndComputeShaderConstantBuffer->pBuffer);
			++m_d3d._11.m_pEndComputeShaderConstantBuffer;

			m_d3d._11.m_ComputeShaderConstantBuffer_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11PixelShaderSampler(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._11.m_PixelShaderSampler_Flags & ixBit))
		{
			m_d3d._11.m_pEndPixelShaderSampler->regIndex = ix;
			pDC->PSGetSamplers(ix, 1, &m_d3d._11.m_pEndPixelShaderSampler->pSampler);
			++m_d3d._11.m_pEndPixelShaderSampler;

			m_d3d._11.m_PixelShaderSampler_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11VertexShaderSampler(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._11.m_VertexShaderSampler_Flags & ixBit))
		{
			m_d3d._11.m_pEndVertexShaderSampler->regIndex = ix;
			pDC->VSGetSamplers(ix, 1, &m_d3d._11.m_pEndVertexShaderSampler->pSampler);
			++m_d3d._11.m_pEndVertexShaderSampler;

			m_d3d._11.m_VertexShaderSampler_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11HullShaderSampler(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._11.m_HullShaderSampler_Flags & ixBit))
		{
			m_d3d._11.m_pEndHullShaderSampler->regIndex = ix;
			pDC->HSGetSamplers(ix, 1, &m_d3d._11.m_pEndHullShaderSampler->pSampler);
			++m_d3d._11.m_pEndHullShaderSampler;

			m_d3d._11.m_HullShaderSampler_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11DomainShaderSampler(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._11.m_DomainShaderSampler_Flags & ixBit))
		{
			m_d3d._11.m_pEndDomainShaderSampler->regIndex = ix;
			pDC->DSGetSamplers(ix, 1, &m_d3d._11.m_pEndDomainShaderSampler->pSampler);
			++m_d3d._11.m_pEndDomainShaderSampler;

			m_d3d._11.m_DomainShaderSampler_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11ComputeShaderSampler(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._11.m_ComputeShaderSampler_Flags & ixBit))
		{
			m_d3d._11.m_pEndComputeShaderSampler->regIndex = ix;
			pDC->CSGetSamplers(ix, 1, &m_d3d._11.m_pEndComputeShaderSampler->pSampler);
			++m_d3d._11.m_pEndComputeShaderSampler;

			m_d3d._11.m_ComputeShaderSampler_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11PixelShaderResource(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		if(!m_d3d._11.m_PixelShaderResource_Flags[ix])
		{
			m_d3d._11.m_pEndPixelShaderResource->regIndex = ix;
			pDC->PSGetShaderResources(ix, 1, &m_d3d._11.m_pEndPixelShaderResource->pResource);
			++m_d3d._11.m_pEndPixelShaderResource;

			m_d3d._11.m_PixelShaderResource_Flags[ix] = 1;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11VertexShaderResource(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		if(!m_d3d._11.m_VertexShaderResource_Flags[ix])
		{
			m_d3d._11.m_pEndVertexShaderResource->regIndex = ix;
			pDC->VSGetShaderResources(ix, 1, &m_d3d._11.m_pEndVertexShaderResource->pResource);
			++m_d3d._11.m_pEndVertexShaderResource;

			m_d3d._11.m_VertexShaderResource_Flags[ix] = 1;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11HullShaderResource(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		if(!m_d3d._11.m_HullShaderResource_Flags[ix])
		{
			m_d3d._11.m_pEndHullShaderResource->regIndex = ix;
			pDC->HSGetShaderResources(ix, 1, &m_d3d._11.m_pEndHullShaderResource->pResource);
			++m_d3d._11.m_pEndHullShaderResource;

			m_d3d._11.m_HullShaderResource_Flags[ix] = 1;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11DomainShaderResource(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		if(!m_d3d._11.m_DomainShaderResource_Flags[ix])
		{
			m_d3d._11.m_pEndDomainShaderResource->regIndex = ix;
			pDC->DSGetShaderResources(ix, 1, &m_d3d._11.m_pEndDomainShaderResource->pResource);
			++m_d3d._11.m_pEndDomainShaderResource;

			m_d3d._11.m_DomainShaderResource_Flags[ix] = 1;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11ComputeShaderResource(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_Samplers)
	{
		if(!m_d3d._11.m_ComputeShaderResource_Flags[ix])
		{
			m_d3d._11.m_pEndComputeShaderResource->regIndex = ix;
			pDC->CSGetShaderResources(ix, 1, &m_d3d._11.m_pEndComputeShaderResource->pResource);
			++m_d3d._11.m_pEndComputeShaderResource;

			m_d3d._11.m_ComputeShaderResource_Flags[ix] = 1;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::PreserveD3D11ComputeShaderUnorderedAccessView(ID3D11DeviceContext* D3D11_ONLY(pDC), UINT D3D11_ONLY(ix))
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_UserPreserveFlags & GFSDK_WaveWorks_StatePreserve_UnorderedAccessViews)
	{
		const WORD ixBit = WORD(0x0001 << ix);
		if(!(m_d3d._11.m_ComputeShaderUAV_Flags & ixBit))
		{
			m_d3d._11.m_pEndComputeShaderUAV->regIndex = ix;
			pDC->CSGetUnorderedAccessViews(ix, 1, &m_d3d._11.m_pEndComputeShaderUAV->pUAV);
			++m_d3d._11.m_pEndComputeShaderUAV;

			m_d3d._11.m_ComputeShaderUAV_Flags |= ixBit;
		}
	}

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::RestoreD3D11(ID3D11DeviceContext* D3D11_ONLY(pDC))
{
#if WAVEWORKS_ENABLE_D3D11
	HRESULT hr;

	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	if(m_d3d._11.m_PreservedFlags & D3D11Objects::RenderTargetsPreserved)
	{
		pDC->OMSetRenderTargets(1, &m_d3d._11.m_pRenderTarget, m_d3d._11.m_pDepthStencil);
	}

	if(m_d3d._11.m_PreservedFlags & D3D11Objects::ViewportPreserved)
	{
		pDC->RSSetViewports(1, &m_d3d._11.m_Viewport);
	}

	if(m_d3d._11.m_PreservedFlags & D3D11Objects::ShadersPreserved)
	{
		pDC->VSSetShader(m_d3d._11.m_VertexShaderState.pShader, m_d3d._11.m_VertexShaderState.pClassInstances, m_d3d._11.m_VertexShaderState.NumClassInstances);
		pDC->HSSetShader(m_d3d._11.m_HullShaderState.pShader, m_d3d._11.m_HullShaderState.pClassInstances, m_d3d._11.m_HullShaderState.NumClassInstances);
		pDC->DSSetShader(m_d3d._11.m_DomainShaderState.pShader, m_d3d._11.m_DomainShaderState.pClassInstances, m_d3d._11.m_DomainShaderState.NumClassInstances);
		pDC->GSSetShader(m_d3d._11.m_GeomShaderState.pShader, m_d3d._11.m_GeomShaderState.pClassInstances, m_d3d._11.m_GeomShaderState.NumClassInstances);
		pDC->PSSetShader(m_d3d._11.m_PixelShaderState.pShader, m_d3d._11.m_PixelShaderState.pClassInstances, m_d3d._11.m_PixelShaderState.NumClassInstances);
	}

	if(m_d3d._11.m_PreservedFlags & D3D11Objects::ComputeShaderPreserved)
	{
		pDC->CSSetShader(m_d3d._11.m_ComputeShaderState.pShader, m_d3d._11.m_ComputeShaderState.pClassInstances, m_d3d._11.m_ComputeShaderState.NumClassInstances);
	}

	if(m_d3d._11.m_PreservedFlags & D3D11Objects::StreamsPreserved)
	{
		pDC->IASetVertexBuffers(0, 1, &m_d3d._11.m_pSlot0VB, &m_d3d._11.m_Slot0VBOffset, &m_d3d._11.m_Slot0VBStride);
		pDC->IASetIndexBuffer(m_d3d._11.m_pIB, m_d3d._11.m_IBFormat, m_d3d._11.m_IBOffset);
		pDC->IASetInputLayout(m_d3d._11.m_pLayout);
		pDC->IASetPrimitiveTopology(m_d3d._11.m_Topology);
	}

	if(m_d3d._11.m_PreservedFlags & D3D11Objects::DepthStencilPreserved)
	{
		pDC->OMSetDepthStencilState(m_d3d._11.m_pDepthStencilState, m_d3d._11.m_StencilRef);
	}

	if(m_d3d._11.m_PreservedFlags & D3D11Objects::BlendPreserved)
	{
		pDC->OMSetBlendState(m_d3d._11.m_pBlendState, m_d3d._11.m_BlendFactors, m_d3d._11.m_SampleMask);
	}

	if(m_d3d._11.m_PreservedFlags & D3D11Objects::RasterPreserved)
	{
		pDC->RSSetState(m_d3d._11.m_pRSState);
	}
	// Restoring Constant Buffers
	for(D3D11Objects::ShaderConstantBuffer* it = m_d3d._11.m_VertexShaderConstantBuffer; it != m_d3d._11.m_pEndVertexShaderConstantBuffer; ++it)
	{
		pDC->VSSetConstantBuffers(it->regIndex, 1, &it->pBuffer);
	}
	m_d3d._11.m_VertexShaderConstantBuffer_Flags = 0;

	for(D3D11Objects::ShaderConstantBuffer* it = m_d3d._11.m_PixelShaderConstantBuffer; it != m_d3d._11.m_pEndPixelShaderConstantBuffer; ++it)
	{
		pDC->PSSetConstantBuffers(it->regIndex, 1, &it->pBuffer);
	}
	m_d3d._11.m_PixelShaderConstantBuffer_Flags = 0;

	for(D3D11Objects::ShaderConstantBuffer* it = m_d3d._11.m_HullShaderConstantBuffer; it != m_d3d._11.m_pEndHullShaderConstantBuffer; ++it)
	{
		pDC->HSSetConstantBuffers(it->regIndex, 1, &it->pBuffer);
	}
	m_d3d._11.m_HullShaderConstantBuffer_Flags = 0;

	for(D3D11Objects::ShaderConstantBuffer* it = m_d3d._11.m_DomainShaderConstantBuffer; it != m_d3d._11.m_pEndDomainShaderConstantBuffer; ++it)
	{
		pDC->DSSetConstantBuffers(it->regIndex, 1, &it->pBuffer);
	}
	m_d3d._11.m_DomainShaderConstantBuffer_Flags = 0;

	for(D3D11Objects::ShaderConstantBuffer* it = m_d3d._11.m_ComputeShaderConstantBuffer; it != m_d3d._11.m_pEndComputeShaderConstantBuffer; ++it)
	{
		pDC->CSSetConstantBuffers(it->regIndex, 1, &it->pBuffer);
	}
	m_d3d._11.m_ComputeShaderConstantBuffer_Flags = 0;

	// Restoring Samplers
	for(D3D11Objects::ShaderSampler* it = m_d3d._11.m_VertexShaderSampler; it != m_d3d._11.m_pEndVertexShaderSampler; ++it)
	{
		pDC->VSSetSamplers(it->regIndex, 1, &it->pSampler);
	}
	m_d3d._11.m_VertexShaderSampler_Flags = 0;

	for(D3D11Objects::ShaderSampler* it = m_d3d._11.m_PixelShaderSampler; it != m_d3d._11.m_pEndPixelShaderSampler; ++it)
	{
		pDC->PSSetSamplers(it->regIndex, 1, &it->pSampler);
	}
	m_d3d._11.m_PixelShaderSampler_Flags = 0;

	for(D3D11Objects::ShaderSampler* it = m_d3d._11.m_HullShaderSampler; it != m_d3d._11.m_pEndHullShaderSampler; ++it)
	{
		pDC->HSSetSamplers(it->regIndex, 1, &it->pSampler);
	}
	m_d3d._11.m_HullShaderSampler_Flags = 0;

	for(D3D11Objects::ShaderSampler* it = m_d3d._11.m_DomainShaderSampler; it != m_d3d._11.m_pEndDomainShaderSampler; ++it)
	{
		pDC->DSSetSamplers(it->regIndex, 1, &it->pSampler);
	}
	m_d3d._11.m_DomainShaderSampler_Flags = 0;

	for(D3D11Objects::ShaderSampler* it = m_d3d._11.m_ComputeShaderSampler; it != m_d3d._11.m_pEndComputeShaderSampler; ++it)
	{
		pDC->CSSetSamplers(it->regIndex, 1, &it->pSampler);
	}
	m_d3d._11.m_ComputeShaderSampler_Flags = 0;

	// Restoring Shader Resources
	for(D3D11Objects::ShaderResource* it = m_d3d._11.m_VertexShaderResource; it != m_d3d._11.m_pEndVertexShaderResource; ++it)
	{
		pDC->VSSetShaderResources(it->regIndex, 1, &it->pResource);
		m_d3d._11.m_VertexShaderResource_Flags[it->regIndex] = 0;
	}

	for(D3D11Objects::ShaderResource* it = m_d3d._11.m_PixelShaderResource; it != m_d3d._11.m_pEndPixelShaderResource; ++it)
	{
		pDC->PSSetShaderResources(it->regIndex, 1, &it->pResource);
		m_d3d._11.m_PixelShaderResource_Flags[it->regIndex] = 0;
	}

	for(D3D11Objects::ShaderResource* it = m_d3d._11.m_HullShaderResource; it != m_d3d._11.m_pEndHullShaderResource; ++it)
	{
		pDC->HSSetShaderResources(it->regIndex, 1, &it->pResource);
		m_d3d._11.m_HullShaderResource_Flags[it->regIndex] = 0;
	}

	for(D3D11Objects::ShaderResource* it = m_d3d._11.m_DomainShaderResource; it != m_d3d._11.m_pEndDomainShaderResource; ++it)
	{
		pDC->DSSetShaderResources(it->regIndex, 1, &it->pResource);
		m_d3d._11.m_DomainShaderResource_Flags[it->regIndex] = 0;
	}

	for(D3D11Objects::ShaderResource* it = m_d3d._11.m_ComputeShaderResource; it != m_d3d._11.m_pEndComputeShaderResource; ++it)
	{
		pDC->CSSetShaderResources(it->regIndex, 1, &it->pResource);
		m_d3d._11.m_ComputeShaderResource_Flags[it->regIndex] = 0;
	}

	// Restore UAVs
	for(D3D11Objects::ShaderUAV* it = m_d3d._11.m_ComputeShaderUAV; it != m_d3d._11.m_pEndComputeShaderUAV; ++it)
	{
		const UINT KeepCurrentCount = UINT(-1);
		pDC->CSSetUnorderedAccessViews(it->regIndex, 1, &it->pUAV, &KeepCurrentCount);
	}
	m_d3d._11.m_ComputeShaderUAV_Flags = 0;

	// Release ref-counts etc.
	V_RETURN(ReleaseD3D11Resources());

	// Reset remaining flags etc.
	m_d3d._11.m_PreservedFlags = 0;

	m_d3d._11.m_pEndVertexShaderConstantBuffer = m_d3d._11.m_VertexShaderConstantBuffer;
	m_d3d._11.m_pEndPixelShaderConstantBuffer = m_d3d._11.m_PixelShaderConstantBuffer;
	m_d3d._11.m_pEndHullShaderConstantBuffer = m_d3d._11.m_HullShaderConstantBuffer;
	m_d3d._11.m_pEndDomainShaderConstantBuffer = m_d3d._11.m_DomainShaderConstantBuffer;
	m_d3d._11.m_pEndComputeShaderConstantBuffer = m_d3d._11.m_ComputeShaderConstantBuffer;
	
	m_d3d._11.m_pEndVertexShaderSampler = m_d3d._11.m_VertexShaderSampler;
	m_d3d._11.m_pEndPixelShaderSampler = m_d3d._11.m_PixelShaderSampler;
	m_d3d._11.m_pEndHullShaderSampler = m_d3d._11.m_HullShaderSampler;
	m_d3d._11.m_pEndDomainShaderSampler = m_d3d._11.m_DomainShaderSampler;
	m_d3d._11.m_pEndComputeShaderSampler = m_d3d._11.m_ComputeShaderSampler;
	
	m_d3d._11.m_pEndVertexShaderResource = m_d3d._11.m_VertexShaderResource;
	m_d3d._11.m_pEndPixelShaderResource = m_d3d._11.m_PixelShaderResource;
	m_d3d._11.m_pEndHullShaderResource = m_d3d._11.m_HullShaderResource;
	m_d3d._11.m_pEndDomainShaderResource = m_d3d._11.m_DomainShaderResource;
	m_d3d._11.m_pEndComputeShaderResource = m_d3d._11.m_ComputeShaderResource;

	m_d3d._11.m_pEndComputeShaderUAV = m_d3d._11.m_ComputeShaderUAV;

	return S_OK;
#else
return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Savestate::ReleaseD3D11Resources()
{
#if WAVEWORKS_ENABLE_D3D11
	assert(m_d3dAPI == nv_water_d3d_api_d3d11);

	SAFE_RELEASE(m_d3d._11.m_pRenderTarget);
	SAFE_RELEASE(m_d3d._11.m_pDepthStencil);

	m_d3d._11.m_VertexShaderState.ReleaseD3D11Resources();
	m_d3d._11.m_HullShaderState.ReleaseD3D11Resources();
	m_d3d._11.m_DomainShaderState.ReleaseD3D11Resources();
	m_d3d._11.m_GeomShaderState.ReleaseD3D11Resources();
	m_d3d._11.m_PixelShaderState.ReleaseD3D11Resources();

	SAFE_RELEASE(m_d3d._11.m_pSlot0VB);
	SAFE_RELEASE(m_d3d._11.m_pIB);
	SAFE_RELEASE(m_d3d._11.m_pLayout);

	SAFE_RELEASE(m_d3d._11.m_pDepthStencilState);

	SAFE_RELEASE(m_d3d._11.m_pBlendState);

	SAFE_RELEASE(m_d3d._11.m_pRSState);

	// Releasing Constant Buffers
	for(D3D11Objects::ShaderConstantBuffer* it = m_d3d._11.m_VertexShaderConstantBuffer; it != m_d3d._11.m_pEndVertexShaderConstantBuffer; ++it)
	{
		SAFE_RELEASE(it->pBuffer);
	}

	for(D3D11Objects::ShaderConstantBuffer* it = m_d3d._11.m_PixelShaderConstantBuffer; it != m_d3d._11.m_pEndPixelShaderConstantBuffer; ++it)
	{
		SAFE_RELEASE(it->pBuffer);
	}

	for(D3D11Objects::ShaderConstantBuffer* it = m_d3d._11.m_HullShaderConstantBuffer; it != m_d3d._11.m_pEndHullShaderConstantBuffer; ++it)
	{
		SAFE_RELEASE(it->pBuffer);
	}

	for(D3D11Objects::ShaderConstantBuffer* it = m_d3d._11.m_DomainShaderConstantBuffer; it != m_d3d._11.m_pEndDomainShaderConstantBuffer; ++it)
	{
		SAFE_RELEASE(it->pBuffer);
	}

	for(D3D11Objects::ShaderConstantBuffer* it = m_d3d._11.m_ComputeShaderConstantBuffer; it != m_d3d._11.m_pEndComputeShaderConstantBuffer; ++it)
	{
		SAFE_RELEASE(it->pBuffer);
	}
	
	// Releasing Samplers
	for(D3D11Objects::ShaderSampler* it = m_d3d._11.m_VertexShaderSampler; it != m_d3d._11.m_pEndVertexShaderSampler; ++it)
	{
		SAFE_RELEASE(it->pSampler);
	}

	for(D3D11Objects::ShaderSampler* it = m_d3d._11.m_PixelShaderSampler; it != m_d3d._11.m_pEndPixelShaderSampler; ++it)
	{
		SAFE_RELEASE(it->pSampler);
	}

	for(D3D11Objects::ShaderSampler* it = m_d3d._11.m_HullShaderSampler; it != m_d3d._11.m_pEndHullShaderSampler; ++it)
	{
		SAFE_RELEASE(it->pSampler);
	}

	for(D3D11Objects::ShaderSampler* it = m_d3d._11.m_DomainShaderSampler; it != m_d3d._11.m_pEndDomainShaderSampler; ++it)
	{
		SAFE_RELEASE(it->pSampler);
	}

	for(D3D11Objects::ShaderSampler* it = m_d3d._11.m_ComputeShaderSampler; it != m_d3d._11.m_pEndComputeShaderSampler; ++it)
	{
		SAFE_RELEASE(it->pSampler);
	}

	// Releasing Shader Resources
	for(D3D11Objects::ShaderResource* it = m_d3d._11.m_VertexShaderResource; it != m_d3d._11.m_pEndVertexShaderResource; ++it)
	{
		SAFE_RELEASE(it->pResource);
	}

	for(D3D11Objects::ShaderResource* it = m_d3d._11.m_PixelShaderResource; it != m_d3d._11.m_pEndPixelShaderResource; ++it)
	{
		SAFE_RELEASE(it->pResource);
	}

	for(D3D11Objects::ShaderResource* it = m_d3d._11.m_HullShaderResource; it != m_d3d._11.m_pEndHullShaderResource; ++it)
	{
		SAFE_RELEASE(it->pResource);
	}

	for(D3D11Objects::ShaderResource* it = m_d3d._11.m_DomainShaderResource; it != m_d3d._11.m_pEndDomainShaderResource; ++it)
	{
		SAFE_RELEASE(it->pResource);
	}

	for(D3D11Objects::ShaderResource* it = m_d3d._11.m_ComputeShaderResource; it != m_d3d._11.m_pEndComputeShaderResource; ++it)
	{
		SAFE_RELEASE(it->pResource);
	}

	for(D3D11Objects::ShaderUAV* it = m_d3d._11.m_ComputeShaderUAV; it != m_d3d._11.m_pEndComputeShaderUAV; ++it)
	{
		SAFE_RELEASE(it->pUAV);
	}

	return S_OK;
#else
return E_FAIL;
#endif
}
