@echo off
cd /D %~dp0../..

echo Directory = '%CD%'
REM pause

REM goto BULK_START

set TOOLCHAIN=ninja
set TOOLCHAIN=mgw112
set TOOLCHAIN=msvc2022
REM set TOOLCHAIN=mgw122
REM set TOOLCHAIN=clang15

echo %CD%
PATH=%CD%;%CD%\build\cmake\python;%PATH%
python build/cmake/python/Start-CMake-XCSoar.py  xcsoar %TOOLCHAIN%  14

if errorlevel 1 pause

exit /B 0


:: 3 times one after another:
: BULK_START
python build/cmake/Start-CMake-XCSoar.py  xcsoar clang14    6
if errorlevel 1 goto ERROR
REM python build/cmake/Start-CMake-XCSoar.py  xcsoar mgw112     6
if errorlevel 1 goto ERROR
python build/cmake/Start-CMake-XCSoar.py  xcsoar msvc2022   14
if errorlevel 1 goto ERROR

goto FINISH

: ERROR
echo Error during the BULK_START build!!!



: FINISH
pause
exit /B 0
