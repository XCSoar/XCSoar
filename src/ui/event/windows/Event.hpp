// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cassert>

#include <windef.h> // for HWND (needed by winuser.h)
#include <winuser.h>

namespace UI {

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

} // namespace UI
