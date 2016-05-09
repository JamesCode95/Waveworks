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

#ifndef _OCEAN_HULL_SENSORS_H
#define _OCEAN_HULL_SENSORS_H

#include <D3DX11.h>
#include "GFSDK_WaveWorks_Types.h"

class BoatMesh;
class OceanSurfaceHeights;

class OceanHullSensors
{
public:
	OceanHullSensors();
	~OceanHullSensors();

	enum { MaxNumSensors = 20000 };
	enum { MaxNumRimSensors = 2000 };

	HRESULT init(BoatMesh* pMesh, const D3DXMATRIX& matMeshToLocal);

	void update(OceanSurfaceHeights* pHeights, const D3DXMATRIX& matLocalToWorld);

	int getNumSensors() const { return m_NumSensors; }
	const D3DXVECTOR3* getSensorPositions() const { return m_SensorPositions; }
	const D3DXVECTOR3* getSensorNormals() const { return m_SensorNormals; }
	const D3DXVECTOR3* getWorldSensorPositions() const { return m_WorldSensorPositions; }
	const D3DXVECTOR3* getWorldSensorNormals() const { return m_WorldSensorNormals; }
	const D3DXVECTOR4* getDisplacements() const { return m_Displacements; }
	const gfsdk_float2* getReadbackCoords() const { return m_ReadbackCoords; }

	int getNumRimSensors() const { return m_NumRimSensors; }
	const D3DXVECTOR3* getRimSensorPositions() const { return m_RimSensorPositions; }
	const D3DXVECTOR3* getRimSensorNormals() const { return m_RimSensorNormals; }
	const D3DXVECTOR3* getWorldRimSensorPositions() const { return m_WorldRimSensorPositions; }
	const D3DXVECTOR3* getWorldRimSensorNormals() const { return m_WorldRimSensorNormals; }
	const D3DXVECTOR4* getRimDisplacements() const { return m_RimDisplacements; }
	const gfsdk_float2* getRimReadbackCoords() const { return m_RimReadbackCoords; }

	const D3DXVECTOR3& getBoundsMinCorner() const { return m_sensorBoundsMinCorner; }
	const D3DXVECTOR3& getBoundsMaxCorner() const { return m_sensorBoundsMaxCorner; }

	float getMeanSensorRadius() const { return m_MeanSensorRadius; }
	float getMeanRimSensorSeparation() const { return m_MeanRimSensorSeparation; }

	ID3D11Buffer* GetVizMeshIndexBuffer() const { return m_pVisualisationMeshIB; }
	int GetVizMeshIndexCount() const { return m_VisualisationMeshIndexCount; }
	DXGI_FORMAT GetVizMeshIndexFormat() const { return m_VisualisationMeshIndexFormat; }

private:

	int				m_NumSensors;
	D3DXVECTOR3 	m_SensorPositions[MaxNumSensors];
	D3DXVECTOR3 	m_SensorNormals[MaxNumSensors];
	FLOAT			m_MeanSensorRadius;

	int				m_NumRimSensors;
	D3DXVECTOR3		m_RimSensorPositions[MaxNumRimSensors];
	D3DXVECTOR3		m_RimSensorNormals[MaxNumRimSensors];
	FLOAT			m_MeanRimSensorSeparation;

	D3DXVECTOR3		m_sensorBoundsMinCorner;
	D3DXVECTOR3		m_sensorBoundsMaxCorner;

	// Sensor data
	gfsdk_float2	m_ReadbackCoords[MaxNumSensors];
	D3DXVECTOR4 	m_Displacements[MaxNumSensors];
	D3DXVECTOR3		m_WorldSensorPositions[MaxNumSensors];
	D3DXVECTOR3		m_WorldSensorNormals[MaxNumSensors];

	gfsdk_float2	m_RimReadbackCoords[MaxNumRimSensors];
	D3DXVECTOR4 	m_RimDisplacements[MaxNumRimSensors];
	D3DXVECTOR3		m_WorldRimSensorPositions[MaxNumRimSensors];
	D3DXVECTOR3		m_WorldRimSensorNormals[MaxNumRimSensors];

	// Diagnostic visualisations
	ID3D11Device* m_pd3dDevice;
	ID3D11Buffer* m_pVisualisationMeshIB;
	int			  m_VisualisationMeshIndexCount;
	DXGI_FORMAT	  m_VisualisationMeshIndexFormat;
};

#endif	// _OCEAN_HULL_SENSORS_H
