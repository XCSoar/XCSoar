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

#include "LoggerImpl.hpp"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Dialogs.h"
#include "Device/Port.h"
#include "SettingsTask.hpp"
#include "Registry.hpp"
#include "Math/Earth.hpp"
#include "LogFile.hpp"
#include "Asset.hpp"
#include "LocalPath.hpp"
#include "Device/device.h"
#include "InputEvents.h"
#include "Compatibility/string.h"
#include "UtilsSystem.hpp" // for FileExistsW()

HINSTANCE GRecordDLLHandle = NULL;

// Procedures for explicitly loaded (optional) GRecord DLL
typedef int (*GRRECORDGETVERSION)(TCHAR * szOut);
GRRECORDGETVERSION GRecordGetVersion;

typedef int (*GRECORDINIT)(void);
GRECORDINIT GRecordInit;

typedef int (*GRECORDGETDIGESTMAXLEN)(void);
GRECORDGETDIGESTMAXLEN GRecordGetDigestMaxLen;

typedef int (*GRECORDAPPENDRECORDTOBUFFER)(TCHAR * szIn);
GRECORDAPPENDRECORDTOBUFFER GRecordAppendRecordToBuffer;

typedef int (*GRECORDFINALIZEBUFFER)(void);
GRECORDFINALIZEBUFFER GRecordFinalizeBuffer;

typedef int (*GRECORDGETDIGEST)(TCHAR * szOut);
GRECORDGETDIGEST GRecordGetDigest;

typedef int (*GRECORDSETFILENAME)(TCHAR * szIn);
GRECORDSETFILENAME GRecordSetFileName;

typedef int (*GRECORDLOADFILETOBUFFER)(void);
GRECORDLOADFILETOBUFFER GRecordLoadFileToBuffer;

typedef int (*GRECORDAPPENDGRECORDTOFILE)(BOOL bValid);
GRECORDAPPENDGRECORDTOFILE GRecordAppendGRecordToFile;

typedef int (*GRECORDREADGRECORDFROMFILE)(TCHAR szOutput []);
GRECORDREADGRECORDFROMFILE GRecordReadGRecordFromFile;

typedef int (*GRECORDVERIFYGRECORDINFILE)(void);
GRECORDVERIFYGRECORDINFILE GRecordVerifyGRecordInFile;


bool IsValidIGCChar(char c) //returns 1 if valid char for IGC files
{//
  if ( c >=0x20  && c <= 0x7E &&
       c != 0x0D &&
       c != 0x0A &&
       c != 0x24 &&
       c != 0x2A &&
       c != 0x2C &&
       c != 0x21 &&
       c != 0x5C &&
       c != 0x5E &&
       c != 0x7E
       )
    return true;
  else
    return false;
}



char * CleanIGCRecord (char * szIn)
{  // replace invalid chars w/ 0x20

  int iLen = strlen(szIn);
  for (int i = 0; i < iLen -2; i++)  // don't clean terminating \r\n!
    if (!IsValidIGCChar(szIn[i]))
      szIn[i]=' ';

  return szIn;
}

bool
LoggerImpl::IGCWriteRecord(const char *szIn, const TCHAR* szLoggerFileName)
{
  HANDLE hFile;
  DWORD dwBytesRead;
  char charbuffer[MAX_IGC_BUFF];
  TCHAR buffer[MAX_IGC_BUFF];
  TCHAR * pbuffer;
  pbuffer = buffer;
  bool bRetVal = false;

  int i=0, iLen=0;
  static BOOL bWriting = false;

  if ( !bWriting )
    {
      bWriting = true;

      hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
			 NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
      SetFilePointer(hFile, 0, NULL, FILE_END);

      strcpy(charbuffer, szIn);
      CleanIGCRecord(charbuffer);

      WriteFile(hFile, charbuffer, strlen(charbuffer), &dwBytesRead,
		(OVERLAPPED *)NULL);

      iLen = strlen(charbuffer);
      for (i = 0; (i <= iLen) && (i < MAX_IGC_BUFF); i++)
	buffer[i] = (TCHAR)charbuffer[i];

      if (!is_simulator() && LoggerGActive())
	GRecordAppendRecordToBuffer(pbuffer);

      FlushFileBuffers(hFile);
      CloseHandle(hFile);
      bWriting = false;
      bRetVal = true;
    }

  return bRetVal;

}

// VENTA3 TODO: if ifdef PPC2002 load correct dll. Put the dll inside
// XCSoarData, so users can place their executable XCS wherever they
// want.
//
// JMW: not sure that would work, I think dll has to be in OS
// directory or same directory as exe

void LoggerImpl::LinkGRecordDLL(void)
{
  static bool bFirstTime = true;
  TCHAR szLoadResults [100];
  TCHAR szGRecordVersion[100];

  if ((GRecordDLLHandle == NULL) && bFirstTime) // only try to load DLL once per session
    {
      bFirstTime=false;

      StartupStore(TEXT("Searching for GRecordDLL\n"));
      if (is_altair()) {
        if (FileExistsW(TEXT("\\NOR Flash\\GRecordDLL.dat"))) {
          StartupStore(TEXT("Updating GRecordDLL.DLL\n"));
          DeleteFile(TEXT("\\NOR Flash\\GRecordDLL.DLL"));
          MoveFile(TEXT("\\NOR Flash\\GRecordDLL.dat"),
                   TEXT("\\NOR Flash\\GRecordDLL.DLL"));
        }

        GRecordDLLHandle = LoadLibrary(TEXT("\\NOR Flash\\GRecordDLL.DLL"));
      } else
        GRecordDLLHandle = LoadLibrary(TEXT("GRecordDLL.DLL"));

      if (GRecordDLLHandle != NULL)
        {
	  BOOL bLoadOK = true;  // if any pointers don't link, disable entire library

#ifndef WINDOWSPC
	  GRecordGetVersion =
	    (GRRECORDGETVERSION)
	    GetProcAddress(GRecordDLLHandle,
			   TEXT("GRecordGetVersion"));

	  if (!GRecordGetVersion) // read version for log
            {
	      bLoadOK=false;
	      _tcscpy(szGRecordVersion, TEXT("version unknown"));
            }
	  else
            {
	      GRecordGetVersion(szGRecordVersion);
            }


	  GRecordInit =
	    (GRECORDINIT)
	    GetProcAddress(GRecordDLLHandle,
			   TEXT("GRecordInit"));

	  if (!GRecordInit)
	    bLoadOK=false;

	  GRecordGetDigestMaxLen =
	    (GRECORDGETDIGESTMAXLEN)
	    GetProcAddress(GRecordDLLHandle,
			   TEXT("GRecordGetDigestMaxLen"));

	  if (!GRecordGetDigestMaxLen)
	    bLoadOK=false;


	  GRecordAppendRecordToBuffer =
	    (GRECORDAPPENDRECORDTOBUFFER)
	    GetProcAddress(GRecordDLLHandle,
			   TEXT("GRecordAppendRecordToBuffer"));

	  if (!GRecordAppendRecordToBuffer)
	    bLoadOK=false;


	  GRecordFinalizeBuffer =
	    (GRECORDFINALIZEBUFFER)
	    GetProcAddress(GRecordDLLHandle,
			   TEXT("GRecordFinalizeBuffer"));

	  if (!GRecordFinalizeBuffer)
	    bLoadOK=false;


	  GRecordGetDigest =
	    (GRECORDGETDIGEST)
	    GetProcAddress(GRecordDLLHandle,
			   TEXT("GRecordGetDigest"));

	  if (!GRecordGetDigest)
	    bLoadOK=false;


	  GRecordSetFileName =
	    (GRECORDSETFILENAME)
	    GetProcAddress(GRecordDLLHandle,
			   TEXT("GRecordSetFileName"));

	  if (!GRecordSetFileName)
	    bLoadOK=false;


	  GRecordLoadFileToBuffer =
	    (GRECORDLOADFILETOBUFFER)
	    GetProcAddress(GRecordDLLHandle,
			   TEXT("GRecordLoadFileToBuffer"));

	  if (!GRecordLoadFileToBuffer)
	    bLoadOK=false;


	  GRecordAppendGRecordToFile =
	    (GRECORDAPPENDGRECORDTOFILE)
	    GetProcAddress(GRecordDLLHandle,
			   TEXT("GRecordAppendGRecordToFile"));

	  if (!GRecordAppendGRecordToFile)
	    bLoadOK=false;


	  GRecordReadGRecordFromFile =
	    (GRECORDREADGRECORDFROMFILE)
	    GetProcAddress(GRecordDLLHandle,
			   TEXT("GRecordReadGRecordFromFile"));

	  if (!GRecordReadGRecordFromFile)
	    bLoadOK=false;


	  GRecordVerifyGRecordInFile =
	    (GRECORDVERIFYGRECORDINFILE)
	    GetProcAddress(GRecordDLLHandle,
			   TEXT("GRecordVerifyGRecordInFile"));
#else
	  GRecordVerifyGRecordInFile = NULL;
#endif

	  if (!GRecordVerifyGRecordInFile)
	    bLoadOK=false;

	  if (!bLoadOK) // all need to link, or disable entire library.
            {
              _stprintf(szLoadResults,TEXT("Found GRecordDLL %s but incomplete\n"), szGRecordVersion);
	      FreeLibrary(GRecordDLLHandle);
	      GRecordDLLHandle = NULL;
            }
	  else {
            _stprintf(szLoadResults,TEXT("Loaded GRecordDLL %s \n"), szGRecordVersion);
	  }
	}
      else {
	_tcscpy(szLoadResults,TEXT("Can't load GRecordDLL\n"));
      }
      StartupStore(szLoadResults);

    }
}

bool
LoggerImpl::LoggerGActive() const
{
  if (GRecordDLLHandle)
    return true;
  else
    return false;
}



void
LoggerImpl::LoggerGStop(TCHAR* szLoggerFileName) {
  BOOL bFileValid = true;
  TCHAR OldGRecordBuff[MAX_IGC_BUFF];
  TCHAR NewGRecordBuff[MAX_IGC_BUFF];

  GRecordFinalizeBuffer();  // buffer is appended w/ each igc file write
  GRecordGetDigest(OldGRecordBuff); // read record built by individual file writes

  // now calc from whats in the igc file on disk
  GRecordInit();
  GRecordSetFileName(szLoggerFileName);
  GRecordLoadFileToBuffer();
  GRecordFinalizeBuffer();
  GRecordGetDigest(NewGRecordBuff);

  for (unsigned int i = 0; i < 128; i++)
    if (OldGRecordBuff[i] != NewGRecordBuff[i] )
      bFileValid = false;

  GRecordAppendGRecordToFile(bFileValid);
}

void
LoggerImpl::LoggerGInit() {
  if (is_simulator())
    return;

  LinkGRecordDLL(); // try to link DLL if it exists
  if (LoggerGActive())
    GRecordInit();
}
