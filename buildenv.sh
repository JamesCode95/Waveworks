#!/bin/bash

# This code contains NVIDIA Confidential Information and is disclosed 
# under the Mutual Non-Disclosure Agreement. 
# 
# Notice 
# ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
# NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
# THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
# MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
# 
# NVIDIA Corporation assumes no responsibility for the consequences of use of such 
# information or for any infringement of patents or other rights of third parties that may 
# result from its use. No license is granted by implication or otherwise under any patent 
# or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
# expressly authorized by NVIDIA.  Details are subject to change without notice. 
# This code supersedes and replaces all information previously supplied. 
# NVIDIA Corporation products are not authorized for use as critical 
# components in life support devices or systems without express written approval of 
# NVIDIA Corporation. 
# 
# Copyright © 2008- 2013 NVIDIA Corporation. All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property and proprietary
# rights in and to this software and related documentation and any modifications thereto.
# Any use, reproduction, disclosure or distribution of this software and related
# documentation without an express license agreement from NVIDIA Corporation is
# strictly prohibited.

export CUDA_DLL_VER=5.5
export CUDA_PATH=../../../external/CUDA/rhel6-5.5-0/
export CUDA_BIN_PATH=${CUDA_PATH}bin
export CUDA_INC_PATH=${CUDA_PATH}targets/x86_64-linux/include
export CUDA_LIB_PATH=${CUDA_PATH}targets/x86_64-linux/lib
XPJ_PATH=../../../external/xpj/1.v2/

