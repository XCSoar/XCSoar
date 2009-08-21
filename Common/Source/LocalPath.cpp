/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "LocalPath.hpp"
#include "Defines.h"

#if defined(PNA) || defined(FIVV)
#include "externs.h" /* for gmfpathname() */
#endif

#include <stdio.h>

// Get local My Documents path - optionally include file to add and location
void LocalPath(TCHAR* buffer, const TCHAR* file, int loc) {
/*

loc = CSIDL_PROGRAMS

File system directory that contains the user's program groups (which
are also file system directories).

CSIDL_PERSONAL               File system directory that serves as a common
                             repository for documents.

CSIDL_PROGRAM_FILES 0x0026   The program files folder.


*/
#if defined(GNAV) && !defined(PCGNAV)
  _tcscpy(buffer,TEXT("\\NOR Flash"));
#elif defined (PNA) && (!defined(WINDOWSPC) || (WINDOWSPC <=0) )
 /*
  * VENTA-ADDON "smartpath" for PNA only
  *
  * (moved up elif from bottom to here to prevent messy behaviour if a
  * PNA exec is loaded on a PPC)
  *
  * For PNAs the localpath is taken from the application exec path
  * example> \sdmmc\bin\Program.exe  results in localpath=\sdmmc\XCSoarData
  *
  * Then the basename is searched for an underscore char, which is
  * used as a separator for getting the model type.  example>
  * program_pna.exe results in GlobalModelType=pna
  *
  */

  /*
   * Force LOCALPATH to be the same of the executing program
   */
  _stprintf(buffer,TEXT("%s%S"),gmfpathname(), XCSDATADIR );
// VENTA2 FIX PC BUG
#elif defined (FIVV) && (!defined(WINDOWSPC) || (WINDOWSPC <=0) )
  _stprintf(buffer,TEXT("%s%S"),gmfpathname(), XCSDATADIR );
#else
  // everything else that's not special
  SHGetSpecialFolderPath(NULL, buffer, loc, false);
  _tcscat(buffer,TEXT("\\"));
  _tcscat(buffer,TEXT(XCSDATADIR));
#endif
  if (_tcslen(file)>0) {
    _tcsncat(buffer, TEXT("\\"), MAX_PATH);
    _tcsncat(buffer, file, MAX_PATH);
  }
}


void LocalPathS(char *buffer, const TCHAR* file, int loc) {
  TCHAR wbuffer[MAX_PATH];
  LocalPath(wbuffer,file,loc);
  sprintf(buffer,"%S",wbuffer);
}


void ExpandLocalPath(TCHAR* filein) {
  // Convert %LOCALPATH% to Local Path

  if (_tcslen(filein)==0) {
    return;
  }

  TCHAR lpath[MAX_PATH];
  TCHAR code[] = TEXT("%LOCAL_PATH%\\");
  TCHAR output[MAX_PATH];
  LocalPath(lpath);

  TCHAR* ptr;
  ptr = _tcsstr(filein, code);
  if (!ptr) return;

  ptr += _tcslen(code);
  if (_tcslen(ptr)>0) {
    _stprintf(output,TEXT("%s%s"),lpath, ptr);
    _tcscpy(filein, output);
  }
}


void ContractLocalPath(TCHAR* filein) {
  // Convert Local Path part to %LOCALPATH%

  if (_tcslen(filein)==0) {
    return;
  }

  TCHAR lpath[MAX_PATH];
  TCHAR code[] = TEXT("%LOCAL_PATH%\\");
  TCHAR output[MAX_PATH];
  LocalPath(lpath);

  TCHAR* ptr;
  ptr = _tcsstr(filein, lpath);
  if (!ptr) return;

  ptr += _tcslen(lpath);
  if (_tcslen(ptr)>0) {
    _stprintf(output,TEXT("%s%s"),code, ptr);
    _tcscpy(filein, output);
  }
}

