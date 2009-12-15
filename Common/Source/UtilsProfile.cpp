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

#include "UtilsText.hpp"
#include "XCSoar.h"
#include "Registry.hpp"
#include "Profile.hpp"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "Asset.hpp"
#include "StringUtil.hpp"

#include <assert.h>

#ifdef ENABLE_UNUSED_CODE
void WriteFileRegistryString(HANDLE hFile, TCHAR *instring) {
    int len;
    char ctempFile[MAX_PATH];
    TCHAR tempFile[MAX_PATH];
    DWORD dwBytesWritten;
    int i;

    tempFile[0]=0;
    for (i=0; i<MAX_PATH; i++) {
      tempFile[i]= 0;
    }
    GetRegistryString(instring, tempFile, MAX_PATH);
#ifdef _UNICODE
    WideCharToMultiByte( CP_ACP, 0, tempFile,
			 _tcslen(tempFile)+1,
			 ctempFile,
			 MAX_PATH, NULL, NULL);
#else
    strcpy(ctempFile, tempFile);
#endif
    for (i=0; i<MAX_PATH; i++) {
      if (ctempFile[i]=='\?') {
	ctempFile[i]=0;
      }
    }
    len = strlen(ctempFile)+1;
    ctempFile[len-1]= '\n';
    WriteFile(hFile,ctempFile,len, &dwBytesWritten, (OVERLAPPED *)NULL);
}
#endif /* ENABLE_UNUSED_CODE */

void WriteProfile(const TCHAR *szFile)
{
  SaveRegistryToFile(szFile);
}

#ifdef ENABLE_UNUSED_CODE
void ReadFileRegistryString(HANDLE hFile, TCHAR *instring) {
    int i;
    TCHAR tempFile[MAX_PATH];

    for (i=0; i<MAX_PATH; i++) {
      tempFile[i]= 0;
    }
    ReadString(hFile, MAX_PATH, tempFile);
    tempFile[_tcslen(tempFile)]= 0;
    SetRegistryString(instring, tempFile);
}
#endif /* ENABLE_UNUSED_CODE */


void ReadProfile(const TCHAR *szFile)
{
  LoadRegistryFromFile(szFile);

  WAYPOINTFILECHANGED = true;
  TERRAINFILECHANGED = true;
  TOPOLOGYFILECHANGED = true;
  AIRSPACEFILECHANGED = true;
  AIRFIELDFILECHANGED = true;
  POLARFILECHANGED = true;

  // assuming all is ok, we can...
  Profile::ReadRegistrySettings();
}


int propGetScaleList(double *List, size_t Size){
  static const TCHAR Name[] = TEXT("ScaleList");
  TCHAR Buffer[128];
  TCHAR *pWClast, *pToken;
  int   Idx = 0;
  double vlast=0;
  double val;

  assert(List != NULL);
  assert(Size > 0);

  SetRegistryString(Name,
   TEXT("0.5,1,2,5,10,20,50,100,150,200,500,1000"));

  if (GetRegistryString(Name, Buffer, sizeof(Buffer)/sizeof(TCHAR)) == 0){

    pToken = strtok_r(Buffer, TEXT(","), &pWClast);

    while(Idx < (int)Size && pToken != NULL){
      val = _tcstod(pToken, NULL);
      if (Idx>0) {
        List[Idx] = (val+vlast)/2;
        Idx++;
      }
      List[Idx] = val;
      Idx++;
      vlast = val;
      pToken = strtok_r(NULL, TEXT(","), &pWClast);
    }

    return(Idx);

  } else {
    return(0);
  }

}


TCHAR startProfileFile[MAX_PATH];
TCHAR defaultProfileFile[MAX_PATH];
TCHAR failsafeProfileFile[MAX_PATH];

void Profile::RestoreRegistry(void) {
  StartupStore(TEXT("Restore registry\n"));
  // load registry backup if it exists
  LoadRegistryFromFile(failsafeProfileFile);
  LoadRegistryFromFile(startProfileFile);
}

void Profile::StoreRegistry(void) {
  StartupStore(TEXT("Store registry\n"));
  // save registry backup first (try a few places)
  SaveRegistryToFile(startProfileFile);
  SaveRegistryToFile(defaultProfileFile);
}

void SetProfileFiles(const TCHAR *override) {
  if (is_altair())
    LocalPath(defaultProfileFile, TEXT("config/xcsoar-registry.prf"));
  else
    LocalPath(defaultProfileFile, TEXT(XCSPROFILE));

  // LocalPath(failsafeProfileFile,TEXT("xcsoar-registry.prf")); VENTA4
  LocalPath(failsafeProfileFile,TEXT(XCSPROFILE));
  _tcscpy(startProfileFile, defaultProfileFile);

  if (!string_is_empty(override))
    _tcsncpy(startProfileFile, override, MAX_PATH-1);
}

#ifdef PNA
void CleanRegistry()
{
   HKEY tKey;
   RegOpenKeyEx(HKEY_CURRENT_USER, szRegistryKey ,0,0,&tKey);

   RegDeleteValue(tKey,_T("CDIWindowFont"));
   RegDeleteValue(tKey,_T("InfoWindowFont"));
   RegDeleteValue(tKey,_T("MapLabelFont"));
   RegDeleteValue(tKey,_T("MapWindowBoldFont"));
   RegDeleteValue(tKey,_T("MapWindowFont"));
   RegDeleteValue(tKey,_T("StatisticsFont"));
   RegDeleteValue(tKey,_T("TitleSmallWindowFont"));
   RegDeleteValue(tKey,_T("TitleWindowFont"));
   RegDeleteValue(tKey,_T("BugsBallastFont"));
   RegDeleteValue(tKey,_T("TeamCodeFont"));

   RegCloseKey(tKey);
}
#endif
