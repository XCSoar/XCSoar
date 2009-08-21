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

