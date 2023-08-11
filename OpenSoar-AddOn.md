Differences OpenSoar to XCSoar-Project
--------------------------------------

Windows:
* use a different serial port name: Unique COMx (instead of 'COMx:' or '\\.\COMxx'

Additional Drivers
==================

| Driver | _ | Manufacture | _ | Remark |
|:----- |:-----  |:----- |:-----  |:----- |
| **FreeVario** |    | Blaubart  | | a driver for the FreeVario... |
| **anemoi**    |    | RS-Flight | | realtime wind |
| **Larus**    |    | Larus Breeze | | realtime wind and variometer|
| **Becker AR62xx** |    | Becker | | Becker Radio driver |

Build-Process
=============

* organizing the (Linux-) make build process with 1 script file ./build/cmake/MakeComplete.sh, dependend from include file ./build/cmake/MakeAll.sh

* CMake build process only available on Windows with VisualStudio, MinGW64 and Clang (and only with Windows executable) yet