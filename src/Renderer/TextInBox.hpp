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

#ifndef XCSOAR_SCREEN_TEXT_IN_BOX_HPP
#define XCSOAR_SCREEN_TEXT_IN_BOX_HPP

#include "LabelShape.hpp"

#include <tchar.h>

struct PixelRect;
class Canvas;
class LabelBlock;

struct TextInBoxMode {
  enum Alignment : uint8_t {
    LEFT,
    CENTER,
    RIGHT,
  };

  enum VerticalPosition : uint8_t {
    ABOVE,
    CENTERED,
    BELOW,
  };

  LabelShape shape = LabelShape::SIMPLE;
  Alignment align = Alignment::LEFT;
  VerticalPosition vertical_position = VerticalPosition::BELOW;
  bool move_in_view = false;
};

bool
TextInBox(Canvas &canvas, const TCHAR *value,
          int x, int y,
          TextInBoxMode mode, const PixelRect &map_rc,
          LabelBlock *label_block=nullptr);

bool
TextInBox(Canvas &canvas, const TCHAR *value, int x, int y,
          TextInBoxMode mode,
          unsigned screen_width, unsigned screen_height,
          LabelBlock *label_block=nullptr);

#endif
