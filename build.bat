@echo off
rem call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64

set CommonCompilerFlags=/MT /nologo /Gm- /GR- /EHa /Od /Oi /WX /W4 /wd4201 /wd4100 /wd4189 /DHANDMADE_INTERNAL=1 /DHANDMADE_SLOW=1 /Z7 /Fmwin32_handmade.map
set CommonLinkerFlags= -opt:ref  user32.lib gdi32.lib

IF NOT EXIST build mkdir build
pushd build

REM compilar pra winXP
rem cl %CommonCompilerFlags% ../handmade/win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

rem compilar normal x64
cl %CommonCompilerFlags% ../handmade/win32_handmade.cpp /link %CommonLinkerFlags%

popd
