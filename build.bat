@echo off

set executable_name="main.exe"

if "%1"=="release" (
    set defines= /DGN_USE_OPENGL /DGN_PLATFORM_WINDOWS /DGN_USE_DEDICATED_GPU /DGN_RELEASE /DNDEBUG /DGN_COMPILER_MSVC
    set compile_flags= /MT /O2 /EHsc /std:c++17 /cgthreads8 /MP7 /GL
    set link_flags= /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcmtd.lib /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /LTCG

    echo BUILDING RELEASE EXECUTABLE
) else (
    set defines= /DGN_USE_OPENGL /DGN_PLATFORM_WINDOWS /DGN_USE_DEDICATED_GPU /DGN_DEBUG /DGN_COMPILER_MSVC
    set compile_flags= /MTd /Zi /EHsc /std:c++17 /cgthreads8 /MP7 /GL
    set link_flags= /DEBUG /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcmtd.lib /LTCG

    echo BUILDING DEBUG EXECUTABLE
)

set includes= /I src ^
              /I dependencies\glad\include   ^
              /I dependencies\wglext\include ^
              /I dependencies\stb\include    ^
              /I dependencies\miniz\include

set libs= shell32.lib                     ^
          user32.lib                      ^
          gdi32.lib                       ^
          openGL32.lib                    ^
          msvcrt.lib                      ^
          comdlg32.lib                    ^
          Xaudio2.lib                     ^
          Ole32.lib                       ^
          dependencies\glad\lib\glad.lib  ^
          dependencies\stb\lib\stb.lib    ^
          dependencies\miniz\lib\miniz.lib

rem Source
cl %compile_flags% /c src/serialization/json/*.cpp %defines% %includes%   &^
cl %compile_flags% /c src/serialization/binary/*.cpp %defines% %includes% &^
cl %compile_flags% /c src/audio/*.cpp %defines% %includes%                &^
cl %compile_flags% /c src/fileio/*.cpp %defines% %includes%               &^
cl %compile_flags% /c src/graphics/*.cpp %defines% %includes%             &^
cl %compile_flags% /c src/platform/*.cpp %defines% %includes%             &^
cl %compile_flags% /c src/application/*.cpp %defines% %includes%          &^
cl %compile_flags% /c src/core/*.cpp %defines% %includes%                 &^
cl %compile_flags% /c src/platform/*.cpp %defines% %includes%             &^
cl %compile_flags% /c src/math/*.cpp %defines% %includes%                 &^
cl %compile_flags% /c src/engine/*.cpp %defines% %includes%               &^
cl %compile_flags% /c src/game/*.cpp %defines% %includes%                 &^
cl %compile_flags% /c src/main.cpp %defines% %includes%

rem Resources
rc resources.rc

link *.obj %libs% *.res /OUT:%executable_name% %link_flags%

rem Remove intermediate files
del *.obj *.exp *.lib *.res