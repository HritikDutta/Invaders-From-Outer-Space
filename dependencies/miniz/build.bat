@echo off

if not exist lib md lib

cl /O2 /EHsc /cgthreads8 /MP7 /GL /c src\*.c /I include
lib *.obj /out:lib\miniz.lib

del *.obj