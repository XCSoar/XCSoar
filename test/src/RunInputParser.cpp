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

#include "InputParser.hpp"
#include "InputConfig.hpp"
#include "InputEvents.hpp"
#include "MenuData.hpp"
#include "IO/FileLineReader.hpp"

#include <stdio.h>
#include <tchar.h>

unsigned
InputEvents::findKey(const TCHAR *data)
{
  return 42;
}

pt2Event
InputEvents::findEvent(const TCHAR *data)
{
  union {
    const TCHAR *in;
    pt2Event out;
  } u;

  u.in = data;
  return u.out;
}

int
InputEvents::findGCE(const TCHAR *data)
{
  return -1;
}

int
InputEvents::findNE(const TCHAR *data)
{
  return -1;
}

static void
Dump(InputConfig::Event &event, unsigned id)
{
  _tprintf(_T("    Event[%u]: '%s' misc='%s'\n"), id,
           (const TCHAR *)event.event, event.misc);
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s PATH\n", argv[0]);
    return 1;
  }

  FileLineReader reader(argv[1]);
  if (reader.error()) {
    fprintf(stderr, "Failed to open input file\n");
    return 1;
  }

  InputConfig config;
  ParseInputFile(config, reader);

  for (unsigned mode = 0; mode < config.mode_map_count; ++mode) {
    _tprintf(_T("Mode '%s'\n"), config.mode_map[mode]);

    for (unsigned key = 0; key < InputConfig::MAX_KEY; ++key) {
      unsigned event = config.Key2Event[mode][key];
      assert(event < InputConfig::MAX_EVENTS);
      if (event == 0)
        continue;

      printf("  Key 0x%x\n", key);
      do {
        Dump(config.Events[event], event);
        assert(config.Events[event].next < InputConfig::MAX_EVENTS);
        event = config.Events[event].next;
      } while (event > 0);
    }

    for (unsigned i = 0; i < Menu::MAX_ITEMS; ++i) {
      const MenuItem &mi = config.menus[mode][i];
      if (mi.defined()) {
        _tprintf(_T("  Menu[%u] = '%s'\n"), i, mi.label);
        unsigned event = mi.event;
        assert(event < InputConfig::MAX_EVENTS);
        do {
          Dump(config.Events[event], event);
          assert(config.Events[event].next < InputConfig::MAX_EVENTS);
          event = config.Events[event].next;
        } while (event > 0);
      }
    }
  }

  return 0;
}
