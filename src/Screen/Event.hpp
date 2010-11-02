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

#ifndef XCSOAR_SCREEN_EVENT_HPP
#define XCSOAR_SCREEN_EVENT_HPP

#include "Util/NonCopyable.hpp"
#include "Compiler.h"

#ifdef ENABLE_SDL

#include <SDL_version.h>
#include <SDL_events.h>

class TopWindow;

class EventLoop : private NonCopyable {
  TopWindow &top_window;

  /**
   * True if working on a bulk of events.  At the end of that bulk,
   * TopWindow::validate() gets called.
   */
  bool bulk;

public:
  EventLoop(TopWindow &_top_window)
    :top_window(_top_window), bulk(false) {}

  bool get(SDL_Event &event);
  void dispatch(SDL_Event &event);
};

#else /* !ENABLE_SDL */

#include <windows.h>

static inline bool
is_user_input(UINT message)
{
  return message == WM_KEYDOWN || message == WM_KEYUP ||
    message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ||
    message == WM_LBUTTONDBLCLK;
}

#endif /* !ENABLE_SDL */

gcc_const
unsigned
TranscodeKey(unsigned key_code);

#endif
