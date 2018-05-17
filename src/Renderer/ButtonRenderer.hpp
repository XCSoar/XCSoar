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

#ifndef XCSOAR_BUTTON_RENDERER_HPP
#define XCSOAR_BUTTON_RENDERER_HPP

#include "Compiler.h"

struct PixelRect;
struct ButtonLook;
class Canvas;

class ButtonFrameRenderer {
  const ButtonLook &look;

public:
  explicit ButtonFrameRenderer(const ButtonLook &_look):look(_look) {}

  const ButtonLook &GetLook() const {
    return look;
  }

  gcc_const
  static unsigned GetMargin();

  void DrawButton(Canvas &canvas, PixelRect rc,
                  bool focused, bool pressed) const;

  gcc_pure
  PixelRect GetDrawingRect(PixelRect rc, bool pressed) const;
};

class ButtonRenderer {
public:
  virtual ~ButtonRenderer() {}

  gcc_pure
  virtual unsigned GetMinimumButtonWidth() const;

  virtual void DrawButton(Canvas &canvas, const PixelRect &rc,
                          bool enabled, bool focused, bool pressed) const = 0;
};

#endif
