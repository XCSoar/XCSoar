@echo off
cd /D %~dp0../..

:: clang 12 - 07.02.2023: scheinbar ordentlich
:: clang 14 - im toolchainfile muss llvm-ar angegeben werden (ar.exe gibt es nicht!)
:: clang 15 - 07.02.2023: ich weiß auch nicht, wie das schon mal gehen konnte: ich musste ja bei der 15.0.0-Version den Path zu MinGW aufmachen, und da lag ja ein clang12 drin.... Heute auf 15.0.7 geupdated, die toolchain angepasst (llvm-ar und llvm-rc) - danach compilierte er erst einmal durch, hatte nur beim Linken Probleme


::=======================
set CLANG_VERSION=clang12
::=======================

echo %CD%
PATH=%CD%;%CD%\build\cmake\python;%PATH%
python build/cmake/python/Start-CMake-OpenSoar.py  opensoar %CLANG_VERSION% 3

if errorlevel 1 pause
