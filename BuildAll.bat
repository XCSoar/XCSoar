@echo off
set EVC="%ProgramFiles%\Microsoft eMbedded Tools\Common\EVC\Bin\EVC.EXE" 
set EVC4="%ProgramFiles%\Microsoft eMbedded C++ 4.0\Common\EVC\Bin\EVC.EXE"

:PPC2003

%EVC4%  PPC2003/XCSoar/XCSoar.vcp /MAKE ALL /REBUILD
%EVC4%  PPC2003/XCSoarSimulator/XCSoarSimulator.vcp /MAKE ALL /REBUILD
%EVC4%  PPC2003/XCSoarLaunch/XCSoarLaunch.vcp /MAKE ALL /REBUILD

echo Build: PPC2003 installer
call buildcab.bat
pause


:PPC2002

echo ARM > processor.lst
echo XCSoar > project.lst
echo XCSoarSimulator >> project.lst
echo XCSoarLaunch >> project.lst
echo XCSoarSetup >> project.lst

for /F %%j in (processor.lst) do (

echo Build: PPC2002 %%j *******************

for /F %%i in (project.lst) do (

  echo        %%i
  %EVC%  PPC2002/%%i/%%i.vcp /MAKE "%%i - Win32 (WCE %%j) Release" /ceconfig="Pocket PC 2002" /REBUILD
  if errorlevel 1 echo error build PPC2002/%%i/%%i.vcp 

)  
) 

del project.lst
del processor.lst 


echo Build: PPC2002 installer
call buildcab2002.bat
pause

:Data

echo Build: Data installer
call buildcabdata.bat
pause

:PPC

echo MIPS > processor.lst
echo ARM >> processor.lst

echo XCSoar > project.lst
echo XCSoarSimulator >> project.lst
echo XCSoarLaunch >> project.lst
echo XCSoarSetup >> project.lst

for /F %%j in (processor.lst) do (

echo Build: PPC %%j *******************

for /F %%i in (project.lst) do (

  echo        %%i
  %EVC%  PPC/%%i/%%i.vcp /MAKE "%%i - Win32 (WCE %%j) Release" /ceconfig="Pocket PC" /REBUILD
  if errorlevel 1 echo error build PPC/%%i/%%i.vcp 

)  
) 

del project.lst
del processor.lst 


echo Build: PPC installer
call buildcabPPC.bat
pause
                              
:error                              
                              
:end

