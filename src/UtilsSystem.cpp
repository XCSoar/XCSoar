/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "UtilsSystem.hpp"
#include "UtilsText.hpp"
#include "Defines.h"
#include "Interface.hpp"
#include "Asset.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Simulator.hpp"
#include "Profile.hpp"

#ifdef PNA
#include "LogFile.hpp"
#include "SettingsUser.hpp"
#include "Audio/WaveThread.h"
#endif

#if defined(WIN32) && !defined(__GNUC__)
#if defined(CECORE)
#include "winbase.h"
#endif
#ifndef WINDOWSPC
#include "projects.h"
#endif
#endif

#include <assert.h>
#include <tchar.h>

#ifdef HAVE_POSIX
#include <sys/statvfs.h>
#include <sys/stat.h>
#endif

#ifdef WINDOWSPC
int SCREENWIDTH = 640;
int SCREENHEIGHT = 480;
#endif

long CheckFreeRam(void) {
#ifdef WIN32
  MEMORYSTATUS memInfo;
  // Program memory
  memInfo.dwLength = sizeof(memInfo);
  GlobalMemoryStatus(&memInfo);

  //	   memInfo.dwTotalPhys,
  //	   memInfo.dwAvailPhys,
  //	   memInfo.dwTotalPhys- memInfo.dwAvailPhys);

  return memInfo.dwAvailPhys;
#else /* !WIN32 */
  return 64 * 1024 * 1024; // XXX
#endif /* !WIN32 */
}

#ifdef WINDOWSPC
#ifdef _DEBUG
_CrtMemState memstate_s1;
#endif
#endif

void MemCheckPoint()
{
#ifdef WINDOWSPC
#ifdef _DEBUG
  _CrtMemCheckpoint(&memstate_s1);
#endif
#endif
}

void MemLeakCheck() {
#ifdef WINDOWSPC
#ifdef _DEBUG
  _CrtMemState memstate_s2, memstate_s3;

   // Store a 2nd memory checkpoint in s2
   _CrtMemCheckpoint(&memstate_s2);

   if ( _CrtMemDifference(&memstate_s3, &memstate_s1, &memstate_s2)) {
     _CrtMemDumpStatistics(&memstate_s3);
     _CrtMemDumpAllObjectsSince(&memstate_s1);
   }

  _CrtCheckMemory();
#endif
#endif
}

// This is necessary to be called periodically to get rid of
// memory defragmentation, since on pocket pc platforms there is no
// automatic defragmentation.
void MyCompactHeaps() {
#ifdef WIN32
#if defined(WINDOWSPC) || (defined(GNAV) && !defined(__GNUC__))
  HeapCompact(GetProcessHeap(), 0);
#else
  typedef DWORD (_stdcall *CompactAllHeapsFn) (void);
  static CompactAllHeapsFn CompactAllHeaps = NULL;
  static bool init = false;
  if (!init) {
    // get the pointer to the function
    CompactAllHeaps = (CompactAllHeapsFn)GetProcAddress(
        LoadLibrary(_T("coredll.dll")), _T("CompactAllHeaps"));
    init = true;
  }
  if (CompactAllHeaps)
    CompactAllHeaps();
#endif
#endif /* WIN32 */
}

/**
 * Calculates the free disk space for the given path
 * @param path The path defining the "drive" to look on
 * @return Number of KiB free on the destination drive
 */
unsigned long FindFreeSpace(const TCHAR *path) {
#ifdef HAVE_POSIX
  struct statvfs s;
  if (statvfs(path, &s) < 0)
    return 0;
  return s.f_bsize * s.f_bavail;
#else /* !HAVE_POSIX */
  ULARGE_INTEGER FreeBytesAvailableToCaller;
  ULARGE_INTEGER TotalNumberOfBytes;
  ULARGE_INTEGER TotalNumberOfFreeBytes;
  if (GetDiskFreeSpaceEx(path,
                         &FreeBytesAvailableToCaller,
                         &TotalNumberOfBytes,
                         &TotalNumberOfFreeBytes)) {
    return FreeBytesAvailableToCaller.LowPart / 1024;
  } else
    return 0;
#endif /* !HAVE_POSIX */
}

/**
 * Creates a new directory in the home directory, if it doesn't exist yet
 * @param filename Name of the new directory
 */
void CreateDirectoryIfAbsent(const TCHAR *filename) {
  TCHAR fullname[MAX_PATH];

  LocalPath(fullname, filename);

#ifdef HAVE_POSIX
  mkdir(filename, 0777);
#else /* !HAVE_POSIX */
  DWORD fattr = GetFileAttributes(fullname);

  if ((fattr != 0xFFFFFFFF) &&
      (fattr & FILE_ATTRIBUTE_DIRECTORY)) {
    // directory exists
  } else
    CreateDirectory(fullname, NULL);

#endif /* !HAVE_POSIX */
}

bool
RotateScreen()
{
#ifdef DM_DISPLAYORIENTATION
  //
  // Change the orientation of the screen
  //
  if (is_altair()) {
    DEVMODE DeviceMode;

    memset(&DeviceMode, 0, sizeof(DeviceMode));
    DeviceMode.dmSize = sizeof(DeviceMode);
    DeviceMode.dmFields = DM_DISPLAYORIENTATION;
    DeviceMode.dmDisplayOrientation = DMDO_90;
    //Put your desired position right here.

    if (DISP_CHANGE_SUCCESSFUL ==
        ChangeDisplaySettingsEx(NULL, &DeviceMode, NULL, CDS_RESET, NULL))
      return true;
    else
      return false;
  } else {
    return false;
  }
#else
  return false;
#endif
}


#ifdef PNA
// VENTA-ADDON MODELTYPE
//
//	Check if the model type is encoded in the executable file name
//
//  GlobalModelName is a global variable, shown during startup and used for printouts only.
//  In order to know what model you are using, GlobalModelType is used.
//
//  This "smartname" facility is used to override the registry/config Model setup to force
//  a model type to be used, just in case. The model types may not follow strictly those in
//  config menu, nor be updated. Does'nt hurt though.
//
void
SmartGlobalModelType()
{
  GlobalModelType = MODELTYPE_PNA; // default for ifdef PNA by now!

	if (GetGlobalModelName()) {
    ConvToUpper(GlobalModelName);

    if (!_tcscmp(GlobalModelName, _T("PNA"))) {
      GlobalModelType = MODELTYPE_PNA_PNA;
      _tcscpy(GlobalModelName, _T("GENERIC"));
    } else if (!_tcscmp(GlobalModelName, _T("HP31X"))) {
      GlobalModelType = MODELTYPE_PNA_HP31X;
    } else if (!_tcscmp(GlobalModelName, _T("PN6000"))) {
      GlobalModelType = MODELTYPE_PNA_PN6000;
    } else if (!_tcscmp(GlobalModelName, _T("MIO"))) {
      GlobalModelType = MODELTYPE_PNA_MIO;
    } else
      _tcscpy(GlobalModelName, _T("UNKNOWN"));
  } else
    _tcscpy(GlobalModelName, _T("UNKNOWN"));
}

//
// Retrieve from the registry the previous set model type
// This value is defined in xcsoar.h , example> MODELTYPE_PNA_HP31X
// is equivalent to a value=10201 (defined in the header file)
//
void
SetModelType()
{
  TCHAR szProfileInfoBoxModel[] = _T("AppInfoBoxModel");
  int Temp = 0;

  Profile::Get(szProfileInfoBoxModel, Temp);

  if (SetModelName(Temp) != true) {
    LogStartUp(_T("SetModelType ERROR! ModelName returned")
               _T("invalid value <%d> from Registry!"), Temp);
    GlobalModelType = MODELTYPE_PNA_PNA;
  } else {
    GlobalModelType = Temp;
  }

  LogStartUp(_T("SetModelType: Name=<%s> Type=%d"), GlobalModelName,
             GlobalModelType);
}

// Parse a MODELTYPE value and set the equivalent model name.
// If the modeltype is invalid or not yet handled, assume that
// the user changed it in the registry or in the profile, and
// correct the error returning false: this will force a Generic Type.
bool
SetModelName(DWORD Temp)
{
  switch (Temp) {
  case MODELTYPE_PNA_PNA:
    _tcscpy(GlobalModelName, _T("GENERIC"));
    return true;

  case MODELTYPE_PNA_HP31X:
    _tcscpy(GlobalModelName, _T("HP31X"));
    return true;

  case MODELTYPE_PNA_PN6000:
    _tcscpy(GlobalModelName, _T("PN6000"));
    return true;

  case MODELTYPE_PNA_MIO:
    _tcscpy(GlobalModelName, _T("MIO"));
    return true;

  case MODELTYPE_PNA_MEDION_P5:
    _tcscpy(GlobalModelName, _T("MEDION P5"));
    return true;

  case MODELTYPE_PNA_NOKIA_500:
    _tcscpy(GlobalModelName, _T("NOKIA500"));
    return true;

  default:
    _tcscpy(GlobalModelName, _T("UNKNOWN"));
    return false;
  }
}

#endif


#if defined(PNA) || defined(FIVV)  // VENTA-ADDON gmfpathname & C.

/*
	Get pathname & c. from GetModuleFilename (gmf)
	In case of problems, always return \ERRORxx\  as path name
	It will be displayed at startup and users will know that
	something is wrong reporting the error code to us.
	Approach not followed: It works but we don't know why
	Approach followed: It doesn't work and we DO know why

	These are temporary solutions to be improved
 */

/**
 * gmfpathname returns the pathname of the current executed program, with
 * leading and trailing slash
 * examples:  \sdmmc\   \SD CARD\
 * In case of double slash, it is assumed currently as a single "\" .
 */
const TCHAR*
gmfpathname()
{
  TCHAR gmfpathname_buffer[MAX_PATH];
  TCHAR *p;

  if (GetModuleFileName(NULL, gmfpathname_buffer, MAX_PATH) <= 0)
    return(_T("\\ERROR_01\\") );

  if (gmfpathname_buffer[0] != '\\' )
    return(_T("\\ERROR_02\\"));

  // truncate for safety
  gmfpathname_buffer[MAX_PATH - 1] = '\0';

  for (p = gmfpathname_buffer + 1; *p != '\0'; p++)
    // search for the very first "\"
    if (*p == '\\')
      break;

  if (*p == '\0')
    return (_T("\\ERROR_03\\"));

  *++p = '\0';

  return gmfpathname_buffer;
}

/**
 * gmfbasename returns the filename of the current executed program, without leading path.
 * Example:  xcsoar.exe
 */
const TCHAR*
gmfbasename()
{
  TCHAR gmfbasename_buffer[MAX_PATH];
  TCHAR *p, *lp;

  if (GetModuleFileName(NULL, gmfbasename_buffer, MAX_PATH) <= 0)
    return _T("ERROR_04");

  if (gmfbasename_buffer[0] != '\\')
    return _T("ERROR_05");

  for (p = gmfbasename_buffer + 1, lp = NULL; *p != '\0'; p++) {
    if (*p == '\\')
      lp = ++p;
  }
  return lp;
}

/*
 *	A little hack in the executable filename: if it contains an
 *	underscore, then the following chars up to the .exe is
 *	considered a modelname
 *  Returns 0 if failed, 1 if name found
 */
int
GetGlobalModelName()
{
  TCHAR modelname_buffer[MAX_PATH];
  TCHAR *p, *lp, *np;

  _tcscpy(GlobalModelName, _T(""));

  if (GetModuleFileName(NULL, modelname_buffer, MAX_PATH) <= 0)
    return 0;

  if (modelname_buffer[0] != '\\' )
    return 0;

  for (p = modelname_buffer + 1, lp = NULL; *p != '\0'; p++) {
    if (*p == '\\')
      lp = ++p;
  }
  // assuming \sd\path\xcsoar_pna.exe  we are now at \xcsoar..

  for (p = lp, np = NULL; *p != '\0'; p++) {
    if (*p == '_') {
      np = ++p;
      break;
    }
  }
  // assuming xcsoar_pna.exe we are now at _pna..

  if (np == NULL)
    return 0;

  for (p = np, lp = NULL; *p != '\0'; p++) {
    if (*p == '.') {
      lp = p;
      break;
    }
  } // we found the . in pna.exe

  if (lp == NULL)
    return 0;

  // we cut .exe
  *lp = '\0';

  _tcscpy(GlobalModelName, np);

  return 1;  // we say ok
}

#endif   // PNA


#ifdef PNA
/**
 * SetBacklight for PNA devices. There is no standard way of managing backlight on CE,
 * and every device may have different value names and settings. Microsoft did not set
 * a standard and thus we need a custom solution for each device.
 * But the approach is always the same: change a value and call an event.
 * We do this in XCSoar.cpp at the beginning, no need to make these settings configurable:
 * max brightness and no timeout if on power is the rule. Otherwise, do it manually..
 */
bool
SetBacklight()
{
  HKEY hKey;
  DWORD Disp = 0;
  HRESULT hRes;
  bool doevent = false;

  if (CommonInterface::EnableAutoBacklight == false)
    return false;

  hRes = RegOpenKeyEx(HKEY_CURRENT_USER, _T("ControlPanel\\Backlight"), 0, 0, &hKey);
  if (hRes != ERROR_SUCCESS)
    return false;

  switch (GlobalModelType) {
  case MODELTYPE_PNA_HP31X:
    // max backlight
    Disp = 20;

    // currently we ignore hres, if registry entries are
    // spoiled out user is already in deep troubles
    hRes = RegSetValueEx(hKey, _T("BackLightCurrentACLevel"), 0, REG_DWORD,
                         (LPBYTE) & Disp, sizeof(DWORD));
    hRes = RegSetValueEx(hKey, _T("BackLightCurrentBatteryLevel"), 0,
                         REG_DWORD, (LPBYTE) & Disp, sizeof(DWORD));
    hRes = RegSetValueEx(hKey, _T("TotalLevels"), 0, REG_DWORD,
                         (LPBYTE) & Disp, sizeof(DWORD));

    Disp = 0;
    hRes = RegSetValueEx(hKey, _T("UseExt"), 0, REG_DWORD,
                         (LPBYTE) & Disp, sizeof(DWORD));

    RegDeleteValue(hKey, _T("ACTimeout"));
    doevent = true;
    break;

  default:
    doevent = false;
    break;
  }

  RegCloseKey(hKey);
  if (doevent == false)
    return false;

  HANDLE BLEvent = CreateEvent(NULL, false, false, _T("BacklightChangeEvent"));
  if (SetEvent(BLEvent) == 0)
    doevent = false;
  else
    CloseHandle(BLEvent);

  return doevent;
}

bool
SetSoundVolume()
{
  if (CommonInterface::EnableAutoSoundVolume == false)
    return false;

/*
 * This does not work, dunno why
 *
  HKEY    hKey;
  DWORD   Disp=0;
  HRESULT hRes;

  hRes = RegOpenKeyEx(HKEY_CURRENT_USER, _T("ControlPanel\\Volume"), 0,  0, &hKey);
  if (hRes != ERROR_SUCCESS) return false;
  switch (GlobalModelType)
  {
	case MODELTYPE_PNA_HP31X:
		Disp=0xFFFFFFFF; // max volume
		hRes = RegSetValueEx(hKey, _T("Volume"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		Disp=65538;
		hRes = RegSetValueEx(hKey, _T("Screen"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		Disp=0;
		hRes = RegSetValueEx(hKey, _T("Key"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		Disp=7;
		hRes = RegSetValueEx(hKey, _T("Mute"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0xF2, 0);
	        RegCloseKey(hKey);
		break;

	default:
		break;
  }
 */

  // should we enter critical section ?  probably...
  waveOutSetVolume(0, 0xffff); // this is working for all platforms

  return true;
}

#endif

#if defined(FIVV) || defined(PNA)
// VENTA2-ADDON fonts install
/*
 * Get the localpath, enter XCSoarData/Config, see if there are fonts to copy,
 * check that they have not already been copied in \Windows\Fonts,
 * and eventually copy everything in place.
 *
 * Returns: 0 if OK .
 * 1 - n other errors not really needed to handle. See below
 *
 * These are currently fonts used by PDA:
 *
 * DejaVuSansCondensed2.ttf
 * DejaVuSansCondensed-Bold2.ttf
 * DejaVuSansCondensed-BoldOblique2.ttf
 * DejaVuSansCondensed-Oblique2.ttf
 */
short
InstallFonts()
{
  TCHAR srcdir[MAX_PATH];
  TCHAR dstdir[MAX_PATH];
  TCHAR srcfile[MAX_PATH];
  TCHAR dstfile[MAX_PATH];

  _stprintf(srcdir, _T("%s%S\\Fonts"), gmfpathname(), XCSDATADIR);
  _stprintf(dstdir, _T("\\Windows\\Fonts"), gmfpathname());


  if (GetFileAttributes(srcdir) != FILE_ATTRIBUTE_DIRECTORY)
    return 1;
  if (GetFileAttributes(dstdir) != FILE_ATTRIBUTE_DIRECTORY)
    return 2;

  _stprintf(srcfile,_T("%s\\DejaVuSansCondensed2.ttf"), srcdir);
  _stprintf(dstfile,_T("%s\\DejaVuSansCondensed2.ttf"), dstdir);
  if (GetFileAttributes(dstfile) != 0xffffffff)
    return 4;
  if (!CopyFile(srcfile, dstfile, true))
    return 5;

  // From now on we attempt to copy without overwriting
  _stprintf(srcfile,_T("%s\\DejaVuSansCondensed-Bold2.ttf"), srcdir);
  _stprintf(dstfile,_T("%s\\DejaVuSansCondensed-Bold2.ttf"), dstdir);
  CopyFile(srcfile, dstfile, true);

  _stprintf(srcfile,_T("%s\\DejaVuSansCondensed-BoldOblique2.ttf"), srcdir);
  _stprintf(dstfile,_T("%s\\DejaVuSansCondensed-BoldOblique2.ttf"), dstdir);
  CopyFile(srcfile, dstfile, true);

  _stprintf(srcfile,_T("%s\\DejaVuSansCondensed-Oblique2.ttf"), srcdir);
  _stprintf(dstfile,_T("%s\\DejaVuSansCondensed-Oblique2.ttf"), dstdir);
  CopyFile(srcfile, dstfile, true);

  return 0;
}

/*
 * Check that XCSoarData exist where it should be
 * Return false if error, true if Ok
 */
bool CheckDataDir() {
	TCHAR srcdir[MAX_PATH];
  _stprintf(srcdir, _T("%s%S"), gmfpathname(), XCSDATADIR);

  if (GetFileAttributes(srcdir) != FILE_ATTRIBUTE_DIRECTORY)
	  return false;
	return true;
}
#endif


/**
 * Reads and parses arguments/options from the command line
 * @param CommandLine not in use
 */
void
ParseCommandLine(LPCTSTR CommandLine)
{
  TCHAR extrnProfileFile[MAX_PATH];
  extrnProfileFile[0] = 0;

#ifdef SIMULATOR_AVAILABLE
  bool bSimTemp=false;
  bSimTemp = _tcsstr(CommandLine, _T("-simulator")) != NULL;
  if (bSimTemp) {
    global_simulator_flag=true;
    sim_set_in_cmd_line_flag=true;
  }
  bSimTemp = _tcsstr(CommandLine, _T("-fly")) != NULL;
  if (bSimTemp) {
    global_simulator_flag=false;
    sim_set_in_cmd_line_flag=true;
  }
#endif

  const TCHAR *pC, *pCe;

  pC = _tcsstr(CommandLine, _T("-profile="));
  if (pC != NULL) {
    pC += strlen("-profile=");
    if (*pC == '"') {
      pC++;
      pCe = pC;
      while (*pCe != '"' && *pCe != '\0')
        pCe++;
    } else {
      pCe = pC;
      while (*pCe != ' ' && *pCe != '\0')
        pCe++;
    }
    if (pCe != NULL && pCe - 1 > pC) {
      _tcsncpy(extrnProfileFile, pC, pCe - pC);
      extrnProfileFile[pCe - pC] = '\0';
    }
  }

  Profile::SetFiles(extrnProfileFile);

#ifdef WINDOWSPC
  SCREENWIDTH = 640;
  SCREENHEIGHT = 480;

  #if defined(SCREENWIDTH_)
  SCREENWIDTH = SCREENWIDTH_;
  #endif
  #if defined(SCREENHEIGHT_)
  SCREENHEIGHT = SCREENHEIGHT_;
  #endif

  pC = _tcsstr(CommandLine, _T("-800x480"));
  if (pC != NULL) {
    SCREENWIDTH = 800;
    SCREENHEIGHT = 480;
  }

  pC = _tcsstr(CommandLine, _T("-480x272"));
  if (pC != NULL) {
    SCREENWIDTH = 480;
    SCREENHEIGHT = 272;
  }

  pC = _tcsstr(CommandLine, _T("-480x234"));
  if (pC != NULL) {
    SCREENWIDTH = 480;
    SCREENHEIGHT = 234;
  }

  pC = _tcsstr(CommandLine, _T("-portrait"));
  if (pC != NULL) {
    SCREENWIDTH = 480;
    SCREENHEIGHT = 640;
  }

  pC = _tcsstr(CommandLine, _T("-square"));
  if (pC != NULL) {
    SCREENWIDTH = 480;
    SCREENHEIGHT = 480;
  }

  pC = _tcsstr(CommandLine, _T("-small"));
  if (pC != NULL) {
    SCREENWIDTH /= 2;
    SCREENHEIGHT /= 2;
  }

  pC = _tcsstr(CommandLine, _T("-320x240"));
  if (pC != NULL) {
    SCREENWIDTH = 320;
    SCREENHEIGHT = 240;
  }

  pC = _tcsstr(CommandLine, _T("-240x320"));
  if (pC != NULL) {
    SCREENWIDTH = 240;
    SCREENHEIGHT = 320;
  }

  pC = _tcsstr(CommandLine, _T("-240x240"));
  if (pC != NULL) {
    SCREENWIDTH = 240;
    SCREENHEIGHT = 240;
  }
#endif
}

void
StartupLogFreeRamAndStorage()
{
  int freeram = CheckFreeRam() / 1024;
  TCHAR buffer[MAX_PATH];
  LocalPath(buffer);
  int freestorage = FindFreeSpace(buffer);
  LogStartUp(_T("Free ram %d; free storage %d"), freeram, freestorage);
}

WPARAM
TranscodeKey(WPARAM wParam)
{
#ifdef PNA
  // VENTA-ADDON HARDWARE KEYS TRANSCODING

  if (GlobalModelType == MODELTYPE_PNA_HP31X) {
      /*
      if (wParam == 0x7b)
        // VK_APP1
        wParam = 0xc1;
      */

      if (wParam == 0x7b)
        // VK_ESCAPE
        wParam = 0x1b;

      /*
      if (wParam == 0x7b)
        // VK_RIGHT
        wParam = 0x27;
      if (wParam == 0x7b)
        // VK_LEFT
        wParam=0x25;
      */
  } else if (GlobalModelType == MODELTYPE_PNA_PN6000) {
    switch(wParam) {
    case 0x79:					// Upper Silver key short press
      wParam = 0xc1;	// F10 -> APP1
      break;
    case 0x7b:					// Lower Silver key short press
      wParam = 0xc2;	// F12 -> APP2
      break;
    case 0x72:					// Back key plus
      wParam = 0xc3;	// F3  -> APP3
      break;
    case 0x71:					// Back key minus
      wParam = 0xc4;	// F2  -> APP4
      break;
    case 0x7a:					// Upper silver key LONG press
      wParam = 0x70;	// F11 -> F1
      break;
    case 0x7c:					// Lower silver key LONG press
      wParam = 0x71;	// F13 -> F2
      break;
    }
  } else if (GlobalModelType == MODELTYPE_PNA_NOKIA_500) {
    switch(wParam) {
    case 0xc1:
      wParam = 0x0d;	// middle key = enter
      break;
    case 0xc5:
      wParam = 0x26;	// + key = pg Up
      break;
    case 0xc6:
      wParam = 0x28;	// - key = pg Down
      break;
    }
  } else if (GlobalModelType == MODELTYPE_PNA_MEDION_P5) {
    switch(wParam) {
    case 0x79:
      wParam = 0x0d;	// middle key = enter
      break;
    case 0x75:
      wParam = 0x26;	// + key = pg Up
      break;
    case 0x76:
      wParam = 0x28;	// - key = pg Down
      break;
    }
  }
#endif

  return wParam;
}

/**
 * Returns the screen dimension rect to be used
 * @return The screen dimension rect to be used
 */
RECT SystemWindowSize(void) {
  RECT WindowSize;

#ifdef WINDOWSPC
  WindowSize.right = SCREENWIDTH + 2 * GetSystemMetrics(SM_CXFIXEDFRAME);
  WindowSize.left = (GetSystemMetrics(SM_CXSCREEN) - WindowSize.right) / 2;
  WindowSize.bottom = SCREENHEIGHT + 2 * GetSystemMetrics(SM_CYFIXEDFRAME) +
                      GetSystemMetrics(SM_CYCAPTION);
  WindowSize.top = (GetSystemMetrics(SM_CYSCREEN) - WindowSize.bottom) / 2;
#else
  WindowSize.left = 0;
  WindowSize.top = 0;

  #ifdef WIN32
  WindowSize.right = GetSystemMetrics(SM_CXSCREEN);
  WindowSize.bottom = GetSystemMetrics(SM_CYSCREEN);
  #else /* !WIN32 */
  /// @todo implement this properly for SDL/UNIX
  WindowSize.right = 640;
  WindowSize.bottom = 480;
  #endif /* !WIN32 */

#endif

  return WindowSize;
}
