// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputConfig.hpp"
#include "LogFile.hpp"
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

  // Convert lowercase letters to uppercase before lookup
  if (key_code >= 'a' && key_code <= 'z') {
    key_code = ToUpperASCII(key_code);
  }

  unsigned key_code_idx = key_code;
  auto key_2_event = Key2Event;

  LogFormat("GetKeyEvent: mode=%u, key_code=%u (after conversion=%u)", mode,
            key_code, key_code_idx);

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
