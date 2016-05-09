@REM This code contains NVIDIA Confidential Information and is disclosed 
@REM under the Mutual Non-Disclosure Agreement. 
@REM 
@REM Notice 
@REM ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
@REM NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
@REM THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
@REM MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
@REM 
@REM NVIDIA Corporation assumes no responsibility for the consequences of use of such 
@REM information or for any infringement of patents or other rights of third parties that may 
@REM result from its use. No license is granted by implication or otherwise under any patent 
@REM or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
@REM expressly authorized by NVIDIA.  Details are subject to change without notice. 
@REM This code supersedes and replaces all information previously supplied. 
@REM NVIDIA Corporation products are not authorized for use as critical 
@REM components in life support devices or systems without express written approval of 
@REM NVIDIA Corporation. 
@REM 
@REM Copyright © 2008- 2013 NVIDIA Corporation. All rights reserved.
@REM
@REM NVIDIA Corporation and its licensors retain all intellectual property and proprietary
@REM rights in and to this software and related documentation and any modifications thereto.
@REM Any use, reproduction, disclosure or distribution of this software and related
@REM documentation without an express license agreement from NVIDIA Corporation is
@REM strictly prohibited.

:config
@set VARIATIONBATCHFILE=%1
@set VARIATIONNAME=%~n1%
@set CONFIG=%2
@set PLATFORM=%3
@set DIFF_TOOL=%~dp0..\..\..\..\..\external\ImageMagick\ImageMagick-6.8.5-6\compare.exe

:map_ref_platform
@set REFPLATFORM=%PLATFORM:64=32%

:info_msg
@echo TESTING: %REFPLATFORM%.%PLATFORM%.%CONFIG%.%VARIATIONNAME% >> %TEST_SUMMARY_FILE%

:dir_paths
@set IMG_OUT=%~dp0\..\testing_output\%PLATFORM%\%CONFIG%\
@set IMG_REF=%~dp0\..\testing_reference\%REFPLATFORM%\
@mkdir %IMG_OUT%

:run_the_test
@call %VARIATIONBATCHFILE%

:compare_output
%DIFF_TOOL% -metric AE %IMG_OUT%_%VARIATIONNAME%.%IMGEXT%  %IMG_REF%_%VARIATIONNAME%.%IMGEXT%  %IMG_OUT%_%VARIATIONNAME%_diff.%IMGEXT% 2>> %TEST_SUMMARY_FILE%
