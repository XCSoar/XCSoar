# Microsoft Developer Studio Project File - Name="XCSoarPC" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=XCSoarPC - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "XCSoarPC.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "XCSoarPC.mak" CFG="XCSoarPC - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "XCSoarPC - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "XCSoarPC - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "XCSoarPC - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /Od /I "include" /I "..\..\..\Common\Header" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_UNICODE" /D "UNICODE" /D WINDOWSPC=1 /D UNDER_CE=300 /D SCREENWIDTH=480 /D SCREENHEIGHT=640 /D "DISABLEAUDIO" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\..\Common\Header" /d "NDEBUG" /d WINDOWSPC=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib comctl32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "XCSoarPC - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "include" /I "..\..\..\Common\Header" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_UNICODE" /D "UNICODE" /D WINDOWSPC=1 /D UNDER_CE=300 /D SCREENWIDTH=480 /D SCREENHEIGHT=640 /D "DISABLEAUDIO" /FD /GZ /c
# SUBTRACT CPP /X /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\Common\Header" /i "..\..\..\Common\Source" /d "_DEBUG" /d "WINDOWSPC"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "XCSoarPC - Win32 Release"
# Name "XCSoarPC - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\Common\Source\AirfieldDetails.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Airspace.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\AirspaceColourDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Atmosphere.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Calculations.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\DebugLog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\devCAI302.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\devEW.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\device.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Dialogs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\dlgBasicSettings.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\dlgStatistics.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\dlgStatus.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\dlgStatusSystem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\dlgTaskOverview.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\dlgTaskWaypoint.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\dlgTools.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\dlgWayPointDetails.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\dlgWayPointSelect.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\dlgWindSettings.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\GaugeCDI.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\GaugeVarioAltA.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\InfoBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\InfoBoxLayout.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\InputEvents.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\leastsqs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Logger.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\mapbits.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\maperror.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\mapprimitive.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\mapsearch.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\mapshape.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\maptree.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\MapWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\mapxbase.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\McReady.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Message.cpp
# End Source File
# Begin Source File

SOURCE=..\..\BCPP\modDummies.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Parser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Port.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Port2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Process.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\RasterTerrain.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\rscalc.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\STScreenBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Task.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Terrain.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Topology.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\units.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\VarioSound.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\VOIMAGE.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\WaveThread.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Waypointparser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\windanalyser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\windmeasurementlist.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\WindowControls.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\windstore.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\XCSoar.cpp
# End Source File
# Begin Source File

SOURCE=.\XCSoarPC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\xmlParser.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\..\Common\Source\Bitmaps\airspace2.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Bitmaps\automcre.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Bitmaps\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Bitmaps\bitmap12.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Bitmaps\xcsoar.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\XCSoar.rc
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Bitmaps\xcsoars.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Source\Bitmaps\xcsoarswift.ico
# End Source File
# End Group
# Begin Source File

SOURCE="..\..\..\Common\Source\Bitmaps\beep-clear.wav"
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
