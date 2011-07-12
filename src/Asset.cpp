/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Profile/Profile.hpp"
#include "UtilsText.hpp"
#include "LogFile.hpp"
#include "UtilsSystem.hpp"
#include "LocalPath.hpp"
#include "Sizes.h"

#if defined(WIN32) && (!defined(__GNUC__) || defined(_WIN32_WCE))
#include <windows.h>
#include <winioctl.h>
#endif

#include <windef.h> /* for MAX_PATH */

// Registration Data
TCHAR strAssetNumber[MAX_LOADSTRING] = _T(""); //4G17DW31L0HY");

#ifdef HAVE_MODEL_TYPE
ModelType GlobalModelType = MODELTYPE_PNA_PNA;
#endif

static void
ReadCompaqID(void)
{
#if defined(_WIN32_WCE)
  PROCESS_INFORMATION pi;

  if(strAssetNumber[0] != '\0')
    return;

  if (CreateProcess(_T("\\windows\\CreateAssetFile.exe"), NULL, NULL, NULL,
                    FALSE, 0, NULL, NULL, NULL, &pi)) {
    WaitForSingleObject(pi.hProcess, 1000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }

  FILE *file = _tfopen(_T("\\windows\\cpqAssetData.dat"), _T("rb"));
  if (file == NULL) {
    // MessageBoxX(hWnd, _T("Unable to open asset data file."), _T("Error!"), MB_OK);
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
#if defined(_WIN32_WCE) && defined(IOCTL_HAL_GET_DEVICEID) && defined(FILE_DEVICE_HAL)
  BOOL fRes;

#define GUIDBuffsize 100
  unsigned char GUIDbuffer[GUIDBuffsize];

  int eLast = 0;
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

    strAssetNumber[0]= '\0';

    iBuffSizeIn = sizeof(Guid);
    memset(GUIDbuffer, 0, iBuffSizeIn);
    fRes = KernelIoControl(IOCTL_HAL_GET_DEVICEID, 0, 0, GUIDbuffer,
                           iBuffSizeIn, &uNumReturned);
    if(fRes == false) {
      // try larger buffer
      eLast = GetLastError();
      if (ERROR_INSUFFICIENT_BUFFER != eLast)
        return;
      else {
        // wrong buffer
        iBuffSizeIn = uNumReturned;
        memset(GUIDbuffer, 0, iBuffSizeIn);
        fRes = KernelIoControl(IOCTL_HAL_GET_DEVICEID, 0, 0, GUIDbuffer,
                               iBuffSizeIn, &uNumReturned);
        eLast = GetLastError();

        if(fRes == false)
          return;
      }
    }

    // here assume we have data in GUIDbuffer of length uNumReturned
    memcpy(&Guid, GUIDbuffer, sizeof(Guid));

    temp = Guid.Data2;
    temp = temp << 16;
    temp += Guid.Data3;

    Asset = temp ^ Guid.Data1;

    temp = 0;
    for(i = 0; i < 4; i++) {
      temp = temp << 8;
      temp += Guid.Data4[i];
    }

    Asset = Asset ^ temp;

    temp = 0;
    for(i = 0; i < 4; i++) {
      temp = temp << 8;
      temp += Guid.Data4[i + 4];
    }

    Asset = Asset ^ temp;

    _stprintf(strAssetNumber, _T("%08X%08X"), Asset, Guid.Data1);
#endif
}

/**
 * Finds the unique ID of this PDA
 */
void ReadAssetNumber(void)
{
  TCHAR val[MAX_PATH];

  val[0]= _T('\0');

  memset(strAssetNumber, 0, MAX_LOADSTRING * sizeof(TCHAR));
  // JMW clear this first just to be safe.

#ifndef _WIN32_WCE
  return;
#endif

  Profile::Get(szProfileLoggerID, val, 100);
  int ifound = 0;
  int len = _tcslen(val);
  for (int i = 0; i < len; i++) {
    if (((val[i] >= _T('A')) && (val[i] <= _T('Z')))
        || ((val[i] >= _T('0')) && (val[i] <= _T('9')))) {
      strAssetNumber[ifound]= val[i];
      ifound++;
    }
    if (ifound >= 3) {
      LogStartUp(_T("Asset ID: %s (reg)"), strAssetNumber);
      return;
    }
  }

  if(strAssetNumber[0] != '\0') {
    LogStartUp(_T("Asset ID: %s (?)"), strAssetNumber);
    return;
  }

  ReadCompaqID();
  if(strAssetNumber[0] != '\0') {
    LogStartUp(_T("Asset ID: %s (compaq)"), strAssetNumber);
    return;
  }

  ReadUUID();
  if(strAssetNumber[0] != '\0') {
    LogStartUp(_T("Asset ID: %s (uuid)"), strAssetNumber);
    return;
  }

  strAssetNumber[0]= _T('A');
  strAssetNumber[1]= _T('A');
  strAssetNumber[2]= _T('A');

  LogStartUp(_T("Asset ID: %s (fallback)"), strAssetNumber);

  return;
}

void
InitAsset()
{
#ifdef HAVE_MODEL_TYPE
  /*
  LocalPath is called for the very first time by CreateDirectoryIfAbsent.
  In order to be able in the future to behave differently for each PNA device
  and maybe also for common PDAs, we need to know the PNA/PDA Model Type
  BEFORE calling LocalPath. This was critical.
   */

  int Temp = (int)MODELTYPE_PNA_PNA;
  Profile::Get(szProfileAppInfoBoxModel, Temp);
  GlobalModelType = (ModelType)Temp;
  #endif

  // VENTA2- TODO fix these directories are not used always!
  CreateDirectoryIfAbsent(_T(""));  // RLD make sure the LocalPath folder actually exists
  CreateDirectoryIfAbsent(_T("logs"));
  CreateDirectoryIfAbsent(_T("cache"));
}
