@echo off
chcp 65001 >nul
set "PATH=D:\Qt\6.11.1\msvc2022_64\bin;%PATH%"
set "QML_IMPORT_PATH=D:\Qt\6.11.1\msvc2022_64\qml"
set "QT_PLUGIN_PATH=D:\Qt\6.11.1\msvc2022_64\plugins"
cd /d "%~dp0"
if not exist build-ninja\Ndt1013Station.exe (
  echo Please run build-msvc.cmd first.
  pause
  exit /b 1
)
start "" build-ninja\Ndt1013Station.exe
