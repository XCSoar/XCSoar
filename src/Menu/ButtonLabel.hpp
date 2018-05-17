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

#ifndef BUTTON_LABEL_HPP
#define BUTTON_LABEL_HPP

#include "Compiler.h"

#include <tchar.h>
#include <stddef.h>

struct PixelRect;
struct ButtonLook;
class Font;
class ContainerWindow;
class Menu;

namespace ButtonLabel {
  struct Expanded {
    bool visible, enabled;
    const TCHAR *text;
  };

  void CreateButtonLabels(ContainerWindow &parent, ButtonLook &look);
  void Destroy();

  gcc_pure
  Expanded Expand(const TCHAR *text, TCHAR *buffer, size_t size);

  void SetLabelText(unsigned i, const TCHAR *text, unsigned event);

  gcc_pure
  bool IsEnabled(unsigned i);

  bool ExpandMacros(const TCHAR *In, TCHAR *OutBuffer, size_t Size);

  void OnResize(const PixelRect &rc);

  /**
   * Show the specified menu.
   *
   * @param full do a full update; if false, then only dynamic buttons
   * are updated (to reduce flickering)
   */
  void Set(const Menu &menu, const Menu *overlay=nullptr, bool full=true);
};

#endif
