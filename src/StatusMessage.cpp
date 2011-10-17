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

#include "StatusMessage.hpp"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "UtilsText.hpp"
#include "StringUtil.hpp"
#include "IO/ConfiguredFile.hpp"

#include <stdio.h>

static gcc_constexpr_data StatusMessageSTRUCT StatusMessageDefaults[] = {
#include "Status_defaults.cpp"
  { NULL }
};

StatusMessageList::StatusMessageList()
  :olddelay(2000)
{
  // DEFAULT - 0 is loaded as default, and assumed to exist
  StatusMessageSTRUCT &first = StatusMessageData.append();
  first.key = _T("DEFAULT");
  first.doStatus = true;
  first.doSound = true;
  first.sound = _T("IDR_WAV_DRIP");
  first.delay_ms = 2500; // 2.5 s

  // Load up other defaults - allow overwrite in config file
  const StatusMessageSTRUCT *src = &StatusMessageDefaults[0];
  while (src->key != NULL)
    StatusMessageData.append(*src++);
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
  StatusMessageSTRUCT current;
  _init_Status(current);
  some_data = false;

  /* Read from the file */
  TCHAR *buffer;
  const TCHAR *key, *value;
  while ((buffer = reader.read()) != NULL) {
    // Check valid line? If not valid, assume next record (primative, but works ok!)
    if (*buffer == _T('#') || !parse_assignment(buffer, key, value)) {
      // Global counter (only if the last entry had some data)
      if (some_data) {
        StatusMessageData.append(current);
        some_data = false;
        _init_Status(current);

        if (StatusMessageData.full())
          break;
      }
    } else {
      location = NULL;

      if (_tcscmp(key, _T("key")) == 0) {
        some_data = true; // Success, we have a real entry
        location = &current.key;
      } else if (_tcscmp(key, _T("sound")) == 0) {
        current.doSound = true;
        location = &current.sound;
      } else if (_tcscmp(key, _T("delay")) == 0) {
        TCHAR *endptr;
        ms = _tcstol(value, &endptr, 10);
        if (endptr > value)
          current.delay_ms = ms;
      } else if (_tcscmp(key, _T("hide")) == 0) {
        if (_tcscmp(value, _T("yes")) == 0)
          current.doStatus = false;
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

  if (some_data)
    StatusMessageData.append(current);
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
  for (int i = StatusMessageData.size() - 1; i > 0; i--)
    if (_tcscmp(key, StatusMessageData[i].key) == 0)
      return &StatusMessageData[i];

  return NULL;
}

// Create a blank entry (not actually used)
void
StatusMessageList::_init_Status(StatusMessageSTRUCT &m)
{
  m.key = _T("");
  m.doStatus = true;
  m.doSound = false;
  m.sound = _T("");
  m.delay_ms = 2500;  // 2.5 s
}
