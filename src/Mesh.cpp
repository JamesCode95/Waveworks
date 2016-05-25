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
#include "Mesh.h"
#include "Savestate_impl.h"
#include "Graphics_Context.h"

#if WAVEWORKS_ENABLE_GNM
#include "orbis\GNM_Util.h"
#include <gnm\buffer.h>
#endif
using namespace sce;

#if WAVEWORKS_ENABLE_D3D11
////////////////////////////////////////////////////////////////////////////////
// D3D11 implementation
////////////////////////////////////////////////////////////////////////////////
class NVWaveWorks_MeshD3D11 : public NVWaveWorks_Mesh
{
public:

	~NVWaveWorks_MeshD3D11();

	virtual HRESULT Draw(	Graphics_Context* pGC,
							PrimitiveType PrimType,
							INT BaseVertexIndex,
							UINT MinIndex,
							UINT NumVertices,
							UINT StartIndex,
							UINT PrimitiveCount,
							const UINT* pShaderInputMappings
							);

	virtual HRESULT PreserveState(Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl);

private:

	friend class NVWaveWorks_Mesh;	// For creation
	NVWaveWorks_MeshD3D11(	ID3D11Device* pD3DDevice,
							ID3D11InputLayout* pLayout,
							ID3D11Buffer* pVertexBuffer,
							ID3D11Buffer* pIndexBuffer,
							UINT VertexStride
							);

	ID3D11Device* m_pd3dDevice;
	ID3D11InputLayout* m_pLayout;
	ID3D11Buffer* m_pVB;
	ID3D11Buffer* m_pIB;
	UINT m_VertexStride;

	// Revoked copy/assign
	NVWaveWorks_MeshD3D11(const NVWaveWorks_MeshD3D11&);
	NVWaveWorks_MeshD3D11& operator=(const NVWaveWorks_MeshD3D11&);
};
#endif
#if WAVEWORKS_ENABLE_GNM
////////////////////////////////////////////////////////////////////////////////
// Gnm implementation
////////////////////////////////////////////////////////////////////////////////
class NVWaveWorks_MeshGnm : public NVWaveWorks_Mesh
{
public:

	~NVWaveWorks_MeshGnm();

	virtual HRESULT Draw(	Graphics_Context* pGC,
		PrimitiveType PrimType,
		INT BaseVertexIndex,
		UINT MinIndex,
		UINT NumVertices,
		UINT StartIndex,
		UINT PrimitiveCount,
		const UINT* pShaderInputMappings
		);

	virtual HRESULT PreserveState(Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl);

private:

	friend class NVWaveWorks_Mesh;	// For creation
	NVWaveWorks_MeshGnm(const Gnm::Buffer& positionBuffer,
		const Gnm::Buffer& texcoordBuffer,
		const DWORD* pIndices,
		UINT numIndices,
		UINT VertexStride
		);

	Gnm::Buffer m_positionBuffer;
	Gnm::Buffer m_texcoordBuffer;
	const DWORD* m_pIndices;
	UINT m_numIndices;
	UINT m_VertexStride;

	// Revoked copy/assign
	NVWaveWorks_MeshGnm(const NVWaveWorks_MeshGnm&);
	NVWaveWorks_MeshGnm& operator=(const NVWaveWorks_MeshGnm&);
};
#endif
#if WAVEWORKS_ENABLE_GL
////////////////////////////////////////////////////////////////////////////////
// OPENGL implementation
////////////////////////////////////////////////////////////////////////////////
class NVWaveWorks_MeshGL2 : public NVWaveWorks_Mesh
{
public:

	~NVWaveWorks_MeshGL2();

	virtual HRESULT Draw(	Graphics_Context* pGC,
							PrimitiveType PrimType,
							INT BaseVertexIndex,
							UINT MinIndex,
							UINT NumVertices,
							UINT StartIndex,
							UINT PrimitiveCount,
							const UINT* pShaderInputMappings
							);

	virtual HRESULT PreserveState(Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl);

private:

	friend class NVWaveWorks_Mesh;	// For creation
	NVWaveWorks_MeshGL2(	const GL_VERTEX_ATTRIBUTE_DESC* AttributeDescs,
							UINT NumAttributeDescs,
							GLuint vb,
							GLuint ib
							);

	GLuint						m_VB;
	GLuint						m_IB;

	GL_VERTEX_ATTRIBUTE_DESC*	m_pVertexAttribDescs;
	GLuint						m_NumVertexAttribs;

	// Revoked copy/assign
	NVWaveWorks_MeshGL2(const NVWaveWorks_MeshGL2&);
	NVWaveWorks_MeshGL2& operator=(const NVWaveWorks_MeshGL2&);
};
#endif

HRESULT NVWaveWorks_Mesh::CreateD3D11(	ID3D11Device* D3D11_ONLY(pD3DDev),
										const D3D11_INPUT_ELEMENT_DESC * D3D11_ONLY(pInputElementDescs),
										UINT D3D11_ONLY(NumElements),
										const void * D3D11_ONLY(pShaderBytecodeWithInputSignature),
										SIZE_T D3D11_ONLY(BytecodeLength),
										UINT D3D11_ONLY(VertexStride),
										const void* D3D11_ONLY(pVertData),
										UINT D3D11_ONLY(NumVerts),
										const DWORD* D3D11_ONLY(pIndexData),
										UINT D3D11_ONLY(NumIndices),
										NVWaveWorks_Mesh** D3D11_ONLY(ppMesh)
										)
{
#if WAVEWORKS_ENABLE_D3D11
	HRESULT hr;

	ID3D11InputLayout* pLayout = NULL;
	V_RETURN(pD3DDev->CreateInputLayout(pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, &pLayout));

	ID3D11Buffer* pVB = NULL;
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = NumVerts * VertexStride;
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vSrd;
	vSrd.pSysMem = pVertData;
	vSrd.SysMemPitch = 0;
	vSrd.SysMemSlicePitch = 0;

	V_RETURN(pD3DDev->CreateBuffer(&vbDesc, &vSrd, &pVB));

	ID3D11Buffer* pIB = NULL;
	D3D11_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = NumIndices * sizeof(DWORD);
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;
	ibDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA iSrd;
	iSrd.pSysMem = pIndexData;
	iSrd.SysMemPitch = 0;
	iSrd.SysMemSlicePitch = 0;

	V_RETURN(pD3DDev->CreateBuffer(&ibDesc, &iSrd, &pIB));

	*ppMesh = new NVWaveWorks_MeshD3D11(pD3DDev, pLayout, pVB, pIB, VertexStride);

	pLayout->Release();
	pVB->Release();
	pIB->Release();

	return S_OK;
#else
	return E_FAIL;
#endif
}

HRESULT NVWaveWorks_Mesh::CreateGnm(UINT GNM_ONLY(VertexStride),
									const void* GNM_ONLY(pVertData),
									UINT GNM_ONLY(NumVerts),
									const DWORD* GNM_ONLY(pIndexData),
									UINT GNM_ONLY(NumIndices),
									NVWaveWorks_Mesh** GNM_ONLY(ppMesh)
									)
{
#if WAVEWORKS_ENABLE_GNM
	// todo: pass in data format instead
	assert(VertexStride == 8 || VertexStride == 20);

	char* buffer = (char*)NVSDK_garlic_malloc(NumVerts * VertexStride, Gnm::kAlignmentOfBufferInBytes);
	memcpy(buffer, pVertData, NumVerts * VertexStride);

	Gnm::DataFormat dataFormat = Gnm::kDataFormatR32G32Float;

	Gnm::Buffer texcoordBuffer;
	if(VertexStride == 20)
	{
		texcoordBuffer.initAsVertexBuffer(buffer + 12, dataFormat, VertexStride, NumVerts);
		texcoordBuffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // it's a vertex buffer, so read-only is OK
		dataFormat = Gnm::kDataFormatR32G32B32Float;
	}

	Gnm::Buffer positionBuffer;
	positionBuffer.initAsVertexBuffer(buffer, dataFormat, VertexStride, NumVerts);
	positionBuffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // it's a vertex buffer, so read-only is OK

	DWORD* indices = (DWORD*)NVSDK_garlic_malloc(NumIndices * sizeof(DWORD), Gnm::kAlignmentOfBufferInBytes);
	memcpy(indices, pIndexData, NumIndices * sizeof(DWORD));

	*ppMesh = new NVWaveWorks_MeshGnm(positionBuffer, texcoordBuffer, indices, NumIndices, VertexStride);

	return S_OK;
#else
	return S_FALSE;
#endif
}

#if WAVEWORKS_ENABLE_D3D11
NVWaveWorks_MeshD3D11::~NVWaveWorks_MeshD3D11()
{
	m_pd3dDevice->Release();
	m_pLayout->Release();
	m_pVB->Release();
	m_pIB->Release();
}

HRESULT NVWaveWorks_MeshD3D11::PreserveState(Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl)
{
	HRESULT hr;

	ID3D11DeviceContext* pDC_d3d11 = pGC->d3d11();
	V_RETURN(pSavestateImpl->PreserveD3D11Streams(pDC_d3d11));

	return S_OK;
}

HRESULT NVWaveWorks_MeshD3D11::Draw(	Graphics_Context* pGC,
										PrimitiveType PrimType,
										INT BaseVertexIndex,
										UINT /*MinIndex*/,
										UINT /*NumVertices*/,
										UINT StartIndex,
										UINT PrimitiveCount,
										const UINT* /* not used: pShaderInputMappings*/
										)
{
	HRESULT hr;

	ID3D11DeviceContext* pDC_d3d11 = pGC->d3d11();

	const UINT VBOffset = 0;
    pDC_d3d11->IASetVertexBuffers(0, 1, &m_pVB, &m_VertexStride, &VBOffset);
    pDC_d3d11->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R32_UINT, 0);
	pDC_d3d11->IASetInputLayout(m_pLayout);

	D3D11_PRIMITIVE_TOPOLOGY d3dPrimTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	UINT IndexCount = 0;
	switch(PrimType)
	{
	case PT_TriangleStrip:
		d3dPrimTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		IndexCount = 2 + PrimitiveCount;
		break;
	case PT_TriangleList:
		d3dPrimTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		IndexCount = 3 * PrimitiveCount;
		break;
	case PT_PatchList_3:
		d3dPrimTopology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
		IndexCount = 3 * PrimitiveCount;
		break;
	}

	if(d3dPrimTopology != D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
	{
		pDC_d3d11->IASetPrimitiveTopology(d3dPrimTopology);
		pDC_d3d11->DrawIndexed(IndexCount, StartIndex, BaseVertexIndex);
		hr = S_OK;
	}
	else
	{
		hr = E_FAIL;
	}

	return hr;
}

NVWaveWorks_MeshD3D11::NVWaveWorks_MeshD3D11(	ID3D11Device* pD3DDevice,
												ID3D11InputLayout* pLayout,
												ID3D11Buffer* pVertexBuffer,
												ID3D11Buffer* pIndexBuffer,
												UINT VertexStride
												) :
	m_pd3dDevice(pD3DDevice),
	m_pLayout(pLayout),
	m_pVB(pVertexBuffer),
	m_pIB(pIndexBuffer),
	m_VertexStride(VertexStride)
{
	m_pd3dDevice->AddRef();
	m_pLayout->AddRef();
	m_pVB->AddRef();
	m_pIB->AddRef();
}
#endif

#if WAVEWORKS_ENABLE_GNM
NVWaveWorks_MeshGnm::~NVWaveWorks_MeshGnm()
{
	NVSDK_garlic_free(m_positionBuffer.getBaseAddress());
	NVSDK_garlic_free((void*)m_pIndices);
}

HRESULT NVWaveWorks_MeshGnm::PreserveState(Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl)
{
	return S_OK;
}

HRESULT NVWaveWorks_MeshGnm::Draw(Graphics_Context* pGC,
								  PrimitiveType PrimType,
								  INT BaseVertexIndex,
								  UINT /*MinIndex*/,
								  UINT /*NumVertices*/,
								  UINT StartIndex,
								  UINT PrimitiveCount,
								  const UINT* /* not used: pShaderInputMappings*/
								  )
{
	HRESULT hr;

	sce::Gnmx::LightweightGfxContext* gfxContext = pGC->gnm();

	Gnm::PrimitiveType primitiveType = Gnm::kPrimitiveTypeNone;
	Gnm::ShaderStage shaderStage = Gnm::kShaderStageVs;
	UINT IndexCount = 0;
	switch(PrimType)
	{
	case PT_TriangleStrip:
		primitiveType = Gnm::kPrimitiveTypeTriStrip;
		IndexCount = 2 + PrimitiveCount;
		break;
	case PT_TriangleList:
		primitiveType = Gnm::kPrimitiveTypeTriList;
		IndexCount = 3 * PrimitiveCount;
		break;
	case PT_PatchList_3:
		primitiveType = Gnm::kPrimitiveTypePatch; 
		shaderStage = Gnm::kShaderStageLs;
		IndexCount = 3 * PrimitiveCount;
		break;
	}

	if(primitiveType != Gnm::kPrimitiveTypeNone)
	{
		GFSDK_WaveWorks_GnmxWrap* gnmxWrap = GFSDK_WaveWorks_GNM_Util::getGnmxWrap();
		gnmxWrap->setVertexBuffers(*gfxContext, shaderStage, 0, 1, &m_positionBuffer);
		gnmxWrap->setVertexBuffers(*gfxContext, shaderStage, 1, 1, &m_texcoordBuffer);
		gnmxWrap->setPrimitiveType(*gfxContext, primitiveType);
		gnmxWrap->setIndexSize(*gfxContext, Gnm::kIndexSize32);
		gnmxWrap->setIndexCount(*gfxContext, m_numIndices);
		gnmxWrap->setIndexOffset(*gfxContext, BaseVertexIndex);
#if 1
		gnmxWrap->setIndexBuffer(*gfxContext, m_pIndices);
		gnmxWrap->drawIndexOffset(*gfxContext, StartIndex, IndexCount);
#else
		gnmxWrap->drawIndex(*gfxContext, IndexCount, m_pIndices + StartIndex);
#endif

		hr = S_OK;
	}
	else
	{
		hr = E_FAIL;
	}

	return hr;
}

NVWaveWorks_MeshGnm::NVWaveWorks_MeshGnm(	const Gnm::Buffer& vertexBuffer,
											const Gnm::Buffer& texcoordBuffer,
											const DWORD* pIndices,
											UINT numIndices,
											UINT VertexStride
											) :
	m_positionBuffer(vertexBuffer),
	m_texcoordBuffer(texcoordBuffer),
	m_pIndices(pIndices),
	m_numIndices(numIndices),
	m_VertexStride(VertexStride)
{
}
#endif
#if WAVEWORKS_ENABLE_GL
HRESULT NVWaveWorks_Mesh::CreateGL2(	const GL_VERTEX_ATTRIBUTE_DESC* AttributeDescs,
										UINT NumAttributeDescs,
										GLuint VertexStride,
										const void* pVertData,
										UINT NumVerts,
										const DWORD* pIndexData,
										UINT NumIndices,
										NVWaveWorks_Mesh** ppMesh
										)
{
	GLuint VB;
	GLuint IB;

	// creating VB/IB and filling with the data
	NVSDK_GLFunctions.glGenBuffers(1,&VB); CHECK_GL_ERRORS;
    NVSDK_GLFunctions.glBindBuffer(GL_ARRAY_BUFFER, VB); CHECK_GL_ERRORS;
	NVSDK_GLFunctions.glBufferData(GL_ARRAY_BUFFER, NumVerts*VertexStride, pVertData, GL_STATIC_DRAW); CHECK_GL_ERRORS;
	NVSDK_GLFunctions.glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_GL_ERRORS;	

	NVSDK_GLFunctions.glGenBuffers(1, &IB);  CHECK_GL_ERRORS;
	NVSDK_GLFunctions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IB); CHECK_GL_ERRORS;
	NVSDK_GLFunctions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, NumIndices * sizeof(DWORD), pIndexData, GL_STATIC_DRAW); CHECK_GL_ERRORS;
	NVSDK_GLFunctions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHECK_GL_ERRORS;

	// creating GL flavor of mesh class instance
	NVWaveWorks_MeshGL2* pMesh = new NVWaveWorks_MeshGL2(AttributeDescs, NumAttributeDescs, VB, IB);

	*ppMesh = pMesh;
	return S_OK; 
}

NVWaveWorks_MeshGL2::NVWaveWorks_MeshGL2(	const GL_VERTEX_ATTRIBUTE_DESC* AttributeDescs,
											UINT NumAttributeDescs,
											GLuint vb,
											GLuint ib
											) :
	m_VB(vb),
	m_IB(ib),
	m_NumVertexAttribs(NumAttributeDescs)
{
	m_pVertexAttribDescs = new GL_VERTEX_ATTRIBUTE_DESC [NumAttributeDescs];
	memcpy(m_pVertexAttribDescs,AttributeDescs,NumAttributeDescs * sizeof(m_pVertexAttribDescs[0]));
}

HRESULT NVWaveWorks_MeshGL2::Draw(	Graphics_Context* /* not used: pGC*/,
									PrimitiveType PrimType,
									INT BaseVertexIndex,
									UINT /* not used: MinIndex*/,
									UINT /* not used: NumVertices*/,
									UINT StartIndex,
									UINT PrimitiveCount,
									const UINT* pShaderInputMappings
									)
{
	// Must supply input mappings if we have attributes to hook up
	if(m_NumVertexAttribs && NULL == pShaderInputMappings)
	{
		return E_FAIL;
	}

	unsigned int IndexCount = 0;
	unsigned char GLPrimTopology = GL_TRIANGLES; 
	switch(PrimType)
	{
		case PT_TriangleStrip:
			GLPrimTopology = GL_TRIANGLE_STRIP;
			IndexCount = 2 + PrimitiveCount;
			break;
		case PT_TriangleList:
			GLPrimTopology = GL_TRIANGLES;
			IndexCount = 3 * PrimitiveCount;
			break;
		case PT_PatchList_3:
			GLPrimTopology = GL_PATCHES;
			NVSDK_GLFunctions.glPatchParameteri(GL_PATCH_VERTICES, 3); CHECK_GL_ERRORS;
			IndexCount = 3 * PrimitiveCount;
			break;
	}

	NVSDK_GLFunctions.glBindBuffer(GL_ARRAY_BUFFER, m_VB); CHECK_GL_ERRORS;
	NVSDK_GLFunctions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IB); CHECK_GL_ERRORS;

	for(GLuint i = 0; i < m_NumVertexAttribs; i++)
	{
		NVSDK_GLFunctions.glEnableVertexAttribArray(pShaderInputMappings[i]); CHECK_GL_ERRORS;
		NVSDK_GLFunctions.glVertexAttribPointer(pShaderInputMappings[i], m_pVertexAttribDescs[i].Size, m_pVertexAttribDescs[i].Type, m_pVertexAttribDescs[i].Normalized,m_pVertexAttribDescs[i].Stride,(const GLvoid *)(m_pVertexAttribDescs[i].Offset + BaseVertexIndex*m_pVertexAttribDescs[i].Stride)); CHECK_GL_ERRORS; 
	}

	NVSDK_GLFunctions.glDrawElements(GLPrimTopology, IndexCount, GL_UNSIGNED_INT, (GLvoid *)(StartIndex * sizeof(GLuint))); CHECK_GL_ERRORS;

	for(GLuint i = 0; i < m_NumVertexAttribs; i++)
	{
		NVSDK_GLFunctions.glDisableVertexAttribArray(pShaderInputMappings[i]); CHECK_GL_ERRORS;
	}

	NVSDK_GLFunctions.glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_GL_ERRORS;
	NVSDK_GLFunctions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHECK_GL_ERRORS;

	return S_OK;
}

HRESULT NVWaveWorks_MeshGL2::PreserveState(Graphics_Context* /* not used: pGC*/, GFSDK_WaveWorks_Savestate* /* not used: pSavestateImpl*/)
{
	// do nothing atm
	return S_OK;
}

NVWaveWorks_MeshGL2::~NVWaveWorks_MeshGL2()
{
	// deleting OpenGL buffers
	if(m_VB != 0) NVSDK_GLFunctions.glDeleteBuffers(1, &m_VB); CHECK_GL_ERRORS;
	if(m_IB != 0) NVSDK_GLFunctions.glDeleteBuffers(1, &m_IB); CHECK_GL_ERRORS;
	
	delete [] m_pVertexAttribDescs;
}
#endif
