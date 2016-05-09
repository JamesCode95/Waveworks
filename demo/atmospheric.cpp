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
#include "atmospheric.h"

#pragma warning(disable:4127)

//-----------------------------------------------------------------------------
// Mie + Raileigh atmospheric scattering code 
// based on Sean O'Neil Accurate Atmospheric Scattering 
// from GPU Gems 2 
//-----------------------------------------------------------------------------

float scale(float fCos, float fScaleDepth)
{
	float x = 1.0f - fCos;
	return fScaleDepth * exp(-0.00287f + x*(0.459f + x*(3.83f + x*(-6.80f + x*5.25f))));
}

float atmospheric_depth(D3DXVECTOR3 position, D3DXVECTOR3 dir){
	float a = D3DXVec3Dot(&dir, &dir);
	float b = 2.0f*D3DXVec3Dot(&dir, &position);
	float c = D3DXVec3Dot(&position, &position)-1.0f;
	float det = b*b-4.0f*a*c;
	float detSqrt = sqrt(det);
	float q = (-b - detSqrt)/2.0f;
	float t1 = c/q;
	return t1;
}

AtmosphereColorsType CalculateAtmosphericScattering(D3DXVECTOR3 EyeVec, D3DXVECTOR3 VecToLight, float LightIntensity)
{
	AtmosphereColorsType output;
	static const int nSamples = 5;
	static const float fSamples = 5.0;

	D3DXVECTOR3 fWavelength = D3DXVECTOR3(0.65f,0.57f,0.47f);		// wavelength for the red, green, and blue channels
	D3DXVECTOR3 fInvWavelength = D3DXVECTOR3(5.60f,9.47f,20.49f);	// 1 / pow(wavelength, 4) for the red, green, and blue channels
	float fOuterRadius = 6520000.0f;								// The outer (atmosphere) radius
	float fOuterRadius2 = 6520000.0f*6520000.0f;					// fOuterRadius^2
	float fInnerRadius = 6400000.0f;								// The inner (planetary) radius
	float fInnerRadius2 = 6400000.0f*6400000.0f;					// fInnerRadius^2
	float fKrESun = 0.0075f * LightIntensity;						// Kr * ESun	// initially was 0.0025 * 20.0
	float fKmESun = 0.0001f * LightIntensity;						// Km * ESun	// initially was 0.0010 * 20.0;
	float fKr4PI = 0.0075f*4.0f*3.14f;								// Kr * 4 * PI
	float fKm4PI = 0.0001f*4.0f*3.14f;								// Km * 4 * PI
	float fScale = 1.0f/(6520000.0f - 6400000.0f);					// 1 / (fOuterRadius - fInnerRadius)
	float fScaleDepth	= 0.25f;									// The scale depth (i.e. the altitude at which the atmosphere's average density is found)
	float fScaleOverScaleDepth = (1.0f/(6520000.0f - 6400000.0f)) / 0.25f;	// fScale / fScaleDepth
	float G =	-0.98f;												// The Mie phase asymmetry factor
	float G2 = (-0.98f)*(-0.98f);

	// Get the ray from the camera to the vertex, and its length (which is the far point of the ray passing through the atmosphere)
	float d = atmospheric_depth(D3DXVECTOR3(0,0,fInnerRadius/fOuterRadius),EyeVec);
	D3DXVECTOR3 Pos = fOuterRadius*EyeVec*d+D3DXVECTOR3(0,0.0,fInnerRadius);
	D3DXVECTOR3 Ray = fOuterRadius*EyeVec*d;
	float  Far = D3DXVec3Length(&Ray);
	Ray /= Far;
	


	// Calculate the ray's starting position, then calculate its scattering offset
	D3DXVECTOR3 Start = D3DXVECTOR3(0,0,fInnerRadius);
	float Height = D3DXVec3Length(&Start);
	float Depth = 1.0;
	float StartAngle = D3DXVec3Dot(&Ray, &Start) / Height;
	float StartOffset = Depth*scale(StartAngle, fScaleDepth);

	// Initialize the scattering loop variables
	float SampleLength = Far / fSamples;
	float ScaledLength = SampleLength * fScale;
	D3DXVECTOR3 SampleRay = Ray * SampleLength;
	D3DXVECTOR3 SamplePoint = Start + SampleRay * 0.5;

	// Now loop through the sample points
	D3DXVECTOR3 SkyColor = D3DXVECTOR3(0.0, 0.0, 0.0);
	D3DXVECTOR3 Attenuate;
	for(int i=0; i<nSamples; i++)
	{
		float Height = D3DXVec3Length(&SamplePoint);
		float Depth = exp(fScaleOverScaleDepth * (fInnerRadius - Height));
		float LightAngle = D3DXVec3Dot(&VecToLight, &SamplePoint) / Height;
		float CameraAngle = D3DXVec3Dot(&Ray, &SamplePoint) / Height;
		float Scatter = (StartOffset + Depth*(scale(LightAngle, fScaleDepth) - scale(CameraAngle, fScaleDepth)));
		Attenuate.x = exp(-Scatter * (fInvWavelength.x * fKr4PI + fKm4PI));
		Attenuate.y = exp(-Scatter * (fInvWavelength.y * fKr4PI + fKm4PI));
		Attenuate.z = exp(-Scatter * (fInvWavelength.z * fKr4PI + fKm4PI));
		SkyColor += Attenuate * (Depth * ScaledLength);
		SamplePoint += SampleRay;
	}
	D3DXVECTOR3 MieColor = SkyColor * fKmESun;
	D3DXVECTOR3 RayleighColor;
	RayleighColor.x = SkyColor.x * (fInvWavelength.x * fKrESun);
	RayleighColor.y = SkyColor.y * (fInvWavelength.y * fKrESun);
	RayleighColor.z = SkyColor.z * (fInvWavelength.z * fKrESun);
	
	float fcos = -D3DXVec3Dot(&VecToLight, &EyeVec) / D3DXVec3Length(&EyeVec);
	float fMiePhase = 1.5f * ((1.0f - G2) / (2.0f + G2)) * (1.0f + fcos*fcos) / pow(1.0f + G2 - 2.0f*G*fcos, 1.5f);
	output.RayleighColor = RayleighColor;
	output.MieColor = fMiePhase* MieColor;
	output.Attenuation = Attenuate;
	return output;   
}
