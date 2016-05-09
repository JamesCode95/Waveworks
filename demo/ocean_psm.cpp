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
 
#include "DXUT.h"
#include "ocean_psm.h"

#include "ocean_shader_common.h"

extern HRESULT LoadFile(LPCTSTR FileName, ID3DXBuffer** ppBuffer);

namespace {

	enum { kPSMLayersPerSlice = 2 };
}

OceanPSM::OceanPSM()
{
	m_pFX = NULL;
	m_pPropagatePSMTechnique = NULL;
	m_pRenderPSMToUITechnique = NULL;

	m_pPSMPropagationMapUAVVariable = NULL;
	m_pPSMMapVariable = NULL;

	m_pd3dDevice = DXUTGetD3D11Device();
	m_pPSMSRV = NULL;
	m_pPSMUAV = NULL;
	m_pPSMRTV = NULL;

	m_PSM_w = 0;
	m_PSM_h = 0;
	m_PSM_d = 0;
	m_PSM_num_slices = 0;

	m_PsmBoundsMax = D3DXVECTOR3(1.f,1.f,1.f);
	m_PsmBoundsMin = D3DXVECTOR3(-1.f,-1.f,-1.f);

	D3DXMatrixIdentity(&m_matPSMView);
	D3DXMatrixIdentity(&m_matPSMProj);
	D3DXMatrixIdentity(&m_matWorldToPSMUV);
}

OceanPSM::~OceanPSM()
{
	SAFE_RELEASE(m_pFX);

	SAFE_RELEASE(m_pPSMSRV);
	SAFE_RELEASE(m_pPSMUAV);
	SAFE_RELEASE(m_pPSMRTV);
}

HRESULT OceanPSM::init(	const D3DXVECTOR3& psm_bounds_min,
						const D3DXVECTOR3& psm_bounds_max,
						int nominal_res
						)
{
	HRESULT hr;

	m_PsmBoundsMin = psm_bounds_min;
	m_PsmBoundsMax = psm_bounds_max;

	const D3DXVECTOR3 psm_bounds_extents = psm_bounds_max - psm_bounds_min;
	const float world_volume_of_psm_bounds = psm_bounds_extents.x * psm_bounds_extents.y * psm_bounds_extents.z;
	const float voxel_volume = world_volume_of_psm_bounds / float(nominal_res*nominal_res*nominal_res);
	const float voxel_length = powf(voxel_volume,1/3.f);

	m_PSM_w = (int)ceilf(psm_bounds_extents.x/voxel_length);
	m_PSM_h = (int)ceilf(psm_bounds_extents.y/voxel_length);
	m_PSM_d = (int)ceilf(psm_bounds_extents.z/voxel_length);
	m_PSM_num_slices = (m_PSM_d/kPSMLayersPerSlice)-1;

	SAFE_RELEASE(m_pFX);
    ID3DXBuffer* pEffectBuffer = NULL;
    V_RETURN(LoadFile(TEXT(".\\Media\\ocean_psm_d3d11.fxo"), &pEffectBuffer));
    V_RETURN(D3DX11CreateEffectFromMemory(pEffectBuffer->GetBufferPointer(), pEffectBuffer->GetBufferSize(), 0, m_pd3dDevice, &m_pFX));
    pEffectBuffer->Release();

	m_pPropagatePSMTechnique = m_pFX->GetTechniqueByName("PropagatePSMTech");
	m_pRenderPSMToUITechnique = m_pFX->GetTechniqueByName("RenderPSMToUITech");

	m_pPSMPropagationMapUAVVariable = m_pFX->GetVariableByName("g_PSMPropagationMapUAV")->AsUnorderedAccessView();
	m_pPSMMapVariable = m_pFX->GetVariableByName("g_PSMMap")->AsShaderResource();

	// Create PSM texture
	{
        D3D11_TEXTURE3D_DESC texDesc;
        texDesc.Format = DXGI_FORMAT_R16G16_TYPELESS;
        texDesc.Width = m_PSM_w;
        texDesc.Height = m_PSM_h;
        texDesc.MipLevels = 1;
        texDesc.Depth = m_PSM_d/kPSMLayersPerSlice;
        // texDesc.SampleDesc.Count = 1; 
        // texDesc.SampleDesc.Quality = 0; 
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

		ID3D11Texture3D* pTex = NULL;
        V_RETURN(m_pd3dDevice->CreateTexture3D(&texDesc, NULL, &pTex));

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MostDetailedMip = 0;
        srvDesc.Texture3D.MipLevels = texDesc.MipLevels;
        srvDesc.Format = DXGI_FORMAT_R16G16_UNORM;
        V_RETURN(m_pd3dDevice->CreateShaderResourceView(pTex, &srvDesc, &m_pPSMSRV) );

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
        rtvDesc.Texture3D.MipSlice = 0;
		rtvDesc.Texture3D.FirstWSlice = 0;
		rtvDesc.Texture3D.WSize = texDesc.Depth;
        rtvDesc.Format = DXGI_FORMAT_R16G16_UNORM;
        V_RETURN(m_pd3dDevice->CreateRenderTargetView(pTex, &rtvDesc, &m_pPSMRTV) );

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.MipSlice = 0;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.WSize = texDesc.Depth;
		uavDesc.Format = DXGI_FORMAT_R32_UINT;
		V_RETURN(m_pd3dDevice->CreateUnorderedAccessView(pTex, &uavDesc, &m_pPSMUAV) );

		SAFE_RELEASE(pTex);
	}

	return S_OK;
}

void OceanPSM::beginRenderToPSM(const D3DXMATRIX& matPSM, ID3D11DeviceContext* pDC)
{
	D3DXMatrixInverse(&m_matPSMView,NULL,&matPSM);

    // We use proj to scale and translate to PSM from local space
    const D3DXVECTOR4 PSM_centre_local = 0.5f * (m_PsmBoundsMax + m_PsmBoundsMin);
    const D3DXVECTOR4 PSM_extents_local = m_PsmBoundsMax - m_PsmBoundsMin;
    D3DXMATRIX TranslationComponent, ScalingComponent;
    D3DXMatrixTranslation(&TranslationComponent, -PSM_centre_local.x, -PSM_centre_local.y, m_PsmBoundsMax.z);		// Using z-max because effective light-dir is pointing down
    D3DXMatrixScaling(&ScalingComponent, 2.f/PSM_extents_local.x, 2.f/PSM_extents_local.y, 1.f/PSM_extents_local.z);
    m_matPSMProj = TranslationComponent * ScalingComponent;

	D3DXMATRIX matProjToUV;
	D3DXMatrixTranslation(&matProjToUV,0.5f,0.5f,0.f);
	matProjToUV._11 = 0.5f;
	matProjToUV._22 = -0.5f;
	m_matWorldToPSMUV = m_matPSMView * m_matPSMProj * matProjToUV;

	// Save rt setup to restore shortly...
	m_num_saved_viewports = sizeof(m_saved_viewports)/sizeof(m_saved_viewports[0]);
	pDC->RSGetViewports( &m_num_saved_viewports, m_saved_viewports);

	pDC->OMSetRenderTargets(1, &m_pPSMRTV, NULL);
	const D3D11_VIEWPORT opacityViewport = {0.f, 0.f, FLOAT(m_PSM_w), FLOAT(m_PSM_h), 0.f, 1.f };
	pDC->RSSetViewports(1, &opacityViewport);

	const D3DXVECTOR4 PSMClearColor(1.f,1.f,1.f,1.f);
	pDC->ClearRenderTargetView(m_pPSMRTV, (FLOAT*)&PSMClearColor);
}

void OceanPSM::endRenderToPSM(ID3D11DeviceContext* pDC)
{
	// Restore original viewports
	pDC->RSSetViewports(m_num_saved_viewports, m_saved_viewports);

	// Release RTV
	ID3D11RenderTargetView* pNullRTV = NULL;
	pDC->OMSetRenderTargets(1, &pNullRTV, NULL);

	// Propagate opacity
	m_pPSMPropagationMapUAVVariable->SetUnorderedAccessView(m_pPSMUAV);
	m_pPropagatePSMTechnique->GetPassByIndex(0)->Apply(0, pDC);

	const int num_groups_w = 1 + (m_PSM_w-1)/PSMPropagationCSBlockSize;
	const int num_groups_h = 1 + (m_PSM_h-1)/PSMPropagationCSBlockSize;
	pDC->Dispatch(num_groups_w,num_groups_h,1);

	// Release inputs
	m_pPSMPropagationMapUAVVariable->SetUnorderedAccessView(NULL);
	m_pPropagatePSMTechnique->GetPassByIndex(0)->Apply(0, pDC);
}

void OceanPSM::renderToUI(ID3D11DeviceContext* pDC)
{
	m_pPSMMapVariable->SetResource(m_pPSMSRV);
	m_pRenderPSMToUITechnique->GetPassByIndex(0)->Apply(0, pDC);

	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pDC->IASetInputLayout(NULL);
	pDC->Draw(4,0);

	m_pPSMMapVariable->SetResource(NULL);
	m_pRenderPSMToUITechnique->GetPassByIndex(0)->Apply(0, pDC);
}

void OceanPSM::setWriteParams(const OceanPSMParams& params)
{
	params.m_pPSMSlicesVariable->SetFloat(float(m_PSM_num_slices));
}

void OceanPSM::setReadParams(const OceanPSMParams& params, const D3DXVECTOR3& tint_color)
{
	params.m_pPSMTintVariable->SetFloatVector((FLOAT*)&tint_color);
	params.m_pPSMSlicesVariable->SetFloat(float(m_PSM_num_slices));
	params.m_pPSMMapVariable->SetResource(m_pPSMSRV);
}

OceanPSMParams::OceanPSMParams(ID3DX11Effect* pFX)
{
	m_pPSMMapVariable = pFX->GetVariableByName("g_PSMMap")->AsShaderResource();
	m_pPSMSlicesVariable = pFX->GetVariableByName("g_PSMSlices")->AsScalar();
	m_pPSMTintVariable = pFX->GetVariableByName("g_PSMTint")->AsVector();
}

void OceanPSM::clearReadParams(const OceanPSMParams& params)
{
	params.m_pPSMMapVariable->SetResource(NULL);
}
