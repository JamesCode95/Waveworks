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
 
#include <DirectXMath.h>
#include "DXUT.h"
#include "SDKMisc.h"
#include "distance_field.h"

#include "GFSDK_WaveWorks_D3D_Util.h"

#pragma warning(disable:4127)

const unsigned int kTopDownDataResolution = 256;

DistanceField::DistanceField( CTerrain* const pTerrainRenderer )
	: m_pTerrainRenderer( pTerrainRenderer )
	, m_viewDirectionWS( 0, -1, 0, 0 )
	, m_pTopDownDataSRV( NULL )
	, m_pTopDownDataRTV( NULL )
	, m_pTopDownDataTexture( NULL )
	, m_pStagingTexture( NULL )
	, m_shouldGenerateDataTexture( true )
{
}

DistanceField::~DistanceField()
{
	SAFE_RELEASE( m_pTopDownDataSRV );
	SAFE_RELEASE( m_pTopDownDataRTV );
	SAFE_RELEASE( m_pTopDownDataTexture );
	SAFE_RELEASE( m_pStagingTexture );
} 

HRESULT DistanceField::Init( ID3D11Device* const pDevice )
{
	HRESULT hr = S_OK;

	if( NULL == m_pTopDownDataTexture )
	{
		D3D11_TEXTURE2D_DESC textureDesc;
		ZeroMemory(&textureDesc, sizeof(textureDesc));

		textureDesc.ArraySize = 1;
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.Height = kTopDownDataResolution;
		textureDesc.Width = kTopDownDataResolution;
		textureDesc.MipLevels = 1;
		textureDesc.MiscFlags = 0;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;

		V_RETURN( pDevice->CreateTexture2D( &textureDesc, nullptr, &m_pTopDownDataTexture ) );


		textureDesc.ArraySize = 1;
		textureDesc.BindFlags = 0;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.Height = kTopDownDataResolution;
		textureDesc.Width = kTopDownDataResolution;
		textureDesc.MipLevels = 1;
		textureDesc.MiscFlags = 0;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_STAGING;

		V_RETURN( pDevice->CreateTexture2D( &textureDesc, nullptr, &m_pStagingTexture ) );


		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory( &srvDesc, sizeof( srvDesc ) );
		srvDesc.Format = textureDesc.Format;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

		V_RETURN( pDevice->CreateShaderResourceView( m_pTopDownDataTexture, &srvDesc, &m_pTopDownDataSRV ) );

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
		ZeroMemory( &rtvDesc, sizeof( rtvDesc ) );
		rtvDesc.Format = textureDesc.Format;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		V_RETURN( pDevice->CreateRenderTargetView( m_pTopDownDataTexture, &rtvDesc, &m_pTopDownDataRTV ) );
	}
	return S_OK;
}

void DistanceField::GenerateDataTexture( ID3D11DeviceContext* pDC )
{
	if( !m_shouldGenerateDataTexture ) return;

	renderTopDownData( pDC, XMVectorSet( 250, 0, 250, 0 ) );
	generateDistanceField( pDC );

	m_shouldGenerateDataTexture = false;
}

void DistanceField::renderTopDownData( ID3D11DeviceContext* pDC, const XMVECTOR eyePositionWS )
{
	const float kHeightAboveSeaLevel = 300;
	const float kMinHeightBelowSeaLevel = 20;

	D3D11_VIEWPORT vp;
	UINT NumViewports = 1;
	pDC->RSGetViewports(&NumViewports,&vp);

	ID3D11RenderTargetView* pRenderTarget;
	ID3D11DepthStencilView* pDepthBuffer;
	pDC->OMGetRenderTargets( 1, &pRenderTarget, &pDepthBuffer );

	// Set the viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Height = kTopDownDataResolution;
	viewport.Width = kTopDownDataResolution;

	float ClearColor[4] = { 0.0f, -kMinHeightBelowSeaLevel, 0.0f, 0.0f };
	pDC->RSSetViewports(1, &viewport);
	pDC->ClearRenderTargetView( m_pTopDownDataRTV, ClearColor );
	pDC->OMSetRenderTargetsAndUnorderedAccessViews( 1, &m_pTopDownDataRTV, NULL, 0, 0, NULL, NULL );

	m_topDownViewPositionWS = XMFLOAT4( XMVectorGetX(eyePositionWS), kHeightAboveSeaLevel, XMVectorGetZ(eyePositionWS), 0 );

	const float kOrthoSize = 700;
	XMStoreFloat4x4(&m_viewToProjectionMatrix, XMMatrixOrthographicLH(kOrthoSize, kOrthoSize, 0.3f, kHeightAboveSeaLevel + kMinHeightBelowSeaLevel));

	const XMVECTOR up = XMVectorSet( 0, 0, 1, 0 );
	XMStoreFloat4x4(&m_worldToViewMatrix, XMMatrixLookAtLH(XMLoadFloat4(&m_topDownViewPositionWS), eyePositionWS, up));

	m_pTerrainRenderer->RenderTerrainToHeightField( pDC, XMLoadFloat4x4(&m_worldToViewMatrix), XMLoadFloat4x4(&m_viewToProjectionMatrix), XMLoadFloat4(&m_topDownViewPositionWS), XMLoadFloat4(&m_viewDirectionWS) );

	pDC->RSSetViewports(NumViewports, &vp);
	pDC->OMSetRenderTargetsAndUnorderedAccessViews( 1, &pRenderTarget, pDepthBuffer, 0, 0, NULL, NULL );
	SAFE_RELEASE( pRenderTarget );
	SAFE_RELEASE( pDepthBuffer );
}

void DistanceField::generateDistanceField( ID3D11DeviceContext* pDC )
{
	float* pTextureReadData = (float*)malloc(kTopDownDataResolution * kTopDownDataResolution * 4*sizeof(float));

	pDC->CopyResource( m_pStagingTexture, m_pTopDownDataTexture );

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	pDC->Map( m_pStagingTexture, 0, D3D11_MAP_READ_WRITE, 0, &mappedResource );
	{
		memcpy( pTextureReadData, mappedResource.pData, kTopDownDataResolution * kTopDownDataResolution * 4*sizeof(float));

		float* pTextureWriteData = reinterpret_cast<float*>( mappedResource.pData );

		// Calculating the distance field to be stored in R channel
		// Seabed level is stored in G channel, leaving it intact 
		for( unsigned int x=0 ; x<kTopDownDataResolution ; x++ )
		{
			for( unsigned int y=0 ; y<kTopDownDataResolution ; y++ )
			{
				float gradientX, gradientY;
				float distanceToNearestPixel = FindNearestPixel( pTextureReadData, x, y , gradientX, gradientY);
				pTextureWriteData[ (x * kTopDownDataResolution + y) * 4 + 0 ] = distanceToNearestPixel;
				pTextureWriteData[ (x * kTopDownDataResolution + y) * 4 + 2] = gradientY;
				pTextureWriteData[ (x * kTopDownDataResolution + y) * 4 + 3] = gradientX;

			}
		}

		
		// now blurring the distance field a bit to smoothen the harsh edges, using channel B as temporaty storage,
		for( unsigned int x = 1 ; x < kTopDownDataResolution - 1 ; x++ )
		{
			for( unsigned int y = 1 ; y < kTopDownDataResolution - 1; y++ )
			{
				pTextureWriteData[ (x * kTopDownDataResolution + y) * 4 + 2] = (pTextureWriteData[ ((x + 1)* kTopDownDataResolution + y + 0) * 4 + 0] + 
																			    pTextureWriteData[ ((x - 1)* kTopDownDataResolution + y + 0) * 4 + 0] + 
																			    pTextureWriteData[ ((x + 0)* kTopDownDataResolution + y - 1) * 4 + 0] + 
																			    pTextureWriteData[ ((x + 0)* kTopDownDataResolution + y + 1) * 4 + 0] + 
																			    pTextureWriteData[ ((x + 0)* kTopDownDataResolution + y + 1) * 4 + 0])*0.2f;
			}
		}
		for( unsigned int x = 1 ; x < kTopDownDataResolution - 1; x++ )
		{
			for( unsigned int y = 1 ; y < kTopDownDataResolution - 1; y++ )
			{
				pTextureWriteData[ (x * kTopDownDataResolution + y) * 4 + 0] = (pTextureWriteData[ ((x + 1)* kTopDownDataResolution + y + 0) * 4 + 2] + 
																			    pTextureWriteData[ ((x - 1)* kTopDownDataResolution + y + 0) * 4 + 2] + 
																			    pTextureWriteData[ ((x + 0)* kTopDownDataResolution + y - 1) * 4 + 2] + 
																			    pTextureWriteData[ ((x + 0)* kTopDownDataResolution + y + 1) * 4 + 2] + 
																			    pTextureWriteData[ ((x + 0)* kTopDownDataResolution + y + 0) * 4 + 2])*0.2f;
			}
		}
		
		// calculating SDF gradients to be stored in B, A channels of the SDF texture
		
		for( unsigned int x = 1 ; x < kTopDownDataResolution - 1; x++ )
		{
			for( unsigned int y = 1 ; y < kTopDownDataResolution - 1; y++ )
			{
				float value_center = pTextureWriteData[ ((x + 0)* kTopDownDataResolution + y + 0) * 4 + 0];
				float value_left = pTextureWriteData[ ((x - 1)* kTopDownDataResolution + y + 0) * 4 + 0];
				float value_right = pTextureWriteData[ ((x + 1)* kTopDownDataResolution + y + 0) * 4 + 0];
				float value_bottom = pTextureWriteData[ ((x + 0)* kTopDownDataResolution + y - 1) * 4 + 0];
				float value_top = pTextureWriteData[ ((x + 0)* kTopDownDataResolution + y + 1) * 4 + 0];
				float gdx = value_right - value_left;
				float gdy = value_top - value_bottom;
				float length = sqrtf(gdx*gdx + gdy*gdy + 0.001f);
				gdx /= length;
				gdy /= length;
				pTextureWriteData[ (x * kTopDownDataResolution + y) * 4 + 2] = -gdy;
				pTextureWriteData[ (x * kTopDownDataResolution + y) * 4 + 3] = gdx;
			}
		}
		
	}
	pDC->Unmap( m_pStagingTexture, 0 );

	pDC->CopyResource( m_pTopDownDataTexture, m_pStagingTexture );
	free(pTextureReadData);
}

bool DistanceField::checkPixel( float* pTextureData, const int cx, const int cy, const int dx, const int dy) const
{
	const int x = (cx+dx) < 0 ? 0 : (cx+dx) >= kTopDownDataResolution ? (kTopDownDataResolution-1) : (cx+dx); 
	const int y = (cy+dy) < 0 ? 0 : (cy+dy) >= kTopDownDataResolution ? (kTopDownDataResolution-1) : (cy+dy); 

	const int idx = (x * kTopDownDataResolution + y) * 4 + 0; // Red channel

	return pTextureData[ idx ] > 0.0f;
}

float DistanceField::FindNearestPixel( float* pTextureData, const int cx, const int cy, float& gradientX, float& gradientY) 
{
	const int kMaxDistance = 20;
	float minDistance = kMaxDistance;
	bool originPositive = checkPixel( pTextureData, cx, cy, 0, 0);
	bool resultPositive;
	for( int dx = -kMaxDistance ; dx <= kMaxDistance ; dx++ )
	{
		for( int dy = -kMaxDistance + 1 ; dy < kMaxDistance ; dy++ )
		{
			resultPositive = checkPixel( pTextureData, cx, cy, dx, dy);
			float pixelDistance = sqrtf((float)(dx * dx + dy * dy));
			if((originPositive != resultPositive) && (pixelDistance < minDistance)) 
			{
				minDistance = pixelDistance;
				gradientX = dx / (pixelDistance+0.001f);
				gradientY = dy/ (pixelDistance+0.001f);
				if(!originPositive) 
				{
					gradientX=-gradientX;
					gradientY=-gradientY;

				}
			}
		}
	}
	return originPositive ? -minDistance/kMaxDistance : minDistance/kMaxDistance;
}

void DistanceField::GetWorldToTopDownTextureMatrix( XMMATRIX &worldToTopDownMatrix )
{
// 	XMMATRIX wtvMat, vtpMat;
// 
// 	XMLoadFloat4x4(&m_worldToViewMatrix);
// 	XMLoadFloat4x4(&m_viewToProjectionMatrix);

	worldToTopDownMatrix = XMLoadFloat4x4(&m_worldToViewMatrix) * XMLoadFloat4x4(&m_viewToProjectionMatrix);
}