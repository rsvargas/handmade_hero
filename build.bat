@echo off
rem call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64

IF NOT EXIST build mkdir build
pushd build
cl /W3 /Zi ../handmade/win32_handmade.cpp user32.lib gdi32.lib
popd
