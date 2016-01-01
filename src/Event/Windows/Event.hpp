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

#ifndef XCSOAR_EVENT_WINDOWS_EVENT_HPP
#define XCSOAR_EVENT_WINDOWS_EVENT_HPP

#include <windows.h>
#include <assert.h>

struct Event {
  MSG msg;

  bool IsKeyDown() const {
    return msg.message == WM_KEYDOWN;
  }

  bool IsKey() const {
    return IsKeyDown() || msg.message == WM_KEYUP;
  }

  unsigned GetKeyCode() const {
    assert(IsKey());

    return msg.wParam;
  }

  size_t GetCharacterCount() const {
    return msg.message == WM_CHAR ? 1 : 0;
  }

  unsigned GetCharacter(size_t characterIdx) const {
    assert(GetCharacterCount() == 1);
    assert(characterIdx == 0);

    return msg.wParam;
  }

  bool IsMouseDown() const {
    return msg.message == WM_LBUTTONDOWN;
  }

  bool IsMouse() const {
    return IsMouseDown() || msg.message == WM_LBUTTONUP ||
      msg.message == WM_LBUTTONDBLCLK;
  }

  bool IsUserInput() const {
    return IsKey() || (GetCharacterCount() > 0) || IsMouse();
  }
};

#endif
