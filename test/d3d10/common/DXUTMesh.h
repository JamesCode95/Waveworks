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
// Copyright � 2008- 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
 
 //-----------------------------------------------------------------------------
// File: DXUTMesh.h
//
// Desc: Support code for loading DirectX .X files.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef DXUTMESH_H
#define DXUTMESH_H





//-----------------------------------------------------------------------------
// Name: class CDXUTMesh
// Desc: Class for loading and rendering file-based meshes
//-----------------------------------------------------------------------------
class CDXUTMesh
{
public:
    WCHAR                   m_strName[512];
    LPD3DXMESH              m_pMesh;   // Managed mesh
    
    // Cache of data in m_pMesh for easy access
    IDirect3DVertexBuffer9* m_pVB;
    IDirect3DIndexBuffer9*  m_pIB;
    IDirect3DVertexDeclaration9* m_pDecl;
    DWORD                   m_dwNumVertices;
    DWORD                   m_dwNumFaces;
    DWORD                   m_dwBytesPerVertex;

    DWORD                   m_dwNumMaterials; // Materials for the mesh
    D3DMATERIAL9*           m_pMaterials;
    CHAR                    (*m_strMaterials)[MAX_PATH];
    IDirect3DBaseTexture9** m_pTextures;
    bool                    m_bUseMaterials;

public:
    // Rendering
    HRESULT Render( LPDIRECT3DDEVICE9 pd3dDevice, 
                    bool bDrawOpaqueSubsets = true,
                    bool bDrawAlphaSubsets = true );
    HRESULT Render( ID3DXEffect *pEffect,
                    D3DXHANDLE hTexture = NULL,
                    D3DXHANDLE hDiffuse = NULL,
                    D3DXHANDLE hAmbient = NULL,
                    D3DXHANDLE hSpecular = NULL,
                    D3DXHANDLE hEmissive = NULL,
                    D3DXHANDLE hPower = NULL,
                    bool bDrawOpaqueSubsets = true,
                    bool bDrawAlphaSubsets = true );

    // Mesh access
    LPD3DXMESH GetMesh() { return m_pMesh; }

    // Rendering options
    void    UseMeshMaterials( bool bFlag ) { m_bUseMaterials = bFlag; }
    HRESULT SetFVF( LPDIRECT3DDEVICE9 pd3dDevice, DWORD dwFVF );
    HRESULT SetVertexDecl( LPDIRECT3DDEVICE9 pd3dDevice, const D3DVERTEXELEMENT9 *pDecl, 
                           bool bAutoComputeNormals = true, bool bAutoComputeTangents = true, 
                           bool bSplitVertexForOptimalTangents = false );

    // Initializing
    HRESULT RestoreDeviceObjects( LPDIRECT3DDEVICE9 pd3dDevice );
    HRESULT InvalidateDeviceObjects();

    // Creation/destruction
    HRESULT Create( LPDIRECT3DDEVICE9 pd3dDevice, LPCWSTR strFilename );
    HRESULT Create( LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXFILEDATA pFileData );
    HRESULT Create( LPDIRECT3DDEVICE9 pd3dDevice, ID3DXMesh* pInMesh, D3DXMATERIAL* pd3dxMaterials, DWORD dwMaterials );
    HRESULT CreateMaterials( LPCWSTR strPath, IDirect3DDevice9 *pd3dDevice, D3DXMATERIAL* d3dxMtrls, DWORD dwNumMaterials );
    HRESULT Destroy();

    CDXUTMesh( LPCWSTR strName = L"CDXUTMeshFile_Mesh" );
    virtual ~CDXUTMesh();
};




//-----------------------------------------------------------------------------
// Name: class CDXUTMeshFrame
// Desc: Class for loading and rendering file-based meshes
//-----------------------------------------------------------------------------
class CDXUTMeshFrame
{
public:
    WCHAR      m_strName[512];
    D3DXMATRIX m_mat;
    CDXUTMesh*  m_pMesh;

    CDXUTMeshFrame* m_pNext;
    CDXUTMeshFrame* m_pChild;

public:
    // Matrix access
    void        SetMatrix( D3DXMATRIX* pmat ) { m_mat = *pmat; }
    D3DXMATRIX* GetMatrix()                   { return &m_mat; }

    CDXUTMesh*   FindMesh( LPCWSTR strMeshName );
    CDXUTMeshFrame*  FindFrame( LPCWSTR strFrameName );
    bool        EnumMeshes( bool (*EnumMeshCB)(CDXUTMesh*,void*), 
                            void* pContext );

    HRESULT Destroy();
    HRESULT RestoreDeviceObjects( LPDIRECT3DDEVICE9 pd3dDevice );
    HRESULT InvalidateDeviceObjects();
    HRESULT Render( LPDIRECT3DDEVICE9 pd3dDevice, 
                    bool bDrawOpaqueSubsets = true,
                    bool bDrawAlphaSubsets = true,
                    D3DXMATRIX* pmatWorldMatrix = NULL);

    CDXUTMeshFrame( LPCWSTR strName = L"CDXUTMeshFile_Frame" );
    virtual ~CDXUTMeshFrame();
};




//-----------------------------------------------------------------------------
// Name: class CDXUTMeshFile
// Desc: Class for loading and rendering file-based meshes
//-----------------------------------------------------------------------------
class CDXUTMeshFile : public CDXUTMeshFrame
{
    HRESULT LoadMesh( LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXFILEDATA pFileData, 
                      CDXUTMeshFrame* pParentFrame );
    HRESULT LoadFrame( LPDIRECT3DDEVICE9 pd3dDevice, LPD3DXFILEDATA pFileData, 
                       CDXUTMeshFrame* pParentFrame );
public:
    HRESULT Create( LPDIRECT3DDEVICE9 pd3dDevice, LPCWSTR strFilename );
    HRESULT CreateFromResource( LPDIRECT3DDEVICE9 pd3dDevice, LPCWSTR strResource, LPCWSTR strType );
    // For pure devices, specify the world transform. If the world transform is not
    // specified on pure devices, this function will fail.
    HRESULT Render( LPDIRECT3DDEVICE9 pd3dDevice, D3DXMATRIX* pmatWorldMatrix = NULL );

    CDXUTMeshFile() : CDXUTMeshFrame( L"CDXUTMeshFile_Root" ) {}
};



#endif



