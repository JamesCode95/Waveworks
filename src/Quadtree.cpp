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
#include "Quadtree_impl.h"
#include "Savestate_impl.h"
#include "Simulation_Util.h"
#include "D3DX_replacement_code.h"
#include "Graphics_Context.h"

#include <algorithm>
#include <string.h>

#if WAVEWORKS_ENABLE_GNM
#include "orbis\GNM_Util.h"
using namespace sce;
#else
#pragma warning(disable:4127)
#endif

namespace {
#if WAVEWORKS_ENABLE_GRAPHICS
// The contents of Quadtree_map.h are generated somewhat indiscriminately, so
// use a pragma to suppress fluffy warnings under gcc
	#ifdef __GNUC__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-variable"
	#endif
	#include "Quadtree_map.h"
		#ifdef __GNUC__
		#pragma GCC diagnostic pop
	#endif
#endif

namespace SM4 {
#if WAVEWORKS_ENABLE_D3D10
#include "Quadtree_SM4_sig.h"
#endif
}

namespace SM5 {
#if WAVEWORKS_ENABLE_D3D11
#include "Quadtree_SM5_sig.h"
#endif
}

}

enum ShaderInputsD3D9
{
	ShaderInputD3D9_g_matLocalWorld = 0,
	ShaderInputD3D9_g_vsEyePos,
	ShaderInputD3D9_g_MorphParam,
	NumShaderInputsD3D9
};

enum ShaderInputsD3D10
{
	ShaderInputD3D10_vs_buffer = 0,
	NumShaderInputsD3D10
};

enum ShaderInputsD3D11
{
	ShaderInputD3D11_vs_buffer = 0,
	ShaderInputD3D11_hs_buffer,
	NumShaderInputsD3D11
};

enum ShaderInputsGnm
{
	ShaderInputGnm_vs_buffer = 0,
	ShaderInputGnm_hs_buffer,
	NumShaderInputsGnm
};

enum ShaderInputsGL2
{
	ShaderInputGL2_g_matLocalWorld = 0,
	ShaderInputGL2_g_vsEyePos,
	ShaderInputGL2_g_MorphParam,
	ShaderInputGL2_attr_vPos,
	NumShaderInputsGL2
};

// NB: These should be kept synchronised with the shader source
#if WAVEWORKS_ENABLE_D3D9
const GFSDK_WaveWorks_ShaderInput_Desc ShaderInputD3D9Descs[NumShaderInputsD3D9] = {
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_FloatConstant, nvsf_g_matLocalWorld, 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_FloatConstant, nvsf_g_vsEyePos,      3 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_FloatConstant, nvsf_g_MorphParam,    4 }
};
#endif

#if WAVEWORKS_ENABLE_D3D10
const GFSDK_WaveWorks_ShaderInput_Desc ShaderInputD3D10Descs[NumShaderInputsD3D10] = {
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_ConstantBuffer, nvsf_geom_buffer, 0 }
};
#endif

#if WAVEWORKS_ENABLE_D3D11
const GFSDK_WaveWorks_ShaderInput_Desc ShaderInputD3D11Descs[NumShaderInputsD3D11] = {
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_ConstantBuffer, nvsf_geom_buffer, 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::HullShader_ConstantBuffer, nvsf_eyepos_buffer, 0 }
};
#endif

#if WAVEWORKS_ENABLE_GNM
const GFSDK_WaveWorks_ShaderInput_Desc ShaderInputGnmDescs[NumShaderInputsGnm] = {
	{ GFSDK_WaveWorks_ShaderInput_Desc::VertexShader_ConstantBuffer, nvsf_geom_buffer, 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::HullShader_ConstantBuffer, nvsf_eyepos_buffer, 0 }
};
#endif

#if WAVEWORKS_ENABLE_GL
const GFSDK_WaveWorks_ShaderInput_Desc ShaderInputGL2Descs[NumShaderInputsGL2] = {
	{ GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_UniformLocation, nvsf_g_matLocalWorld, 0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_UniformLocation, nvsf_g_vsEyePos,      0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_UniformLocation, nvsf_g_MorphParam,    0 },
	{ GFSDK_WaveWorks_ShaderInput_Desc::GL_AttribLocation,               nvsf_vPos,            0 }
};
#endif
struct vs_cbuffer
{
	float g_matLocalWorld[12];
	float g_vsEyePos[4];
	float g_MorphParam[4];
};

struct hs_cbuffer
{
	float g_eyePos[4];
	float g_tessellationParams[4];
};

struct water_quadtree_vertex
{
	float index_x;
	float index_y;
};

struct QuadNode
{
	gfsdk_float2 bottom_left;
	float length;
	int lod;

	int sub_node[4];

	float morph_sign;
};

namespace
{
	bool compareQuadNodeLength(const QuadNode& a, const QuadNode& b)
	{
		return (a.length<b.length);
	}
}

bool GFSDK_WaveWorks_Quadtree::QuadCoord::operator<(const GFSDK_WaveWorks_Quadtree::QuadCoord& rhs) const
{
	// NB: We reverse the direction of the lod order, this causes the largest quads to sort
	// to the start of the collection, where we can use them as traversal roots
	if(lod > rhs.lod)
		return true;
	else if(lod < rhs.lod)
		return false;

	if(x < rhs.x)
		return true;
	else if(x > rhs.x)
		return false;

	if(y < rhs.y)
		return true;
	else
		return false;
}

bool GFSDK_WaveWorks_Quadtree::AllocQuad::operator<(const GFSDK_WaveWorks_Quadtree::AllocQuad& rhs) const
{
	return coords < rhs.coords;
}

GFSDK_WaveWorks_Quadtree::GFSDK_WaveWorks_Quadtree()
{
	frustum_cull_margin = 0;
	m_stats.CPU_quadtree_update_time = 0;
	memset(&m_params, 0, sizeof(m_params));
	memset(&m_d3d, 0, sizeof(m_d3d));

	m_pMesh = NULL;
	m_d3dAPI = nv_water_d3d_api_undefined;
}

GFSDK_WaveWorks_Quadtree::~GFSDK_WaveWorks_Quadtree()
{
	releaseD3DObjects();

	m_unsorted_render_list.clear();
	m_render_roots_list.clear();
	m_sorted_render_list.clear();
}

void GFSDK_WaveWorks_Quadtree::releaseD3DObjects()
{
	SAFE_DELETE(m_pMesh);

#if WAVEWORKS_ENABLE_GRAPHICS
	switch(m_d3dAPI)
	{
#if WAVEWORKS_ENABLE_D3D9
	case nv_water_d3d_api_d3d9:
		{
			SAFE_RELEASE(m_d3d._9.m_pd3d9Device);
			m_d3dAPI = nv_water_d3d_api_undefined;
		}
		break;
#endif
#if WAVEWORKS_ENABLE_D3D10
	case nv_water_d3d_api_d3d10:
		{
			SAFE_RELEASE(m_d3d._10.m_pd3d10VertexShaderCB);
			SAFE_RELEASE(m_d3d._10.m_pd3d10Device);
			m_d3dAPI = nv_water_d3d_api_undefined;
		}
		break;
#endif
#if WAVEWORKS_ENABLE_D3D11
	case nv_water_d3d_api_d3d11:
		{
			SAFE_RELEASE(m_d3d._11.m_pd3d11VertexShaderCB);
			SAFE_RELEASE(m_d3d._11.m_pd3d11HullShaderCB);
			SAFE_RELEASE(m_d3d._11.m_pd3d11Device);
			m_d3dAPI = nv_water_d3d_api_undefined;
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			m_d3dAPI = nv_water_d3d_api_undefined;
		}
		break;
#endif
	default:
		break;
#if WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_gl2:
		{
			// nothing to release
			m_d3dAPI = nv_water_d3d_api_undefined;
		}
		break;
#endif
	}
#endif // WAVEWORKS_ENABLE_GRAPHICS
}

HRESULT GFSDK_WaveWorks_Quadtree::allocateD3DObjects()
{
#if WAVEWORKS_ENABLE_GRAPHICS
	switch(m_d3dAPI)
	{
#if WAVEWORKS_ENABLE_D3D9
	case nv_water_d3d_api_d3d9:
		{
		}
		break;
#endif
#if WAVEWORKS_ENABLE_D3D10
	case nv_water_d3d_api_d3d10:
		{
			HRESULT hr;
			SAFE_RELEASE(m_d3d._10.m_pd3d10VertexShaderCB);

			D3D10_BUFFER_DESC vscbDesc;
			vscbDesc.ByteWidth = sizeof(vs_cbuffer);
			vscbDesc.Usage = D3D10_USAGE_DEFAULT;
			vscbDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
			vscbDesc.CPUAccessFlags = 0;
			vscbDesc.MiscFlags = 0;
			V_RETURN(m_d3d._10.m_pd3d10Device->CreateBuffer(&vscbDesc, NULL, &m_d3d._10.m_pd3d10VertexShaderCB));
		}
		break;
#endif
#if WAVEWORKS_ENABLE_D3D11
	case nv_water_d3d_api_d3d11:
		{
			HRESULT hr;
			SAFE_RELEASE(m_d3d._11.m_pd3d11VertexShaderCB);
			SAFE_RELEASE(m_d3d._11.m_pd3d11HullShaderCB);

			D3D11_BUFFER_DESC vscbDesc;
			vscbDesc.ByteWidth = sizeof(vs_cbuffer);
			vscbDesc.Usage = D3D11_CB_CREATION_USAGE;
			vscbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			vscbDesc.CPUAccessFlags = D3D11_CB_CREATION_CPU_ACCESS_FLAGS;
			vscbDesc.MiscFlags = 0;
			vscbDesc.StructureByteStride = 0;
			V_RETURN(m_d3d._11.m_pd3d11Device->CreateBuffer(&vscbDesc, NULL, &m_d3d._11.m_pd3d11VertexShaderCB));
			
			D3D11_BUFFER_DESC hscbDesc;
			hscbDesc.ByteWidth = sizeof(hs_cbuffer);
			hscbDesc.Usage = D3D11_CB_CREATION_USAGE;
			hscbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			hscbDesc.CPUAccessFlags = D3D11_CB_CREATION_CPU_ACCESS_FLAGS;
			hscbDesc.MiscFlags = 0;
			hscbDesc.StructureByteStride = 0;
			V_RETURN(m_d3d._11.m_pd3d11Device->CreateBuffer(&hscbDesc, NULL, &m_d3d._11.m_pd3d11HullShaderCB));
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			// nothing to do
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_gl2:
		{
			// nothing to do
		}
		break;
#endif
	default:
		// Unexpected API
		return E_FAIL;
	}
#endif // WAVEWORKS_ENABLE_GRAPHICS

	return S_OK;
}

HRESULT GFSDK_WaveWorks_Quadtree::initD3D9(const GFSDK_WaveWorks_Quadtree_Params& D3D9_ONLY(params), IDirect3DDevice9* D3D9_ONLY(pD3DDevice))
{
#if WAVEWORKS_ENABLE_D3D9
	HRESULT hr;

	if(nv_water_d3d_api_d3d9 != m_d3dAPI)
	{
		releaseD3DObjects();
	}
	else if(m_d3d._9.m_pd3d9Device != pD3DDevice)
	{
		releaseD3DObjects();
	}

	if(nv_water_d3d_api_undefined == m_d3dAPI)
	{
		m_d3dAPI = nv_water_d3d_api_d3d9;
		m_d3d._9.m_pd3d9Device = pD3DDevice;
		m_d3d._9.m_pd3d9Device->AddRef();

		V_RETURN(allocateD3DObjects());
	}

	return reinit(params);
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Quadtree::initD3D10(const GFSDK_WaveWorks_Quadtree_Params& D3D10_ONLY(params), ID3D10Device* D3D10_ONLY(pD3DDevice))
{
#if WAVEWORKS_ENABLE_D3D10
	HRESULT hr;

	if(nv_water_d3d_api_d3d10 != m_d3dAPI)
	{
		releaseD3DObjects();
	}
	else if(m_d3d._10.m_pd3d10Device != pD3DDevice)
	{
		releaseD3DObjects();
	}

	if(nv_water_d3d_api_undefined == m_d3dAPI)
	{
		m_d3dAPI = nv_water_d3d_api_d3d10;
		m_d3d._10.m_pd3d10Device = pD3DDevice;
		m_d3d._10.m_pd3d10Device->AddRef();

		V_RETURN(allocateD3DObjects());
	}

	return reinit(params);
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Quadtree::initD3D11(const GFSDK_WaveWorks_Quadtree_Params& D3D11_ONLY(params), ID3D11Device* D3D11_ONLY(pD3DDevice))
{
#if WAVEWORKS_ENABLE_D3D11
	HRESULT hr;

	if(nv_water_d3d_api_d3d11 != m_d3dAPI)
	{
		releaseD3DObjects();
	}
	else if(m_d3d._11.m_pd3d11Device != pD3DDevice)
	{
		releaseD3DObjects();
	}

	if(nv_water_d3d_api_undefined == m_d3dAPI)
	{
		// Only accept true DX11 devices if use_tessellation is set to true
		D3D_FEATURE_LEVEL FeatureLevel = pD3DDevice->GetFeatureLevel();
		if((FeatureLevel < D3D_FEATURE_LEVEL_11_0) && (m_params.use_tessellation == true))
		{
			return E_FAIL;
		}
		m_d3dAPI = nv_water_d3d_api_d3d11;
		m_d3d._11.m_pd3d11Device = pD3DDevice;
		m_d3d._11.m_pd3d11Device->AddRef();

		V_RETURN(allocateD3DObjects());
	}

	return reinit(params);
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Quadtree::initGnm(const GFSDK_WaveWorks_Quadtree_Params& GNM_ONLY(params))
{
#if WAVEWORKS_ENABLE_GNM
	HRESULT hr;

	if(nv_water_d3d_api_gnm != m_d3dAPI)
	{
		releaseD3DObjects();
	}

	if(nv_water_d3d_api_undefined == m_d3dAPI)
	{
		m_d3dAPI = nv_water_d3d_api_gnm;
		V_RETURN(allocateD3DObjects());
	}

	return reinit(params);
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Quadtree::initGL2(const GFSDK_WaveWorks_Quadtree_Params& GL_ONLY(params), GLuint GL_ONLY(Program))
{
#if WAVEWORKS_ENABLE_GL
	HRESULT hr;

	if(nv_water_d3d_api_gl2 != m_d3dAPI)
	{
		releaseD3DObjects();
	}

	if(nv_water_d3d_api_undefined == m_d3dAPI)
	{
		m_d3dAPI = nv_water_d3d_api_gl2;
		V_RETURN(allocateD3DObjects());
	}
	m_d3d._GL2.m_pGL2QuadtreeProgram = Program;
	return reinit(params);
#else
	return S_FALSE;
#endif
}


HRESULT GFSDK_WaveWorks_Quadtree::reinit(const GFSDK_WaveWorks_Quadtree_Params& params)
{
	HRESULT hr;

	BOOL reinitGeometry = FALSE;
	if(NULL == m_pMesh || params.mesh_dim != m_params.mesh_dim || params.use_tessellation != m_params.use_tessellation)
	{
		reinitGeometry = TRUE;
	}
	m_params = params;

	if(reinitGeometry)
	{
		V_RETURN(initGeometry());
	}

	return S_OK;
}

#define MESH_INDEX_2D(x, y)	(((y) + vert_rect.bottom) * (mesh_dim + 1) + (x) + vert_rect.left)

#if !defined(TARGET_PLATFORM_MICROSOFT)
struct RECT {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
};
#endif

// Generate boundary mesh for a patch. Return the number of generated indices
int generateBoundaryMesh(int left_degree, int right_degree, int bottom_degree, int top_degree,
						 RECT vert_rect, int mesh_dim, DWORD* output)
{
	int i, j;
	int counter = 0;
	int width = vert_rect.right - vert_rect.left;
	int height = vert_rect.top - vert_rect.bottom;

	int b_step = bottom_degree ? width / bottom_degree : 0;
	int r_step = right_degree ? height / right_degree : 0;
	int t_step = top_degree ? width / top_degree : 0;
	int l_step = left_degree ? height / left_degree : 0;

	// Triangle list for bottom boundary
	if (b_step > 0)
	{
		const int b_min = b_step < l_step ? b_step : 0;
		const int b_max = b_step < r_step ? width - b_step : width;
		for (i = b_min; i < b_max; i += b_step)
		{
			output[counter++] = MESH_INDEX_2D(i, 0);
			output[counter++] = MESH_INDEX_2D(i + b_step, 0);
			output[counter++] = MESH_INDEX_2D(i + b_step / 2, b_step / 2);

			if(i != 0 || b_step != l_step) {
				for (j = 0; j < b_step / 2; j ++)
				{
					output[counter++] = MESH_INDEX_2D(i, 0);
					output[counter++] = MESH_INDEX_2D(i + j + 1, b_step / 2);
					output[counter++] = MESH_INDEX_2D(i + j, b_step / 2);
				}
			}

			if(i != width - b_step || b_step != r_step) {
				for (j = b_step / 2; j < b_step; j ++)
				{
					output[counter++] = MESH_INDEX_2D(i + b_step, 0);
					output[counter++] = MESH_INDEX_2D(i + j + 1, b_step / 2);
					output[counter++] = MESH_INDEX_2D(i + j, b_step / 2);
				}
			}
		}
	}

	// Right boundary
	if (r_step > 0)
	{
		const int r_min = r_step < b_step ? r_step : 0;
		const int r_max = r_step < t_step ? height - r_step : height;
		for (i = r_min; i < r_max; i += r_step)
		{
			output[counter++] = MESH_INDEX_2D(width, i);
			output[counter++] = MESH_INDEX_2D(width, i + r_step);
			output[counter++] = MESH_INDEX_2D(width - r_step / 2, i + r_step / 2);

			if(i != 0 || r_step != b_step) {
				for (j = 0; j < r_step / 2; j ++)
				{
					output[counter++] = MESH_INDEX_2D(width, i);
					output[counter++] = MESH_INDEX_2D(width - r_step / 2, i + j + 1);
					output[counter++] = MESH_INDEX_2D(width - r_step / 2, i + j);
				}
			}

			if(i != height - r_step || r_step != t_step) {
				for (j = r_step / 2; j < r_step; j ++)
				{
					output[counter++] = MESH_INDEX_2D(width, i + r_step);
					output[counter++] = MESH_INDEX_2D(width - r_step / 2, i + j + 1);
					output[counter++] = MESH_INDEX_2D(width - r_step / 2, i + j);
				}
			}
		}
	}

	// Top boundary
	if (t_step > 0)
	{
		const int t_min = t_step < l_step ? t_step : 0;
		const int t_max = t_step < r_step ? width - t_step : width;
		for (i = t_min; i < t_max; i += t_step)
		{
			output[counter++] = MESH_INDEX_2D(i, height);
			output[counter++] = MESH_INDEX_2D(i + t_step / 2, height - t_step / 2);
			output[counter++] = MESH_INDEX_2D(i + t_step, height);

			if(i != 0 || t_step != l_step) {
				for (j = 0; j < t_step / 2; j ++)
				{
					output[counter++] = MESH_INDEX_2D(i, height);
					output[counter++] = MESH_INDEX_2D(i + j, height - t_step / 2);
					output[counter++] = MESH_INDEX_2D(i + j + 1, height - t_step / 2);
				}
			}

			if(i != width - t_step || t_step != r_step) {
				for (j = t_step / 2; j < t_step; j ++)
				{
					output[counter++] = MESH_INDEX_2D(i + t_step, height);
					output[counter++] = MESH_INDEX_2D(i + j, height - t_step / 2);
					output[counter++] = MESH_INDEX_2D(i + j + 1, height - t_step / 2);
				}
			}
		}
	}

	// Left boundary
	if (l_step > 0)
	{
		const int l_min = l_step < b_step ? l_step : 0;
		const int l_max = l_step < t_step ? height - l_step : height;
		for (i = l_min; i < l_max; i += l_step)
		{
			output[counter++] = MESH_INDEX_2D(0, i);
			output[counter++] = MESH_INDEX_2D(l_step / 2, i + l_step / 2);
			output[counter++] = MESH_INDEX_2D(0, i + l_step);

			if(i != 0 || l_step != b_step) {
				for (j = 0; j < l_step / 2; j ++)
				{
					output[counter++] = MESH_INDEX_2D(0, i);
					output[counter++] = MESH_INDEX_2D(l_step / 2, i + j);
					output[counter++] = MESH_INDEX_2D(l_step / 2, i + j + 1);
				}
			}

			if(i != height - l_step || l_step != t_step) {
				for (j = l_step / 2; j < l_step; j ++)
				{
					output[counter++] = MESH_INDEX_2D(0, i + l_step);
					output[counter++] = MESH_INDEX_2D(l_step / 2, i + j);
					output[counter++] = MESH_INDEX_2D(l_step / 2, i + j + 1);
				}
			}
		}
	}

	return counter;
}

// Generate inner mesh for a patch. Return the number of generated indices
int generateInnerMesh(RECT vert_rect, int mesh_dim, bool generate_diamond_grid, DWORD* output)
{
	int i, j;
	int counter = 0;
	int width = vert_rect.right - vert_rect.left;
	int height = vert_rect.top - vert_rect.bottom;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			
				if(((i + j + vert_rect.left + vert_rect.bottom) % 2 == 0) || (!generate_diamond_grid))
				{
					output[counter++] = MESH_INDEX_2D(j, i);
					output[counter++] = MESH_INDEX_2D(j + 1, i);
					output[counter++] = MESH_INDEX_2D(j + 1, i + 1);
					output[counter++] = MESH_INDEX_2D(j, i);
					output[counter++] = MESH_INDEX_2D(j + 1, i + 1);
					output[counter++] = MESH_INDEX_2D(j, i + 1);
				}
				
				else
				{
					output[counter++] = MESH_INDEX_2D(j + 1, i);
					output[counter++] = MESH_INDEX_2D(j + 1, i + 1);
					output[counter++] = MESH_INDEX_2D(j, i + 1);
					output[counter++] = MESH_INDEX_2D(j, i);
					output[counter++] = MESH_INDEX_2D(j + 1, i);
					output[counter++] = MESH_INDEX_2D(j, i + 1);
				}
		}
	}

	return counter;
}

HRESULT GFSDK_WaveWorks_Quadtree::initGeometry()
{
	SAFE_DELETE(m_pMesh);

	m_lods = 0;

	m_params.mesh_dim = min(max(8, m_params.mesh_dim), 256);


	int mesh_dim = m_params.mesh_dim;
	// Added check for tessellation friendly flag: if we don't use tessellation, 
	// then we don't need to decrease mesh density
	if((m_d3dAPI == nv_water_d3d_api_d3d11 || m_d3dAPI == nv_water_d3d_api_gnm) && (m_params.use_tessellation == true))
	{
		m_params.mesh_dim = min(max(32, m_params.mesh_dim), 256);
		mesh_dim = m_params.mesh_dim / 4;
	}

	for (int i = mesh_dim; i > 1; i >>= 1)
		m_lods ++;


	int num_vert = (mesh_dim + 1) * (mesh_dim + 1);

	// --------------------------------- Vertex Buffer -------------------------------
	water_quadtree_vertex* vertex_array = new water_quadtree_vertex[num_vert];

	int i, j;
	for (i = 0; i <= mesh_dim; i++)
	{
		for (j = 0; j <= mesh_dim; j++)
		{
			vertex_array[i * (mesh_dim + 1) + j].index_x = (float)j;
			vertex_array[i * (mesh_dim + 1) + j].index_y = (float)i;
		}
	}

	// --------------------------------- Index Buffer -------------------------------

	// The index numbers for all mesh LODs (up to 256x256)
	const int index_size_lookup[] = {0, 0, 0, 23328, 131544, 596160, 2520072, 10348560, 41930136};

	memset(&m_mesh_patterns[0][0][0][0][0], 0, sizeof(m_mesh_patterns));

	// Generate patch meshes. Each patch contains two parts: the inner mesh which is a regular
	// grids in a triangle list. The boundary mesh is constructed w.r.t. the edge degrees to
	// meet water-tight requirement.
	DWORD* index_array = new DWORD[index_size_lookup[m_lods]]; 
	int offset = 0;
	int level_size = mesh_dim;

	// Enumerate patterns
	for (int level = 0; level <= m_lods - 3; level ++)
	{
		int left_degree = level_size;

		for (int left_type = 0; left_type < 3; left_type ++)
		{
			int right_degree = level_size;

			for (int right_type = 0; right_type < 3; right_type ++)
			{
				int bottom_degree = level_size;

				for (int bottom_type = 0; bottom_type < 3; bottom_type ++)
				{
					int top_degree = level_size;

					for (int top_type = 0; top_type < 3; top_type ++)
					{
						QuadRenderParam* pattern = &m_mesh_patterns[level][left_type][right_type][bottom_type][top_type];

						// Inner mesh (triangle list) 
						RECT inner_rect;
						inner_rect.left   = left_type;
						inner_rect.right  = level_size - right_type;
						inner_rect.bottom = bottom_type;
						inner_rect.top    = level_size - top_type;

						int num_new_indices = generateInnerMesh(inner_rect, mesh_dim, !m_params.use_tessellation, index_array + offset);

						pattern->inner_start_index = offset;
						pattern->num_inner_faces = num_new_indices / 3;
						offset += num_new_indices;

						// Boundary mesh (triangle list)
						int l_degree = (left_degree   == level_size) ? 0 : left_degree;
						int r_degree = (right_degree  == level_size) ? 0 : right_degree;
						int b_degree = (bottom_degree == level_size) ? 0 : bottom_degree;
						int t_degree = (top_degree    == level_size) ? 0 : top_degree;

						RECT outer_rect = {0, level_size, level_size, 0};
						num_new_indices = generateBoundaryMesh(l_degree, r_degree, b_degree, t_degree, outer_rect, mesh_dim, index_array + offset);

						pattern->boundary_start_index = offset;
						pattern->num_boundary_faces = num_new_indices / 3;
						offset += num_new_indices;

						top_degree /= 2;
					}
					bottom_degree /= 2;
				}
				right_degree /= 2;
			}
			left_degree /= 2;
		}
		level_size /= 2;
	}

	assert(offset == index_size_lookup[m_lods]);

#if WAVEWORKS_ENABLE_GRAPHICS
	// --------------------------------- Initialise mesh -------------------------------
	HRESULT hr;
	switch(m_d3dAPI)
	{
#if WAVEWORKS_ENABLE_D3D9
	case nv_water_d3d_api_d3d9:
		{
			const D3DVERTEXELEMENT9 grid_decl[] =
			{
				{0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
				D3DDECL_END()
			};

			V_RETURN(NVWaveWorks_Mesh::CreateD3D9(m_d3d._9.m_pd3d9Device, grid_decl, sizeof(vertex_array[0]), vertex_array, num_vert, index_array, index_size_lookup[m_lods], &m_pMesh));
		}
		break;
#endif
#if WAVEWORKS_ENABLE_D3D10
	case nv_water_d3d_api_d3d10:
		{
			const D3D10_INPUT_ELEMENT_DESC grid_layout[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 }
			};
			const UINT num_layout_elements = sizeof(grid_layout)/sizeof(grid_layout[0]);

			
			V_RETURN(NVWaveWorks_Mesh::CreateD3D10(	m_d3d._10.m_pd3d10Device,
													grid_layout, num_layout_elements,
													SM4::g_GFSDK_WAVEWORKS_VERTEX_INPUT_Sig, sizeof(SM4::g_GFSDK_WAVEWORKS_VERTEX_INPUT_Sig),
													sizeof(vertex_array[0]), vertex_array, num_vert,
													index_array, index_size_lookup[m_lods],
													&m_pMesh
													));
		}
		break;
#endif
#if WAVEWORKS_ENABLE_D3D11
	case nv_water_d3d_api_d3d11:
		{
			const D3D11_INPUT_ELEMENT_DESC grid_layout[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
			};
			const UINT num_layout_elements = sizeof(grid_layout)/sizeof(grid_layout[0]);

			
			V_RETURN(NVWaveWorks_Mesh::CreateD3D11(	m_d3d._11.m_pd3d11Device,
													grid_layout, num_layout_elements,
													SM5::g_GFSDK_WAVEWORKS_VERTEX_INPUT_Sig, sizeof(SM5::g_GFSDK_WAVEWORKS_VERTEX_INPUT_Sig),
													sizeof(vertex_array[0]), vertex_array, num_vert,
													index_array, index_size_lookup[m_lods],
													&m_pMesh
													));
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			V_RETURN(NVWaveWorks_Mesh::CreateGnm(
				sizeof(vertex_array[0]), vertex_array, num_vert,
				index_array, index_size_lookup[m_lods],
				&m_pMesh
				));
		}
		break;
#endif
#if WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_gl2:
		{
			const NVWaveWorks_Mesh::GL_VERTEX_ATTRIBUTE_DESC attribute_descs[] = 
				{
					{2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0}	// vPos
				};

			V_RETURN(NVWaveWorks_Mesh::CreateGL2(	attribute_descs,
													sizeof(attribute_descs)/sizeof(attribute_descs[0]),
													2*sizeof(GLfloat), vertex_array, num_vert,
													index_array, index_size_lookup[m_lods],
													&m_pMesh
													));
													
		}
		break;
#endif
	default:
		// Unexpected API
		return E_FAIL;
	};
	assert(m_pMesh);
#endif // WAVEWORKS_ENABLE_GRAPHICS

	SAFE_DELETE_ARRAY(vertex_array);
	SAFE_DELETE_ARRAY(index_array);

	return S_OK;
}

bool checkNodeVisibility(const QuadNode& quad_node, const gfsdk_float4x4& matView, const gfsdk_float4x4& matProj, float sea_level, float margin)
{
	// Transform corners to clip space and building bounding box
	gfsdk_float4 bbox_verts[8];
	gfsdk_float4 bbox_verts_transformed[8];
	bbox_verts[0] = gfsdk_make_float4(quad_node.bottom_left.x - margin, quad_node.bottom_left.y - margin, sea_level - margin, 1);
	bbox_verts[1] = bbox_verts[0] + gfsdk_make_float4(quad_node.length + 2.0f * margin, 0, 0, 0);
	bbox_verts[2] = bbox_verts[0] + gfsdk_make_float4(quad_node.length + 2.0f * margin, quad_node.length + 2.0f * margin, 0, 0);
	bbox_verts[3] = bbox_verts[0] + gfsdk_make_float4(0, quad_node.length + 2.0f * margin, 0, 0);

	bbox_verts[4] = bbox_verts[0] + gfsdk_make_float4(0, 0, margin * 2.0f, 0);
	bbox_verts[5] = bbox_verts[1] + gfsdk_make_float4(0, 0, margin * 2.0f, 0);
	bbox_verts[6] = bbox_verts[2] + gfsdk_make_float4(0, 0, margin * 2.0f, 0);
	bbox_verts[7] = bbox_verts[3] + gfsdk_make_float4(0, 0, margin * 2.0f, 0);


	gfsdk_float4x4 mat_view_proj;
	mat4Mat4Mul(mat_view_proj,matProj,matView);

	vec4Mat4Mul(bbox_verts_transformed[0], bbox_verts[0], mat_view_proj);
	vec4Mat4Mul(bbox_verts_transformed[1], bbox_verts[1], mat_view_proj);
	vec4Mat4Mul(bbox_verts_transformed[2], bbox_verts[2], mat_view_proj);
	vec4Mat4Mul(bbox_verts_transformed[3], bbox_verts[3], mat_view_proj);
	vec4Mat4Mul(bbox_verts_transformed[4], bbox_verts[4], mat_view_proj);
	vec4Mat4Mul(bbox_verts_transformed[5], bbox_verts[5], mat_view_proj);
	vec4Mat4Mul(bbox_verts_transformed[6], bbox_verts[6], mat_view_proj);
	vec4Mat4Mul(bbox_verts_transformed[7], bbox_verts[7], mat_view_proj);

	if (bbox_verts_transformed[0].x < -bbox_verts_transformed[0].w && bbox_verts_transformed[1].x < -bbox_verts_transformed[1].w && bbox_verts_transformed[2].x < -bbox_verts_transformed[2].w && bbox_verts_transformed[3].x < -bbox_verts_transformed[3].w &&
		bbox_verts_transformed[4].x < -bbox_verts_transformed[4].w && bbox_verts_transformed[5].x < -bbox_verts_transformed[5].w && bbox_verts_transformed[6].x < -bbox_verts_transformed[6].w && bbox_verts_transformed[7].x < -bbox_verts_transformed[7].w)
		return false;

	if (bbox_verts_transformed[0].x > bbox_verts_transformed[0].w && bbox_verts_transformed[1].x > bbox_verts_transformed[1].w && bbox_verts_transformed[2].x > bbox_verts_transformed[2].w && bbox_verts_transformed[3].x > bbox_verts_transformed[3].w &&
		bbox_verts_transformed[4].x > bbox_verts_transformed[4].w && bbox_verts_transformed[5].x > bbox_verts_transformed[5].w && bbox_verts_transformed[6].x > bbox_verts_transformed[6].w && bbox_verts_transformed[7].x > bbox_verts_transformed[7].w)
		return false;

	if (bbox_verts_transformed[0].y < -bbox_verts_transformed[0].w && bbox_verts_transformed[1].y < -bbox_verts_transformed[1].w && bbox_verts_transformed[2].y < -bbox_verts_transformed[2].w && bbox_verts_transformed[3].y < -bbox_verts_transformed[3].w &&
		bbox_verts_transformed[4].y < -bbox_verts_transformed[4].w && bbox_verts_transformed[5].y < -bbox_verts_transformed[5].w && bbox_verts_transformed[6].y < -bbox_verts_transformed[6].w && bbox_verts_transformed[7].y < -bbox_verts_transformed[7].w)
		return false;

	if (bbox_verts_transformed[0].y > bbox_verts_transformed[0].w && bbox_verts_transformed[1].y > bbox_verts_transformed[1].w && bbox_verts_transformed[2].y > bbox_verts_transformed[2].w && bbox_verts_transformed[3].y > bbox_verts_transformed[3].w &&
		bbox_verts_transformed[4].y > bbox_verts_transformed[4].w && bbox_verts_transformed[5].y > bbox_verts_transformed[5].w && bbox_verts_transformed[6].y > bbox_verts_transformed[6].w && bbox_verts_transformed[7].y > bbox_verts_transformed[7].w)
		return false;

	if (bbox_verts_transformed[0].z < 0.f && bbox_verts_transformed[1].z < 0.f && bbox_verts_transformed[2].z < 0.f && bbox_verts_transformed[3].z < 0.f &&
		bbox_verts_transformed[4].z < 0.f && bbox_verts_transformed[5].z < 0.f && bbox_verts_transformed[6].z < 0.f && bbox_verts_transformed[7].z < 0.f)
		return false;

	if (bbox_verts_transformed[0].z > bbox_verts_transformed[0].w && bbox_verts_transformed[1].z > bbox_verts_transformed[1].w && bbox_verts_transformed[2].z > bbox_verts_transformed[2].w && bbox_verts_transformed[3].z > bbox_verts_transformed[3].w &&
		bbox_verts_transformed[4].z > bbox_verts_transformed[4].w && bbox_verts_transformed[5].z > bbox_verts_transformed[5].w && bbox_verts_transformed[6].z > bbox_verts_transformed[6].w && bbox_verts_transformed[7].z > bbox_verts_transformed[7].w)
		return false;

	return true;
}

float estimateGridCoverage(	const QuadNode& quad_node,
							const GFSDK_WaveWorks_Quadtree_Params& quad_tree_param,
							const gfsdk_float4x4& matProj,
							float screen_area,
							const gfsdk_float3& eye_point
							)
{
	// Estimate projected area

	// Test 16 points on the quad and find out the biggest one.
	const static float sample_pos[16][2] =
	{
		{0, 0},
		{0, 1},
		{1, 0},
		{1, 1},
		{0.5f, 0.333f},
		{0.25f, 0.667f},
		{0.75f, 0.111f},
		{0.125f, 0.444f},
		{0.625f, 0.778f},
		{0.375f, 0.222f},
		{0.875f, 0.556f},
		{0.0625f, 0.889f},
		{0.5625f, 0.037f},
		{0.3125f, 0.37f},
		{0.8125f, 0.704f},
		{0.1875f, 0.148f},
	};

	float grid_len_world = quad_node.length / quad_tree_param.mesh_dim;

	float max_area_proj = 0;
	for (int i = 0; i < 16; i++)
	{
		gfsdk_float3 test_point = gfsdk_make_float3(quad_node.bottom_left.x + quad_node.length * sample_pos[i][0], quad_node.bottom_left.y + quad_node.length * sample_pos[i][1], quad_tree_param.sea_level);
		gfsdk_float3 eye_vec = test_point - eye_point;
		float dist = length(eye_vec);

		float area_world = grid_len_world * grid_len_world;// * abs(eye_point.z) / sqrt(nearest_sqr_dist);
		float area_proj = area_world * matProj._11 * matProj._22 / (dist * dist);

		if (max_area_proj < area_proj)
			max_area_proj = area_proj;
	}

	float pixel_coverage = max_area_proj * screen_area * 0.25f;

	return pixel_coverage;
}

bool isLeaf(const QuadNode& quad_node)
{
	return (quad_node.sub_node[0] < 0 && quad_node.sub_node[1] < 0 && quad_node.sub_node[2] < 0 && quad_node.sub_node[3] < 0);
}

int searchLeaf(const std::vector<QuadNode>& root_node_list, const std::vector<QuadNode>& node_list, const gfsdk_float2& point)
{
	int index = -1;

	QuadNode node;

	bool foundRoot = false;
	const std::vector<QuadNode>::const_iterator rootEndIt = root_node_list.end();
	for(std::vector<QuadNode>::const_iterator it = root_node_list.begin(); it != rootEndIt; ++it)
	{
		if (point.x >= it->bottom_left.x && point.x <= it->bottom_left.x + it->length &&
			point.y >= it->bottom_left.y && point.y <= it->bottom_left.y + it->length)
		{
			node = *it;
			foundRoot = true;
			break;
		}
	}

	if(!foundRoot)
		return -1;

	while (!isLeaf(node))
	{
		bool found = false;

		for (int i = 0; i < 4; i++)
		{
			index = node.sub_node[i];
			if (index < 0)
				continue;

			QuadNode sub_node = node_list[index];
			if (point.x >= sub_node.bottom_left.x && point.x <= sub_node.bottom_left.x + sub_node.length &&
				point.y >= sub_node.bottom_left.y && point.y <= sub_node.bottom_left.y + sub_node.length)
			{
				assert(node.length > sub_node.length);
				node = sub_node;
				found = true;
				break;
			}
		}

		if (!found)
			return -1;
	}

	return index;
}

GFSDK_WaveWorks_Quadtree::QuadRenderParam& GFSDK_WaveWorks_Quadtree::selectMeshPattern(const QuadNode& quad_node)
{
	// Check 4 adjacent quad.
	gfsdk_float2 point_left = quad_node.bottom_left + gfsdk_make_float2(-m_params.min_patch_length * 0.5f, quad_node.length * 0.5f);
	int left_adj_index = searchLeaf(m_render_roots_list, m_unsorted_render_list, point_left);

	gfsdk_float2 point_right = quad_node.bottom_left + gfsdk_make_float2(quad_node.length + m_params.min_patch_length * 0.5f, quad_node.length * 0.5f);
	int right_adj_index = searchLeaf(m_render_roots_list, m_unsorted_render_list, point_right);

	gfsdk_float2 point_bottom = quad_node.bottom_left + gfsdk_make_float2(quad_node.length * 0.5f, -m_params.min_patch_length * 0.5f);
	int bottom_adj_index = searchLeaf(m_render_roots_list, m_unsorted_render_list, point_bottom);

	gfsdk_float2 point_top = quad_node.bottom_left + gfsdk_make_float2(quad_node.length * 0.5f, quad_node.length + m_params.min_patch_length * 0.5f);
	int top_adj_index = searchLeaf(m_render_roots_list, m_unsorted_render_list, point_top);

	int left_type = 0;
	if (left_adj_index >= 0 && m_unsorted_render_list[left_adj_index].length > quad_node.length * 0.999f)
	{
		QuadNode adj_node = m_unsorted_render_list[left_adj_index];
		float scale = adj_node.length / quad_node.length * (m_params.mesh_dim >> quad_node.lod) / (m_params.mesh_dim >> adj_node.lod);
		if (scale > 3.999f)
			left_type = 2;
		else if (scale > 1.999f)
			left_type = 1;
	}

	int right_type = 0;
	if (right_adj_index >= 0 && m_unsorted_render_list[right_adj_index].length > quad_node.length * 0.999f)
	{
		QuadNode adj_node = m_unsorted_render_list[right_adj_index];
		float scale = adj_node.length / quad_node.length * (m_params.mesh_dim >> quad_node.lod) / (m_params.mesh_dim >> adj_node.lod);
		if (scale > 3.999f)
			right_type = 2;
		else if (scale > 1.999f)
			right_type = 1;
	}

	int bottom_type = 0;
	if (bottom_adj_index >= 0 && m_unsorted_render_list[bottom_adj_index].length > quad_node.length * 0.999f)
	{
		QuadNode adj_node = m_unsorted_render_list[bottom_adj_index];
		float scale = adj_node.length / quad_node.length * (m_params.mesh_dim >> quad_node.lod) / (m_params.mesh_dim >> adj_node.lod);
		if (scale > 3.999f)
			bottom_type = 2;
		else if (scale > 1.999f)
			bottom_type = 1;
	}

	int top_type = 0;
	if (top_adj_index >= 0 && m_unsorted_render_list[top_adj_index].length > quad_node.length * 0.999f)
	{
		QuadNode adj_node = m_unsorted_render_list[top_adj_index];
		float scale = adj_node.length / quad_node.length * (m_params.mesh_dim >> quad_node.lod) / (m_params.mesh_dim >> adj_node.lod);
		if (scale > 3.999f)
			top_type = 2;
		else if (scale > 1.999f)
			top_type = 1;
	}

	// Check lookup table, [L][R][B][T]
	return m_mesh_patterns[quad_node.lod][left_type][right_type][bottom_type][top_type];
}

// Return value: if successful pushed into the list, return the position. If failed, return -1.
int GFSDK_WaveWorks_Quadtree::buildNodeList(	QuadNode& quad_node,
												FLOAT NumPixelsInViewport,
												const gfsdk_float4x4& matView,
												const gfsdk_float4x4& matProj,
												const gfsdk_float3& eyePoint,
												const QuadCoord* quad_coords
												)
{
	// Check if the node is disabled
	if(quad_coords)
	{
		typedef std::vector<AllocQuad>::iterator it_type;
		const it_type endIt = m_allocated_patches_list.end();
		const AllocQuad dummy_quad = { *quad_coords, TRUE };
		const std::pair<it_type, it_type> er = std::equal_range(m_allocated_patches_list.begin(), endIt, dummy_quad);
		if(er.first != er.second)
		{
			if(!er.first->enabled)
				return -2;
		}
	}

	// Check against view frustum
	if (!checkNodeVisibility(quad_node, matView, matProj, m_params.sea_level, frustum_cull_margin))
		return -1;

	// Estimate the min grid coverage
	float min_coverage = estimateGridCoverage(quad_node, m_params, matProj, NumPixelsInViewport, eyePoint);
	float geomorphing_degree = max(0.f,min(m_params.geomorphing_degree,1.f));

	// Recursively attatch sub-nodes.
	bool visible = true;
	if (min_coverage > m_params.upper_grid_coverage && quad_node.length > m_params.min_patch_length)
	{
		QuadCoord sub_quad_coords[4];
		QuadCoord* sub_quad_coords_0 = NULL;
		QuadCoord* sub_quad_coords_1 = NULL;
		QuadCoord* sub_quad_coords_2 = NULL;
		QuadCoord* sub_quad_coords_3 = NULL;
		if(quad_coords)
		{
			sub_quad_coords[0].x = 2 * quad_coords->x;
			sub_quad_coords[0].y = 2 * quad_coords->y;
			sub_quad_coords[0].lod = quad_coords->lod - 1;
			sub_quad_coords_0 = &sub_quad_coords[0];

			sub_quad_coords[1].x = sub_quad_coords[0].x + 1;
			sub_quad_coords[1].y = sub_quad_coords[0].y;
			sub_quad_coords[1].lod = sub_quad_coords[0].lod;
			sub_quad_coords_1 = &sub_quad_coords[1];

			sub_quad_coords[2].x = sub_quad_coords[0].x + 1;
			sub_quad_coords[2].y = sub_quad_coords[0].y + 1;
			sub_quad_coords[2].lod = sub_quad_coords[0].lod;
			sub_quad_coords_2 = &sub_quad_coords[2];

			sub_quad_coords[3].x = sub_quad_coords[0].x;
			sub_quad_coords[3].y = sub_quad_coords[0].y + 1;
			sub_quad_coords[3].lod = sub_quad_coords[0].lod;
			sub_quad_coords_3 = &sub_quad_coords[3];
		}

		// Flip the morph sign on each change of level
		const FLOAT sub_morph_sign = -1.f * quad_node.morph_sign;

		// Recursive rendering for sub-quads.
		QuadNode sub_node_0 = {quad_node.bottom_left, quad_node.length / 2, 0, {-1, -1, -1, -1}, sub_morph_sign};
		quad_node.sub_node[0] = buildNodeList(sub_node_0, NumPixelsInViewport, matView, matProj, eyePoint, sub_quad_coords_0);

		QuadNode sub_node_1 = {quad_node.bottom_left + gfsdk_make_float2(quad_node.length/2, 0), quad_node.length / 2, 0, {-1, -1, -1, -1}, sub_morph_sign};
		quad_node.sub_node[1] = buildNodeList(sub_node_1, NumPixelsInViewport, matView, matProj, eyePoint, sub_quad_coords_1);

		QuadNode sub_node_2 = {quad_node.bottom_left + gfsdk_make_float2(quad_node.length/2, quad_node.length/2), quad_node.length / 2, 0, {-1, -1, -1, -1}, sub_morph_sign};
		quad_node.sub_node[2] = buildNodeList(sub_node_2, NumPixelsInViewport, matView, matProj, eyePoint, sub_quad_coords_2);

		QuadNode sub_node_3 = {quad_node.bottom_left + gfsdk_make_float2(0, quad_node.length/2), quad_node.length / 2, 0, {-1, -1, -1, -1}, sub_morph_sign};
		quad_node.sub_node[3] = buildNodeList(sub_node_3, NumPixelsInViewport, matView, matProj, eyePoint, sub_quad_coords_3);

		// If all the sub-nodes are invisible, then we need to revise our original assessment
		// that the current node was visible
		visible = !isLeaf(quad_node);
	}

	if (visible)
	{
		// Estimate mesh LOD - we don't use 1x1, 2x2 or 4x4 patch. So the highest level is m_lods - 3.
		int lod = 0;
		for (lod = 0; lod < m_lods - 3; lod++)
		{
			if (min_coverage > m_params.upper_grid_coverage)
				break;
			quad_node.morph_sign *= -1.f;
			min_coverage *= 4;
		}

		quad_node.lod = lod;
	}
	else
		return -1;

	// Insert into the list
	int position = (int)m_unsorted_render_list.size();
	m_unsorted_render_list.push_back(quad_node);

	return position;
}

HRESULT GFSDK_WaveWorks_Quadtree::flushRenderList(	Graphics_Context* pGC,
													const UINT* GFX_ONLY(pShaderInputRegisterMappings),
													GFSDK_WaveWorks_Savestate* pSavestateImpl
													)
{
	HRESULT hr;

	// Zero counters
	m_stats.num_patches_drawn = 0;

#if WAVEWORKS_ENABLE_D3D11
	// Fetch DC, if D3D11
	ID3D11DeviceContext* pDC_d3d11 = NULL;
	if(nv_water_d3d_api_d3d11 == m_d3dAPI)
	{
		pDC_d3d11 = pGC->d3d11();
	}
#endif

#if WAVEWORKS_ENABLE_GNM
	// Fetch Gnmx ctx, if gnm
	sce::Gnmx::LightweightGfxContext* gfxContext_gnm = NULL;
	if(nv_water_d3d_api_gnm == m_d3dAPI)
	{
		gfxContext_gnm = pGC->gnm();
	}
#endif

	// Preserve state, if necessary
	if(m_sorted_render_list.size() && NULL != pSavestateImpl)
	{
		V_RETURN(m_pMesh->PreserveState(pGC, pSavestateImpl));

#if WAVEWORKS_ENABLE_GRAPHICS
		switch(m_d3dAPI)
		{
#if WAVEWORKS_ENABLE_D3D9
		case nv_water_d3d_api_d3d9:
			{
				const UINT rm_g_matLocalWorld = pShaderInputRegisterMappings[ShaderInputD3D9_g_matLocalWorld];
				const UINT rm_g_vsEyePos = pShaderInputRegisterMappings[ShaderInputD3D9_g_vsEyePos];
				const UINT rm_g_MorphParam = pShaderInputRegisterMappings[ShaderInputD3D9_g_MorphParam];
				if(rm_g_matLocalWorld != nvrm_unused)
					V_RETURN(pSavestateImpl->PreserveD3D9VertexShaderConstantF(rm_g_matLocalWorld, 3));
				if(rm_g_vsEyePos != nvrm_unused)
					V_RETURN(pSavestateImpl->PreserveD3D9VertexShaderConstantF(rm_g_vsEyePos, 1));
				if(rm_g_MorphParam != nvrm_unused)
					V_RETURN(pSavestateImpl->PreserveD3D9VertexShaderConstantF(rm_g_MorphParam, 1));
			}
			break;
#endif
#if WAVEWORKS_ENABLE_D3D10
		case nv_water_d3d_api_d3d10:
			{
				const UINT reg = pShaderInputRegisterMappings[ShaderInputD3D10_vs_buffer];
				if(reg != nvrm_unused)
				{
					V_RETURN(pSavestateImpl->PreserveD3D10PixelShaderConstantBuffer(reg));
				}
			}
			break;
#endif
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			{
				const UINT reg = pShaderInputRegisterMappings[ShaderInputD3D10_vs_buffer];
				if(reg != nvrm_unused)
				{
					V_RETURN(pSavestateImpl->PreserveD3D11PixelShaderConstantBuffer(pDC_d3d11, reg));
				}
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				// no savestate implementation in GL
			}
			break;
#endif
		default:
			break;
		}
#endif // WAVEWORKS_ENABLE_GRAPHICS
	}

#if WAVEWORKS_ENABLE_GNM
	GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();
	gnmxWrap->pushMarker(*gfxContext_gnm, "GFSDK_WaveWorks_Quadtree::flushRenderList");
#endif

	// We assume the center of the water surface is at (0, 0, 0).
	for (int i = 0; i < (int)m_sorted_render_list.size(); i++)
	{
		QuadNode& node = m_sorted_render_list[i];

		if (!isLeaf(node))
			continue;

		// Check adjacent patches and select mesh pattern
		QuadRenderParam& render_param = selectMeshPattern(node);

		// Find the right LOD to render
		int level_size = m_params.mesh_dim >> node.lod;

		gfsdk_float4x4 matLocalWorld;
		setIdentity(matLocalWorld);
		matLocalWorld._11 = node.length / level_size;
		matLocalWorld._22 = node.length / level_size;
		matLocalWorld._33 = 0;
		matLocalWorld._14 = node.bottom_left.x;
		matLocalWorld._24 = node.bottom_left.y;
		matLocalWorld._34 = m_params.sea_level;

		NVWaveWorks_Mesh::PrimitiveType prim_type = NVWaveWorks_Mesh::PT_TriangleList;
		if(m_d3dAPI == nv_water_d3d_api_d3d11 || m_d3dAPI == nv_water_d3d_api_gnm)
		{
			if(m_params.use_tessellation)
			{
				prim_type = NVWaveWorks_Mesh::PT_PatchList_3;
				// decrease mesh density when using tessellation
				matLocalWorld._11 *= 4.0f;
				matLocalWorld._22 *= 4.0f;
			}
		}

		UINT* pMeshShaderInputMapping = NULL;

#if WAVEWORKS_ENABLE_GRAPHICS
		gfsdk_float4 eyePos = gfsdk_make_float4(m_eyePos[0],m_eyePos[1],m_eyePos[2],1.f);

#if WAVEWORKS_ENABLE_GL
		UINT meshShaderInputMapping = (UINT)nvrm_unused;
#endif

		const FLOAT morph_distance_constant = m_geomorphCoeff * float(level_size) / node.length;
		gfsdk_float4 morphParam = gfsdk_make_float4(morph_distance_constant,0.f,0.f,node.morph_sign);
		switch(m_d3dAPI)
		{
#if WAVEWORKS_ENABLE_D3D9
		case nv_water_d3d_api_d3d9:
			{
				UINT rm_g_matLocalWorld = pShaderInputRegisterMappings[ShaderInputD3D9_g_matLocalWorld];
				UINT rm_g_vsEyePos = pShaderInputRegisterMappings[ShaderInputD3D9_g_vsEyePos];
				UINT rm_g_MorphParam = pShaderInputRegisterMappings[ShaderInputD3D9_g_MorphParam];
				if(rm_g_matLocalWorld != nvrm_unused)
					V_RETURN(m_d3d._9.m_pd3d9Device->SetVertexShaderConstantF(rm_g_matLocalWorld, &matLocalWorld._11, 3));
				if(rm_g_vsEyePos != nvrm_unused)
					V_RETURN(m_d3d._9.m_pd3d9Device->SetVertexShaderConstantF(rm_g_vsEyePos, &eyePos.x, 1));
				if(rm_g_MorphParam != nvrm_unused)
					V_RETURN(m_d3d._9.m_pd3d9Device->SetVertexShaderConstantF(rm_g_MorphParam, &morphParam.x, 1));
			}
			break;
#endif
#if WAVEWORKS_ENABLE_D3D10
		case nv_water_d3d_api_d3d10:
			{
				const UINT reg = pShaderInputRegisterMappings[ShaderInputD3D10_vs_buffer];
				if(reg != nvrm_unused)
				{
					vs_cbuffer VSCB;
					memcpy(&VSCB.g_matLocalWorld, &matLocalWorld, sizeof(VSCB.g_matLocalWorld));
					memcpy(&VSCB.g_vsEyePos, &eyePos, sizeof(VSCB.g_vsEyePos));
					memcpy(&VSCB.g_MorphParam, &morphParam, sizeof(VSCB.g_MorphParam));
					m_d3d._10.m_pd3d10Device->UpdateSubresource(m_d3d._10.m_pd3d10VertexShaderCB, 0, NULL, &VSCB, 0, 0);
					m_d3d._10.m_pd3d10Device->VSSetConstantBuffers(reg, 1, &m_d3d._10.m_pd3d10VertexShaderCB);

				}
			}
			break;
#endif
#if WAVEWORKS_ENABLE_D3D11
		case nv_water_d3d_api_d3d11:
			{
				const UINT regvs = pShaderInputRegisterMappings[ShaderInputD3D11_vs_buffer];
				if(regvs != nvrm_unused)
				{
					{
						D3D11_CB_Updater<vs_cbuffer> cbu(pDC_d3d11,m_d3d._11.m_pd3d11VertexShaderCB);
						memcpy(&cbu.cb().g_matLocalWorld, &matLocalWorld, sizeof(cbu.cb().g_matLocalWorld));
						memcpy(&cbu.cb().g_vsEyePos, &eyePos, sizeof(cbu.cb().g_vsEyePos));
						memcpy(&cbu.cb().g_MorphParam, &morphParam, sizeof(cbu.cb().g_MorphParam));
					}
					pDC_d3d11->VSSetConstantBuffers(regvs, 1, &m_d3d._11.m_pd3d11VertexShaderCB);
				}
				const UINT reghs = pShaderInputRegisterMappings[ShaderInputD3D11_hs_buffer];
				if(reghs != nvrm_unused)
				{
					{
						D3D11_CB_Updater<hs_cbuffer> cbu(pDC_d3d11,m_d3d._11.m_pd3d11HullShaderCB);
						memcpy(&cbu.cb().g_eyePos, &m_eyePos, sizeof(m_eyePos));
						memset(&cbu.cb().g_tessellationParams,0,sizeof(cbu.cb().g_tessellationParams));
						memcpy(&cbu.cb().g_tessellationParams, &m_params.tessellation_lod, sizeof(m_params.tessellation_lod));
					}
					pDC_d3d11->HSSetConstantBuffers(reghs, 1, &m_d3d._11.m_pd3d11HullShaderCB);
				}
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GNM
		case nv_water_d3d_api_gnm:
			{
				const UINT regvs = pShaderInputRegisterMappings[ShaderInputD3D11_vs_buffer];
				if(regvs != nvrm_unused)
				{
					vs_cbuffer* pVSCB = (vs_cbuffer*)gnmxWrap->allocateFromCommandBuffer(*gfxContext_gnm, sizeof(vs_cbuffer), Gnm::kEmbeddedDataAlignment4);
					memcpy(&pVSCB->g_matLocalWorld, &matLocalWorld, sizeof(pVSCB->g_matLocalWorld));
					memcpy(&pVSCB->g_vsEyePos, &eyePos, sizeof(pVSCB->g_vsEyePos));
					memcpy(&pVSCB->g_MorphParam, &morphParam, sizeof(pVSCB->g_MorphParam));

					Gnm::Buffer buffer;
					buffer.initAsConstantBuffer(pVSCB, sizeof(vs_cbuffer));
					buffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO);
					gnmxWrap->setConstantBuffers(*gfxContext_gnm, m_params.use_tessellation ? Gnm::kShaderStageLs : Gnm::kShaderStageVs, regvs, 1, &buffer);
				}
				const UINT reghs = pShaderInputRegisterMappings[ShaderInputD3D11_hs_buffer];
				if(reghs != nvrm_unused)
				{
					hs_cbuffer* pHSCB = (hs_cbuffer*)gnmxWrap->allocateFromCommandBuffer(*gfxContext_gnm, sizeof(vs_cbuffer), Gnm::kEmbeddedDataAlignment4);
					memcpy(&pHSCB->g_eyePos, &m_eyePos, sizeof(m_eyePos));
					memset(&pHSCB->g_tessellationParams,0,sizeof(pHSCB->g_tessellationParams));
					memcpy(&pHSCB->g_tessellationParams, &m_params.tessellation_lod, sizeof(m_params.tessellation_lod));

					Gnm::Buffer buffer;
					buffer.initAsConstantBuffer(pHSCB, sizeof(hs_cbuffer));
					buffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO);
					gnmxWrap->setConstantBuffers(*gfxContext_gnm, Gnm::kShaderStageHs, reghs, 1, &buffer);
				}
			}
			break;
#endif
#if WAVEWORKS_ENABLE_GL
		case nv_water_d3d_api_gl2:
			{
				const UINT rm_g_matLocalWorld = pShaderInputRegisterMappings[ShaderInputGL2_g_matLocalWorld];
				const UINT rm_g_vsEyePos = pShaderInputRegisterMappings[ShaderInputGL2_g_vsEyePos];
				const UINT rm_g_MorphParam = pShaderInputRegisterMappings[ShaderInputGL2_g_MorphParam];
				const UINT rm_attr_vPos = pShaderInputRegisterMappings[ShaderInputGL2_attr_vPos];
				
				GLfloat mlv[12];
				mlv[0] = matLocalWorld._11;
				mlv[1] = matLocalWorld._12;
				mlv[2] = matLocalWorld._13;
				mlv[3] = matLocalWorld._14;
				mlv[4] = matLocalWorld._21;
				mlv[5] = matLocalWorld._22;
				mlv[6] = matLocalWorld._23;
				mlv[7] = matLocalWorld._24;
				mlv[8] = matLocalWorld._31;
				mlv[9] = matLocalWorld._32;
				mlv[10]= matLocalWorld._33;
				mlv[11]= matLocalWorld._34;
				
				if(rm_g_matLocalWorld != nvrm_unused)
					NVSDK_GLFunctions.glUniformMatrix3x4fv(rm_g_matLocalWorld, 1, GL_FALSE, (GLfloat*)mlv); CHECK_GL_ERRORS;
				if(rm_g_vsEyePos != nvrm_unused)
					NVSDK_GLFunctions.glUniform4fv(rm_g_vsEyePos, 1, (GLfloat*)&(eyePos.x)); CHECK_GL_ERRORS;
				if(rm_g_MorphParam != nvrm_unused)
					NVSDK_GLFunctions.glUniform4fv(rm_g_MorphParam, 1, (GLfloat*)&(morphParam.x)); CHECK_GL_ERRORS;
				if(rm_attr_vPos != nvrm_unused) {
					meshShaderInputMapping = rm_attr_vPos;
					pMeshShaderInputMapping = &meshShaderInputMapping;
				}
			}
			break;
#endif
		default:
			// Unexpected API
			return E_FAIL;
		}
#endif // WAVEWORKS_ENABLE_GRAPHICS

		// Render
		int mesh_dim = m_params.mesh_dim;	
		int num_vert = (mesh_dim + 1) * (mesh_dim + 1);
		if (render_param.num_inner_faces > 0)
		{
			V_RETURN(m_pMesh->Draw(pGC, prim_type, 0, 0, num_vert, render_param.inner_start_index, render_param.num_inner_faces, pMeshShaderInputMapping));
		}
		if (render_param.num_boundary_faces > 0)
		{
			V_RETURN(m_pMesh->Draw(pGC, prim_type, 0, 0, num_vert, render_param.boundary_start_index, render_param.num_boundary_faces, pMeshShaderInputMapping));
		}
		++m_stats.num_patches_drawn;
	}

#if WAVEWORKS_ENABLE_GNM
	gnmxWrap->popMarker(*gfxContext_gnm);
#endif

	return S_OK;
}

void GFSDK_WaveWorks_Quadtree::sortRenderList()
{
	m_sorted_render_list = m_unsorted_render_list;
	std::sort(m_sorted_render_list.begin(), m_sorted_render_list.end(), compareQuadNodeLength);
}

HRESULT GFSDK_WaveWorks_Quadtree::buildRenderList(	Graphics_Context* pGC,
													const gfsdk_float4x4& matView,
													const gfsdk_float4x4& matProj,
													const gfsdk_float2* GNM_ONLY(pViewportDims)
													)
{
	HRESULT hr;

	FLOAT viewportW = 0;
	FLOAT viewportH = 0;

	TickType tStart, tStop;

	if(m_params.enable_CPU_timers)
	{
		// tying thread to core #0 to ensure OS doesn't reallocathe thread to other cores which might corrupt QueryPerformanceCounter readings
		GFSDK_WaveWorks_Simulation_Util::tieThreadToCore(0);
		// getting the timestamp
		tStart = GFSDK_WaveWorks_Simulation_Util::getTicks();
	}

#if WAVEWORKS_ENABLE_GRAPHICS
	switch(m_d3dAPI)
	{
#if WAVEWORKS_ENABLE_D3D9
	case nv_water_d3d_api_d3d9:
		{
			D3DVIEWPORT9 vp;
			V_RETURN(m_d3d._9.m_pd3d9Device->GetViewport(&vp));
			viewportW = FLOAT(vp.Width);
			viewportH = FLOAT(vp.Height);
			break;
		}
#endif
#if WAVEWORKS_ENABLE_D3D10
	case nv_water_d3d_api_d3d10:
		{
			D3D10_VIEWPORT vp;
			UINT NumViewports = 1;
			m_d3d._10.m_pd3d10Device->RSGetViewports(&NumViewports,&vp);
			viewportW = FLOAT(vp.Width);
			viewportH = FLOAT(vp.Height);
			break;
		}
#endif
#if WAVEWORKS_ENABLE_D3D11
	case nv_water_d3d_api_d3d11:
		{
			ID3D11DeviceContext* pDC_d3d11 = pGC->d3d11();

			D3D11_VIEWPORT vp;
			UINT NumViewports = 1;
			pDC_d3d11->RSGetViewports(&NumViewports,&vp);
			viewportW = vp.Width;
			viewportH = vp.Height;

			break;
		}
#endif
#if WAVEWORKS_ENABLE_GNM
	case nv_water_d3d_api_gnm:
		{
			assert(pViewportDims);
			viewportW = pViewportDims->x;
			viewportH = pViewportDims->y;

			break;
		}
#endif
#if WAVEWORKS_ENABLE_GL
	case nv_water_d3d_api_gl2:
		{
			GLint vp[4];
			NVSDK_GLFunctions.glGetIntegerv( GL_VIEWPORT, vp);
			viewportW = (FLOAT)vp[2];
			viewportH = (FLOAT)vp[3];
			break;
		}
#endif
	default:
		// Unexpected API
		return E_FAIL;
	}
#endif // WAVEWORKS_ENABLE_GRAPHICS

	// Compute eye point
	gfsdk_float4x4 inv_mat_view;
	gfsdk_float4 vec_original = {0,0,0,1};
	gfsdk_float4 vec_transformed;
	mat4Inverse(inv_mat_view, matView);
	vec4Mat4Mul(vec_transformed, vec_original, inv_mat_view);
	gfsdk_float3 eyePoint = gfsdk_make_float3(vec_transformed.x,vec_transformed.y,vec_transformed.z);
	m_eyePos[0] = vec_transformed.x;
	m_eyePos[1] = vec_transformed.y;
	m_eyePos[2] = vec_transformed.z;

	// Compute geomorphing coefficient
	const FLOAT geomorphing_degree = max(0.f,min(m_params.geomorphing_degree,1.f));
	m_geomorphCoeff = geomorphing_degree * 2.f * sqrtf(m_params.upper_grid_coverage/(matProj._11 * matProj._22 * viewportW * viewportH));

	if(m_allocated_patches_list.empty())
	{
		V_RETURN(buildRenderListAuto(matView,matProj,eyePoint,viewportW,viewportH));
	}
	else
	{
		V_RETURN(buildRenderListExplicit(matView,matProj,eyePoint,viewportW,viewportH));
	}

	// Sort the resulting list front-to-back
	sortRenderList();

	if(m_params.enable_CPU_timers)
	{
		// getting the timestamp and calculating time
		tStop = GFSDK_WaveWorks_Simulation_Util::getTicks();
		m_stats.CPU_quadtree_update_time = GFSDK_WaveWorks_Simulation_Util::getMilliseconds(tStart,tStop);
	}
	else
	{
		m_stats.CPU_quadtree_update_time = 0;
	}

	return S_OK;
}

HRESULT GFSDK_WaveWorks_Quadtree::buildRenderListAuto(	const gfsdk_float4x4& matView,
														const gfsdk_float4x4& matProj,
														const gfsdk_float3& eyePoint,
														FLOAT viewportW,
														FLOAT viewportH
														)
{
	// Centre the top-level node on the nearest largest-patch boundary
	const float patch_length = m_params.min_patch_length; 
	const float root_patch_length = patch_length * float(0x00000001 << m_params.auto_root_lod);
	const float centreX = root_patch_length * floor(eyePoint.x/root_patch_length + 0.5f);
	const float centreY = root_patch_length * floor(eyePoint.y/root_patch_length + 0.5f);

	// Build rendering list
	m_unsorted_render_list.clear();
	m_render_roots_list.clear();
	QuadNode root_node00 = {gfsdk_make_float2(centreX, centreY), root_patch_length, 0, {-1,-1,-1,-1}, 1.f};
	QuadNode root_node01 = {gfsdk_make_float2(centreX, centreY - root_patch_length), root_patch_length, 0, {-1,-1,-1,-1}, 1.f};
	QuadNode root_node10 = {gfsdk_make_float2(centreX - root_patch_length, centreY), root_patch_length, 0, {-1,-1,-1,-1}, 1.f};
	QuadNode root_node11 = {gfsdk_make_float2(centreX - root_patch_length, centreY - root_patch_length), root_patch_length, 0, {-1,-1,-1,-1}, 1.f};

	if(buildNodeList(root_node00, viewportW * viewportH, matView, matProj, eyePoint, NULL) >= 0)
		m_render_roots_list.push_back(root_node00);
	if(buildNodeList(root_node01, viewportW * viewportH, matView, matProj, eyePoint, NULL) >= 0)
		m_render_roots_list.push_back(root_node01);
	if(buildNodeList(root_node10, viewportW * viewportH, matView, matProj, eyePoint, NULL) >= 0)
		m_render_roots_list.push_back(root_node10);
	if(buildNodeList(root_node11, viewportW * viewportH, matView, matProj, eyePoint, NULL) >= 0)
		m_render_roots_list.push_back(root_node11);
		
	return S_OK;

}

HRESULT GFSDK_WaveWorks_Quadtree::buildRenderListExplicit(	const gfsdk_float4x4& matView,
															const gfsdk_float4x4& matProj,
															const gfsdk_float3& eyePoint,
															FLOAT viewportW,
															FLOAT viewportH
															)
{
	assert(!m_allocated_patches_list.empty());

	m_unsorted_render_list.clear();
	m_render_roots_list.clear();

	// Use the first lod as the root lod
	const UINT root_lod = m_allocated_patches_list.front().coords.lod;
	const float root_patch_length = m_params.min_patch_length * float(0x00000001 << root_lod);
	const std::vector<AllocQuad>::const_iterator endIt = m_allocated_patches_list.end();
	for(std::vector<AllocQuad>::const_iterator it = m_allocated_patches_list.begin(); it != endIt; ++it)
	{
		// Stop when we encounter the first non-root lod
		if(root_lod != it->coords.lod)
			break;

		const gfsdk_float2 patch_offset = gfsdk_make_float2(root_patch_length * float(it->coords.x), root_patch_length * float(it->coords.y));
		QuadNode root_node = {m_params.patch_origin + patch_offset, root_patch_length, 0, {-1,-1,-1,-1}, 1.f};
		const int ix = buildNodeList(root_node, viewportW * viewportH, matView, matProj, eyePoint, &(it->coords));
		if(ix >= 0)
			m_render_roots_list.push_back(root_node);
	}

	return S_OK;
}

HRESULT GFSDK_WaveWorks_Quadtree::getShaderInputCountD3D9()
{
	return NumShaderInputsD3D9;
}

HRESULT GFSDK_WaveWorks_Quadtree::getShaderInputCountD3D10()
{
	return NumShaderInputsD3D10;
}

HRESULT GFSDK_WaveWorks_Quadtree::getShaderInputCountD3D11()
{
	return NumShaderInputsD3D11;
}

HRESULT GFSDK_WaveWorks_Quadtree::getShaderInputCountGnm()
{
	return NumShaderInputsGnm;
}

HRESULT GFSDK_WaveWorks_Quadtree::getShaderInputCountGL2()
{
	return NumShaderInputsGL2;
}

HRESULT GFSDK_WaveWorks_Quadtree::getShaderInputDescD3D9(UINT D3D9_ONLY(inputIndex), GFSDK_WaveWorks_ShaderInput_Desc* D3D9_ONLY(pDesc))
{
#if WAVEWORKS_ENABLE_D3D9
	if(inputIndex >= NumShaderInputsD3D9)
		return E_FAIL;

	*pDesc = ShaderInputD3D9Descs[inputIndex];

	return S_OK;
#else // WAVEWORKS_ENABLE_D3D9
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Quadtree::getShaderInputDescD3D10(UINT D3D10_ONLY(inputIndex), GFSDK_WaveWorks_ShaderInput_Desc* D3D10_ONLY(pDesc))
{
#if WAVEWORKS_ENABLE_D3D10
	if(inputIndex >= NumShaderInputsD3D10)
		return E_FAIL;

	*pDesc = ShaderInputD3D10Descs[inputIndex];

	return S_OK;
#else // WAVEWORKS_ENABLE_D3D10
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Quadtree::getShaderInputDescD3D11(UINT D3D11_ONLY(inputIndex), GFSDK_WaveWorks_ShaderInput_Desc* D3D11_ONLY(pDesc))
{
#if WAVEWORKS_ENABLE_D3D11
	if(inputIndex >= NumShaderInputsD3D11)
		return E_FAIL;

	*pDesc = ShaderInputD3D11Descs[inputIndex];

	return S_OK;
#else // WAVEWORKS_ENABLE_D3D11
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Quadtree::getShaderInputDescGnm(UINT GNM_ONLY(inputIndex), GFSDK_WaveWorks_ShaderInput_Desc* GNM_ONLY(pDesc))
{
#if WAVEWORKS_ENABLE_GNM
	if(inputIndex >= NumShaderInputsGnm)
		return E_FAIL;

	*pDesc = ShaderInputGnmDescs[inputIndex];

	return S_OK;
#else // WAVEWORKS_ENABLE_GNM
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Quadtree::getShaderInputDescGL2(UINT GL_ONLY(inputIndex), GFSDK_WaveWorks_ShaderInput_Desc* GL_ONLY(pDesc))
{
#if WAVEWORKS_ENABLE_GL
	if(inputIndex >= NumShaderInputsGL2)
		return E_FAIL;

	*pDesc = ShaderInputGL2Descs[inputIndex];

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT GFSDK_WaveWorks_Quadtree::allocPatch(INT x, INT y, UINT lod, BOOL enabled)
{
	const AllocQuad quad = { {x,y,lod}, enabled };

	typedef std::vector<AllocQuad>::iterator it_type;
	const it_type endIt = m_allocated_patches_list.end();
	const std::pair<it_type, it_type> er = std::equal_range(m_allocated_patches_list.begin(), endIt, quad);
	if(er.first != er.second)
	{
		// Already in the list - that's an error
		return E_FAIL;
	}

	m_allocated_patches_list.insert(er.first, quad);
	return S_OK;
}

HRESULT GFSDK_WaveWorks_Quadtree::freePatch(INT x, INT y, UINT lod)
{
	const AllocQuad dummy_quad = { {x,y,lod}, TRUE };

	typedef std::vector<AllocQuad>::iterator it_type;
	const it_type endIt = m_allocated_patches_list.end();
	const std::pair<it_type, it_type> er = std::equal_range(m_allocated_patches_list.begin(), endIt, dummy_quad);
	if(er.first == er.second)
	{
		// Not in the list - that's an error
		return E_FAIL;
	}

	m_allocated_patches_list.erase(er.first);
	return S_OK;
}

HRESULT GFSDK_WaveWorks_Quadtree::getStats(GFSDK_WaveWorks_Quadtree_Stats& stats) const
{
	stats = m_stats;
	return S_OK;
}

HRESULT GFSDK_WaveWorks_Quadtree::setFrustumCullMargin( float margin)
{
	frustum_cull_margin = margin;
	return S_OK;
}
