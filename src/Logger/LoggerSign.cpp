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

#include "Logger/LoggerImpl.hpp"
#include "UtilsText.hpp" // for ConvertToC()

#include <assert.h>
#include <tchar.h>

/**
 * Checks a string for invalid characters and replaces them with 0x20 (space)
 * @param szIn Input/Output string (pointer)
 */
void
LoggerImpl::CleanIGCRecord(char * szIn)
{  
  // don't clean terminating \r\n!
  int iLen = strlen(szIn) - 2;
  for (int i = 0; i < iLen; i++) {
    if (!oGRecord.IsValidIGCChar(szIn[i]))
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

/**
 * Flushes the data in the DiskBuffer to the disk
 */
void
LoggerImpl::DiskBufferFlush()
{
  FILE * LoggerFILE;

  ConvertTToC(szLoggerFileName_c, szLoggerFileName);
  szLoggerFileName_c[_tcslen(szLoggerFileName)] = 0;
  // stays open for buffered io
  LoggerFILE = fopen (szLoggerFileName_c,"ab");

  if (!LoggerFILE)
    return;

  for (int i = 0; i < LoggerDiskBufferCount; i++) {
    unsigned int iLen = strlen(LoggerDiskBuffer[i]);

    // if (file write successful)
    if (fwrite(LoggerDiskBuffer[i], (size_t)1, (size_t)iLen, LoggerFILE) == (size_t)iLen) {
      if (!Simulator) {
        oGRecord.AppendRecordToBuffer(LoggerDiskBuffer[i]);
      }
    }
  }

  fclose(LoggerFILE);
  DiskBufferReset();
}

/**
 * Adds the given string to the DiskBuffer
 * @param sIn Input string
 * @return True if adding was successful, False otherwise (Buffer full)
 */
bool
LoggerImpl::DiskBufferAdd(char *sIn)
{
  if (LoggerDiskBufferCount >= LOGGER_DISK_BUFFER_NUM_RECS) {
    DiskBufferFlush();
  }

  if (LoggerDiskBufferCount >= LOGGER_DISK_BUFFER_NUM_RECS)
    return false;

  strncpy(LoggerDiskBuffer[LoggerDiskBufferCount], sIn, MAX_IGC_BUFF);
  LoggerDiskBuffer[LoggerDiskBufferCount][MAX_IGC_BUFF - 1] = '\0';
  LoggerDiskBufferCount++;

  return true;
}

/**
 * Resets the DiskBuffer
 */
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
LoggerImpl::LoggerGStop(TCHAR* szLoggerFileName)
{
  BOOL bFileValid = true;
  TCHAR OldGRecordBuff[MAX_IGC_BUFF];
  TCHAR NewGRecordBuff[MAX_IGC_BUFF];

  // buffer is appended w/ each igc file write
  oGRecord.FinalizeBuffer();
  // read record built by individual file writes
  oGRecord.GetDigest(OldGRecordBuff);

  // now calc from whats in the igc file on disk
  oGRecord.Init();
  oGRecord.SetFileName(szLoggerFileName);
  oGRecord.LoadFileToBuffer();
  oGRecord.FinalizeBuffer();
  oGRecord.GetDigest(NewGRecordBuff);

  for (unsigned int i = 0; i < 128; i++)
    if (OldGRecordBuff[i] != NewGRecordBuff[i])
      bFileValid = false;

  oGRecord.AppendGRecordToFile(bFileValid);
}

/**
 * Initialize the GRecord part of the Logger
 */
void
LoggerImpl::LoggerGInit()
{
  oGRecord.Init();
}
