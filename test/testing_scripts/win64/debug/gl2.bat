set IMGEXT=tga 
%~dp0\..\..\..\..\test\gl2\win64\debug\waveworks_test_gl2_debug.exe -noaa -startframe 0 -endframe 50 - sliceframe 60 -readback 1 -quality 0 -screenshot %IMG_OUT%_gl2.%IMGEXT% 
