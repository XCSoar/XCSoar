/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/Point.hpp"
#include "LabelShape.hpp"

#include <tchar.h>

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

  LabelShape shape;
  Alignment align;
  VerticalPosition vertical_position;
  bool move_in_view;

  constexpr TextInBoxMode()
    :shape(LabelShape::SIMPLE), align(Alignment::LEFT),
     vertical_position(VerticalPosition::BELOW),
     move_in_view(false) {}
};

bool
TextInBox(Canvas &canvas, const TCHAR *value,
          PixelScalar x, PixelScalar y,
          TextInBoxMode mode, const PixelRect &map_rc,
          LabelBlock *label_block=nullptr);

bool
TextInBox(Canvas &canvas, const TCHAR *value, PixelScalar x, PixelScalar y,
          TextInBoxMode mode,
          UPixelScalar screen_width, UPixelScalar screen_height,
          LabelBlock *label_block=nullptr);

#endif
