<html>
<body>
<pre>
<h1>Build Log</h1>
<h3>
--------------------Configuration: XCSoar - Win32 (WCE x86) Debug--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"X86Dbg/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "DEBUG" /d "WIN32_PLATFORM_PSPC=310" /d "_X86_" /d "x86" /d "_i386_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP25E.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "_i386_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC=310" /D "i_386_" /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "x86" /Fp"X86Dbg/XCSoar.pch" /Yu"stdafx.h" /Fo"X86Dbg/" /Fd"X86Dbg/" /Gs8192 /GF /c 
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
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP25E.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP25F.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "_i386_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC=310" /D "i_386_" /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "x86" /Fp"X86Dbg/XCSoar.pch" /Yc"stdafx.h" /Fo"X86Dbg/" /Fd"X86Dbg/" /Gs8192 /GF /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC2002\XCSoar\StdAfx.cpp"
]
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP25F.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP260.tmp" with contents
[
commctrl.lib coredll.lib corelibc.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:yes /pdb:"X86Dbg/XCSoar.pdb" /debug /nodefaultlib:"OLDNAMES.lib" /nodefaultlib:libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib /out:"X86Dbg/XCSoar.exe" /subsystem:windowsce,3.00 /MACHINE:IX86 
".\X86Dbg\Airspace.obj"
".\X86Dbg\Calculations.obj"
".\X86Dbg\Dialogs.obj"
".\X86Dbg\Logger.obj"
".\X86Dbg\MapWindow.obj"
".\X86Dbg\McReady.obj"
".\X86Dbg\Parser.obj"
".\X86Dbg\Port.obj"
".\X86Dbg\Process.obj"
".\X86Dbg\StdAfx.obj"
".\X86Dbg\Utils.obj"
".\X86Dbg\Waypointparser.obj"
".\X86Dbg\XCSoar.obj"
".\X86Dbg\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP260.tmp"
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
Creating command line "rc.exe /l 0x409 /fo"ARMRel/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WIN32_PLATFORM_PSPC=310" /d "ARM" /d "_ARM_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP263.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC=310" /D "ARM" /D "_ARM_" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /FR"ARMRel/" /Fp"ARMRel/XCSoar.pch" /Yu"stdafx.h" /Fo"ARMRel/" /Oxs /MC /c 
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
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP263.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP264.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC=310" /D "ARM" /D "_ARM_" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /FR"ARMRel/" /Fp"ARMRel/XCSoar.pch" /Yc"stdafx.h" /Fo"ARMRel/" /Oxs /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC2002\XCSoar\StdAfx.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP264.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP265.tmp" with contents
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
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP265.tmp"
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
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP268.tmp" with contents
[
/nologo /o"ARMRel/XCSoar.bsc" 
".\ARMRel\StdAfx.sbr"
".\ARMRel\Airspace.sbr"
".\ARMRel\Calculations.sbr"
".\ARMRel\Dialogs.sbr"
".\ARMRel\Logger.sbr"
".\ARMRel\MapWindow.sbr"
".\ARMRel\McReady.sbr"
".\ARMRel\Parser.sbr"
".\ARMRel\Port.sbr"
".\ARMRel\Process.sbr"
".\ARMRel\Utils.sbr"
".\ARMRel\Waypointparser.sbr"
".\ARMRel\XCSoar.sbr"]
Creating command line "bscmake.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP268.tmp"
Creating browse info file...
<h3>Output Window</h3>




<h3>Results</h3>
XCSoar.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoar - Win32 (WCE x86) Release--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"X86Rel/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "WIN32_PLATFORM_PSPC=310" /d "_X86_" /d "x86" /d "_i386_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP269.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC=310" /D "_i386_" /D UNDER_CE=300 /D "i_386_" /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "x86" /D "NDEBUG" /FR"X86Rel/" /Fp"X86Rel/XCSoar.pch" /Yu"stdafx.h" /Fo"X86Rel/" /Gs8192 /GF /Oxs /c 
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
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP269.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP26A.tmp" with contents
[
/nologo /W3 /I "..\..\Common\Header" /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC=310" /D "_i386_" /D UNDER_CE=300 /D "i_386_" /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "x86" /D "NDEBUG" /FR"X86Rel/" /Fp"X86Rel/XCSoar.pch" /Yc"stdafx.h" /Fo"X86Rel/" /Gs8192 /GF /Oxs /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC2002\XCSoar\StdAfx.cpp"
]
Creating command line "cl.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP26A.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP26B.tmp" with contents
[
commctrl.lib coredll.lib corelibc.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"X86Rel/XCSoar.pdb" /nodefaultlib:"OLDNAMES.lib" /nodefaultlib:libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib /out:"X86Rel/XCSoar.exe" /subsystem:windowsce,3.00 /MACHINE:IX86 
".\X86Rel\Airspace.obj"
".\X86Rel\Calculations.obj"
".\X86Rel\Dialogs.obj"
".\X86Rel\Logger.obj"
".\X86Rel\MapWindow.obj"
".\X86Rel\McReady.obj"
".\X86Rel\Parser.obj"
".\X86Rel\Port.obj"
".\X86Rel\Process.obj"
".\X86Rel\StdAfx.obj"
".\X86Rel\Utils.obj"
".\X86Rel\Waypointparser.obj"
".\X86Rel\XCSoar.obj"
".\X86Rel\XCSoar.res"
]
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP26B.tmp"
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
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP26E.tmp" with contents
[
/nologo /o"X86Rel/XCSoar.bsc" 
".\X86Rel\StdAfx.sbr"
".\X86Rel\Airspace.sbr"
".\X86Rel\Calculations.sbr"
".\X86Rel\Dialogs.sbr"
".\X86Rel\Logger.sbr"
".\X86Rel\MapWindow.sbr"
".\X86Rel\McReady.sbr"
".\X86Rel\Parser.sbr"
".\X86Rel\Port.sbr"
".\X86Rel\Process.sbr"
".\X86Rel\Utils.sbr"
".\X86Rel\Waypointparser.sbr"
".\X86Rel\XCSoar.sbr"]
Creating command line "bscmake.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP26E.tmp"
Creating browse info file...
<h3>Output Window</h3>




<h3>Results</h3>
XCSoar.exe - 0 error(s), 0 warning(s)
<h3>
--------------------Configuration: XCSoar - Win32 (WCE ARM) Debug--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"ARMDbg/XCSoar.res" /i "..\..\Common\Header" /i "\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "DEBUG" /d "WIN32_PLATFORM_PSPC=310" /d "ARM" /d "_ARM_" /r "C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\Common\Source\XCSoar.rc"" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP26F.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "ARM" /D "_ARM_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC=310" /D "UNICODE" /D "_UNICODE" /Fp"ARMDbg/XCSoar.pch" /Yu"stdafx.h" /Fo"ARMDbg/" /Fd"ARMDbg/" /MC /c 
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
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP26F.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP270.tmp" with contents
[
/nologo /W3 /Zi /Od /I "..\..\Common\Header" /D "DEBUG" /D "ARM" /D "_ARM_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC=310" /D "UNICODE" /D "_UNICODE" /Fp"ARMDbg/XCSoar.pch" /Yc"stdafx.h" /Fo"ARMDbg/" /Fd"ARMDbg/" /MC /c 
"C:\Documents and Settings\Mike\My Documents\Projects\XCSoar\PPC2002\XCSoar\StdAfx.cpp"
]
Creating command line "clarm.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP270.tmp" 
Creating temporary file "C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP271.tmp" with contents
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
Creating command line "link.exe @C:\DOCUME~1\Mike\LOCALS~1\Temp\RSP271.tmp"
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
