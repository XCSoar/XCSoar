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

#ifndef XCSOAR_TEXT_RENDERER_HPP
#define XCSOAR_TEXT_RENDERER_HPP

#include "Compiler.h"

#include <tchar.h>

struct PixelRect;
class Canvas;
class Font;

/**
 * Render multi-line text.
 */
class TextRenderer {
  bool center, vcenter;

#ifndef USE_GDI
  bool control;
#endif

public:
  TextRenderer()
    :center(false), vcenter(false)
#ifndef USE_GDI
    , control(false)
#endif
  {}

  void SetCenter(bool _center=true) {
    center = _center;
  }

  void SetVCenter(bool _vcenter=true) {
    vcenter = _vcenter;
  }

  void SetControl(bool _control=true) {
#ifndef USE_GDI
    control = _control;
#endif
  }

  void InvalidateLayout() {}

  gcc_pure
  unsigned GetHeight(Canvas &canvas, PixelRect rc, const TCHAR *text) const;

  gcc_pure
  unsigned GetHeight(Canvas &canvas, unsigned width, const TCHAR *text) const;

  gcc_pure
  unsigned GetHeight(const Font &font, unsigned width,
                     const TCHAR *text) const;

  void Draw(Canvas &canvas, PixelRect rc, const TCHAR *text) const;
};

#endif
