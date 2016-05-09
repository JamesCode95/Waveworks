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
// Copyright  2008- 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#ifndef _NVWAVEWORKS_FFT_API_SUPPORT_H
#define _NVWAVEWORKS_FFT_API_SUPPORT_H

#if defined(TARGET_PLATFORM_WINDOWS) || defined(TARGET_PLATFORM_LINUX)
	// On open platforms, CPU path is NDA-only
	#if defined(WAVEWORKS_NDA_BUILD)
		#define SUPPORT_FFTCPU
	#endif
#else
	// Always offer CPU path on closed platforms
	#define SUPPORT_FFTCPU
#endif

#if defined(TARGET_PLATFORM_WINDOWS)
	// Can choose between CUDA and DirectCompute on Windows
	#if WAVEWORKS_ENABLE_DIRECTCOMPUTE
		#define SUPPORT_DIRECTCOMPUTE
	#else
		#define SUPPORT_CUDA
	#endif
#endif

#if defined(TARGET_PLATFORM_LINUX)
	// CUDA only on Linux
	#define SUPPORT_CUDA
#endif

#if defined(TARGET_PLATFORM_XBONE)
// CPU-only on Xbone, for now... #define SUPPORT_DIRECTCOMPUTE
#endif

#if defined(TARGET_PLATFORM_MICROSOFT) || defined(TARGET_PLATFORM_NIXLIKE)
#define WAVEWORKS_ENABLE_PROFILING
#endif

#if defined(SUPPORT_CUDA)
#define CUDA_ONLY(x) x
#else
#define CUDA_ONLY(x)
#endif

#endif //_NVWAVEWORKS_FFT_API_SUPPORT_H
