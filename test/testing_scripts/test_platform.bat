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
@set PLATFORMDIR=%1
@set PLATFORMNAME=%~n1%

:init_summary
@set PLATFORM_OUTDIR=%~dp0\..\testing_output\%PLATFORMNAME%\
@mkdir %PLATFORM_OUTDIR%
@set TEST_SUMMARY_FILE=%PLATFORM_OUTDIR%summary.txt
@echo Results summary... > %TEST_SUMMARY_FILE%

forfiles /p %PLATFORMDIR% /c "cmd /c %~dp0\test_platform_config.bat %PLATFORMDIR%@relpath %PLATFORMNAME%"

@type %TEST_SUMMARY_FILE%

pause
