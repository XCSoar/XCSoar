@echo off
cd /D %~dp0../..

::=======================
set MINGW_VERSION=mgw122
::=======================

echo %CD%
PATH=%CD%;%CD%\build\cmake\python;%PATH%
python build/cmake/python/Start-CMake-OpenSoar.py  opensoar %MINGW_VERSION%  15

if errorlevel 1 pause
