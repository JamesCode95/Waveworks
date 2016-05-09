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
 
#include "dxstdafx.h"
#include "ocean_vessel.h"
#include "SDKmisc.h"

#include "GFSDK_WaveWorks.h"
#include "GFSDK_WaveWorks_D3D_Util.h"

#pragma warning(disable:4127)

struct vessel_vertex
{
	D3DXVECTOR4 position;
	DWORD color;
};

OceanVessel::OceanVessel()
{
	m_pIB = NULL;
	m_pVB = NULL;
	m_pLayout = NULL;
	m_NumTris = 0;
	m_NumVerts = 0;

	m_pFX = NULL;
	m_pRenderTechnique = NULL;

	m_pMatWorldViewProjVariable = NULL;

	m_pd3dDevice = DXUTGetD3D10Device();

	m_PitchSpringConstant = 10.f;
	m_PitchDampingConstant = 2.f;
	m_RollSpringConstant = 10.f;
	m_RollDampingConstant = 2.f;
	m_HeightSpringConstant = 10.f;
	m_HeightDampingConstant = 1.f;
}

OceanVessel::~OceanVessel()
{
	SAFE_RELEASE(m_pIB);
	SAFE_RELEASE(m_pVB);
	SAFE_RELEASE(m_pLayout);
	SAFE_RELEASE(m_pFX);
}

HRESULT OceanVessel::init(FLOAT w, FLOAT h, FLOAT l)
{
	HRESULT hr = S_OK;

	m_Draft = 0.6f * h;
	m_KeelLength = 0.8f * l;	// Effective length, based on draft
	m_Beam = w;

	// HLSL
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3D10_SHADER_DEBUG;
	#endif

	SAFE_RELEASE(m_pFX);
	TCHAR path[MAX_PATH];
	V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, TEXT("..\\Media\\ocean_vessel_d3d10.fxo")));
	V_RETURN( D3DX10CreateEffectFromFile(	path, NULL, NULL,
											"fx_4_0", dwShaderFlags, 0, m_pd3dDevice, NULL,
											NULL, &m_pFX, NULL, NULL ) );

	m_pRenderTechnique = m_pFX->GetTechniqueByName("RenderVesselTech");
	m_pMatWorldViewProjVariable = m_pFX->GetVariableByName("g_matWorldViewProj")->AsMatrix();

	const D3D10_INPUT_ELEMENT_DESC vessel_layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,                   D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, sizeof(D3DXVECTOR4), D3D10_INPUT_PER_VERTEX_DATA, 0 },
	};
	const UINT num_layout_elements = sizeof(vessel_layout)/sizeof(vessel_layout[0]);

	D3D10_PASS_DESC PassDesc;
	V_RETURN(m_pRenderTechnique->GetPassByIndex(0)->GetDesc(&PassDesc));

	SAFE_RELEASE(m_pLayout);
	V_RETURN(m_pd3dDevice->CreateInputLayout(	vessel_layout, num_layout_elements,
												PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize,
												&m_pLayout
												));

	// Update VB
	SAFE_RELEASE(m_pVB);

	enum { NumVerts = 8 };
	m_NumVerts = NumVerts;
	vessel_vertex verts[NumVerts];

	// Keel
	verts[0].color = 0xFF6F6F6F; verts[0].position = D3DXVECTOR4(0.f, 0.f, 0.5f * m_KeelLength, 1.f);	// Fore
	verts[1].color = 0xFF6F6F6F; verts[1].position = D3DXVECTOR4(0.f, 0.f,-0.5f * m_KeelLength, 1.f);	// Aft

	// Gunwales
	verts[2].color = 0xFF9F9F9F; verts[2].position = D3DXVECTOR4( 0.5f * w, h,  0.25f * l, 1.f);	// Fore, starboard
	verts[3].color = 0xFF9F9F9F; verts[3].position = D3DXVECTOR4(-0.5f * w, h,  0.25f * l, 1.f);	// Fore, port
	verts[4].color = 0xFF9F9F9F; verts[4].position = D3DXVECTOR4( 0.5f * w, h, -0.25f * l, 1.f);	// Aft, starboard
	verts[5].color = 0xFF9F9F9F; verts[5].position = D3DXVECTOR4(-0.5f * w, h, -0.25f * l, 1.f);	// Aft, port

	verts[6].color = 0xFF9F9F9F; verts[6].position = D3DXVECTOR4(0.f, h,  0.6f * l, 1.f);	// Bow
	verts[7].color = 0xFF9F9F9F; verts[7].position = D3DXVECTOR4(0.f, h, -0.4f * l, 1.f);	// Stern

	D3D10_BUFFER_DESC vBufferDesc;
	vBufferDesc.ByteWidth = m_NumVerts * sizeof(verts[0]);
	vBufferDesc.Usage = D3D10_USAGE_IMMUTABLE;
	vBufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	vBufferDesc.CPUAccessFlags = 0;
	vBufferDesc.MiscFlags = 0;

	D3D10_SUBRESOURCE_DATA vSrd;
	vSrd.pSysMem = verts;
	vSrd.SysMemPitch = 0;
	vSrd.SysMemSlicePitch = 0;

	V_RETURN(m_pd3dDevice->CreateBuffer(&vBufferDesc, &vSrd, &m_pVB));
	assert(m_pVB);

	// Update IB
	SAFE_RELEASE(m_pIB);

	enum { NumTris = 12 };
	m_NumTris = NumTris;
	DWORD indices[3 * NumTris];

	int ixix = 0;

	// Port hull
	indices[ixix++] = 0; indices[ixix++] = 3; indices[ixix++] = 5;
	indices[ixix++] = 0; indices[ixix++] = 5; indices[ixix++] = 1;

	// Starboard hull
	indices[ixix++] = 1; indices[ixix++] = 4; indices[ixix++] = 2;
	indices[ixix++] = 1; indices[ixix++] = 2; indices[ixix++] = 0;

	// Bow
	indices[ixix++] = 0; indices[ixix++] = 6; indices[ixix++] = 3;	// Port
	indices[ixix++] = 0; indices[ixix++] = 2; indices[ixix++] = 6;	// Starboard

	// Stern
	indices[ixix++] = 1; indices[ixix++] = 7; indices[ixix++] = 4;	// Starboard
	indices[ixix++] = 1; indices[ixix++] = 5; indices[ixix++] = 7;	// Port

	// Deck
	indices[ixix++] = 6; indices[ixix++] = 2; indices[ixix++] = 3;
	indices[ixix++] = 2; indices[ixix++] = 3; indices[ixix++] = 4;
	indices[ixix++] = 3; indices[ixix++] = 4; indices[ixix++] = 5;
	indices[ixix++] = 4; indices[ixix++] = 5; indices[ixix++] = 7;

	D3D10_BUFFER_DESC iBufferDesc;
	iBufferDesc.ByteWidth = 3 * m_NumTris * sizeof(indices[0]);
	iBufferDesc.Usage = D3D10_USAGE_IMMUTABLE;
	iBufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
	iBufferDesc.CPUAccessFlags = 0;
	iBufferDesc.MiscFlags = 0;

	D3D10_SUBRESOURCE_DATA iSrd;
	iSrd.pSysMem = indices;
	iSrd.SysMemPitch = 0;
	iSrd.SysMemSlicePitch = 0;

	V_RETURN(m_pd3dDevice->CreateBuffer(&iBufferDesc, &iSrd, &m_pIB));
	assert(m_pIB);

	return S_OK;
}

void OceanVessel::renderVessel(const CBaseCamera& camera)
{
	if(NULL == m_pIB || NULL == m_pVB)
		return;

	// Matrices
	D3DXMATRIX matView = *camera.GetViewMatrix();
	D3DXMATRIX matProj = *camera.GetProjMatrix();
	
	// View-proj
	D3DXMATRIX matWVP = m_LocalToWorld * matView * matProj;
	m_pMatWorldViewProjVariable->SetMatrix((FLOAT*)&matWVP);

	// Stream
	const UINT vbOffset = 0;
	const UINT vertexStride = sizeof(vessel_vertex);
	m_pd3dDevice->IASetInputLayout(m_pLayout);
    m_pd3dDevice->IASetVertexBuffers(0, 1, &m_pVB, &vertexStride, &vbOffset);
    m_pd3dDevice->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R32_UINT, 0);

	// Draw
	m_pRenderTechnique->GetPassByIndex(0)->Apply(0);
	m_pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pd3dDevice->DrawIndexed(m_NumTris * 3, 0, 0);
}

void OceanVessel::setPosition(D3DXVECTOR2 pos)
{
	m_Position = pos;
	ResetDynamicState();
}

void OceanVessel::setHeading(D3DXVECTOR2 heading)
{
	m_Heading = heading;
	D3DXVec2Normalize(&m_Heading, &m_Heading);
	ResetDynamicState();
}

void OceanVessel::updateVesselMotion(GFSDK_WaveWorks_SimulationHandle hSim, FLOAT sea_level, FLOAT time_delta, FLOAT water_scale)
{
	// Sample ocean heights along the keel and across the beam
	enum { NumSamples = 5 };
	gfsdk_float2 sample_pos[2*NumSamples];

	{
		const D3DXVECTOR2 keel_vec = m_Heading * m_KeelLength;
		const D3DXVECTOR2 keel_step_vec = keel_vec * 1.f/float(NumSamples-1);
		D3DXVECTOR2 curr_sample_pos = m_Position - 0.5f * keel_vec;
		for(int i = 0; i != NumSamples; ++i)
		{
			sample_pos[i] = NvFromDX(curr_sample_pos / water_scale);
			curr_sample_pos += keel_step_vec;
		}
	}

	{
		const D3DXVECTOR2 beam_vec = D3DXVECTOR2(m_Heading.y, -m_Heading.x) * m_KeelLength;
		const D3DXVECTOR2 beam_step_vec = beam_vec * 1.f/float(NumSamples-1);
		D3DXVECTOR2 curr_sample_pos = m_Position - 0.5f * beam_vec;
		for(int i = 0; i != NumSamples; ++i)
		{
			sample_pos[NumSamples+i] = NvFromDX(curr_sample_pos / water_scale);
			curr_sample_pos += beam_step_vec;
		}
	}

	gfsdk_float4 displacements[2*NumSamples];
	GFSDK_WaveWorks_Simulation_GetDisplacements(hSim, sample_pos, displacements, 2*NumSamples);

	// Caculate the mean displacement
	D3DXVECTOR4 mean_displacement(0.f,0.f,0.f,0.f);
	FLOAT xz_sum = 0.f;
	FLOAT x_sum = 0.f;
	FLOAT z_sum_pitch = 0.f;
	FLOAT xx_sum = 0.f;
	FLOAT curr_x = 0.f;
	FLOAT yz_sum = 0.f;
	FLOAT y_sum = 0.f;
	FLOAT z_sum_roll = 0.f;
	FLOAT yy_sum = 0.f;
	FLOAT curr_y = 0.f;
	const FLOAT x_step = m_KeelLength/float(NumSamples-1);
	const FLOAT y_step = m_Beam/float(NumSamples-1);
	for(int i = 0; i != NumSamples; ++i)
	{
		// Use mean of keel samples for displacement
		mean_displacement += NvToDX(displacements[i]);

		// Best-fit slope est for pitch
		xz_sum += curr_x * displacements[i].z;
		x_sum += curr_x;
		z_sum_pitch += displacements[i].z;
		xx_sum += curr_x * curr_x;
		curr_x += x_step;

		// Best-fit slope est for roll
		yz_sum += curr_y * displacements[NumSamples+i].z;
		y_sum += curr_y;
		z_sum_roll += displacements[NumSamples+i].z;
		yy_sum += curr_y * curr_y;
		curr_y += y_step;
	}
	mean_displacement *= water_scale/float(NumSamples);
	const FLOAT mean_pitch_slope = water_scale * (float(NumSamples)*xz_sum - x_sum*z_sum_pitch)/(float(NumSamples)*xx_sum - x_sum * x_sum);
	const FLOAT mean_roll_slope = water_scale * (float(NumSamples)*yz_sum - y_sum*z_sum_roll)/(float(NumSamples)*yy_sum - y_sum * y_sum);

	const FLOAT surface_roll = atan2f(mean_roll_slope, 1.f);
	const FLOAT surface_pitch = atan2f(mean_pitch_slope, 1.f);
	const FLOAT surface_height = mean_displacement.z;

	if(m_DynamicsCountdown) {
		// Snap to surface on first few updates
		m_Pitch = surface_pitch;
		m_Roll = surface_roll;
		m_Height = surface_height;
		--m_DynamicsCountdown;
	} else {
		// Run pseudo-dynamics
		const FLOAT kMinTimeStep = 0.02f;
		const int num_steps = int(ceilf(time_delta/kMinTimeStep));
		const FLOAT time_step = time_delta/float(num_steps);
		for(int i = 0; i != num_steps; ++i)
		{
			const FLOAT pitch_accel = (surface_pitch - m_Pitch) * m_PitchSpringConstant - m_PitchRate *m_PitchDampingConstant;
			m_PitchRate += pitch_accel * time_step;
			m_Pitch += m_PitchRate * time_step;

			const FLOAT roll_accel = (surface_roll - m_Roll) * m_RollSpringConstant - m_RollRate *m_RollDampingConstant;
			m_RollRate += roll_accel * time_step;
			m_Roll += m_RollRate * time_step;

			const FLOAT height_accel = (surface_height - m_Height) * m_HeightSpringConstant - m_HeightRate *m_HeightDampingConstant;
			m_HeightRate += height_accel * time_step;
			m_Height += m_HeightRate * time_step;
		}
	}

	D3DXMATRIX mat_roll;
	D3DXMatrixRotationZ(&mat_roll, m_Roll);

	D3DXMATRIX mat_pitch;
	D3DXMatrixRotationX(&mat_pitch, -m_Pitch);

	D3DXMATRIX mat_heading;
	const FLOAT heading = atan2f(m_Heading.y, m_Heading.x);
	D3DXMatrixRotationY(&mat_heading, heading);

	m_LocalToWorld = mat_roll * mat_pitch * mat_heading;

	m_LocalToWorld._41 = m_Position.x + mean_displacement.x;
	m_LocalToWorld._42 = sea_level + m_Height - m_Draft;
	m_LocalToWorld._43 = m_Position.y + mean_displacement.y;

	D3DXMATRIX cameraToLocal;
	D3DXMatrixIdentity(&cameraToLocal);
	cameraToLocal._42 = 3.f * m_Draft;	// Place the camera at 2x 'draft' above mean water line
	cameraToLocal._43 = -0.5f * m_KeelLength;	// And near the back, where the 'bridge' might be
	m_CameraToWorld = cameraToLocal * m_LocalToWorld;
}

void OceanVessel::ResetDynamicState()
{
	m_Pitch = 0.f;
	m_PitchRate = 0.f;
	m_Roll = 0.f;
	m_RollRate = 0.f;
	m_Height = 0.f;
	m_HeightRate = 0.f;
	m_DynamicsCountdown = 3;
}