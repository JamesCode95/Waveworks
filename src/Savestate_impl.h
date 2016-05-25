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

#ifndef _NVWAVEWORKS_SAVESTATE_IMPL_H
#define _NVWAVEWORKS_SAVESTATE_IMPL_H

#ifndef D3D10_SDK_VERSION
typedef int D3DSAMPLERSTATETYPE;
typedef int D3DRENDERSTATETYPE;
#endif

struct ID3D11DeviceContext;

struct GFSDK_WaveWorks_Savestate
{
public:
    GFSDK_WaveWorks_Savestate(ID3D11Device* pD3DDevice, GFSDK_WaveWorks_StatePreserveFlags PreserveFlags);
	~GFSDK_WaveWorks_Savestate();



	HRESULT PreserveD3D11Viewport(ID3D11DeviceContext* pDC);
	HRESULT PreserveD3D11RenderTargets(ID3D11DeviceContext* pDC);
	HRESULT PreserveD3D11Shaders(ID3D11DeviceContext* pDC);
	HRESULT PreserveD3D11ComputeShader(ID3D11DeviceContext* pDC);
	HRESULT PreserveD3D11Streams(ID3D11DeviceContext* pDC);
	HRESULT PreserveD3D11DepthStencil(ID3D11DeviceContext* pDC);
	HRESULT PreserveD3D11Blend(ID3D11DeviceContext* pDC);
	HRESULT PreserveD3D11Raster(ID3D11DeviceContext* pDC);

	HRESULT PreserveD3D11PixelShaderConstantBuffer(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11VertexShaderConstantBuffer(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11DomainShaderConstantBuffer(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11HullShaderConstantBuffer(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11ComputeShaderConstantBuffer(ID3D11DeviceContext* pDC, UINT ix);

	HRESULT PreserveD3D11PixelShaderSampler(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11VertexShaderSampler(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11DomainShaderSampler(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11HullShaderSampler(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11ComputeShaderSampler(ID3D11DeviceContext* pDC, UINT ix);

	HRESULT PreserveD3D11PixelShaderResource(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11VertexShaderResource(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11DomainShaderResource(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11HullShaderResource(ID3D11DeviceContext* pDC, UINT ix);
	HRESULT PreserveD3D11ComputeShaderResource(ID3D11DeviceContext* pDC, UINT ix);

	HRESULT PreserveD3D11ComputeShaderUnorderedAccessView(ID3D11DeviceContext* pDC, UINT ix);

	HRESULT Restore(Graphics_Context* pGC);

protected:

	GFSDK_WaveWorks_StatePreserveFlags m_UserPreserveFlags;

private:

	HRESULT RestoreD3D11(ID3D11DeviceContext* pDC);
	HRESULT ReleaseD3D11Resources();

	// D3D API handling
	nv_water_d3d_api m_d3dAPI;

#if WAVEWORKS_ENABLE_D3D11
	struct D3D11Objects
	{
		ID3D11Device* m_pd3d11Device;

		// What is preserved?
		enum PreservedFlags
		{
			ViewportPreserved				= 1,
			RenderTargetsPreserved			= 2,
			ShadersPreserved				= 4,
			StreamsPreserved				= 8,
			DepthStencilPreserved			= 16,
			BlendPreserved					= 32,
			RasterPreserved					= 64,
			ComputeShaderPreserved			= 128
		};

		DWORD m_PreservedFlags;

		D3D11_VIEWPORT m_Viewport;

		ID3D11RenderTargetView* m_pRenderTarget;
		ID3D11DepthStencilView* m_pDepthStencil;

		template<class ShaderType>
		struct ShaderState
		{
			ShaderType* pShader;

			enum { MaxClassInstances = 256 };
			ID3D11ClassInstance* pClassInstances[MaxClassInstances];
			UINT NumClassInstances;

			void ReleaseD3D11Resources()
			{
				SAFE_RELEASE(pShader);

				for(UINT i = 0; i != NumClassInstances; ++i)
				{
					SAFE_RELEASE(pClassInstances[i]);
				}
			}
		};

		ShaderState<ID3D11PixelShader> m_PixelShaderState;
		ShaderState<ID3D11GeometryShader> m_GeomShaderState;
		ShaderState<ID3D11DomainShader> m_DomainShaderState;
		ShaderState<ID3D11HullShader> m_HullShaderState;
		ShaderState<ID3D11VertexShader> m_VertexShaderState;
		ShaderState<ID3D11ComputeShader> m_ComputeShaderState;

		ID3D11DepthStencilState* m_pDepthStencilState;
		UINT m_StencilRef;

		ID3D11BlendState* m_pBlendState;
		UINT m_SampleMask;
		FLOAT m_BlendFactors[4];

		ID3D11RasterizerState* m_pRSState;

		// Shader constant buffers
		enum { NumShaderConstantBuffer = 16 };
		WORD m_VertexShaderConstantBuffer_Flags;
		WORD m_PixelShaderConstantBuffer_Flags;
		WORD m_HullShaderConstantBuffer_Flags;
		WORD m_DomainShaderConstantBuffer_Flags;
		WORD m_ComputeShaderConstantBuffer_Flags;

		struct ShaderConstantBuffer
		{
			UINT regIndex;
			ID3D11Buffer* pBuffer;
		};

		ShaderConstantBuffer m_VertexShaderConstantBuffer[NumShaderConstantBuffer];
		ShaderConstantBuffer* m_pEndVertexShaderConstantBuffer;
		ShaderConstantBuffer m_PixelShaderConstantBuffer[NumShaderConstantBuffer];
		ShaderConstantBuffer* m_pEndPixelShaderConstantBuffer;
		ShaderConstantBuffer m_HullShaderConstantBuffer[NumShaderConstantBuffer];
		ShaderConstantBuffer* m_pEndHullShaderConstantBuffer;
		ShaderConstantBuffer m_DomainShaderConstantBuffer[NumShaderConstantBuffer];
		ShaderConstantBuffer* m_pEndDomainShaderConstantBuffer;
		ShaderConstantBuffer m_ComputeShaderConstantBuffer[NumShaderConstantBuffer];
		ShaderConstantBuffer* m_pEndComputeShaderConstantBuffer;

		// Shader samplers
		enum { NumShaderSampler = 16 };
		WORD m_VertexShaderSampler_Flags;
		WORD m_PixelShaderSampler_Flags;
		WORD m_HullShaderSampler_Flags;
		WORD m_DomainShaderSampler_Flags;
		WORD m_ComputeShaderSampler_Flags;

		struct ShaderSampler
		{
			UINT regIndex;
			ID3D11SamplerState* pSampler;
		};

		ShaderSampler m_VertexShaderSampler[NumShaderSampler];
		ShaderSampler* m_pEndVertexShaderSampler;
		ShaderSampler m_PixelShaderSampler[NumShaderSampler];
		ShaderSampler* m_pEndPixelShaderSampler;
		ShaderSampler m_HullShaderSampler[NumShaderSampler];
		ShaderSampler* m_pEndHullShaderSampler;
		ShaderSampler m_DomainShaderSampler[NumShaderSampler];
		ShaderSampler* m_pEndDomainShaderSampler;
		ShaderSampler m_ComputeShaderSampler[NumShaderSampler];
		ShaderSampler* m_pEndComputeShaderSampler;

		// Shader resources
		enum { NumShaderResource = 128 };
		UCHAR m_VertexShaderResource_Flags[NumShaderResource];
		UCHAR m_PixelShaderResource_Flags[NumShaderResource];
		UCHAR m_HullShaderResource_Flags[NumShaderResource];
		UCHAR m_DomainShaderResource_Flags[NumShaderResource];
		UCHAR m_ComputeShaderResource_Flags[NumShaderResource];

		struct ShaderResource
		{
			UINT regIndex;
			ID3D11ShaderResourceView* pResource;
		};

		ShaderResource m_VertexShaderResource[NumShaderResource];
		ShaderResource* m_pEndVertexShaderResource;
		ShaderResource m_PixelShaderResource[NumShaderResource];
		ShaderResource* m_pEndPixelShaderResource;
		ShaderResource m_HullShaderResource[NumShaderResource];
		ShaderResource* m_pEndHullShaderResource;
		ShaderResource m_DomainShaderResource[NumShaderResource];
		ShaderResource* m_pEndDomainShaderResource;
		ShaderResource m_ComputeShaderResource[NumShaderResource];
		ShaderResource* m_pEndComputeShaderResource;

		// UAVs
		enum { NumShaderUAV = 8 };
		WORD m_ComputeShaderUAV_Flags;

		struct ShaderUAV
		{
			UINT regIndex;
			ID3D11UnorderedAccessView* pUAV;
		};

		ShaderUAV m_ComputeShaderUAV[NumShaderUAV];
		ShaderUAV* m_pEndComputeShaderUAV;

		// Stream
		ID3D11InputLayout* m_pLayout;
		ID3D11Buffer* m_pSlot0VB;
		UINT m_Slot0VBOffset;
		UINT m_Slot0VBStride;
		ID3D11Buffer* m_pIB;
		UINT m_IBOffset;
		DXGI_FORMAT m_IBFormat;
		D3D11_PRIMITIVE_TOPOLOGY m_Topology;
	};
#endif

	union
	{
#if WAVEWORKS_ENABLE_D3D11
        D3D11Objects _11;
#endif
	} m_d3d;
};

#endif	// _NVWAVEWORKS_SAVESTATE_IMPL_H
