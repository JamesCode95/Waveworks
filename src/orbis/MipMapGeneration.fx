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

// #include "Common.fxh"

RW_Texture2D<float4> DestinationTex : register(u0);
Texture2D<float4> SourceTex : register(t0);

[NUM_THREADS(8,8,1)]
void main( uint2 group : S_GROUP_ID, uint2 thread : S_GROUP_THREAD_ID )
{
	uint2 dstIndex = group * 8 + thread;
	uint2 srcIndex = dstIndex * 2;

	uint width, height;
	SourceTex.GetDimensions(width, height);
	if(srcIndex.x >= width || srcIndex.y >= height)
		return;

	float4 x0 = SourceTex[srcIndex + uint2(0, 0)];
	float4 x1 = SourceTex[srcIndex + uint2(1, 0)];
	float4 x2 = SourceTex[srcIndex + uint2(0, 1)];
	float4 x3 = SourceTex[srcIndex + uint2(1, 1)];

	DestinationTex[dstIndex] = 0.25 * (x0 + x1 + x2 + x3);
}
