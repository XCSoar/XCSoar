@echo off
cd /D %~dp0../..

:: clang 12 - 07.02.2023: scheinbar ordentlich
::          - 01.03.2023: scheitert jetzt am   sodium 1.0.18?   
:: clang 14 - im toolchainfile muss llvm-ar angegeben werden (ar.exe gibt es nicht!)
:: clang 15 - 07.02.2023: ich weiß auch nicht, wie das schon mal gehen konnte: ich musste ja bei der 15.0.0-Version den Path zu MinGW aufmachen, und da lag ja ein clang12 drin.... Heute auf 15.0.7 geupdated, die toolchain angepasst (llvm-ar und llvm-rc) - danach compilierte er erst einmal durch, hatte nur beim Linken Probleme
::          - 01.03.2023: mit 3 Änderungen lief es besser (durch?)!
::                        * llvm-ar.exe kopiert in ar.exe (boost wollte immer das "ar", obwohl im Toolchain-File 'llvm-ar.exe' als CMAKE_AR_COMPILER angegeben war)
::                        * in (link_libs)/ares_build.h Zeile 5 #define CARES_TYPEOF_ARES_SSIZE_T __int64 (geändert von ...ssize_t)??
::                        * ABER Boost-Json und Boost-Container-Lib wird falsch angefordert: libboost_***-clangw15-mt-gd-x64-1_81.lib
::                          statt libboost_***-clangw15-mt-d-x64-1_81.lib! Warum? eigentlich soll doch das ganze mit HeadersOnly laufen....


::=======================
set CLANG_VERSION=clang16
::=======================

echo %CD%
PATH=%CD%;%CD%\build\cmake\python;%PATH%
python build/cmake/python/Start-CMake-OpenSoar.py  opensoar %CLANG_VERSION% 3

if errorlevel 1 pause
