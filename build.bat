@echo off

mkdir build
pushd build
cl -Zi ..\code\win32_window.cpp user32.lib gdi32.lib

popd

