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
#include "IO/TextWriter.hpp"

#include <assert.h>
#include <tchar.h>

/**
 * Checks a string for invalid characters and replaces them with 0x20 (space)
 * @param szIn Input/Output string (pointer)
 */
void
LoggerImpl::CleanIGCRecord(char * szIn)
{  
  int iLen = strlen(szIn);
  for (int i = 0; i < iLen; i++) {
    if (!oGRecord.IsValidIGCChar(szIn[i]))
      szIn[i] = ' ';
  }
}

bool
LoggerImpl::IGCWriteRecord(const char *szIn, const TCHAR* szLoggerFileName)
{
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
bool
LoggerImpl::DiskBufferFlush()
{
  TextWriter writer(szLoggerFileName, true);
  if (writer.error())
    return false;

  for (unsigned i = 0; i < DiskBuffer.length(); ++i) {
    const char *record = DiskBuffer[i];

    if (!writer.writeln(record))
      return false;

    if (!Simulator)
      oGRecord.AppendRecordToBuffer(record);
  }

  if (!writer.flush())
    return false;

  DiskBufferReset();
  return true;
}

/**
 * Adds the given string to the DiskBuffer
 * @param sIn Input string
 * @return True if adding was successful, False otherwise (Buffer full)
 */
bool
LoggerImpl::DiskBufferAdd(char *sIn)
{
  if (DiskBuffer.full() && !DiskBufferFlush())
    return false;

  assert(!DiskBuffer.full());

  char *dest = DiskBuffer.append();

  strncpy(dest, sIn, MAX_IGC_BUFF);
  dest[MAX_IGC_BUFF - 1] = '\0';

  return true;
}

/**
 * Resets the DiskBuffer
 */
void
LoggerImpl::DiskBufferReset()
{
  DiskBuffer.clear();
}

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
