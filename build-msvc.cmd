@echo off
chcp 65001 >nul
call "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
cd /d "%~dp0"
cmake -S . -B build-ninja -G Ninja -DCMAKE_MAKE_PROGRAM=D:/Qt/Tools/Ninja/ninja.exe -DCMAKE_PREFIX_PATH=D:/Qt/6.11.1/msvc2022_64 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build build-ninja
