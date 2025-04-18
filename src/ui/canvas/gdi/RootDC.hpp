// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <windef.h> // for HWND (needed by winuser.h)
#include <winuser.h>

/**
 * A HDC pointing to the root window, i.e. the whole screen.
 */
class RootDC {
  HDC dc;

public:
  RootDC()
    :dc(::GetDC(nullptr)) {}

  ~RootDC() {
    ::ReleaseDC(nullptr, dc);
  }

  operator HDC() {
    return dc;
  }
};
