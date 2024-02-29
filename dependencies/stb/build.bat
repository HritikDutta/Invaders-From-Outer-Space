@echo off

if not exist lib md lib

cl /O2 /EHsc /std:c++17 /cgthreads8 /MP7 /GL /c src\*.cpp /I ..\..\src
lib *.obj /out:lib\stb.lib

del *.obj