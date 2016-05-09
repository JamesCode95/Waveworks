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

#include "Internal.h"
#include "D3DX_replacement_code.h"

void mat4Mat4Mul(gfsdk_float4x4& result, const gfsdk_float4x4& a, const gfsdk_float4x4& b)
{
	int i,j,k;
	float temp;

	for (i=0;i<4;++i)
	{
		for (j=0;j<4;++j)
		{
			temp=0;
			for(k=0;k<4;++k)
			{
				temp+=(&a._11)[k*4+j]*(&b._11)[i*4+k];
			}
			(&result._11)[i*4+j]=temp;
		}
	}
}

void vec4Mat4Mul(gfsdk_float4& result, const gfsdk_float4& a, const gfsdk_float4x4& b)
{
	int i,k;
	float temp;
	for (i=0;i<4;++i)
	{
		temp=0;
		for(k=0;k<4;++k)
		{
			temp+=(&b._11)[k*4+i]*(&a.x)[k];
		}
		(&result.x)[i]=temp;
	}
}

float det2x2(float a,float b,
			 float c,float d)
{
	return a * d - b * c;
}

float det3x3(float a1,float a2,float a3,
			 float b1,float b2,float b3,
			 float c1,float c2,float c3)
{
	float   ans;
	ans = a1 * det2x2( b2, b3, c2, c3)
		- b1 * det2x2( a2, a3, c2, c3)
		+ c1 * det2x2( a2, a3, b2, b3);
	return ans;
}

float det4x4(const gfsdk_float4x4& m)
{
	float   a1,a2,a3,a4,b1,b2,b3,b4,c1,c2,c3,c4,d1,d2,d3,d4;


	a1 = (&m._11)[0*4+0]; b1 = (&m._11)[0*4+1];
	c1 = (&m._11)[0*4+2]; d1 = (&m._11)[0*4+3];

	a2 = (&m._11)[1*4+0]; b2 = (&m._11)[1*4+1];
	c2 = (&m._11)[1*4+2]; d2 = (&m._11)[1*4+3];

	a3 = (&m._11)[2*4+0]; b3 = (&m._11)[2*4+1];
	c3 = (&m._11)[2*4+2]; d3 = (&m._11)[2*4+3];

	a4 = (&m._11)[3*4+0]; b4 = (&m._11)[3*4+1];
	c4 = (&m._11)[3*4+2]; d4 = (&m._11)[3*4+3];

	return  a1 * det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4)
		- b1 * det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4)
		+ c1 * det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4)
		- d1 * det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);
}

void adjoint(gfsdk_float4x4& adj, const gfsdk_float4x4& m)
{
	float   a1,a2,a3,a4,b1,b2,b3,b4,c1,c2,c3,c4,d1,d2,d3,d4;

	a1 = (&m._11)[0*4+0]; b1 = (&m._11)[0*4+1];
	c1 = (&m._11)[0*4+2]; d1 = (&m._11)[0*4+3];

	a2 = (&m._11)[1*4+0]; b2 = (&m._11)[1*4+1];
	c2 = (&m._11)[1*4+2]; d2 = (&m._11)[1*4+3];

	a3 = (&m._11)[2*4+0]; b3 = (&m._11)[2*4+1];
	c3 = (&m._11)[2*4+2]; d3 = (&m._11)[2*4+3];

	a4 = (&m._11)[3*4+0]; b4 = (&m._11)[3*4+1];
	c4 = (&m._11)[3*4+2]; d4 = (&m._11)[3*4+3];

	(&adj._11)[0*4+0] =   det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4);
	(&adj._11)[1*4+0] = - det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4);
	(&adj._11)[2*4+0] =   det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4);
	(&adj._11)[3*4+0] = - det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);

	(&adj._11)[0*4+1] = - det3x3( b1, b3, b4, c1, c3, c4, d1, d3, d4);
	(&adj._11)[1*4+1] =   det3x3( a1, a3, a4, c1, c3, c4, d1, d3, d4);
	(&adj._11)[2*4+1] = - det3x3( a1, a3, a4, b1, b3, b4, d1, d3, d4);
	(&adj._11)[3*4+1] =   det3x3( a1, a3, a4, b1, b3, b4, c1, c3, c4);

	(&adj._11)[0*4+2] =   det3x3( b1, b2, b4, c1, c2, c4, d1, d2, d4);
	(&adj._11)[1*4+2] = - det3x3( a1, a2, a4, c1, c2, c4, d1, d2, d4);
	(&adj._11)[2*4+2] =   det3x3( a1, a2, a4, b1, b2, b4, d1, d2, d4);
	(&adj._11)[3*4+2] = - det3x3( a1, a2, a4, b1, b2, b4, c1, c2, c4);

	(&adj._11)[0*4+3] = - det3x3( b1, b2, b3, c1, c2, c3, d1, d2, d3);
	(&adj._11)[1*4+3] =   det3x3( a1, a2, a3, c1, c2, c3, d1, d2, d3);
	(&adj._11)[2*4+3] = - det3x3( a1, a2, a3, b1, b2, b3, d1, d2, d3);
	(&adj._11)[3*4+3] =   det3x3( a1, a2, a3, b1, b2, b3, c1, c2, c3);
}

void mat4Inverse(gfsdk_float4x4& result, const gfsdk_float4x4& source)
{
	gfsdk_float4x4 adj;
	float det;
	int i,j;

	adjoint(adj,source);

	det = det4x4(source);
	if (fabs(det) < 1e-8f)
	{
		return ;
	}
	else
	{
		det = 1 / det;
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				(&result._11)[i*4+j] = (&adj._11)[i*4+j] * det;
	}
}
