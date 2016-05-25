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

#ifndef _NVWAVEWORKS_QUADTREE_IMPL_H
#define _NVWAVEWORKS_QUADTREE_IMPL_H

#include <vector>

#if WAVEWORKS_ENABLE_GNM
#include <gnm/buffer.h>
struct vs_cbuffer;
struct hs_cbuffer;
#endif

// Fwd. decls
struct IDirect3DDevice9;
struct ID3D10Device;
class NVWaveWorks_Mesh;
struct QuadNode;

struct GFSDK_WaveWorks_Quadtree
{
public:
	GFSDK_WaveWorks_Quadtree();
	~GFSDK_WaveWorks_Quadtree();

    HRESULT initD3D11(const GFSDK_WaveWorks_Quadtree_Params& param, ID3D11Device* pD3DDevice);
	HRESULT initGnm(const GFSDK_WaveWorks_Quadtree_Params& param);
	HRESULT initGL2(const GFSDK_WaveWorks_Quadtree_Params& param, GLuint Program);


	// API-independent init
	HRESULT reinit(const GFSDK_WaveWorks_Quadtree_Params& param);

	HRESULT setFrustumCullMargin (float margin);

	HRESULT buildRenderList(	Graphics_Context* pGC,
								const gfsdk_float4x4& matView,
								const gfsdk_float4x4& matProj,
								const gfsdk_float2* pViewportDims
								);

	HRESULT flushRenderList(	Graphics_Context* pGC,
								const UINT* pShaderInputRegisterMappings,
								GFSDK_WaveWorks_Savestate* pSavestateImpl
								);

	HRESULT allocPatch(INT x, INT y, UINT lod, BOOL enabled);
	HRESULT freePatch(INT x, INT y, UINT lod);

	HRESULT getStats(GFSDK_WaveWorks_Quadtree_Stats& stats) const;

    static HRESULT getShaderInputCountD3D11();
	static HRESULT getShaderInputDescD3D11(UINT inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc);
    static HRESULT getShaderInputCountGnm();
	static HRESULT getShaderInputDescGnm(UINT inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc);
    static HRESULT getShaderInputCountGL2();
	static HRESULT getShaderInputDescGL2(UINT inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc);

private:

	// create a triangle strip mesh for water surface.
	HRESULT initGeometry();

	GFSDK_WaveWorks_Quadtree_Params m_params;

	NVWaveWorks_Mesh* m_pMesh;

	// Quad-tree LOD, 0 to 9 (1x1 ~ 256x256) 
	int m_lods;

	float m_eyePos[4];

	float m_geomorphCoeff;

	// Margin for frustum culling routines
	float frustum_cull_margin;
	
	struct QuadRenderParam
	{
		UINT num_inner_faces;
		UINT inner_start_index;

		UINT num_boundary_faces;
		UINT boundary_start_index;
	};

	// Pattern lookup array. Filled at init time.
	QuadRenderParam m_mesh_patterns[9][3][3][3][3];
	// Pick a proper mesh pattern according to the adjacent patches.
	QuadRenderParam& selectMeshPattern(const QuadNode& quad_node);

	// List of allocated patches
	struct QuadCoord
	{
		int x;
		int y;
		UINT lod;

		bool operator<(const QuadCoord& rhs) const;
	};

	struct AllocQuad
	{
		QuadCoord coords;
		BOOL enabled;

		bool operator<(const AllocQuad& rhs) const;
	};

	std::vector<AllocQuad> m_allocated_patches_list;

	// Rendering list
	std::vector<QuadNode> m_unsorted_render_list;
	int buildNodeList(	QuadNode& quad_node,
						FLOAT NumPixelsInViewport,
						const gfsdk_float4x4& matView,
						const gfsdk_float4x4& matProj,
						const gfsdk_float3& eyePoint,
						const QuadCoord* quad_coords
						);

	HRESULT buildRenderListAuto(	const gfsdk_float4x4& matView,
									const gfsdk_float4x4& matProj,
									const gfsdk_float3& eyePoint,
									FLOAT viewportW,
									FLOAT viewportH
									);

	HRESULT buildRenderListExplicit(	const gfsdk_float4x4& matView,
										const gfsdk_float4x4& matProj,
										const gfsdk_float3& eyePoint,
										FLOAT viewportW,
										FLOAT viewportH
										);

	std::vector<QuadNode> m_render_roots_list;

	// We sort the render list approx front to back, in order to maximise any depth-rejection benefits
	std::vector<QuadNode> m_sorted_render_list;
	void sortRenderList();

	// Stats
	GFSDK_WaveWorks_Quadtree_Stats m_stats;

	// D3D API handling
	nv_water_d3d_api m_d3dAPI;

#if WAVEWORKS_ENABLE_D3D11
	struct D3D11Objects
	{
		ID3D11Device* m_pd3d11Device;
		ID3D11Buffer* m_pd3d11VertexShaderCB;
		ID3D11Buffer* m_pd3d11HullShaderCB;
	};
#endif

#if WAVEWORKS_ENABLE_GNM
	struct GnmObjects
	{
	};
#endif
#if WAVEWORKS_ENABLE_GL
	struct GL2Objects
	{
		GLuint m_pGL2QuadtreeProgram;
		GLuint m_pGL2UniformLocations[3];
	};
#endif
	union
	{
#if WAVEWORKS_ENABLE_D3D11
		D3D11Objects _11;
#endif
#if WAVEWORKS_ENABLE_GNM
		GnmObjects _gnm;
#endif
#if WAVEWORKS_ENABLE_GL
		GL2Objects _GL2;
#endif
	} m_d3d;

	void releaseD3DObjects();
	HRESULT allocateD3DObjects();

};

#endif	// _NVWAVEWORKS_QUADTREE_IMPL_H
