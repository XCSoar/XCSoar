<html>
<body>
<pre>
<h1>Build Log</h1>
<h3>
--------------------Configuration: XCSoar - Win32 (WCE MIPS) Release--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"MIPSRel/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WIN32_PLATFORM_PSPC" /d "MIPS" /d "_MIPS_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP20B.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "MIPS" /D "_MIPS_" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /Fp"MIPSRel/XCSoar.pch" /Yu"stdafx.h" /Fo"MIPSRel/" /Oxs /MC /c 
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
Creating command line "clmips.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP20B.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP20C.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "MIPS" /D "_MIPS_" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /Fp"MIPSRel/XCSoar.pch" /Yc"stdafx.h" /Fo"MIPSRel/" /Oxs /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC\XCSoar\StdAfx.cpp"
]
Creating command line "clmips.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP20C.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP20D.tmp" with contents
[
commctrl.lib coredll.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"MIPSRel/XCSoar.pdb" /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib" /out:"MIPSRel/XCSoar.exe" /subsystem:windowsce,3.00 /MACHINE:MIPS 
".\MIPSRel\Airspace.obj"
".\MIPSRel\Calculations.obj"
".\MIPSRel\Dialogs.obj"
".\MIPSRel\Logger.obj"
".\MIPSRel\MapWindow.obj"
".\MIPSRel\McReady.obj"
".\MIPSRel\Parser.obj"
".\MIPSRel\Port.obj"
".\MIPSRel\Process.obj"
".\MIPSRel\StdAfx.obj"
".\MIPSRel\Utils.obj"
".\MIPSRel\Waypointparser.obj"
".\MIPSRel\XCSoar.obj"
".\MIPSRel\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP20D.tmp"
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
XCSoar.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoar - Win32 (WCE ARM) Release--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"ARMRel/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WIN32_PLATFORM_PSPC" /d "ARM" /d "_ARM_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP210.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "ARM" /D "_ARM_" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /Fp"ARMRel/XCSoar.pch" /Yu"stdafx.h" /Fo"ARMRel/" /Oxs /MC /c 
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
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP210.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP211.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "ARM" /D "_ARM_" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /Fp"ARMRel/XCSoar.pch" /Yc"stdafx.h" /Fo"ARMRel/" /Oxs /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC\XCSoar\StdAfx.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP211.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP212.tmp" with contents
[
commctrl.lib coredll.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"ARMRel/XCSoar.pdb" /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib" /out:"ARMRel/XCSoar.exe" /subsystem:windowsce,3.00 /align:"4096" /MACHINE:ARM 
".\ARMRel\Airspace.obj"
".\ARMRel\Calculations.obj"
".\ARMRel\Dialogs.obj"
".\ARMRel\Logger.obj"
".\ARMRel\MapWindow.obj"
".\ARMRel\McReady.obj"
".\ARMRel\Parser.obj"
".\ARMRel\Port.obj"
".\ARMRel\Process.obj"
".\ARMRel\StdAfx.obj"
".\ARMRel\Utils.obj"
".\ARMRel\Waypointparser.obj"
".\ARMRel\XCSoar.obj"
".\ARMRel\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP212.tmp"
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
XCSoar.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoar - Win32 (WCE ARM) Debug--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"ARMDbg/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "DEBUG" /d "WIN32_PLATFORM_PSPC" /d "ARM" /d "_ARM_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP215.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "ARM" /D "_ARM_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "UNICODE" /D "_UNICODE" /Fp"ARMDbg/XCSoar.pch" /Yu"stdafx.h" /Fo"ARMDbg/" /Fd"ARMDbg/" /MC /c 
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
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP215.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP216.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "ARM" /D "_ARM_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "UNICODE" /D "_UNICODE" /Fp"ARMDbg/XCSoar.pch" /Yc"stdafx.h" /Fo"ARMDbg/" /Fd"ARMDbg/" /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC\XCSoar\StdAfx.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP216.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP217.tmp" with contents
[
commctrl.lib coredll.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:yes /pdb:"ARMDbg/XCSoar.pdb" /debug /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib" /out:"ARMDbg/XCSoar.exe" /subsystem:windowsce,3.00 /align:"4096" /MACHINE:ARM 
".\ARMDbg\Airspace.obj"
".\ARMDbg\Calculations.obj"
".\ARMDbg\Dialogs.obj"
".\ARMDbg\Logger.obj"
".\ARMDbg\MapWindow.obj"
".\ARMDbg\McReady.obj"
".\ARMDbg\Parser.obj"
".\ARMDbg\Port.obj"
".\ARMDbg\Process.obj"
".\ARMDbg\StdAfx.obj"
".\ARMDbg\Utils.obj"
".\ARMDbg\Waypointparser.obj"
".\ARMDbg\XCSoar.obj"
".\ARMDbg\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP217.tmp"
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
XCSoar.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoar - Win32 (WCE SH3) Debug--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"SH3Dbg/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "DEBUG" /d "WIN32_PLATFORM_PSPC" /d "SHx" /d "SH3" /d "_SH3_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP21A.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "UNICODE" /D "_UNICODE" /Fp"SH3Dbg/XCSoar.pch" /Yu"stdafx.h" /Fo"SH3Dbg/" /Fd"SH3Dbg/" /MC /c 
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
Creating command line "shcl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP21A.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP21B.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "UNICODE" /D "_UNICODE" /Fp"SH3Dbg/XCSoar.pch" /Yc"stdafx.h" /Fo"SH3Dbg/" /Fd"SH3Dbg/" /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC\XCSoar\StdAfx.cpp"
]
Creating command line "shcl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP21B.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP21C.tmp" with contents
[
commctrl.lib coredll.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:yes /pdb:"SH3Dbg/XCSoar.pdb" /debug /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib" /out:"SH3Dbg/XCSoar.exe" /subsystem:windowsce,3.00 /MACHINE:SH3 
".\SH3Dbg\Airspace.obj"
".\SH3Dbg\Calculations.obj"
".\SH3Dbg\Dialogs.obj"
".\SH3Dbg\Logger.obj"
".\SH3Dbg\MapWindow.obj"
".\SH3Dbg\McReady.obj"
".\SH3Dbg\Parser.obj"
".\SH3Dbg\Port.obj"
".\SH3Dbg\Process.obj"
".\SH3Dbg\StdAfx.obj"
".\SH3Dbg\Utils.obj"
".\SH3Dbg\Waypointparser.obj"
".\SH3Dbg\XCSoar.obj"
".\SH3Dbg\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP21C.tmp"
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
XCSoar.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoar - Win32 (WCE MIPS) Debug--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"MIPSDbg/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "DEBUG" /d "WIN32_PLATFORM_PSPC" /d "MIPS" /d "_MIPS_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP21F.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "UNICODE" /D "_UNICODE" /Fp"MIPSDbg/XCSoar.pch" /Yu"stdafx.h" /Fo"MIPSDbg/" /Fd"MIPSDbg/" /MC /c 
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
Creating command line "clmips.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP21F.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP220.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "UNICODE" /D "_UNICODE" /Fp"MIPSDbg/XCSoar.pch" /Yc"stdafx.h" /Fo"MIPSDbg/" /Fd"MIPSDbg/" /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC\XCSoar\StdAfx.cpp"
]
Creating command line "clmips.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP220.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP221.tmp" with contents
[
commctrl.lib coredll.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:yes /pdb:"MIPSDbg/XCSoar.pdb" /debug /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib" /out:"MIPSDbg/XCSoar.exe" /subsystem:windowsce,3.00 /MACHINE:MIPS 
".\MIPSDbg\Airspace.obj"
".\MIPSDbg\Calculations.obj"
".\MIPSDbg\Dialogs.obj"
".\MIPSDbg\Logger.obj"
".\MIPSDbg\MapWindow.obj"
".\MIPSDbg\McReady.obj"
".\MIPSDbg\Parser.obj"
".\MIPSDbg\Port.obj"
".\MIPSDbg\Process.obj"
".\MIPSDbg\StdAfx.obj"
".\MIPSDbg\Utils.obj"
".\MIPSDbg\Waypointparser.obj"
".\MIPSDbg\XCSoar.obj"
".\MIPSDbg\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP221.tmp"
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
XCSoar.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoar - Win32 (WCE x86em) Debug--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"X86EMDbg/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "DEBUG" /d "WIN32_PLATFORM_PSPC" /d "_X86_" /d "x86" /d "i486" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP224.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "i486" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32" /D "STRICT" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "WIN32_PLATFORM_PSPC" /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "x86" /Fp"X86EMDbg/XCSoar.pch" /Yu"stdafx.h" /Fo"X86EMDbg/" /Fd"X86EMDbg/" /Gz /c 
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
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP224.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP225.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "i486" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32" /D "STRICT" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "WIN32_PLATFORM_PSPC" /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "x86" /Fp"X86EMDbg/XCSoar.pch" /Yc"stdafx.h" /Fo"X86EMDbg/" /Fd"X86EMDbg/" /Gz /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC\XCSoar\StdAfx.cpp"
]
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP225.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP226.tmp" with contents
[
commctrl.lib coredll.lib corelibc.lib aygshell.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /incremental:yes /pdb:"X86EMDbg/XCSoar.pdb" /debug /nodefaultlib:"OLDNAMES.lib" /nodefaultlib:libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib /out:"X86EMDbg/XCSoar.exe" /windowsce:emulation /MACHINE:IX86 
".\X86EMDbg\Airspace.obj"
".\X86EMDbg\Calculations.obj"
".\X86EMDbg\Dialogs.obj"
".\X86EMDbg\Logger.obj"
".\X86EMDbg\MapWindow.obj"
".\X86EMDbg\McReady.obj"
".\X86EMDbg\Parser.obj"
".\X86EMDbg\Port.obj"
".\X86EMDbg\Process.obj"
".\X86EMDbg\StdAfx.obj"
".\X86EMDbg\Utils.obj"
".\X86EMDbg\Waypointparser.obj"
".\X86EMDbg\XCSoar.obj"
".\X86EMDbg\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP226.tmp"
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
XCSoar.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoar - Win32 (WCE SH3) Release--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"SH3Rel/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WIN32_PLATFORM_PSPC" /d "SHx" /d "SH3" /d "_SH3_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP229.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /Fp"SH3Rel/XCSoar.pch" /Yu"stdafx.h" /Fo"SH3Rel/" /Oxs /MC /c 
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
Creating command line "shcl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP229.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP22A.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /Fp"SH3Rel/XCSoar.pch" /Yc"stdafx.h" /Fo"SH3Rel/" /Oxs /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC\XCSoar\StdAfx.cpp"
]
Creating command line "shcl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP22A.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP22B.tmp" with contents
[
commctrl.lib coredll.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"SH3Rel/XCSoar.pdb" /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib" /out:"SH3Rel/XCSoar.exe" /subsystem:windowsce,3.00 /MACHINE:SH3 
".\SH3Rel\Airspace.obj"
".\SH3Rel\Calculations.obj"
".\SH3Rel\Dialogs.obj"
".\SH3Rel\Logger.obj"
".\SH3Rel\MapWindow.obj"
".\SH3Rel\McReady.obj"
".\SH3Rel\Parser.obj"
".\SH3Rel\Port.obj"
".\SH3Rel\Process.obj"
".\SH3Rel\StdAfx.obj"
".\SH3Rel\Utils.obj"
".\SH3Rel\Waypointparser.obj"
".\SH3Rel\XCSoar.obj"
".\SH3Rel\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP22B.tmp"
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
XCSoar.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoar - Win32 (WCE x86em) Release--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"X86EMRel/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WIN32_PLATFORM_PSPC" /d "_X86_" /d "x86" /d "i486" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP22E.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32" /D "STRICT" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "WIN32_PLATFORM_PSPC" /D "i486" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "x86" /D "NDEBUG" /Fp"X86EMRel/XCSoar.pch" /Yu"stdafx.h" /Fo"X86EMRel/" /Gz /Oxs /c 
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
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP22E.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP22F.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32" /D "STRICT" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "WIN32_PLATFORM_PSPC" /D "i486" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "x86" /D "NDEBUG" /Fp"X86EMRel/XCSoar.pch" /Yc"stdafx.h" /Fo"X86EMRel/" /Gz /Oxs /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC\XCSoar\StdAfx.cpp"
]
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP22F.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP230.tmp" with contents
[
commctrl.lib coredll.lib corelibc.lib aygshell.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /incremental:no /pdb:"X86EMRel/XCSoar.pdb" /nodefaultlib:"OLDNAMES.lib" /nodefaultlib:libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib /out:"X86EMRel/XCSoar.exe" /windowsce:emulation /MACHINE:IX86 
".\X86EMRel\Airspace.obj"
".\X86EMRel\Calculations.obj"
".\X86EMRel\Dialogs.obj"
".\X86EMRel\Logger.obj"
".\X86EMRel\MapWindow.obj"
".\X86EMRel\McReady.obj"
".\X86EMRel\Parser.obj"
".\X86EMRel\Port.obj"
".\X86EMRel\Process.obj"
".\X86EMRel\StdAfx.obj"
".\X86EMRel\Utils.obj"
".\X86EMRel\Waypointparser.obj"
".\X86EMRel\XCSoar.obj"
".\X86EMRel\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP230.tmp"
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
XCSoar.exe - 0 error(s), 0 warning(s)
</pre>
</body>
</html>
