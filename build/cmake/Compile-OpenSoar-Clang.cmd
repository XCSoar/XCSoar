@echo off
cd /D %~dp0../..

::=======================
set CLANG_VERSION=clang15
::=======================

echo %CD%
PATH=%CD%;%CD%\build\cmake\python;%PATH%
python build/cmake/python/Start-CMake-OpenSoar.py  opensoar %CLANG_VERSION% 14

if errorlevel 1 pause
