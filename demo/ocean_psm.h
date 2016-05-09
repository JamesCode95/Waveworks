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

#ifndef _OCEAN_PSM_H
#define _OCEAN_PSM_H

#include <D3DX11Effect.h>

class OceanPSMParams
{
public:
	
	OceanPSMParams(ID3DX11Effect* pFX);

private:

	friend class OceanPSM;
	ID3DX11EffectShaderResourceVariable* m_pPSMMapVariable;
	ID3DX11EffectScalarVariable* m_pPSMSlicesVariable;
	ID3DX11EffectVectorVariable* m_pPSMTintVariable;
};

class OceanPSM
{
public:
	OceanPSM();
	~OceanPSM();

	HRESULT init(	const D3DXVECTOR3& psm_bounds_min,
					const D3DXVECTOR3& psm_bounds_max,
					int nominal_res
					);

	void beginRenderToPSM(const D3DXMATRIX& matPSM, ID3D11DeviceContext* pDC);
	void endRenderToPSM(ID3D11DeviceContext* pDC);

	void setWriteParams(const OceanPSMParams& params);
	void setReadParams(const OceanPSMParams& params, const D3DXVECTOR3& tint_color);
	void clearReadParams(const OceanPSMParams& params);

	void renderToUI(ID3D11DeviceContext* pDC);

	ID3D11ShaderResourceView* getPSMSRV() const { return m_pPSMSRV; }

	const D3DXMATRIX* getPSMView() const { return &m_matPSMView; }
	const D3DXMATRIX* getPSMProj() const { return &m_matPSMProj; }
	const D3DXMATRIX* getWorldToPSMUV() const { return &m_matWorldToPSMUV; }

	int getNumSlices() const { return m_PSM_num_slices; }

private:

	void renderParticles(ID3D11DeviceContext* pDC, ID3DX11EffectTechnique* pTech);

	ID3DX11Effect* m_pFX;
	ID3DX11EffectTechnique* m_pPropagatePSMTechnique;
	ID3DX11EffectTechnique* m_pRenderPSMToUITechnique;

	ID3DX11EffectShaderResourceVariable* m_pPSMMapVariable;
	ID3DX11EffectUnorderedAccessViewVariable* m_pPSMPropagationMapUAVVariable;

	// D3D resources
	ID3D11Device* m_pd3dDevice;
	ID3D11ShaderResourceView* m_pPSMSRV;
	ID3D11UnorderedAccessView* m_pPSMUAV;
	ID3D11RenderTargetView* m_pPSMRTV;

	int m_PSM_w;
	int m_PSM_h;
	int m_PSM_d;
	int m_PSM_num_slices;

	D3DXVECTOR3 m_PsmBoundsMin;
	D3DXVECTOR3 m_PsmBoundsMax;

	D3DXMATRIX m_matPSMView;
	D3DXMATRIX m_matPSMProj;
	D3DXMATRIX m_matWorldToPSMUV;

	// State save/restore
	D3D11_VIEWPORT m_saved_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	UINT m_num_saved_viewports;
};

#endif	// _OCEAN_PSM_H
