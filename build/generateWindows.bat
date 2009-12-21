@echo off
IF NOT EXIST ..\..\output\include\*.* mkdir ..\..\output
IF NOT EXIST ..\..\output\include\*.* mkdir ..\..\output\include

REM  touch ..\..\output\include\dirstamp
IF EXIST "C:\msysgit\mingw\bin\WINDRES.exe" SET WINDRES="C:\msysgit\mingw\bin\WINDRES.exe "
IF EXIST "C:\msysgit\mingw\bin\WINDRES.exe" GOTO FOUNDWINDRES

IF EXIST "C:\msysgit\msysgit\bin\WINDRES.exe" SET WINDRES="C:\msysgit\msysgit\bin\WINDRES.exe "
IF EXIST "C:\msysgit\msysgit\bin\WINDRES.exe" GOTO FOUNDWINDRES

IF EXIST "C:\mingw\bin\WINDRES.exe" SET WINDRES="C:\mingw\bin\WINDRES.exe "
IF EXIST "C:\mingw\bin\WINDRES.exe" GOTO FOUNDWINDRES

Echo "Error: xcsoar\build\generateWindows.bat could not find WINDRES.exe"
echo goto EXIT

:FOUNDWINDRES
echo Using %WINDRES%


IF EXIST "C:\program files\git\bin\perl.exe" SET PERL="C:\program files\git\bin\perl.exe "
IF EXIST "C:\program files\git\bin\perl.exe" GOTO FOUNDPERL

IF EXIST "C:\msysgit\bin\perl.exe" SET PERL="C:\msysgit\bin\perl.exe "
IF EXIST "C:\msysgit\bin\perl.exe" GOTO FOUNDPERL

IF EXIST "C:\msysgit\msysgit\bin\perl.exe" SET PERL="C:\msysgit\msysgit\bin\perl.exe "
IF EXIST "C:\msysgit\msysgit\bin\perl.exe" GOTO FOUNDPERL

Echo "Error: xcsoar\build\generateWindows.bat could not find Perl.exe"
echo goto EXIT

:FOUNDPERL
echo Using %PERL%

%WINDRES% -i ..\Source\XCSoar.rc -J rc -o  ..\Source\XCSoar.res -O coff -I..\Header  -I..\Source  -D_MINGW32_ -DWINDOWSPC 

IF EXIST ..\..\output\include\InputEvents_pc.cpp GOTO InputEvents_pc_cpp_done
echo "  GEN     ..\..\output\include\InputEvents_pc.cpp"
%PERL% %1\Data\Input\xci2cpp.pl %1\Data\Input\pc.xci >..\..\output\include\InputEvents_pc.cpp.tmp

ren ..\..\output\include\InputEvents_pc.cpp.tmp InputEvents_pc.cpp
:InputEvents_pc_cpp_done

IF EXIST ..\..\output\include\InputEvents_altair.cpp GOTO InputEvents_altair_cpp_done
echo "  GEN     ..\..\output\include\InputEvents_altair.cpp"
%PERL% %1\Data\Input\xci2cpp.pl %1\Data\Input\altair.xci >..\..\output\include\InputEvents_altair.cpp.tmp
ren ..\..\output\include\InputEvents_altair.cpp.tmp InputEvents_altair.cpp
:InputEvents_altair_cpp_done

IF EXIST ..\..\output\include\InputEvents_pna.cpp GOTO InputEvents_pna_cpp_done
echo "  GEN     ..\..\output\include\InputEvents_pna.cpp"
%PERL% %1\Data\Input\xci2cpp.pl %1\Data\Input\pna.xci >..\..\output\include\InputEvents_pna.cpp.tmp
ren ..\..\output\include\InputEvents_pna.cpp.tmp InputEvents_pna.cpp
:InputEvents_pna_cpp_done

IF EXIST ..\..\output\include\InputEvents_fivv.cpp GOTO InputEvents_fivv_cpp_done
echo "  GEN     ..\..\output\include\InputEvents_fivv.cpp"
%PERL% %1\Data\Input\xci2cpp.pl %1\Data\Input\fivv.xci >..\..\output\include\InputEvents_fivv.cpp.tmp
ren ..\..\output\include\InputEvents_fivv.cpp.tmp InputEvents_fivv.cpp
:InputEvents_fivv_cpp_done

IF EXIST ..\..\output\include\InputEvents_default.cpp GOTO InputEvents_default_cpp_done
echo "  GEN     ..\..\output\include\InputEvents_default.cpp"
%PERL% %1\Data\Input\xci2cpp.pl %1\Data\Input\default.xci >..\..\output\include\InputEvents_default.cpp.tmp
ren ..\..\output\include\InputEvents_default.cpp.tmp InputEvents_default.cpp
:InputEvents_default_cpp_done

IF EXIST ..\..\output\include\InputEvents_Text2Event.cpp GOTO InputEvents_Text2Event_cpp_done
echo "  GEN     ..\..\output\include\InputEvents_Text2Event.cpp"
%PERL% %1\Data\Input\h2cpp.pl %1\Header\InputEvents.h >..\..\output\include\InputEvents_Text2Event.cpp.tmp
ren ..\..\output\include\InputEvents_Text2Event.cpp.tmp InputEvents_Text2Event.cpp
:InputEvents_Text2Event_cpp_done


IF EXIST ..\..\output\include\Status_defaults.cpp GOTO Status_defaults_cpp_done
echo "  GEN     output/include/Status_defaults.cpp"
%PERL% %1\Data\Status\xcs2cpp.pl %1\Data\Status\default.xcs >..\..\output\include\Status_defaults.cpp.tmp
ren ..\..\output\include\Status_defaults.cpp.tmp Status_defaults.cpp
:Status_defaults_cpp_done


:EXIT
