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

#include "Logger/NMEALogger.hpp"
#include "IO/BatchTextWriter.hpp"
#include "LocalPath.hpp"
#include "NMEA/Info.hpp"
#include "Compatibility/path.h"
#include "Thread/Mutex.hpp"
#include "Interface.hpp"

#include <windef.h> // for MAX_PATH
#include <stdio.h>

static Mutex RawLoggerMutex;
static BatchTextWriter *RawLoggerWriter;

bool EnableLogNMEA = false;

static bool
RawLoggerStart()
{
  if (RawLoggerWriter != NULL)
    return true;

  BrokenDateTime dt = XCSoarInterface::Basic().DateTime;
  assert(dt.Plausible());

  TCHAR path[MAX_PATH];
  LocalPath(path, _T("logs"));
  unsigned len = _tcslen(path);
  _sntprintf(path+len, MAX_PATH-len,
             _T(DIR_SEPARATOR_S "%04u-%02u-%02u_%02u-%02u.nmea"),
             dt.year, dt.month, dt.day,
             dt.hour, dt.minute);

  RawLoggerWriter = new BatchTextWriter(path, false);
  return RawLoggerWriter != NULL;
}

void
RawLoggerShutdown()
{
  delete RawLoggerWriter;
}

void
LogNMEA(const char *text)
{
  if (!EnableLogNMEA)
    return;

  ScopeLock protect(RawLoggerMutex);
  if (RawLoggerStart())
    RawLoggerWriter->writeln(text);
}
