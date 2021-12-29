/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "InputQueue.hpp"
#include "Menu/MenuData.hpp"
#include "util/RadixTree.hpp"
#include "util/StaticString.hxx"
#include "util/TrivialArray.hxx"
#include "util/TStringView.hxx"

#include <array>
#include <cassert>

#include <tchar.h>

struct InputConfig {
  // Sensible maximums

  static constexpr std::size_t MAX_MODE = 32;
  static constexpr std::size_t MAX_MODE_STRING = 24;
#ifdef ENABLE_SDL
  static constexpr std::size_t MAX_KEY = 400;
#elif defined(USE_X11)
  static constexpr std::size_t MAX_KEY = 0x100;
#elif defined(USE_POLL_EVENT)
  static constexpr std::size_t MAX_KEY = 0600;
#else
  static constexpr std::size_t MAX_KEY = 255;
#endif
  static constexpr std::size_t MAX_EVENTS = 2048;

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
  TrivialArray<StaticString<MAX_MODE_STRING>, MAX_MODE> modes;

  // Key map to Event - Keys (per mode) mapped to events
  unsigned short Key2Event[MAX_MODE][MAX_KEY];		// Points to Events location
#ifdef ENABLE_SDL
  /* In SDL2, keycodes without character representations are large values,
  AND-ed with SDLK_SCANCODE_MASK (0x40000000). A seperate array is therefore
  used here and the keycode is stored here with an index without
  SDLK_SCANCODE_MASK. */
  unsigned short Key2EventNonChar[MAX_MODE][MAX_KEY];
#endif

#ifdef USE_X11
  /**
   * Same as #Key2Event but with key code offset 0xff00.
   */
  unsigned short Key2EventFF00[MAX_MODE][MAX_KEY];
#endif

  RadixTree<unsigned> Gesture2Event;

  // Glide Computer Events
  std::array<short, GCE_COUNT> GC2Event;

  // NMEA Triggered Events
  std::array<short, NE_COUNT> N2Event;

  TrivialArray<Event, MAX_EVENTS> events;

  std::array<Menu, MAX_MODE> menus;

  void SetDefaults() noexcept;

  [[gnu::pure]]
  int LookupMode(TStringView name) const noexcept {
    for (std::size_t i = 0, size = modes.size(); i < size; ++i)
      if (name.Equals(modes[i].c_str()))
        return i;

    return -1;
  }

  int AppendMode(TStringView name) noexcept {
    if (modes.full())
      return -1;

    modes.append().assign(name.data, name.size);
    return modes.size() - 1;
  }

  [[gnu::pure]]
  int MakeMode(TStringView name) noexcept {
    int mode = LookupMode(name);
    if (mode < 0)
      mode = AppendMode(name);

    return mode;
  }

  std::size_t AppendEvent(pt2Event handler, const TCHAR *misc,
                          unsigned next) noexcept {
    if (events.full())
      return 0;

    Event &event = events.append();
    event.event = handler;
    event.misc = misc;
    event.next = next;

    return events.size() - 1;
  }

  void AppendMenu(std::size_t mode_id, const TCHAR *label,
                  unsigned location, unsigned event_id) noexcept {
    assert(mode_id < MAX_MODE);

    menus[mode_id].Add(label, location, event_id);
  }

  [[gnu::pure]]
  unsigned GetKeyEvent(unsigned mode, unsigned key_code) const noexcept;

  void SetKeyEvent(unsigned mode, unsigned key_code,
                   unsigned event_id) noexcept;

  [[gnu::pure]]
  const MenuItem &GetMenuItem(std::size_t mode,
                              std::size_t location) const noexcept {
    assert(mode < MAX_MODE);
    assert(location < Menu::MAX_ITEMS);

    return menus[mode][location];
  }
};

#endif
