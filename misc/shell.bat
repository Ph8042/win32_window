@echo off
call %comspec% /k "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set path=w:\win32_window\misc;%path%
cls	