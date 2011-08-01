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

#ifndef XCSOAR_SCREEN_GDI_EVENT_HPP
#define XCSOAR_SCREEN_GDI_EVENT_HPP

#include "Util/NonCopyable.hpp"
#include "Compiler.h"

#include <windows.h>

static inline bool
is_user_input(UINT message)
{
  return message == WM_KEYDOWN || message == WM_KEYUP ||
    message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ||
    message == WM_LBUTTONDBLCLK;
}

static inline bool
is_user_input(const MSG &msg)
{
  return is_user_input(msg.message);
}

class EventLoop : private NonCopyable {
public:
  bool get(MSG &msg);
  void dispatch(const MSG &msg);
};

class DialogEventLoop : public EventLoop {
  HWND dialog;

public:
  DialogEventLoop(HWND _dialog):dialog(_dialog) {}

  void dispatch(MSG &msg);
};

namespace EventQueue {
  /**
   * Handle all pending repaint messages.
   */
  void
  HandlePaintMessages();
}

gcc_const
unsigned
TranscodeKey(unsigned key_code);

#endif
