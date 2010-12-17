/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "InputParser.hpp"
#include "InputConfig.hpp"
#include "InputEvents.hpp"
#include "IO/LineReader.hpp"
#include "Util/StringUtil.hpp"
#include "Util/StaticString.hpp"
#include "UtilsText.hpp"
#include "LogFile.hpp"
#include "Compatibility/string.h" /* for _ttoi() */

#include <string.h>
#include <tchar.h>
#include <stdio.h>

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

struct EventBuilder {
  unsigned event_id, location;
  StaticString<1024> mode;
  StaticString<256> type, data, label;

  void clear() {
    event_id = 0;
    location = 0;
    mode.clear();
    type.clear();
    data.clear();
    label.clear();
  }

  bool empty() const {
    return mode.empty();
  }

  void commit(InputConfig &config, unsigned line) {
    if (empty())
      return;

    TCHAR *token;

    // For each mode
    token = mode.first_token(_T(" "));

    // General errors - these should be true
    assert(location < 1024);
    assert(mode != NULL);

    const TCHAR *new_label = NULL;
    while (token != NULL) {

      // All modes are valid at this point
      int mode_id = config.make_mode(token);
      assert(mode_id >= 0);

      // Make label event
      // TODO code: Consider Reuse existing entries...
      if (location > 0) {
        // Only copy this once per object - save string space
        if (!new_label) {
          new_label = StringMallocParse(label);
        }

        config.append_menu(mode_id, new_label,
                           location, event_id);
      }

      // Make key (Keyboard input)
      // key - Hardware key or keyboard
      if (type.equals(_T("key"))) {
        // Get the int key (eg: APP1 vs 'a')
        unsigned key = InputEvents::findKey(data);
        if (key > 0)
          config.Key2Event[mode_id][key] = event_id;
        else
          LogStartUp(_T("Invalid key data: %s at %u"), data.c_str(), line);

        // Make gce (Glide Computer Event)
        // GCE - Glide Computer Event
      } else if (type.equals(_T("gce"))) {
        // Get the int key (eg: APP1 vs 'a')
        int key = InputEvents::findGCE(data);
        if (key >= 0)
          config.GC2Event[mode_id][key] = event_id;
        else
          LogStartUp(_T("Invalid GCE data: %s at %u"), data.c_str(), line);

        // Make ne (NMEA Event)
        // NE - NMEA Event
      } else if (type.equals(_T("ne"))) {
        // Get the int key (eg: APP1 vs 'a')
        int key = InputEvents::findNE(data);
        if (key >= 0)
          config.N2Event[mode_id][key] = event_id;
        else
          LogStartUp(_T("Invalid GCE data: %s at %u"), data.c_str(), line);

        // label only - no key associated (label can still be touch screen)
      } else if (type.equals(_T("label"))) {
        // Nothing to do here...

      } else {
        LogStartUp(_T("Invalid type: %s at %u"), type.c_str(), line);
      }

      token = mode.next_token(_T(" "));
    }
  }
};

void
ParseInputFile(InputConfig &config, TLineReader &reader)
{
  // TODO code - Safer sizes, strings etc - use C++ (can scanf restrict length?)

  // Multiple modes (so large string)
  EventBuilder current;
  current.clear();

  int line = 0;

  // Read from the file
  TCHAR *buffer;
  while ((buffer = reader.read()) != NULL) {
    TrimRight(buffer);
    line++;

    const TCHAR *key, *value;

    // experimental: if the first line is "#CLEAR" then the whole default config is cleared
    //               and can be overwritten by file
    if (line == 1 && _tcscmp(buffer, _T("#CLEAR")) == 0) {
      config.clear_all_events();
    } else if (buffer[0] == _T('\0')) {
      // Check valid line? If not valid, assume next record (primative, but works ok!)
      // General checks before continue...
      current.commit(config, line);

      // Clear all data.
      current.clear();

    } else if (string_is_empty(buffer) || buffer[0] == _T('#')) {
      // Do nothing - we probably just have a comment line
      // NOTE: Do NOT display buffer to user as it may contain an invalid stirng !

    } else if (parse_assignment(buffer, key, value)) {
      if (_tcscmp(key, _T("mode")) == 0) {
        current.mode = value;
      } else if (_tcscmp(key, _T("type")) == 0) {
        current.type = value;
      } else if (_tcscmp(key, _T("data")) == 0) {
        current.data = value;
      } else if (_tcscmp(key, _T("event")) == 0) {
        if (_tcslen(value) < 256) {
          TCHAR d_event[256] = _T("");
          TCHAR d_misc[256] = _T("");
          int ef;

          #if defined(__BORLANDC__)
          memset(d_event, 0, sizeof(d_event));
          memset(d_misc, 0, sizeof(d_event));
          if (_tcschr(value, ' ') == NULL) {
            _tcscpy(d_event, value);
          } else {
          #endif

          ef = _stscanf(value, _T("%[^ ] %[A-Za-z0-9 \\/().,]"), d_event,
              d_misc);

          #if defined(__BORLANDC__)
          }
          #endif

          if ((ef == 1) || (ef == 2)) {

            // TODO code: Consider reusing existing identical events

            pt2Event event = InputEvents::findEvent(d_event);
            if (event) {
              TCHAR *allocated = StringMallocParse(d_misc);
              current.event_id = config.append_event(event, allocated,
                                                     current.event_id);

              /* not freeing the string, because
                 InputConfig::append_event() stores the string point
                 without duplicating it; strictly speaking, this is a
                 memory leak, but the input file is only loaded once
                 at startup, so this is acceptable; in return, we
                 don't have to duplicate the hard-coded defaults,
                 which saves some memory */
              //free(allocated);

            } else {
              LogStartUp(_T("Invalid event type: %s at %i"), d_event, line);
            }
          } else {
            LogStartUp(_T("Invalid event type at %i"), line);
          }
        }
      } else if (_tcscmp(key, _T("label")) == 0) {
        current.label = value;
      } else if (_tcscmp(key, _T("location")) == 0) {
        current.location = _ttoi(value);

      } else {
        LogStartUp(_T("Invalid key/value pair %s=%s at %i"), key, value, line);
      }
    } else  {
      LogStartUp(_T("Invalid line at %i"), line);
    }

  }

  current.commit(config, line);
}
