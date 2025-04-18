// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputParser.hpp"
#include "InputConfig.hpp"
#include "InputKeys.hpp"
#include "InputLookup.hpp"
#include "io/BufferedReader.hxx"
#include "io/StringConverter.hpp"
#include "util/StringAPI.hxx"
#include "util/StaticString.hxx"
#include "util/StringSplit.hxx"
#include "util/StringStrip.hxx"
#include "util/EscapeBackslash.hpp"
#include "util/NumberParser.hpp"
#include "util/IterableSplitString.hxx"
#include "LogFile.hpp"

#include <tchar.h>
#include <stdio.h>

static bool
parse_assignment(char *buffer, const char *&key, const char *&value)
{
  char *separator = StringFind(buffer, '=');
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

    // General errors - these should be true
    assert(location < 1024);

    const TCHAR *new_label = NULL;

    // For each mode
    for (const auto token : TIterableSplitString(mode.c_str(), ' ')) {
      if (token.empty())
        continue;

      // All modes are valid at this point
      int mode_id = config.MakeMode(token);
      if (mode_id < 0) {
        LogFormat(_T("Too many modes: %.*s at %u"),
                  int(token.size()), token.data(), line);
        continue;
      }

      // Make label event
      // TODO code: Consider Reuse existing entries...
      if (location > 0) {
        // Only copy this once per object - save string space
        if (!new_label) {
          new_label = UnescapeBackslash(label.c_str());
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
    }
  }
};

void
ParseInputFile(InputConfig &config, BufferedReader &reader)
{
  // TODO code - Safer sizes, strings etc - use C++
  StringConverter string_converter{Charset::UTF8};

  // Multiple modes (so large string)
  EventBuilder current;
  current.clear();

  int line = 0;

  // Read from the file
  char *buffer;
  while ((buffer = reader.ReadLine()) != nullptr) {
    StripRight(buffer);
    line++;

    const char *key, *value;

    // experimental: if the first line is "#CLEAR" then the whole default config is cleared
    //               and can be overwritten by file
    if (line == 1 && StringIsEqual(buffer, "#CLEAR")) {
      config.SetDefaults();
    } else if (buffer[0] == '\0') {
      // Check valid line? If not valid, assume next record (primative, but works ok!)
      // General checks before continue...
      current.commit(config, line);

      // Clear all data.
      current.clear();

    } else if (StringIsEmpty(buffer) || buffer[0] == '#') {
      // Do nothing - we probably just have a comment line
      // NOTE: Do NOT display buffer to user as it may contain an invalid stirng !

    } else if (parse_assignment(buffer, key, value)) {
      if (StringIsEqual(key, "mode")) {
        current.mode = string_converter.Convert(value);
      } else if (StringIsEqual(key, "type")) {
        current.type = string_converter.Convert(value);
      } else if (StringIsEqual(key, "data")) {
        current.data = string_converter.Convert(value);
      } else if (StringIsEqual(key, "event")) {
        const std::string_view v{value};
        const auto [d_event, d_misc] = Split(v, ' ');

        if (d_event.empty()) {
          LogFormat("Invalid event type at %i", line);
          continue;
        }

        // TODO code: Consider reusing existing identical events

        pt2Event event = InputEvents::findEvent(string_converter.Convert(d_event));
        if (!event) {
          LogFmt("Invalid event type: {} at {}",
                 d_event, line);
          continue;
        }

        TCHAR *allocated = UnescapeBackslash(string_converter.Convert(d_misc));
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
      } else if (StringIsEqual(key, "label")) {
        current.label = string_converter.Convert(value);
      } else if (StringIsEqual(key, "location")) {
        current.location = ParseUnsigned(value);

      } else {
        LogFmt("Invalid key/value pair {}={} at {}", key, value, line);
      }
    } else  {
      LogFormat("Invalid line at %i", line);
    }

  }

  current.commit(config, line);
}
