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

#ifndef _MESSAGETYPES_H
#define _MESSAGETYPES_H

// Message types for client-server test framework in WaveWorks
enum MessageTypeID {
	MessageTypeID_ClientRequestMarkersFromServer = 0,
	MessageTypeID_ServerSendMarkersToClient,
	MessageTypeID_ClientRequestTimeFromServer,
	MessageTypeID_ServerSendTimeToClient,
	MessageTypeID_ClientRequestConfigFromServer,
	MessageTypeID_ServerSendConfigToClient
};

enum {
	ServerPort = 1982
};

template<class PayloadType>
struct Message
{
	int messageTypeID;
	int pad1;	// In case the payload is 128b aligned
	int pad2;
	int pad3;
	PayloadType payload;
};

struct SimulationConfig
{
	// Assumes use_Beaufort_scale = true
	float wind_dir_x;
	float wind_dir_y;
	float wind_speed;
    float wind_dependency;
    float time_scale;
	float small_wave_fraction;
};

inline void UpdateWaveWorksParams(GFSDK_WaveWorks_Simulation_Params& dst, const SimulationConfig& src)
{
	dst.wind_speed = src.wind_speed;
	dst.wind_dependency = src.wind_dependency;
	dst.time_scale = src.time_scale;
	dst.small_wave_fraction = src.small_wave_fraction;
	dst.wind_dir.x = src.wind_dir_x;
	dst.wind_dir.y = src.wind_dir_y;
}

#endif	// _MESSAGETYPES_H
