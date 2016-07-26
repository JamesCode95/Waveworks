@echo off
set PM_PACKMAN_VERSION=3.0
:: The external root may already be configured and we should do minimal work in that case
if defined PM_PACKAGES_ROOT goto ENSURE_DIR

:: If the folder isn't set we assume that the best place for it is on the drive that we are currently
:: running from
set PM_DRIVE=%CD:~0,2%

set PM_PACKAGES_ROOT=%PM_DRIVE%\NVIDIA\packman-repo

:: We use *setx* here so that the variable is persisted in the user environment
echo Setting user environment variable PM_PACKAGES_ROOT to %PM_PACKAGES_ROOT%
setx PM_PACKAGES_ROOT %PM_PACKAGES_ROOT%
if errorlevel 1 goto ERROR

:: Check for the directory that we need. Note that mkdir will create any directories
:: that may be needed in the path 
:ENSURE_DIR
if not exist %PM_PACKAGES_ROOT% (
	echo Creating directory %PM_PACKAGES_ROOT%
	mkdir %PM_PACKAGES_ROOT%
)

:: The Python interpreter may already be externally configured
if defined PM_PYTHON_EXT (
	set PM_PYTHON=%PM_PYTHON_EXT%
	goto PACKMAN
)

set PM_PYTHON_DIR=%PM_PACKAGES_ROOT%\python\2.7.6-windows-x86
set PM_PYTHON=%PM_PYTHON_DIR%\python.exe

if exist %PM_PYTHON% goto PACKMAN

set PM_PYTHON_PACKAGE=python@2.7.6-windows-x86.exe
set TARGET=%TEMP%\%PM_PYTHON_PACKAGE%
echo Fetching %PM_PYTHON_PACKAGE% from S3  ...
powershell -ExecutionPolicy ByPass -NoLogo -NoProfile -File %~dp0\fetch_file_from_s3.ps1 -sourceName %PM_PYTHON_PACKAGE% -output %TARGET%
:: A bug in powershell prevents the errorlevel code from being set when using the -File execution option
if errorlevel 1 goto ERROR

echo Unpacking ...
%TARGET% -o%PM_PYTHON_DIR% -y
if errorlevel 1 goto ERROR

del %TARGET%

:PACKMAN
:: The packman module may already be externally configured
if defined PM_MODULE_EXT (
	set PM_MODULE=%PM_MODULE_EXT%
	goto END
)

set PM_MODULE_DIR=%PM_PACKAGES_ROOT%\packman\%PM_PACKMAN_VERSION%-common
set PM_MODULE=%PM_MODULE_DIR%\packman.py

if exist %PM_MODULE% goto END

set PM_MODULE_PACKAGE=packman@%PM_PACKMAN_VERSION%-common.zip
set TARGET=%TEMP%\%PM_MODULE_PACKAGE%
echo Fetching %PM_MODULE_PACKAGE% from S3 ...
powershell -ExecutionPolicy ByPass -NoLogo -NoProfile -File %~dp0\fetch_file_from_s3.ps1 -sourceName %PM_MODULE_PACKAGE% -output %TARGET%
:: A bug in powershell prevents the errorlevel code from being set when using the -File execution option
if errorlevel 1 goto ERROR

echo Unpacking ...
%PM_PYTHON% %~dp0\install_package.py %TARGET% %PM_MODULE_DIR%
if errorlevel 1 goto ERROR

del %TARGET%

goto END

:ERROR
echo !!! Failure while configuring local machine :( !!!

:END
