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

#ifndef _CLIENT_H
#define _CLIENT_H

#include "GFSDK_WaveWorks.h"

struct ClientImpl;
struct SimulationConfig;

/*********************************************************************************
 Re-usable class that takes care of client-side of duties in client-server test apps
*********************************************************************************/
class Client
{
public:

	static Client* Create();
	static void Destroy(Client* pClient);

	bool IsUsable() const;

	void Tick();

	bool HasPendingMessage() const;
	int ConsumePendingMessage();

	const SimulationConfig& GetSimulationConfig() const;

	double GetSimulationTime() const;

	bool GetRemoteMarkerPositions(const gfsdk_float4*& pPositions, size_t& numMarkers) const;

	void RequestRemoteMarkerPositions(const gfsdk_float2* pMarkerCoords, size_t numMarkers);

private:

	~Client();
	ClientImpl* pImpl;
};

#endif	// _MESSAGETYPES_H
