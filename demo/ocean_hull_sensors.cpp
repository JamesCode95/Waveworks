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
#include "DXUTcamera.h"
#include "ocean_hull_sensors.h"
#include "ocean_surface_heights.h"
#include "ocean_vessel.h"

#include "SDKmesh.h"

#include <vector>

#pragma warning(disable:4127)

inline bool operator!=(const D3DVERTEXELEMENT9& lhs, const D3DVERTEXELEMENT9& rhs) {
	return lhs.Stream != rhs.Stream ||
		lhs.Offset != rhs.Offset ||
		lhs.Type != rhs.Type ||
		lhs.Method != rhs.Method ||
		lhs.Usage != rhs.Usage ||
		lhs.UsageIndex != rhs.UsageIndex;
}

namespace {
	template<class T, class F>
	void for_each_triangle(T& functor, const F& filter, BoatMesh* pMesh, const D3DXMATRIX& matMeshToLocal)
	{
		const UINT num_meshes = pMesh->GetNumMeshes();

		for(UINT mesh_ix = 0; mesh_ix != num_meshes; ++mesh_ix)
		{
			SDKMESH_MESH* pSubMesh = pMesh->GetMesh(mesh_ix);

			// Find the positions stream
			BYTE* pVertexPositionBytes = NULL;
			BYTE* pVertexNormalBytes = NULL;
			UINT64 positions_stride = (UINT64)-1;
			UINT64 normals_stride = (UINT64)-1;
			for(UINT vbix = 0; vbix != pSubMesh->NumVertexBuffers && (NULL == pVertexPositionBytes || NULL == pVertexNormalBytes); ++vbix) {
				const SDKMESH_VERTEX_BUFFER_HEADER* pVBHeader = pMesh->getVBHeader(pSubMesh->VertexBuffers[vbix]);
				const D3DVERTEXELEMENT9 d3d9_decl_end = D3DDECL_END();
				for(UINT declix = 0; pVBHeader->Decl[declix] != d3d9_decl_end && (NULL == pVertexPositionBytes || NULL == pVertexNormalBytes); ++declix) {
					const D3DVERTEXELEMENT9& decl_elem = pVBHeader->Decl[declix];
					if(D3DDECLUSAGE_POSITION == decl_elem.Usage && 0 == decl_elem.UsageIndex)
					{
						if(D3DDECLTYPE_FLOAT3 == decl_elem.Type || D3DDECLTYPE_FLOAT4 == decl_elem.Type) {
							BYTE* pVertexBytes = (BYTE*)pMesh->GetRawVerticesAt(pSubMesh->VertexBuffers[vbix]);
							pVertexPositionBytes = pVertexBytes + decl_elem.Offset;
							positions_stride = pVBHeader->StrideBytes;
						}
					} else if(D3DDECLUSAGE_NORMAL == decl_elem.Usage && 0 == decl_elem.UsageIndex)
					{
						if(D3DDECLTYPE_FLOAT3 == decl_elem.Type || D3DDECLTYPE_FLOAT4 == decl_elem.Type) {
							BYTE* pVertexBytes = (BYTE*)pMesh->GetRawVerticesAt(pSubMesh->VertexBuffers[vbix]);
							pVertexNormalBytes = pVertexBytes + decl_elem.Offset;
							normals_stride = pVBHeader->StrideBytes;
						}
					}
				}
			}

			if(pVertexPositionBytes && pVertexNormalBytes) {
				const int index_size_in_bytes = (IT_16BIT == pMesh->GetIndexType(pSubMesh->IndexBuffer)) ? 2 : 4;
				BYTE* pIndexBytes = (BYTE*)pMesh->GetRawIndicesAt(pSubMesh->IndexBuffer);
				for (UINT subset = 0; subset < pMesh->GetNumSubsets(mesh_ix); ++subset)
				{
					SDKMESH_SUBSET* pSubset = pMesh->GetSubset(mesh_ix, subset);
					BYTE* pCurrIndexByte = pIndexBytes + index_size_in_bytes * pSubset->IndexStart;
					BYTE* pEndIndexByte = pCurrIndexByte + index_size_in_bytes * pSubset->IndexCount;
					switch(pSubset->PrimitiveType) {
					case PT_TRIANGLE_LIST:
						while((pCurrIndexByte+2*index_size_in_bytes) < pEndIndexByte) {
							DWORD i0, i1, i2;
							if(index_size_in_bytes == 2) {
								WORD* pCurrIndex = (WORD*)pCurrIndexByte;
								i0 = (DWORD)pSubset->VertexStart + pCurrIndex[0];
								i1 = (DWORD)pSubset->VertexStart + pCurrIndex[1];
								i2 = (DWORD)pSubset->VertexStart + pCurrIndex[2];
							} else {
								DWORD* pCurrIndex = (DWORD*)pCurrIndexByte;
								i0 = (DWORD)pSubset->VertexStart + pCurrIndex[0];
								i1 = (DWORD)pSubset->VertexStart + pCurrIndex[1];
								i2 = (DWORD)pSubset->VertexStart + pCurrIndex[2];
							}

							D3DXVECTOR3 v0,v1,v2;
							D3DXVec3TransformCoord(&v0, (D3DXVECTOR3*)(pVertexPositionBytes + positions_stride * i0), &matMeshToLocal);
							D3DXVec3TransformCoord(&v1, (D3DXVECTOR3*)(pVertexPositionBytes + positions_stride * i1), &matMeshToLocal);
							D3DXVec3TransformCoord(&v2, (D3DXVECTOR3*)(pVertexPositionBytes + positions_stride * i2), &matMeshToLocal);

							D3DXVECTOR3 n0,n1,n2;
							D3DXVec3TransformNormal(&n0, (D3DXVECTOR3*)(pVertexNormalBytes + normals_stride * i0), &matMeshToLocal);
							D3DXVec3TransformNormal(&n1, (D3DXVECTOR3*)(pVertexNormalBytes + normals_stride * i1), &matMeshToLocal);
							D3DXVec3TransformNormal(&n2, (D3DXVECTOR3*)(pVertexNormalBytes + normals_stride * i2), &matMeshToLocal);

							if(filter(v0,v1,v2,n0,n1,n2)) {
								functor(i0,i1,i2,v0,v1,v2,n0,n1,n2);
							}

							pCurrIndexByte += 3 * index_size_in_bytes;
						}
						break;
					case PT_TRIANGLE_STRIP:
						assert(false); // tristrips TBD
						break;
					}
				}
			}
		}
	}

	inline int PoissonOutcomeKnuth(float lambda) {

		// Adapted to log space to avoid exp() overflow for large lambda
		int k = 0;
		float p = 0.f;
		do {
			k = k+1;
			p = p - logf((float)rand()/(float)(RAND_MAX+1));
		}
		while (p < lambda);

		return k-1;
	}

	inline int PoissonOutcome(float lambda) {
		return PoissonOutcomeKnuth(lambda);
	}

	const float kRimVerticalOffset = 0.6f;
}

OceanHullSensors::OceanHullSensors()
{
	m_pd3dDevice = DXUTGetD3D11Device();

	m_NumSensors = 0;
	m_NumRimSensors = 0;

	m_pVisualisationMeshIB = NULL;
	m_VisualisationMeshIndexCount = 0;
	m_VisualisationMeshIndexFormat = DXGI_FORMAT_UNKNOWN;

	m_MeanSensorRadius = 0.f;
	m_MeanRimSensorSeparation = 0.f;
}

OceanHullSensors::~OceanHullSensors()
{
	SAFE_RELEASE(m_pVisualisationMeshIB);
}

namespace
{
	float adhoc_deck_profile(float z)
	{
		if(z < 0.f)  {
			// Straight-line for rear half
			return 1.5f;
		} else {
			// Elliptical upward sweep for front half
			const float major_r = 40.f;
			const float minor_r = 10.f;
			return 2.5f + minor_r - sqrtf(major_r*major_r-z*z) * minor_r/major_r;
		}
	}

	float adhoc_deck_profile_for_rim(float z)
	{
		if(z < 0.f)  {
			// Straight-line for rear half
			return 1.5f;
		} else {
			// Elliptical upward sweep for front half
			const float major_r = 40.f;
			const float minor_r = 10.f;
			return 1.5f + minor_r - sqrtf(major_r*major_r-z*z) * minor_r/major_r;
		}
	}

	D3DXVECTOR3 adhoc_get_deck_intersection(const D3DXVECTOR3& v0,const D3DXVECTOR3& v1)
	{
		float h0 = v0.y - adhoc_deck_profile_for_rim(v0.z);
		float h1 = v1.y - adhoc_deck_profile_for_rim(v1.z);

		float lambda = (0.f-h0)/(h1-h0);

		return lambda * v1 + (1.f - lambda) * v0;
	}

	void adhoc_get_deck_intersection(const D3DXVECTOR3& v0,const D3DXVECTOR3& v1, const D3DXVECTOR3& n0,const D3DXVECTOR3& n1, D3DXVECTOR3& o, D3DXVECTOR3& on)
	{
		float h0 = v0.y - adhoc_deck_profile_for_rim(v0.z);
		float h1 = v1.y - adhoc_deck_profile_for_rim(v1.z);

		float lambda = (0.f-h0)/(h1-h0);

		o = lambda * v1 + (1.f - lambda) * v0;
		on = lambda * n1 + (1.f - lambda) * n0;
	}

	bool adhoc_get_deck_line(const D3DXVECTOR3& v0,const D3DXVECTOR3& v1,const D3DXVECTOR3& v2,D3DXVECTOR3& o0, D3DXVECTOR3& o1)
	{
		int above_flags = 0;
		if(v0.y > adhoc_deck_profile_for_rim(v0.z))
			above_flags += 1;
		if(v1.y > adhoc_deck_profile_for_rim(v1.z))
			above_flags += 2;
		if(v2.y > adhoc_deck_profile_for_rim(v2.z))
			above_flags += 4;

		if(above_flags == 0 || above_flags ==7)
			return false;

		switch(above_flags) {
		case 1:
		case 6:
			o0 = adhoc_get_deck_intersection(v0,v1);
			o1 = adhoc_get_deck_intersection(v0,v2);
			break;
		case 2:
		case 5:
			o0 = adhoc_get_deck_intersection(v1,v2);
			o1 = adhoc_get_deck_intersection(v1,v0);
			break;
		case 3:
		case 4:
			o0 = adhoc_get_deck_intersection(v2,v0);
			o1 = adhoc_get_deck_intersection(v2,v1);
			break;
		}

		return true;
	}

	bool adhoc_get_deck_line(const D3DXVECTOR3& v0,const D3DXVECTOR3& v1,const D3DXVECTOR3& v2,const D3DXVECTOR3& n0,const D3DXVECTOR3& n1,const D3DXVECTOR3& n2,D3DXVECTOR3& o0, D3DXVECTOR3& o1,D3DXVECTOR3& on0, D3DXVECTOR3& on1)
	{
		int above_flags = 0;
		if(v0.y > adhoc_deck_profile_for_rim(v0.z))
			above_flags += 1;
		if(v1.y > adhoc_deck_profile_for_rim(v1.z))
			above_flags += 2;
		if(v2.y > adhoc_deck_profile_for_rim(v2.z))
			above_flags += 4;

		if(above_flags == 0 || above_flags ==7)
			return false;

		switch(above_flags) {
		case 1:
		case 6:
			adhoc_get_deck_intersection(v0,v1,n0,n1,o0,on0);
			adhoc_get_deck_intersection(v0,v2,n0,n2,o1,on1);
			break;
		case 2:
		case 5:
			adhoc_get_deck_intersection(v1,v2,n1,n2,o0,on0);
			adhoc_get_deck_intersection(v1,v0,n1,n0,o1,on1);
			break;
		case 3:
		case 4:
			adhoc_get_deck_intersection(v2,v0,n2,n0,o0,on0);
			adhoc_get_deck_intersection(v2,v1,n2,n1,o1,on1);
			break;
		}

		return true;
	}
}

HRESULT OceanHullSensors::init(BoatMesh* pMesh, const D3DXMATRIX& matMeshToLocal)
{
	HRESULT hr;

	// We use an ad-hoc filter to try and pick outer-hull-skin only. This is a stop-gap until we have
	// a dedicated hull mesh for this
	struct ad_hoc_filter {
		bool operator()(const D3DXVECTOR3& v0,const D3DXVECTOR3& v1,const D3DXVECTOR3& v2,const D3DXVECTOR3& n0,const D3DXVECTOR3& n1,const D3DXVECTOR3& n2) const {

			D3DXVECTOR3 n0n,n1n,n2n;
			D3DXVec3Normalize(&n0n,&n0);
			D3DXVec3Normalize(&n1n,&n1);
			D3DXVec3Normalize(&n2n,&n2);

			// Reject if all of the verts are out-range
			if(!test_vertex(v0,n0n) && !test_vertex(v1,n1n) && !test_vertex(v2,n2n))
				return false;

			// Reject thin and outward-facing features
			float min_z = min(min(v0.z,v1.z),v2.z);
			float max_z = max(max(v0.z,v1.z),v2.z);
			D3DXVECTOR3 mean_n = (n0n+n1n+n2n)/3.f;
			if(fabs(mean_n.x) > 0.9f && (max_z-min_z) < 0.235f)
				return false;

			return true;
		}

		static bool test_vertex(const D3DXVECTOR3& v, const D3DXVECTOR3& n) {

			// Reject triangles that do not point down
			if(n.y > -0.1f)
				return false;

			// Anything that does not face outwards we also kill
			/*if(v.y > 0.f)*/ {
				// Project vertex onto centreline
				D3DXVECTOR3 v_centreline = v;
				v_centreline.x = 0;
				if(v_centreline.y < 0.f)
					v_centreline.y = 0;

				// Clamp ends
				if(v_centreline.z > 30.f)
					v_centreline.z = 30.f;
				else if(v_centreline.z < -30.f)
					v_centreline.z = -30.f;

				D3DXVECTOR3 vn = v - v_centreline;
				D3DXVec3Normalize(&vn,&vn);
				if(D3DXVec3Dot(&vn,&n) < 0.5f)
					return false;
			}

			return test_vertex_pos(v);
		}

		static bool test_vertex_pos(const D3DXVECTOR3& v) {

			// Reject verts that are part of the above-deck
			if(v.y > adhoc_deck_profile(v.z))
				return false;

			// Strip our sundry internal fittings
			if(fabs(v.x) < 3.f && v.y > 0.f && v.z < 25.f && v.z > -30.f)
				return false;
			if(fabs(v.x) < 4.f && v.y > 0.f && v.z < 15.f && v.z > -25.f)
				return false;

			return true;
		}
	} filter;

	struct preprocess_functor {
		float area;
		float rim_length;
		std::vector<DWORD> spray_mesh_indices;
		preprocess_functor() : area(0.f), rim_length(0.f) {}

		void operator()(DWORD i0, DWORD i1, DWORD i2, const D3DXVECTOR3& v0,const D3DXVECTOR3& v1,const D3DXVECTOR3& v2,const D3DXVECTOR3&,const D3DXVECTOR3&,const D3DXVECTOR3&) {
			area += calc_area(v0,v1,v2);
			spray_mesh_indices.push_back(i0);
			spray_mesh_indices.push_back(i1);
			spray_mesh_indices.push_back(i2);

			// Test for a deck intersection
			D3DXVECTOR3 d0,d1;
			if(adhoc_get_deck_line(v0,v1,v2,d0,d1)) {
				D3DXVECTOR3 e01 = d1 - d0;
				rim_length += D3DXVec3Length(&e01);
			}
		}

		static float calc_area(const D3DXVECTOR3& v0,const D3DXVECTOR3& v1,const D3DXVECTOR3& v2) {
			D3DXVECTOR3 edge01 = v1-v0;
			D3DXVECTOR3 edge02 = v2-v0;
			D3DXVECTOR3 cp;
			D3DXVec3Cross(&cp,&edge01,&edge02);
			return 0.5f * D3DXVec3Length(&cp);
		}

	} preprocess;
	for_each_triangle(preprocess, filter, pMesh, matMeshToLocal);

	const float length_multiplier = 1.02f;	// We use a Poission distribution to choose sensor locations,
											// so we add a little slop in the length calc to allow for
											// the likelihood that the Poisson process will generate more
											// sensors than expected ('expect' used here in the strict
											// probability theory sense)

	const float area = preprocess.area * length_multiplier * length_multiplier;
	const float mean_density = float(MaxNumSensors)/area;
	const float mean_area_per_gen = area/float(MaxNumSensors);
	m_MeanSensorRadius = sqrt(mean_area_per_gen/D3DX_PI);

	const float rim_len = preprocess.rim_length * length_multiplier;
	const float mean_rim_density = float(MaxNumRimSensors)/rim_len;
	m_MeanRimSensorSeparation = rim_len/float(MaxNumRimSensors);

	// Set up spray mesh
	m_VisualisationMeshIndexCount = preprocess.spray_mesh_indices.size();
	m_VisualisationMeshIndexFormat = DXGI_FORMAT_R32_UINT;

	D3D11_BUFFER_DESC ib_buffer_desc;
	ib_buffer_desc.ByteWidth = sizeof(DWORD) * m_VisualisationMeshIndexCount;
	ib_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	ib_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ib_buffer_desc.CPUAccessFlags = 0;
	ib_buffer_desc.MiscFlags = 0;
	ib_buffer_desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA ib_srd;
	ib_srd.pSysMem = &preprocess.spray_mesh_indices[0];
	ib_srd.SysMemPitch = ib_buffer_desc.ByteWidth;
	ib_srd.SysMemSlicePitch = ib_buffer_desc.ByteWidth;

	V_RETURN(m_pd3dDevice->CreateBuffer(&ib_buffer_desc, &ib_srd, &m_pVisualisationMeshIB));

	struct init_sensors_functor {
		float mean_density;
		float mean_rim_density;
		D3DXVECTOR3* pPositions;
		D3DXVECTOR3* pNormals;
		D3DXVECTOR3* pRimPositions;
		D3DXVECTOR3* pRimNormals;
		D3DXVECTOR3 bounds_min;
		D3DXVECTOR3 bounds_max;
		int max_num_sensors;
		int num_sensors;
		int max_num_rim_sensors;
		int num_rim_sensors;
		init_sensors_functor(D3DXVECTOR3* pPositionsArg, D3DXVECTOR3* pNormalsArg, D3DXVECTOR3* pRimPositionsArg, D3DXVECTOR3* pRimNormalsArg, float mean_density_arg, float mean_rim_density_arg, int max_num_sensors_arg, int max_num_rim_sensors_arg) :
			mean_density(mean_density_arg),
			mean_rim_density(mean_rim_density_arg),
			pPositions(pPositionsArg),
			pNormals(pNormalsArg),
			pRimPositions(pRimPositionsArg),
			pRimNormals(pRimNormalsArg),
			num_sensors(0),
			max_num_sensors(max_num_sensors_arg),
			num_rim_sensors(0),
			max_num_rim_sensors(max_num_rim_sensors_arg)
		{}

		void operator()(DWORD i0, DWORD i1, DWORD i2, const D3DXVECTOR3& v0,const D3DXVECTOR3& v1,const D3DXVECTOR3& v2,const D3DXVECTOR3& n0,const D3DXVECTOR3& n1,const D3DXVECTOR3& n2) {

			// Add area sensors
			{
				const float area = preprocess_functor::calc_area(v0,v1,v2);
				const float mean_num = mean_density * area;
				int actual_num = PoissonOutcome(mean_num);
				if(num_sensors + actual_num > max_num_sensors) {
					actual_num = max_num_sensors - num_sensors;
				}
				for(int i = 0; i != actual_num; ++i) {
					// Pick random points in triangle - note that for an unbiased pick, we first pick a point in the
					// entire parallelogram, then transform the outer half back onto the inner half
					float u = (float)rand()/(float)(RAND_MAX);
					float v = (float)rand()/(float)(RAND_MAX);
					if((u+v)>1.f) {
						float new_u = 1.f-v;
						float new_v = 1.f-u;
						u = new_u;
						v = new_v;
					}

					D3DXVECTOR3 pos = (1.f-u-v)*v0 + u*v1 + v*v2;
					D3DXVECTOR3 nrm = (1.f-u-v)*n0 + u*n1 + v*n2;
					if(ad_hoc_filter::test_vertex_pos(pos)) {
						pPositions[num_sensors] = pos;
						D3DXVec3Normalize(&pNormals[num_sensors],&nrm);
						if(num_sensors+num_rim_sensors) {
							bounds_min.x = min(pos.x,bounds_min.x);
							bounds_min.y = min(pos.y,bounds_min.y);
							bounds_min.z = min(pos.z,bounds_min.z);
							bounds_max.x = max(pos.x,bounds_max.x);
							bounds_max.y = max(pos.y,bounds_max.y);
							bounds_max.z = max(pos.z,bounds_max.z);
						} else {
							bounds_min = pos;
							bounds_max = pos;
						}
						++num_sensors;
					}
				}
			}

			// Add rim sensors
			D3DXVECTOR3 d0,d1,dn0,dn1;
			if(adhoc_get_deck_line(v0,v1,v2,n0,n1,n2,d0,d1,dn0,dn1)) {
				D3DXVECTOR3 e01 = d1 - d0;
				float edge_length = D3DXVec3Length(&e01);
				const float mean_num = mean_rim_density * edge_length;
				int actual_num = PoissonOutcome(mean_num);
				if(num_rim_sensors + actual_num > max_num_rim_sensors) {
					actual_num = max_num_rim_sensors - num_rim_sensors;
				}
				for(int i = 0; i != actual_num; ++i) {

					// Pick random points on line
					float u = (float)rand()/(float)(RAND_MAX);
					D3DXVECTOR3 pos = u * d0 + (1.f-u) * d1;
					pos.y += kRimVerticalOffset;
					D3DXVECTOR3 nrm = u * dn0 + (1.f-u) * dn1;
					pRimPositions[num_rim_sensors] = pos;
					D3DXVec3Normalize(&pRimNormals[num_rim_sensors],&nrm);
					if(num_sensors+num_rim_sensors) {
						bounds_min.x = min(pos.x,bounds_min.x);
						bounds_min.y = min(pos.y,bounds_min.y);
						bounds_min.z = min(pos.z,bounds_min.z);
						bounds_max.x = max(pos.x,bounds_max.x);
						bounds_max.y = max(pos.y,bounds_max.y);
						bounds_max.z = max(pos.z,bounds_max.z);
					} else {
						bounds_min = pos;
						bounds_max = pos;
					}
					++num_rim_sensors;
				}
			}
		}
	};
	init_sensors_functor init_sensors(m_SensorPositions, m_SensorNormals, m_RimSensorPositions, m_RimSensorNormals, mean_density, mean_rim_density, MaxNumSensors, MaxNumRimSensors);
	for_each_triangle(init_sensors, filter, pMesh, matMeshToLocal);

	m_NumSensors = init_sensors.num_sensors;
	m_NumRimSensors = init_sensors.num_rim_sensors;
	m_sensorBoundsMinCorner = init_sensors.bounds_min;
	m_sensorBoundsMaxCorner = init_sensors.bounds_max;

	return S_OK;
}

void OceanHullSensors::update(OceanSurfaceHeights* pHeights, const D3DXMATRIX& matLocalToWorld)
{
	D3DXVECTOR3 rv;

	for(int i = 0; i<m_NumSensors; i++)
	{
		D3DXVec3TransformCoord(&rv,&m_SensorPositions[i], &matLocalToWorld);
		m_WorldSensorPositions[i].x = rv.x;
		m_WorldSensorPositions[i].y = rv.z;
		m_WorldSensorPositions[i].z = rv.y;
		m_ReadbackCoords[i].x = m_WorldSensorPositions[i].x;
		m_ReadbackCoords[i].y = m_WorldSensorPositions[i].y;

		D3DXVec3TransformNormal(&rv,&m_SensorNormals[i], &matLocalToWorld);
		m_WorldSensorNormals[i].x = rv.x;
		m_WorldSensorNormals[i].y = rv.z;
		m_WorldSensorNormals[i].z = rv.y;
	}

	pHeights->getDisplacements(m_ReadbackCoords, (gfsdk_float4*)m_Displacements, m_NumSensors);

	for(int i = 0; i<m_NumRimSensors; i++)
	{
		D3DXVec3TransformCoord(&rv,&m_RimSensorPositions[i], &matLocalToWorld);
		m_WorldRimSensorPositions[i].x = rv.x;
		m_WorldRimSensorPositions[i].y = rv.z;
		m_WorldRimSensorPositions[i].z = rv.y;
		m_RimReadbackCoords[i].x = m_WorldRimSensorPositions[i].x;
		m_RimReadbackCoords[i].y = m_WorldRimSensorPositions[i].y;

		D3DXVec3TransformNormal(&rv,&m_RimSensorNormals[i], &matLocalToWorld);
		m_WorldRimSensorNormals[i].x = rv.x;
		m_WorldRimSensorNormals[i].y = rv.z;
		m_WorldRimSensorNormals[i].z = rv.y;
	}

	pHeights->getDisplacements(m_RimReadbackCoords, (gfsdk_float4*)m_RimDisplacements, m_NumRimSensors);
}
