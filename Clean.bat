set EVC="C:\Program Files\Microsoft eMbedded Tools\Common\EVC\Bin\EVC.EXE" 
set EVC4="C:\Program Files\Microsoft eMbedded C++ 4.0\Common\EVC\Bin\EVC.EXE"

%EVC%  PPC/XCSoar/XCSoar.vcp /MAKE ALL /CLEAN
%EVC%  PPC/XCSoarSimulator/XCSoarSimulator.vcp /MAKE ALL /CLEAN
rem %EVC%  PPC/XCSoarMap/XCSoar.vcp /MAKE ALL /CLEAN


%EVC%  PPC2002/XCSoar/XCSoar.vcp /MAKE ALL /CLEAN
%EVC%  PPC2002/XCSoarSimulator/XCSoarSimulator.vcp /MAKE ALL /CLEAN
rem %EVC%  PPC2002/XCSoarMap/XCSoar.vcp /MAKE ALL /CLEAN


%EVC4%  PPC2003/XCSoar/XCSoar.vcp /MAKE ALL /CLEAN
%EVC4%  PPC2003/XCSoarSimulator/XCSoarSimulator.vcp /MAKE ALL /CLEAN
rem %EVC4%  PPC2003/XCSoarMap/XCSoar.vcp /MAKE ALL /CLEAN


%EVC4%  SmartPhone2003/XCSoar/XCSoar.vcp /MAKE ALL /CLEAN
%EVC4%  SmartPhone2003/XCSoarSimulator/XCSoarSimulator.vcp /MAKE ALL /CLEAN
rem %EVC4%  SmartPhone2003/XCSoarMap/XCSoar.vcp /MAKE ALL /CLEAN


pause

