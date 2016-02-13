/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "InputKeys.hpp"
#include "InputLookup.hpp"
#include "IO/LineReader.hpp"
#include "Util/StringUtil.hpp"
#include "Util/StringAPI.hxx"
#include "Util/StaticString.hxx"
#include "Util/EscapeBackslash.hpp"
#include "Util/NumberParser.hpp"
#include "LogFile.hpp"

#include <tchar.h>
#include <stdio.h>

static bool
parse_assignment(TCHAR *buffer, const TCHAR *&key, const TCHAR *&value)
{
  TCHAR *separator = StringFind(buffer, '=');
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

    const TCHAR *new_label = NULL;
    while (token != NULL) {

      // All modes are valid at this point
      int mode_id = config.MakeMode(token);
      assert(mode_id >= 0);

      // Make label event
      // TODO code: Consider Reuse existing entries...
      if (location > 0) {
        // Only copy this once per object - save string space
        if (!new_label) {
          new_label = UnescapeBackslash(label);
        }

        config.AppendMenu(mode_id, new_label, location, event_id);
      }

      // Make key (Keyboard input)
      // key - Hardware key or keyboard
      if (type.equals(_T("key"))) {
        // Get the int key (eg: APP1 vs 'a')
        unsigned key = ParseKeyCode(data);
        if (key > 0)
          config.SetKeyEvent(mode_id, key, event_id);
        else
          LogFormat(_T("Invalid key data: %s at %u"), data.c_str(), line);

        // Make gce (Glide Computer Event)
        // GCE - Glide Computer Event
      } else if (type.equals(_T("gce"))) {
        // Get the int key (eg: APP1 vs 'a')
        int key = InputEvents::findGCE(data);
        if (key >= 0)
          config.GC2Event[key] = event_id;
        else
          LogFormat(_T("Invalid GCE data: %s at %u"), data.c_str(), line);

        // Make gesture (Gesture Event)
        // Key - Key Event
      } else if (type.equals(_T("gesture"))) {
        // Check data for invalid characters:
        bool valid = true;
        for (const TCHAR* c = data; *c; c++)
          if (*c != _T('U') &&
              *c != _T('D') &&
              *c != _T('R') &&
              *c != _T('L'))
            valid = false;
        
        if (valid) {
          // One entry per key: delete old, create new
          config.Gesture2Event.Remove(data.c_str());
          config.Gesture2Event.Add(data.c_str(), event_id);
        } else
          LogFormat(_T("Invalid gesture data: %s at %u"), data.c_str(), line);

        // Make ne (NMEA Event)
        // NE - NMEA Event
      } else if (type.equals(_T("ne"))) {
        // Get the int key (eg: APP1 vs 'a')
        int key = InputEvents::findNE(data);
        if (key >= 0)
          config.N2Event[key] = event_id;
        else
          LogFormat(_T("Invalid GCE data: %s at %u"), data.c_str(), line);

        // label only - no key associated (label can still be touch screen)
      } else if (type.equals(_T("label"))) {
        // Nothing to do here...

      } else {
        LogFormat(_T("Invalid type: %s at %u"), type.c_str(), line);
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
  while ((buffer = reader.ReadLine()) != NULL) {
    StripRight(buffer);
    line++;

    const TCHAR *key, *value;

    // experimental: if the first line is "#CLEAR" then the whole default config is cleared
    //               and can be overwritten by file
    if (line == 1 && StringIsEqual(buffer, _T("#CLEAR"))) {
      config.SetDefaults();
    } else if (buffer[0] == _T('\0')) {
      // Check valid line? If not valid, assume next record (primative, but works ok!)
      // General checks before continue...
      current.commit(config, line);

      // Clear all data.
      current.clear();

    } else if (StringIsEmpty(buffer) || buffer[0] == _T('#')) {
      // Do nothing - we probably just have a comment line
      // NOTE: Do NOT display buffer to user as it may contain an invalid stirng !

    } else if (parse_assignment(buffer, key, value)) {
      if (StringIsEqual(key, _T("mode"))) {
        current.mode = value;
      } else if (StringIsEqual(key, _T("type"))) {
        current.type = value;
      } else if (StringIsEqual(key, _T("data"))) {
        current.data = value;
      } else if (StringIsEqual(key, _T("event"))) {
        if (_tcslen(value) < 256) {
          TCHAR d_event[256] = _T("");
          TCHAR d_misc[256] = _T("");
          int ef;

          #if defined(__BORLANDC__)
          memset(d_event, 0, sizeof(d_event));
          memset(d_misc, 0, sizeof(d_event));
          if (StringFind(value, ' ') == nullptr) {
            _tcscpy(d_event, value);
          } else {
          #endif

          ef = _stscanf(value, _T("%[^ ] %[A-Za-z0-9_ \\/().,]"), d_event,
              d_misc);

          #if defined(__BORLANDC__)
          }
          #endif

          if ((ef == 1) || (ef == 2)) {

            // TODO code: Consider reusing existing identical events

            pt2Event event = InputEvents::findEvent(d_event);
            if (event) {
              TCHAR *allocated = UnescapeBackslash(d_misc);
              current.event_id = config.AppendEvent(event, allocated,
                                                    current.event_id);

              /* not freeing the string, because
                 InputConfig::AppendEvent() stores the string point
                 without duplicating it; strictly speaking, this is a
                 memory leak, but the input file is only loaded once
                 at startup, so this is acceptable; in return, we
                 don't have to duplicate the hard-coded defaults,
                 which saves some memory */
              //free(allocated);

            } else {
              LogFormat(_T("Invalid event type: %s at %i"), d_event, line);
            }
          } else {
            LogFormat("Invalid event type at %i", line);
          }
        }
      } else if (StringIsEqual(key, _T("label"))) {
        current.label = value;
      } else if (StringIsEqual(key, _T("location"))) {
        current.location = ParseUnsigned(value);

      } else {
        LogFormat(_T("Invalid key/value pair %s=%s at %i"), key, value, line);
      }
    } else  {
      LogFormat("Invalid line at %i", line);
    }

  }

  current.commit(config, line);
}
