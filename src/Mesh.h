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

#ifndef _NVWAVEWORKS_MESH_H
#define _NVWAVEWORKS_MESH_H

struct IDirect3DDevice9;
struct ID3D10Device;
struct D3D10_INPUT_ELEMENT_DESC;
struct D3D11_INPUT_ELEMENT_DESC;

typedef struct _D3DVERTEXELEMENT9 D3DVERTEXELEMENT9;

// forward declaration
namespace sce
{
	namespace Gnmx
	{
		class LightweightGfxContext;
	}
}

////////////////////////////////////////////////////////////////////////////////
// ABC representing a mesh that can be locked, populated and rendered, plus
// factory classes for DX9 + DX10
////////////////////////////////////////////////////////////////////////////////
class NVWaveWorks_Mesh
{
public:

	typedef struct
	{
		GLint Size;
		GLenum Type;
		GLboolean Normalized;
		GLsizei Stride;
		GLint Offset;
	} GL_VERTEX_ATTRIBUTE_DESC;

	static HRESULT CreateD3D11(	ID3D11Device* pD3DDev,
								const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
								UINT NumElements,
								const void *pShaderBytecodeWithInputSignature,
								SIZE_T BytecodeLength,
								UINT VertexStride,
								const void* pVertData,
								UINT NumVerts,
								const DWORD* pIndexData,
								UINT NumIndices,
								NVWaveWorks_Mesh** ppMesh
								);

	static HRESULT CreateGnm(	UINT VertexStride,
								const void* pVertData,
								UINT NumVerts,
								const DWORD* pIndexData,
								UINT NumIndices,
								NVWaveWorks_Mesh** ppMesh
								);
	static HRESULT CreateGL2(   const GL_VERTEX_ATTRIBUTE_DESC* AttributeDescs,
								UINT NumAttributeDescs,
								GLuint VertexStride,
								const void* pVertData,
								UINT NumVerts,
								const DWORD* pIndexData,
								UINT NumIndices,
								NVWaveWorks_Mesh** ppMesh
								);
	enum PrimitiveType
	{
		PT_TriangleStrip = 0,
		PT_TriangleList,
		PT_PatchList_3
	};

	virtual ~NVWaveWorks_Mesh() {}

	virtual HRESULT Draw(	Graphics_Context* pGC,
							PrimitiveType PrimType,
							INT BaseVertexIndex,
							UINT MinIndex,
							UINT NumVertices,
							UINT StartIndex,
							UINT PrimitiveCount,
							const UINT* pShaderInputMappings
							) = 0;

	virtual HRESULT PreserveState(Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl) = 0;
};

#endif	// _NVWAVEWORKS_MESH_H
