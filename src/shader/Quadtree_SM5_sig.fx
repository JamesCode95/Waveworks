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

#define GFSDK_WAVEWORKS_SM5
#define GFSDK_WAVEWORKS_USE_TESSELLATION

#define GFSDK_WAVEWORKS_DECLARE_GEOM_VS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_GEOM_VS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_GEOM_VS_CBUFFER };

#define GFSDK_WAVEWORKS_DECLARE_GEOM_HS_CONSTANT(Type,Label,Regoff) Type Label;
#define GFSDK_WAVEWORKS_BEGIN_GEOM_HS_CBUFFER(Label) cbuffer Label {
#define GFSDK_WAVEWORKS_END_GEOM_HS_CBUFFER };

#include "Quadtree.fxh"

float4 GFSDK_WAVEWORKS_VERTEX_INPUT_Sig(GFSDK_WAVEWORKS_VERTEX_INPUT In) : SV_Position
{
	return In.nvsf_vPos;
}
