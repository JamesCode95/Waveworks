:preliminaries
@set ORG_CD=%CD%

:args_check
@if "%1" NEQ "" goto has_arg1
@echo ERROR: XPJ path arg not defined
@goto build_failed
:has_arg1
@if "%2" NEQ "" goto has_arg2
@echo ERROR: tool string arg not defined
@goto build_failed
:has_arg2
@if "%3" NEQ "" goto has_arg3
@echo ERROR: platform string arg not defined
@goto build_failed
:has_arg3
@if "%4" NEQ "" goto has_arg4
@echo ERROR: platform tag arg not defined
@goto build_failed
:has_arg4
@if "%5" NEQ "" goto has_arg5
@echo ERROR: CUDA version arg not defined
@goto build_failed
:has_arg5
@if "%6" NEQ "" goto has_arg6
@echo ERROR: output dir arg not defined
@goto build_failed
:has_arg6
@if "%7" NEQ "" goto has_arg7
@echo ERROR: XPJ file arg not defined
@goto build_failed
:has_arg7

:capture_args
@set XPJ_BIN_PATH=%1\win32\xpj4
@set TOOL_STRING=%2
@set PLATFORM_STRING=%3
@set PLATFORM_TAG=%4
@set CUDA_DLL_VER=%5
@set ROOT_2_PROJ_OUTPUT_DIR=%6
@set XPJ_FILE=%7
@set XPJ_FILENAME=%~nx7
@set XPJ_DIR=%~dp7

:platform_env
@call %~dp0\..\..\build\script\XPJ_WIN_ENV.bat

:do_xpj
@set PROJ_OUTPUT_DIR=%XPJ_DIR%\..\..\%ROOT_2_PROJ_OUTPUT_DIR%
@set XPJ_TEMP=%PROJ_OUTPUT_DIR%\%XPJ_FILENAME%
@if not exist %PROJ_OUTPUT_DIR% mkdir %PROJ_OUTPUT_DIR%
@if errorlevel 1 goto build_failed
@copy %XPJ_FILE% %XPJ_TEMP%
@if errorlevel 1 goto build_failed
%ORG_CD%\%XPJ_BIN_PATH% -v 3 -t %TOOL_STRING% -p %PLATFORM_STRING% -d xpj2root=../ -d xpjdir=%XPJ_DIR% -d platform_tag=%PLATFORM_TAG% -x %XPJ_TEMP%
@if errorlevel 1 goto build_failed
@del %XPJ_TEMP%
@if errorlevel 1 goto build_failed

@goto build_succeeded
:build_failed
@cd %ORG_CD%
@echo ***BUILDERROR: deploy_demo_xpj failed
@exit /b 1

:build_succeeded
@cd %ORG_CD%
@echo ***BUILDINFO: deploy_demo_xpj succeeded