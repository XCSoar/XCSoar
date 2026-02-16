// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Input/InputParser.hpp"
#include "Input/InputConfig.hpp"
#include "Input/InputLookup.hpp"
#include "Menu/MenuData.hpp"
#include "io/FileLineReader.hpp"
#include "system/Args.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <tchar.h>

pt2Event
InputEvents::findEvent(tstring_view name) noexcept
{
  union {
    const char *in;
    pt2Event out;
  } u;

  u.in = name.data();
  return u.out;
}

int
InputEvents::findGCE([[maybe_unused]] const char *data)
{
  return -1;
}

int
InputEvents::findNE([[maybe_unused]] const char *data)
{
  return -1;
}

static void
Dump(InputConfig::Event &event, unsigned id)
{
  _tprintf(_T("    Event[%u]: '%s' misc='%s'\n"), id,
           (const char *)event.event, event.misc);
}

int main(int argc, char **argv)
try {
  Args args(argc, argv, "PATH");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  InputConfig config;
  config.SetDefaults();

  {
    FileReader file_reader{path};
    BufferedReader reader{file_reader};
    ParseInputFile(config, reader);
  }

  for (unsigned mode = 0; mode < config.modes.size(); ++mode) {
    _tprintf(_T("Mode '%s'\n"), config.modes[mode].c_str());

    for (unsigned key = 0; key < InputConfig::MAX_KEY; ++key) {
      unsigned event = config.Key2Event[mode][key];
      assert(event < InputConfig::MAX_EVENTS);
      if (event == 0)
        continue;

      printf("  Key 0x%x\n", key);
      do {
        Dump(config.events[event], event);
        assert(config.events[event].next < InputConfig::MAX_EVENTS);
        event = config.events[event].next;
      } while (event > 0);
    }

    for (unsigned i = 0; i < Menu::MAX_ITEMS; ++i) {
      const MenuItem &mi = config.menus[mode][i];
      if (mi.IsDefined()) {
        _tprintf(_T("  Menu[%u] = '%s'\n"), i, mi.label);
        unsigned event = mi.event;
        assert(event < InputConfig::MAX_EVENTS);
        do {
          Dump(config.events[event], event);
          assert(config.events[event].next < InputConfig::MAX_EVENTS);
          event = config.events[event].next;
        } while (event > 0);
      }
    }
  }

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
