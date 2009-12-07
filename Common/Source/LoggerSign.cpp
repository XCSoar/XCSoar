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
#include "UtilsText.hpp" // for ConvertToC()


#include <assert.h>

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

typedef int (*GRECORDREADGRECORDFROMFILE)(TCHAR szOutput[]);
GRECORDREADGRECORDFROMFILE GRecordReadGRecordFromFile;

typedef int (*GRECORDVERIFYGRECORDINFILE)(void);
GRECORDVERIFYGRECORDINFILE GRecordVerifyGRecordInFile;

/**
 * Checks whether the character c is a valid IGC character
 * @param c Character to check
 * @return True if valid character, False otherwise
 */
bool
IsValidIGCChar(char c) //returns 1 if valid char for IGC files
{
  if (c >= 0x20 &&
      c <= 0x7E &&
      c != 0x0D &&
      c != 0x0A &&
      c != 0x24 &&
      c != 0x2A &&
      c != 0x2C &&
      c != 0x21 &&
      c != 0x5C &&
      c != 0x5E &&
      c != 0x7E)
    return true;
  else
    return false;
}

/**
 * Checks a string for invalid characters and replaces them with 0x20 (space)
 * @param szIn Input/Output string (pointer)
 */
void
CleanIGCRecord(char * szIn)
{  
  // don't clean terminating \r\n!
  int iLen = strlen(szIn) - 2;
  for (int i = 0; i < iLen; i++) {
    if (!IsValidIGCChar(szIn[i]))
      szIn[i] = ' ';
  }
}

bool
LoggerImpl::IGCWriteRecord(const char *szIn, const TCHAR* szLoggerFileName)
{
  Poco::ScopedRWLock protect(lock, true);

  char charbuffer[MAX_IGC_BUFF];

  strncpy(charbuffer, szIn, MAX_IGC_BUFF);
  // just to be safe
  charbuffer[MAX_IGC_BUFF - 1] = '\0';
  CleanIGCRecord(charbuffer);
  return DiskBufferAdd(charbuffer);
}

void
LoggerImpl::DiskBufferFlush()
{
  FILE * LoggerFILE;

  ConvertTToC(szLoggerFileName_c, szLoggerFileName);
  szLoggerFileName_c[_tcslen(szLoggerFileName)] = 0;
  // stays open for buffered io
  LoggerFILE = fopen (szLoggerFileName_c,"ab");

  bool bWriteSuccess = true;
  TCHAR buffer_G[MAX_IGC_BUFF];
  TCHAR * pbuffer_G;
  pbuffer_G = buffer_G;

  if (LoggerFILE) {
    for (int i = 0; i < LoggerDiskBufferCount; i++) {

      unsigned int ilen = strlen(LoggerDiskBuffer[i]);
      if (fwrite(LoggerDiskBuffer[i], (size_t)ilen, (size_t)1, LoggerFILE)
          != (size_t)ilen) {
        bWriteSuccess = false;
      }

      if (bWriteSuccess) {
        int iLen = strlen(LoggerDiskBuffer[i]);
        for (int j = 0; (j <= iLen) && (j < MAX_IGC_BUFF); j++) {
          buffer_G[j] = (TCHAR)LoggerDiskBuffer[i][j];
        }
        if (!is_simulator() && LoggerGActive()) {
          GRecordAppendRecordToBuffer(pbuffer_G);
        }
      }
    }

    fclose(LoggerFILE);
    DiskBufferReset();
  }
}

bool
LoggerImpl::DiskBufferAdd(char *sIn)
{
  bool bRetVal = false;

  if (LoggerDiskBufferCount == LOGGER_DISK_BUFFER_NUM_RECS) {
    DiskBufferFlush();
  }

  if (LoggerDiskBufferCount < LOGGER_DISK_BUFFER_NUM_RECS) {
    strncpy(LoggerDiskBuffer[LoggerDiskBufferCount], sIn, MAX_IGC_BUFF);
    LoggerDiskBuffer[LoggerDiskBufferCount][MAX_IGC_BUFF - 1] = '\0';
    LoggerDiskBufferCount++;
    bRetVal = true;
  }

  return bRetVal;
}
void
LoggerImpl::DiskBufferReset()
{
  for (int i = 0; i < LOGGER_DISK_BUFFER_NUM_RECS; i++) {
    LoggerDiskBuffer[i][0] = '\0';
  }
  LoggerDiskBufferCount = 0;
}

// VENTA3 TODO: if ifdef PPC2002 load correct dll. Put the dll inside
// XCSoarData, so users can place their executable XCS wherever they
// want.
//
// JMW: not sure that would work, I think dll has to be in OS
// directory or same directory as exe

void
LoggerImpl::LinkGRecordDLL()
{
  static bool bFirstTime = true;
  TCHAR szLoadResults [100];
  TCHAR szGRecordVersion[100];

  // only try to load DLL once per session
  if ((GRecordDLLHandle == NULL) && bFirstTime) {
    bFirstTime = false;

    StartupStore(TEXT("Searching for GRecordDLL\n"));
    if (is_altair()) {
      if (FileExistsW(TEXT("\\NOR Flash\\GRecordDLL.dat"))) {
        StartupStore(TEXT("Updating GRecordDLL.DLL\n"));
        DeleteFile(TEXT("\\NOR Flash\\GRecordDLL.DLL"));
        MoveFile(TEXT("\\NOR Flash\\GRecordDLL.dat"),
            TEXT("\\NOR Flash\\GRecordDLL.DLL"));
      }

      GRecordDLLHandle = LoadLibrary(TEXT("\\NOR Flash\\GRecordDLL.DLL"));
    } else {
      GRecordDLLHandle = LoadLibrary(TEXT("GRecordDLL.DLL"));
    }

    if (GRecordDLLHandle != NULL) {
      // if any pointers don't link, disable entire library
      BOOL bLoadOK = true;

#ifndef WINDOWSPC
      GRecordGetVersion = (GRRECORDGETVERSION)GetProcAddress(GRecordDLLHandle,
          TEXT("GRecordGetVersion"));

      // read version for log
      if (!GRecordGetVersion) {
        bLoadOK = false;
        _tcscpy(szGRecordVersion, TEXT("version unknown"));
      } else {
        GRecordGetVersion(szGRecordVersion);
      }

      GRecordInit = (GRECORDINIT)GetProcAddress(GRecordDLLHandle,
          TEXT("GRecordInit"));

      if (!GRecordInit)
        bLoadOK = false;

      GRecordGetDigestMaxLen = (GRECORDGETDIGESTMAXLEN)GetProcAddress(
          GRecordDLLHandle, TEXT("GRecordGetDigestMaxLen"));

      if (!GRecordGetDigestMaxLen)
        bLoadOK = false;

      GRecordAppendRecordToBuffer
          = (GRECORDAPPENDRECORDTOBUFFER)GetProcAddress(GRecordDLLHandle,
              TEXT("GRecordAppendRecordToBuffer"));

      if (!GRecordAppendRecordToBuffer)
        bLoadOK = false;

      GRecordFinalizeBuffer = (GRECORDFINALIZEBUFFER)GetProcAddress(
          GRecordDLLHandle, TEXT("GRecordFinalizeBuffer"));

      if (!GRecordFinalizeBuffer)
        bLoadOK = false;

      GRecordGetDigest = (GRECORDGETDIGEST)GetProcAddress(GRecordDLLHandle,
          TEXT("GRecordGetDigest"));

      if (!GRecordGetDigest)
        bLoadOK = false;

      GRecordSetFileName = (GRECORDSETFILENAME)GetProcAddress(GRecordDLLHandle,
          TEXT("GRecordSetFileName"));

      if (!GRecordSetFileName)
        bLoadOK = false;

      GRecordLoadFileToBuffer = (GRECORDLOADFILETOBUFFER)GetProcAddress(
          GRecordDLLHandle, TEXT("GRecordLoadFileToBuffer"));

      if (!GRecordLoadFileToBuffer)
        bLoadOK = false;

      GRecordAppendGRecordToFile = (GRECORDAPPENDGRECORDTOFILE)GetProcAddress(
          GRecordDLLHandle, TEXT("GRecordAppendGRecordToFile"));

      if (!GRecordAppendGRecordToFile)
        bLoadOK = false;

      GRecordReadGRecordFromFile = (GRECORDREADGRECORDFROMFILE)GetProcAddress(
          GRecordDLLHandle, TEXT("GRecordReadGRecordFromFile"));

      if (!GRecordReadGRecordFromFile)
        bLoadOK = false;

      GRecordVerifyGRecordInFile = (GRECORDVERIFYGRECORDINFILE)GetProcAddress(
          GRecordDLLHandle, TEXT("GRecordVerifyGRecordInFile"));
#else
      GRecordVerifyGRecordInFile = NULL;
#endif

      if (!GRecordVerifyGRecordInFile)
        bLoadOK=false;

      // all need to link, or disable entire library.
      if (!bLoadOK) {
        _stprintf(szLoadResults, TEXT("Found GRecordDLL %s but incomplete\n"),
            szGRecordVersion);
        FreeLibrary(GRecordDLLHandle);
        GRecordDLLHandle = NULL;
      } else {
        _stprintf(szLoadResults, TEXT("Loaded GRecordDLL %s \n"),
            szGRecordVersion);
      }
    } else {
      _tcscpy(szLoadResults, TEXT("Can't load GRecordDLL\n"));
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
LoggerImpl::LoggerGStop(TCHAR* szLoggerFileName)
{
  BOOL bFileValid = true;
  TCHAR OldGRecordBuff[MAX_IGC_BUFF];
  TCHAR NewGRecordBuff[MAX_IGC_BUFF];

  // buffer is appended w/ each igc file write
  GRecordFinalizeBuffer();
  // read record built by individual file writes
  GRecordGetDigest(OldGRecordBuff);

  // now calc from whats in the igc file on disk
  GRecordInit();
  GRecordSetFileName(szLoggerFileName);
  GRecordLoadFileToBuffer();
  GRecordFinalizeBuffer();
  GRecordGetDigest(NewGRecordBuff);

  for (unsigned int i = 0; i < 128; i++)
    if (OldGRecordBuff[i] != NewGRecordBuff[i])
      bFileValid = false;

  GRecordAppendGRecordToFile(bFileValid);
}

void
LoggerImpl::LoggerGInit()
{
  if (is_simulator())
    return;

  // try to link DLL if it exists
  LinkGRecordDLL();
  if (LoggerGActive())
    GRecordInit();
}
