set IMGEXT=bmp 
%~dp0\..\..\..\..\test\d3d10\win64\debug\waveworks_test_d3d10_debug.exe -noaa -startframe 0 -endframe 50 - sliceframe 60 -readback 1 -quality 0 -screenshot %IMG_OUT%_d3d10.%IMGEXT% 
