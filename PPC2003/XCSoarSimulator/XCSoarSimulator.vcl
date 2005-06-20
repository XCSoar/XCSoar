<html>
<body>
<pre>
<h1>Build Log</h1>
<h3>
--------------------Configuration: XCSoarSimulator - Win32 (WCE emulator) Debug--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"emulatorDbg/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d "WIN32_PLATFORM_PSPC=400" /d UNDER_CE=420 /d _WIN32_WCE=420 /d "UNICODE" /d "_UNICODE" /d "DEBUG" /d "_X86_" /d "x86" /d "_i386_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A2.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "_i386_" /D "i_386_" /D "_X86_" /D "x86" /D "_SIM_" /D UNDER_CE=420 /D _WIN32_WCE=420 /D "WIN32_PLATFORM_PSPC=400" /D "UNICODE" /D "_UNICODE" /FR"emulatorDbg/" /Fp"emulatorDbg/XCSoarSimulator.pch" /Yu"stdafx.h" /Fo"emulatorDbg/" /Fd"emulatorDbg/" /Gs8192 /GF /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Airspace.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Calculations.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Dialogs.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Logger.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\MapWindow.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\McReady.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Parser.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Port.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Process.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Utils.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Waypointparser.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.cpp"
]
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A2.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A3.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "_i386_" /D "i_386_" /D "_X86_" /D "x86" /D "_SIM_" /D UNDER_CE=420 /D _WIN32_WCE=420 /D "WIN32_PLATFORM_PSPC=400" /D "UNICODE" /D "_UNICODE" /FR"emulatorDbg/" /Fp"emulatorDbg/XCSoarSimulator.pch" /Yc"stdafx.h" /Fo"emulatorDbg/" /Fd"emulatorDbg/" /Gs8192 /GF /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC2003\XCSoarSimulator\StdAfx.cpp"
]
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A3.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A4.tmp" with contents
[
commctrl.lib coredll.lib corelibc.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:yes /pdb:"emulatorDbg/XCSoarSimulator.pdb" /debug /nodefaultlib:"OLDNAMES.lib" /nodefaultlib:libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /out:"emulatorDbg/XCSoarSimulator.exe" /subsystem:windowsce,4.20 /MACHINE:IX86 
".\emulatorDbg\Airspace.obj"
".\emulatorDbg\Calculations.obj"
".\emulatorDbg\Dialogs.obj"
".\emulatorDbg\Logger.obj"
".\emulatorDbg\MapWindow.obj"
".\emulatorDbg\McReady.obj"
".\emulatorDbg\Parser.obj"
".\emulatorDbg\Port.obj"
".\emulatorDbg\Process.obj"
".\emulatorDbg\StdAfx.obj"
".\emulatorDbg\Utils.obj"
".\emulatorDbg\Waypointparser.obj"
".\emulatorDbg\XCSoar.obj"
".\emulatorDbg\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A4.tmp"
<h3>Output Window</h3>
Compiling resources...
Compiling...
StdAfx.cpp
Compiling...
Airspace.cpp
Calculations.cpp
Dialogs.cpp
Logger.cpp
MapWindow.cpp
McReady.cpp
Parser.cpp
Port.cpp
Process.cpp
Utils.cpp
Waypointparser.cpp
XCSoar.cpp
Generating Code...
Linking...
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A7.tmp" with contents
[
/nologo /o"emulatorDbg/XCSoarSimulator.bsc" 
".\emulatorDbg\StdAfx.sbr"
".\emulatorDbg\Airspace.sbr"
".\emulatorDbg\Calculations.sbr"
".\emulatorDbg\Dialogs.sbr"
".\emulatorDbg\Logger.sbr"
".\emulatorDbg\MapWindow.sbr"
".\emulatorDbg\McReady.sbr"
".\emulatorDbg\Parser.sbr"
".\emulatorDbg\Port.sbr"
".\emulatorDbg\Process.sbr"
".\emulatorDbg\Utils.sbr"
".\emulatorDbg\Waypointparser.sbr"
".\emulatorDbg\XCSoar.sbr"]
Creating command line "bscmake.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A7.tmp"
Creating browse info file...
<h3>Output Window</h3>





<h3>Results</h3>
XCSoarSimulator.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoarSimulator - Win32 (WCE ARMV4) Debug--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"ARMV4Dbg/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=420 /d _WIN32_WCE=420 /d "DEBUG" /d "UNICODE" /d "_UNICODE" /d "WIN32_PLATFORM_PSPC=400" /d "ARM" /d "_ARM_" /d "ARMV4" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A8.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "ARM" /D "_ARM_" /D "ARMV4" /D "_SIM_" /D UNDER_CE=420 /D _WIN32_WCE=420 /D "WIN32_PLATFORM_PSPC=400" /D "UNICODE" /D "_UNICODE" /Fp"ARMV4Dbg/XCSoarSimulator.pch" /Yu"stdafx.h" /Fo"ARMV4Dbg/" /Fd"ARMV4Dbg/" /Zp16 /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Airspace.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Calculations.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Dialogs.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Logger.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\MapWindow.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\McReady.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Parser.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Port.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Process.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Utils.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Waypointparser.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A8.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A9.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "ARM" /D "_ARM_" /D "ARMV4" /D "_SIM_" /D UNDER_CE=420 /D _WIN32_WCE=420 /D "WIN32_PLATFORM_PSPC=400" /D "UNICODE" /D "_UNICODE" /Fp"ARMV4Dbg/XCSoarSimulator.pch" /Yc"stdafx.h" /Fo"ARMV4Dbg/" /Fd"ARMV4Dbg/" /Zp16 /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC2003\XCSoarSimulator\StdAfx.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2A9.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2AA.tmp" with contents
[
commctrl.lib coredll.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x20000,0x2000 /entry:"WinMainCRTStartup" /incremental:yes /pdb:"ARMV4Dbg/XCSoarSimulator.pdb" /debug /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib" /out:"ARMV4Dbg/XCSoarSimulator.exe" /subsystem:windowsce,4.20 /align:"4096" /MACHINE:ARM 
".\ARMV4Dbg\Airspace.obj"
".\ARMV4Dbg\Calculations.obj"
".\ARMV4Dbg\Dialogs.obj"
".\ARMV4Dbg\Logger.obj"
".\ARMV4Dbg\MapWindow.obj"
".\ARMV4Dbg\McReady.obj"
".\ARMV4Dbg\Parser.obj"
".\ARMV4Dbg\Port.obj"
".\ARMV4Dbg\Process.obj"
".\ARMV4Dbg\StdAfx.obj"
".\ARMV4Dbg\Utils.obj"
".\ARMV4Dbg\Waypointparser.obj"
".\ARMV4Dbg\XCSoar.obj"
".\ARMV4Dbg\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2AA.tmp"
<h3>Output Window</h3>
Compiling resources...
Compiling...
StdAfx.cpp
Compiling...
Airspace.cpp
Calculations.cpp
Dialogs.cpp
Logger.cpp
MapWindow.cpp
McReady.cpp
Parser.cpp
Port.cpp
Process.cpp
Utils.cpp
Waypointparser.cpp
XCSoar.cpp
Generating Code...
Linking...





<h3>Results</h3>
XCSoarSimulator.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoarSimulator - Win32 (WCE emulator) Release--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"emulatorRel/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=420 /d _WIN32_WCE=420 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WIN32_PLATFORM_PSPC=400" /d "_X86_" /d "x86" /d "_i386_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2AD.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D "_i386_" /D "i_386_" /D "_X86_" /D "x86" /D "NDEBUG" /D "_SIM_" /D UNDER_CE=420 /D _WIN32_WCE=420 /D "WIN32_PLATFORM_PSPC=400" /D "UNICODE" /D "_UNICODE" /Fp"emulatorRel/XCSoarSimulator.pch" /Yu"stdafx.h" /Fo"emulatorRel/" /Gs8192 /GF /O2 /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Airspace.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Calculations.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Dialogs.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Logger.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\MapWindow.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\McReady.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Parser.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Port.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Process.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Utils.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Waypointparser.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.cpp"
]
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2AD.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2AE.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D "_i386_" /D "i_386_" /D "_X86_" /D "x86" /D "NDEBUG" /D "_SIM_" /D UNDER_CE=420 /D _WIN32_WCE=420 /D "WIN32_PLATFORM_PSPC=400" /D "UNICODE" /D "_UNICODE" /Fp"emulatorRel/XCSoarSimulator.pch" /Yc"stdafx.h" /Fo"emulatorRel/" /Gs8192 /GF /O2 /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC2003\XCSoarSimulator\StdAfx.cpp"
]
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2AE.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2AF.tmp" with contents
[
commctrl.lib coredll.lib corelibc.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"emulatorRel/XCSoarSimulator.pdb" /nodefaultlib:"OLDNAMES.lib" /nodefaultlib:libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /out:"emulatorRel/XCSoarSimulator.exe" /subsystem:windowsce,4.20 /MACHINE:IX86 
".\emulatorRel\Airspace.obj"
".\emulatorRel\Calculations.obj"
".\emulatorRel\Dialogs.obj"
".\emulatorRel\Logger.obj"
".\emulatorRel\MapWindow.obj"
".\emulatorRel\McReady.obj"
".\emulatorRel\Parser.obj"
".\emulatorRel\Port.obj"
".\emulatorRel\Process.obj"
".\emulatorRel\StdAfx.obj"
".\emulatorRel\Utils.obj"
".\emulatorRel\Waypointparser.obj"
".\emulatorRel\XCSoar.obj"
".\emulatorRel\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2AF.tmp"
<h3>Output Window</h3>
Compiling resources...
Compiling...
StdAfx.cpp
Compiling...
Airspace.cpp
Calculations.cpp
Dialogs.cpp
Logger.cpp
MapWindow.cpp
McReady.cpp
Parser.cpp
Port.cpp
Process.cpp
Utils.cpp
Waypointparser.cpp
XCSoar.cpp
Generating Code...
Linking...





<h3>Results</h3>
XCSoarSimulator.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoarSimulator - Win32 (WCE ARMV4) Release--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"ARMV4Rel/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=420 /d _WIN32_WCE=420 /d "NDEBUG" /d "UNICODE" /d "_UNICODE" /d "WIN32_PLATFORM_PSPC=400" /d "ARM" /d "_ARM_" /d "ARMV4" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2B2.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D "ARM" /D "_ARM_" /D "ARMV4" /D "NDEBUG" /D "_SIM_" /D UNDER_CE=420 /D _WIN32_WCE=420 /D "WIN32_PLATFORM_PSPC=400" /D "UNICODE" /D "_UNICODE" /Fp"ARMV4Rel/XCSoarSimulator.pch" /Yu"stdafx.h" /Fo"ARMV4Rel/" /O2 /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Airspace.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Calculations.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Dialogs.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Logger.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\MapWindow.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\McReady.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Parser.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Port.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Process.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Utils.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\Waypointparser.cpp"
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2B2.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2B3.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D "ARM" /D "_ARM_" /D "ARMV4" /D "NDEBUG" /D "_SIM_" /D UNDER_CE=420 /D _WIN32_WCE=420 /D "WIN32_PLATFORM_PSPC=400" /D "UNICODE" /D "_UNICODE" /Fp"ARMV4Rel/XCSoarSimulator.pch" /Yc"stdafx.h" /Fo"ARMV4Rel/" /O2 /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC2003\XCSoarSimulator\StdAfx.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2B3.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2B4.tmp" with contents
[
commctrl.lib coredll.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"ARMV4Rel/XCSoarSimulator.pdb" /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib" /out:"ARMV4Rel/XCSoarSimulator.exe" /subsystem:windowsce,4.20 /align:"4096" /MACHINE:ARM 
".\ARMV4Rel\Airspace.obj"
".\ARMV4Rel\Calculations.obj"
".\ARMV4Rel\Dialogs.obj"
".\ARMV4Rel\Logger.obj"
".\ARMV4Rel\MapWindow.obj"
".\ARMV4Rel\McReady.obj"
".\ARMV4Rel\Parser.obj"
".\ARMV4Rel\Port.obj"
".\ARMV4Rel\Process.obj"
".\ARMV4Rel\StdAfx.obj"
".\ARMV4Rel\Utils.obj"
".\ARMV4Rel\Waypointparser.obj"
".\ARMV4Rel\XCSoar.obj"
".\ARMV4Rel\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP2B4.tmp"
<h3>Output Window</h3>
Compiling resources...
Compiling...
StdAfx.cpp
Compiling...
Airspace.cpp
Calculations.cpp
Dialogs.cpp
Logger.cpp
MapWindow.cpp
McReady.cpp
Parser.cpp
Port.cpp
Process.cpp
Utils.cpp
Waypointparser.cpp
XCSoar.cpp
Generating Code...
Linking...





<h3>Results</h3>
XCSoarSimulator.exe - 0 error(s), 0 warning(s)
</pre>
</body>
</html>
