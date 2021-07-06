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

#include "InputConfig.hpp"
#include "util/Macros.hpp"

#ifdef ENABLE_SDL
#include <SDL_keycode.h>
#endif

#include <algorithm>

void
InputConfig::SetDefaults() noexcept
{
  modes.resize(4);
  modes[0] = _T("default");
  modes[1] = _T("pan");
  modes[2] = _T("infobox");
  modes[3] = _T("Menu");

  std::fill_n(&Key2Event[0][0], MAX_MODE*MAX_KEY, 0);
#ifdef ENABLE_SDL
  std::fill_n(&Key2EventNonChar[0][0], MAX_MODE*MAX_KEY, 0);
#endif
#ifdef USE_X11
  std::fill_n(&Key2EventFF00[0][0], MAX_MODE * MAX_KEY, 0);
#endif

  Gesture2Event.Clear();

  std::fill(GC2Event.begin(), GC2Event.end(), 0);
  std::fill(N2Event.begin(), N2Event.end(), 0);

  /* This is initialized with 1 because event 0 is reserved - it
     stands for "no event" */
  events.resize(1);

  for (auto &i : menus)
    i.Clear();
}

unsigned
InputConfig::GetKeyEvent(unsigned mode, unsigned key_code) const noexcept
{
  assert(mode < MAX_MODE);

  unsigned key_code_idx = key_code;
  auto key_2_event = Key2Event;
#ifdef ENABLE_SDL
  if (key_code & SDLK_SCANCODE_MASK) {
    key_code_idx = key_code & ~SDLK_SCANCODE_MASK;
    key_2_event = Key2EventNonChar;
  }
#endif

#ifdef USE_X11
  if (key_code_idx >= 0xff00) {
    key_code_idx -= 0xff00;
    key_2_event = Key2EventFF00;
  }
#endif

  if (key_code_idx >= MAX_KEY)
    return 0;

  if (mode > 0 && key_2_event[mode][key_code_idx] != 0)
    return key_2_event[mode][key_code_idx];

  /* fall back to the default mode */
  return key_2_event[0][key_code_idx];
}

void
InputConfig::SetKeyEvent(unsigned mode, unsigned key_code,
                         unsigned event_id) noexcept
{
  assert(mode < MAX_MODE);

  auto key_2_event = Key2Event;
#ifdef ENABLE_SDL
  if (key_code & SDLK_SCANCODE_MASK) {
    key_2_event = Key2EventNonChar;
    key_code &= ~SDLK_SCANCODE_MASK;
  }
#endif

#ifdef USE_X11
  if (key_code >= 0xff00) {
    key_code -= 0xff00;
    key_2_event = Key2EventFF00;
  }
#endif

  if (key_code < MAX_KEY)
    key_2_event[mode][key_code] = event_id;
}
