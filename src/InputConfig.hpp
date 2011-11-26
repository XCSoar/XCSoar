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

#ifndef XCSOAR_INPUT_CONFIG_HPP
#define XCSOAR_INPUT_CONFIG_HPP

#include "InputEvents.hpp"
#include "MenuData.hpp"
#include "Util/RadixTree.hpp"
#include "Util/StaticString.hpp"

#include <assert.h>
#include <tchar.h>

struct InputConfig {
  // Sensible maximums
  enum {
    MAX_MODE = 32,
    MAX_MODE_STRING = 24,
#if defined(ENABLE_SDL) && !defined(ANDROID)
    MAX_KEY = 400,
#else
    MAX_KEY = 255,
#endif
    MAX_EVENTS = 2048,
  };

  typedef void (*pt2Event)(const TCHAR *);

  // Events - What do you want to DO
  struct Event {
    // Which function to call (can be any, but should be here)
    pt2Event event;
    // Parameters
    const TCHAR *misc;
    // Next in event list - eg: Macros
    unsigned next;
  };

  /** Map mode to location */
  StaticString<MAX_MODE_STRING> mode_map[MAX_MODE];
  unsigned mode_map_count;

  // Key map to Event - Keys (per mode) mapped to events
  unsigned short Key2Event[MAX_MODE][MAX_KEY];		// Points to Events location

  RadixTree<unsigned> Gesture2Event[MAX_MODE];

  // Glide Computer Events
  unsigned short GC2Event[MAX_MODE][GCE_COUNT];

  // NMEA Triggered Events
  unsigned short N2Event[MAX_MODE][NE_COUNT];

  Event Events[MAX_EVENTS];
  unsigned Events_count;

  Menu menus[MAX_MODE];

  void SetDefaults();

  gcc_pure
  int LookupMode(const TCHAR *name) const {
    for (unsigned i = 0; i < mode_map_count; ++i)
      if (mode_map[i] == name)
        return i;

    return -1;
  }

  int AppendMode(const TCHAR *name) {
    if (mode_map_count >= MAX_MODE)
      return -1;

    mode_map[mode_map_count] = name;
    return mode_map_count++;
  }

  gcc_pure
  int MakeMode(const TCHAR *name) {
    int mode = LookupMode(name);
    if (mode < 0)
      mode = AppendMode(name);

    return mode;
  }

  unsigned AppendEvent(pt2Event event, const TCHAR *misc,
                       unsigned next) {
    if (Events_count >= MAX_EVENTS)
      return 0;

    Events[Events_count].event = event;
    Events[Events_count].misc = misc;
    Events[Events_count].next = next;

    return Events_count++;
  }

  void AppendMenu(unsigned mode_id, const TCHAR* label,
                  unsigned location, unsigned event_id) {
    assert(mode_id < MAX_MODE);

    menus[mode_id].Add(label, location, event_id);
  }
};

#endif
