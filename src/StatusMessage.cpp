/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "LocalPath.hpp"
#include "Util/EscapeBackslash.hpp"
#include "Util/StringUtil.hpp"
#include "Util/NumberParser.hpp"
#include "IO/ConfiguredFile.hpp"

#include <stdio.h>
#include <memory>

static constexpr StatusMessage default_status_messages[] = {
#include "Status_defaults.cpp"
  { NULL }
};

StatusMessageList::StatusMessageList()
  :old_delay(2000)
{
  // DEFAULT - 0 is loaded as default, and assumed to exist
  StatusMessage &first = list.append();
  first.key = _T("DEFAULT");
  first.visible = true;
  first.sound = _T("IDR_WAV_DRIP");
  first.delay_ms = 2500; // 2.5 s

  // Load up other defaults - allow overwrite in config file
  const StatusMessage *src = &default_status_messages[0];
  while (src->key != NULL)
    list.append(*src++);
}

void
StatusMessageList::LoadFile()
{
  std::unique_ptr<TLineReader> reader(OpenConfiguredTextFile(ProfileKeys::StatusFile));
  if (reader)
    LoadFile(*reader);
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
  // Init first entry
  StatusMessage current;
  current.Clear();

  /* Read from the file */
  TCHAR *buffer;
  const TCHAR *key, *value;
  while ((buffer = reader.ReadLine()) != NULL) {
    // Check valid line? If not valid, assume next record (primative, but works ok!)
    if (*buffer == _T('#') || !parse_assignment(buffer, key, value)) {
      // Global counter (only if the last entry had some data)
      if (!current.IsEmpty()) {
        list.append(current);
        current.Clear();

        if (list.full())
          break;
      }
    } else {
      if (_tcscmp(key, _T("key")) == 0) {
        if (current.key == NULL)
          current.key = UnescapeBackslash(value);
      } else if (_tcscmp(key, _T("sound")) == 0) {
        if (current.sound == NULL)
          current.sound = UnescapeBackslash(value);
      } else if (_tcscmp(key, _T("delay")) == 0) {
        TCHAR *endptr;
        unsigned ms = ParseUnsigned(value, &endptr);
        if (endptr > value)
          current.delay_ms = ms;
      } else if (_tcscmp(key, _T("hide")) == 0) {
        if (_tcscmp(value, _T("yes")) == 0)
          current.visible = false;
      }
    }
  }

  if (!current.IsEmpty())
    list.append(current);
}

void
StatusMessageList::Startup(bool first)
{
  if (first) {
    // NOTE: Must show errors AFTER all windows ready
    old_delay = list[0].delay_ms;
    list[0].delay_ms = 20000; // 20 seconds
  } else {
    list[0].delay_ms = old_delay;
  }
}

const StatusMessage *
StatusMessageList::Find(const TCHAR *key) const
{
  for (int i = list.size() - 1; i > 0; i--)
    if (_tcscmp(key, list[i].key) == 0)
      return &list[i];

  return NULL;
}
