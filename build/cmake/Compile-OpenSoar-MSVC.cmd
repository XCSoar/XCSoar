@echo off
cd /D %~dp0../..

echo %CD%
PATH=%CD%;%CD%\build\cmake\python;%PATH%
python build/cmake/python/Start-CMake-OpenSoar.py  opensoar msvc2022 14

if errorlevel 1 pause
