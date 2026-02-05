// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class Menu;
class MenuBar;

namespace MenuGlue {

void
SetLabelText(MenuBar &bar, unsigned i,
             const char *text, unsigned event) noexcept;

  /**
   * Show the specified menu.
   *
   * @param full do a full update; if false, then only dynamic buttons
   * are updated (to reduce flickering)
   */
void
Set(MenuBar &bar, const Menu &menu,
    const Menu *overlay=nullptr, bool full=true) noexcept;

} // namespace MenuGlue
