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

#include "StatusMessage.hpp"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "UtilsText.hpp"
#include "StringUtil.hpp"
#include "IO/ConfiguredFile.hpp"

#include <stdio.h>

static const StatusMessageSTRUCT StatusMessageDefaults[] = {
#include "Status_defaults.cpp"
  { NULL }
};

StatusMessageList::StatusMessageList()
  :StatusMessageData_Size(0), olddelay(2000)
{
  // DEFAULT - 0 is loaded as default, and assumed to exist
  StatusMessageData[0].key = _T("DEFAULT");
  StatusMessageData[0].doStatus = true;
  StatusMessageData[0].doSound = true;
  StatusMessageData[0].sound = _T("IDR_WAV_DRIP");
  StatusMessageData[0].delay_ms = 2500; // 2.5 s
  StatusMessageData_Size=1;

  // Load up other defaults - allow overwrite in config file
  StatusMessageSTRUCT *dest = StatusMessageData;
  const StatusMessageSTRUCT *src = StatusMessageDefaults;
  while (src->key != NULL)
    *dest++ = *src++;
}

void
StatusMessageList::LoadFile()
{
  LogStartUp(_T("Loading status file"));

  TLineReader *reader = OpenConfiguredTextFile(szProfileStatusFile);
  if (reader != NULL) {
    LoadFile(*reader);
    delete reader;
  }
}

static bool
parse_assignment(TCHAR *buffer, const TCHAR *&key, const TCHAR *&value)
{
  TCHAR *separator = _tcschr(buffer, '=');
  if (separator == NULL || separator == buffer)
    return false;

  *separator = _T('\0');

  key = buffer;
  value = separator + 1;

  return true;
}

void
StatusMessageList::LoadFile(TLineReader &reader)
{
  int ms; // Found ms for delay
  const TCHAR **location; // Where to put the data
  bool some_data; // Did we find some in the last loop...

  // Init first entry
  _init_Status(StatusMessageData_Size);
  some_data = false;

  /* Read from the file */
  TCHAR *buffer;
  const TCHAR *key, *value;
  while ((StatusMessageData_Size < MAXSTATUSMESSAGECACHE) &&
         (buffer = reader.read()) != NULL) {
    // Check valid line? If not valid, assume next record (primative, but works ok!)
    if (*buffer == _T('#') || !parse_assignment(buffer, key, value)) {
      // Global counter (only if the last entry had some data)
      if (some_data) {
        StatusMessageData_Size++;
        some_data = false;
        _init_Status(StatusMessageData_Size);
      }
    } else {
      location = NULL;

      if (_tcscmp(key, _T("key")) == 0) {
        some_data = true; // Success, we have a real entry
        location = &StatusMessageData[StatusMessageData_Size].key;
      } else if (_tcscmp(key, _T("sound")) == 0) {
        StatusMessageData[StatusMessageData_Size].doSound = true;
        location = &StatusMessageData[StatusMessageData_Size].sound;
      } else if (_tcscmp(key, _T("delay")) == 0) {
        TCHAR *endptr;
        ms = _tcstol(value, &endptr, 10);
        if (endptr > value)
          StatusMessageData[StatusMessageData_Size].delay_ms = ms;
      } else if (_tcscmp(key, _T("hide")) == 0) {
        if (_tcscmp(value, _T("yes")) == 0)
          StatusMessageData[StatusMessageData_Size].doStatus = false;
      }

      // Do we have somewhere to put this &&
      // is it currently empty ? (prevent lost at startup)
      if (location && (_tcscmp(*location, _T("")) == 0)) {
        // TODO code: this picks up memory lost from no entry, but not duplicates - fix.
        if (*location) {
          // JMW fix memory leak
          free((void*)*location);
        }
        *location = StringMallocParse(value);
      }
    }
  }

  // How many we really got (blank next just in case)
  StatusMessageData_Size++;
  _init_Status(StatusMessageData_Size);
}

void
StatusMessageList::Startup(bool first)
{
  if (first) {
    // NOTE: Must show errors AFTER all windows ready
    olddelay = StatusMessageData[0].delay_ms;
    StatusMessageData[0].delay_ms = 20000; // 20 seconds
  } else {
    StatusMessageData[0].delay_ms = olddelay;
  }
}

const StatusMessageSTRUCT *
StatusMessageList::Find(const TCHAR *key) const
{
  for (int i = StatusMessageData_Size - 1; i > 0; i--)
    if (_tcscmp(key, StatusMessageData[i].key) == 0)
      return &StatusMessageData[i];

  return NULL;
}

// Create a blank entry (not actually used)
void
StatusMessageList::_init_Status(int num)
{
  StatusMessageData[num].key = _T("");
  StatusMessageData[num].doStatus = true;
  StatusMessageData[num].doSound = false;
  StatusMessageData[num].sound = _T("");
  StatusMessageData[num].delay_ms = 2500;  // 2.5 s
}
