REM @echo off

REM Make sure the various variables that we need are set

IF NOT DEFINED GW_DEPS_ROOT GOTO GW_DEPS_ROOT_UNDEFINED

REM Generate projects here

rmdir /s /q compiler\vc12win64-cmake\
mkdir compiler\vc12win64-cmake\
pushd compiler\vc12win64-cmake\
cmake ..\.. -G "Visual Studio 12 2013" -Ax64 -DTARGET_BUILD_PLATFORM=Windows -DWW_OUTPUT_DIR=bin\vc12win64-cmake\
popd

REM rmdir /s /q compiler\vc14win64-cmake\
REM mkdir compiler\vc14win64-cmake\
REM pushd compiler\vc14win64-cmake\
REM cmake ..\.. -G "Visual Studio 14 2015" -Ax64 -DTARGET_BUILD_PLATFORM=Windows -DWW_OUTPUT_DIR=bin\vc14win64-cmake\
REM popd

REM rmdir /s /q compiler\vc12-ps4-cmake\
REM mkdir compiler\vc12-ps4-cmake\
REM pushd compiler\vc12-ps4-cmake\
REM cmake ..\.. -G "Visual Studio 12 2013" -DTARGET_BUILD_PLATFORM=PS4 -DPX_OUTPUT_DIR=lib\PS4\VS2013\ -DCMAKE_TOOLCHAIN_FILE="%GW_DEPS_ROOT%\CMakeModules\PS4Toolchain.txt" -DCMAKE_GENERATOR_PLATFORM=ORBIS
REM popd

REM rmdir /s /q compiler\vc11xbone-cmake\
REM mkdir compiler\vc11xbone-cmake\
REM pushd compiler\vc11xbone-cmake\
REM cmake ..\.. -G "Visual Studio 11 2012" -DTARGET_BUILD_PLATFORM=XBoxOne -DPX_OUTPUT_DIR=lib\XboxOne\VS2012\ -DCMAKE_TOOLCHAIN_FILE="%GW_DEPS_ROOT%\CMakeModules\XBoneToolchain.txt" -DCMAKE_GENERATOR_PLATFORM=Durango

REM Because XBone build is a bit wonky, delete these meta projects.

REM del ZERO_CHECK.*
REM del ALL_BUILD.*

REM popd

GOTO :End

:GW_DEPS_ROOT_UNDEFINED
ECHO GW_DEPS_ROOT has to be defined, pointing to the root of the dependency tree.
PAUSE
GOTO END

:CUDA_ROOT_UNDEFINED
ECHO CUDA_BIN_PATH has to be defined, pointing to the bin folder of your local CUDA install.
PAUSE

:End
