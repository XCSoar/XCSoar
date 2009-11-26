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

#include "Asset.hpp"
#include "Registry.hpp"
#include "UtilsText.hpp"
#include "LogFile.hpp"
#include "UtilsSystem.hpp"
#include "LocalPath.hpp"
#include "Sizes.h"

#if defined(WIN32) && !(defined(__GNUC__) && defined(WINDOWSPC))
#include "uniqueid.h"
#endif

// Registration Data
TCHAR strAssetNumber[MAX_LOADSTRING] = TEXT(""); //4G17DW31L0HY");
TCHAR strRegKey[MAX_LOADSTRING] = TEXT("");

static void
ReadCompaqID(void);

static void
ReadUUID(void);

/**
 * Finds the unique ID of this PDA
 */
void ReadAssetNumber(void)
{
  TCHAR val[MAX_PATH];

  val[0]= _T('\0');

  memset(strAssetNumber, 0, MAX_LOADSTRING*sizeof(TCHAR));
  // JMW clear this first just to be safe.

  StartupStore(TEXT("Asset ID: "));

#ifdef WINDOWSPC
  return;
#endif

  GetRegistryString(szRegistryLoggerID, val, 100);
  int ifound = 0;
  int len = _tcslen(val);
  for (int i = 0; i < len; i++) {
    if (((val[i] >= _T('A')) && (val[i] <= _T('Z')))
        ||((val[i] >= _T('0')) && (val[i] <= _T('9')))) {
      strAssetNumber[ifound]= val[i];
      ifound++;
    }
    if (ifound>=3) {
      StartupStore(strAssetNumber);
      StartupStore(TEXT(" (reg)\n"));
      return;
    }
  }

  if(strAssetNumber[0] != '\0') {
    StartupStore(strAssetNumber);
    StartupStore(TEXT(" (?)\n"));
    return;
  }

  ReadCompaqID();
  if(strAssetNumber[0] != '\0') {
    StartupStore(strAssetNumber);
    StartupStore(TEXT(" (compaq)\n"));
    return;
  }

  ReadUUID();
  if(strAssetNumber[0] != '\0') {
    StartupStore(strAssetNumber);
    StartupStore(TEXT(" (uuid)\n"));
    return;
  }

  strAssetNumber[0]= _T('A');
  strAssetNumber[1]= _T('A');
  strAssetNumber[2]= _T('A');

  StartupStore(strAssetNumber);
  StartupStore(TEXT(" (fallback)\n"));

  return;
}

static void
ReadCompaqID(void)
{
#if defined(WIN32) && !defined(WINDOWSPC)
  PROCESS_INFORMATION pi;

  if(strAssetNumber[0] != '\0') {
    return;
  }

  CreateProcess(TEXT("\\windows\\CreateAssetFile.exe"), NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi);

  FILE *file = _tfopen(TEXT("\\windows\\cpqAssetData.dat"), TEXT("rb"));
  if (file == NULL) {
    // MessageBoxX(hWnd, TEXT("Unable to open asset data file."), TEXT("Error!"), MB_OK);
    return;
  }
  fseek(file, 976, SEEK_SET);
  memset(strAssetNumber, 0, 64 * sizeof(TCHAR));
  fread(&strAssetNumber, 64, 1, file);
  fclose(file);
#endif
}

static void
ReadUUID(void)
{
#if defined(WIN32) && !(defined(__GNUC__) && defined(WINDOWSPC))
  BOOL fRes;

#define GUIDBuffsize 100
  unsigned char GUIDbuffer[GUIDBuffsize];

  int eLast=0;
  int i;
  unsigned long uNumReturned=0;
  int iBuffSizeIn=0;
  unsigned long temp, Asset;


  GUID Guid;


  // approach followed: http://blogs.msdn.com/jehance/archive/2004/07/12/181116.aspx
  // 1) send 16 byte buffer - some older devices need this
  // 2) if buffer is wrong size, resize buffer accordingly and retry
  // 3) take first 16 bytes of buffer and process.  Buffer returned may be any size
  // First try exactly 16 bytes, some older PDAs require exactly 16 byte buffer

      #ifdef HAVEEXCEPTIONS
    __try {
      #else
	  strAssetNumber[0]= '\0';
      #endif

	  iBuffSizeIn=sizeof(Guid);
	  memset(GUIDbuffer, 0, iBuffSizeIn);
	  fRes = KernelIoControl(IOCTL_HAL_GET_DEVICEID, 0, 0, GUIDbuffer, iBuffSizeIn, &uNumReturned);
	  if(fRes == FALSE)
	  { // try larger buffer
		  eLast = GetLastError();
		  if (ERROR_INSUFFICIENT_BUFFER != eLast)
		  {
			return;
		  }
		  else
		  { // wrong buffer
			iBuffSizeIn = uNumReturned;
			memset(GUIDbuffer, 0, iBuffSizeIn);
			fRes = KernelIoControl(IOCTL_HAL_GET_DEVICEID, 0, 0, GUIDbuffer, iBuffSizeIn, &uNumReturned);
  			eLast = GetLastError();
			if(FALSE == fRes)
				return;
		  }
	  }

	  // here assume we have data in GUIDbuffer of length uNumReturned
	  memcpy(&Guid,GUIDbuffer, sizeof(Guid));


	  temp = Guid.Data2; temp = temp << 16;
	  temp += Guid.Data3 ;

	  Asset = temp ^ Guid.Data1 ;

	  temp = 0;
	  for(i=0;i<4;i++)
		{
		  temp = temp << 8;
		  temp += Guid.Data4[i];
		}

	  Asset = Asset ^ temp;

	  temp = 0;
	  for(i=0;i<4;i++)
		{
		  temp = temp << 8;
		  temp += Guid.Data4[i+4];
		}

	  Asset = Asset ^ temp;

	  _stprintf(strAssetNumber,TEXT("%08X%08X"),Asset,Guid.Data1 );

#ifdef HAVEEXCEPTIONS
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
	  strAssetNumber[0]= '\0';
  }
#endif
#endif
  return;
}

#if 0
void ReadUUIDold(void)
{
#ifndef __GNUC__
  BOOL fRes;
  DWORD dwBytesReturned =0;
  DEVICE_ID DevID;
  int wSize;
  int i;

  GUID Guid;

  unsigned long temp, Asset;

  memset(&Guid, 0, sizeof(GUID));

  memset(&DevID, 0, sizeof(DEVICE_ID));
  DevID.dwSize = sizeof(DEVICE_ID);

  fRes = KernelIoControl( IOCTL_HAL_GET_DEVICEID, NULL, 0,
			  &DevID, sizeof( DEVICE_ID ), &dwBytesReturned );

  wSize = DevID.dwSize;

  if( (FALSE != fRes) || (ERROR_INSUFFICIENT_BUFFER != GetLastError()))
    return;

  memset(&DevID, 0, sizeof(wSize));
  DevID.dwSize = wSize;

  fRes = KernelIoControl( IOCTL_HAL_GET_DEVICEID, NULL, 0,
			  &DevID, wSize, &dwBytesReturned );

  if((FALSE == fRes) || (ERROR_INSUFFICIENT_BUFFER == GetLastError()) )
    return;

  BYTE* pDat = (BYTE*)&Guid.Data1;
  BYTE* pSrc = (BYTE*)(&DevID) + DevID.dwPresetIDOffset;
  memcpy(pDat, pSrc, DevID.dwPresetIDBytes);
  pDat +=  DevID.dwPresetIDBytes;
  pSrc =  (BYTE*)(&DevID) + DevID.dwPlatformIDOffset;
  memcpy(pDat, pSrc, DevID.dwPlatformIDBytes);

  temp = Guid.Data2; temp = temp << 16;
  temp += Guid.Data3 ;

  Asset = temp ^ Guid.Data1 ;

  temp = 0;
  for(i=0;i<4;i++)
    {
      temp = temp << 8;
      temp += Guid.Data4[i];
    }

  Asset = Asset ^ temp;

  temp = 0;
  for(i=0;i<4;i++)
    {
      temp = temp << 8;
      temp += Guid.Data4[i+4];
    }

  Asset = Asset ^ temp;

  _stprintf(strAssetNumber,TEXT("%08X%08X"),Asset,Guid.Data1 );
  return;
#endif
}
#endif

void InitAsset() {
  if (is_altair()) {
    #ifdef FORCEPORTRAIT
    // JMW testing only for portrait mode of Altair
    RotateScreen();
    #endif
  }

  // VENTA2- delete registries at startup, but not on PC!
  #if defined(FIVV) && ( !defined(WINDOWSPC) || WINDOWSPC==0 )
  if (!is_pna()) {
    RegDeleteKey(HKEY_CURRENT_USER, _T(REGKEYNAME));
  }
  #endif

  #ifdef PNA
    // VENTA2-ADDON MODEL TYPE
    /*
    LocalPath is called for the very first time by CreateDirectoryIfAbsent.
    In order to be able in the future to behave differently for each PNA device
    and maybe also for common PDAs, we need to know the PNA/PDA Model Type
    BEFORE calling LocalPath. This was critical.
     */

    SmartGlobalModelType(); // First we check the exec filename, which
                            // has priority over registry values

    if (!_tcscmp(GlobalModelName, _T("UNKNOWN"))) // Then if there is no smart name...
      SetModelType();                             // get the modeltype from
                                                  // the registry as usual
  #endif

// VENTA2- TODO fix these directories are not used always!
  CreateDirectoryIfAbsent(TEXT(""));  // RLD make sure the LocalPath folder actually exists
  CreateDirectoryIfAbsent(TEXT("persist"));
  CreateDirectoryIfAbsent(TEXT("logs"));
  CreateDirectoryIfAbsent(TEXT("config"));

// VENTA2-ADDON install fonts on PDAs and check XCSoarData existance
  #if defined(FIVV) && ( !defined(WINDOWSPC) || WINDOWSPC==0 )
  //#ifndef PNA

    bool datadir=CheckDataDir();
    if (datadir) StartupStore(TEXT("XCSoarData directory found.\n"));
    else StartupStore(TEXT("ERROR: NO XCSOARDATA DIRECTORY FOUND!\n"));

    StartupStore(TEXT("Check for installing fonts\n"));
    short didfonts=InstallFonts();  // check if really did it, and maybe restart
    TCHAR nTmp[100];
    _stprintf(nTmp,TEXT("InstallFonts() result=%d (0=installed >0 not installed)\n"), didfonts);
    StartupStore(nTmp);

    //#endif
  #endif

  StartupLogFreeRamAndStorage();
}
