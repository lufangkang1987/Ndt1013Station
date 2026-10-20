@echo off
chcp 65001 >nul
call "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist build-qmake mkdir build-qmake
cd build-qmake
"D:\Qt\6.11.1\msvc2022_64\bin\qmake.exe" ..\Ndt1013Station.pro -spec win32-msvc "CONFIG+=release" "CONFIG-=debug_and_release"
if errorlevel 1 exit /b 1
nmake /nologo
