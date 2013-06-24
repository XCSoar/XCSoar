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

#ifndef XCSOAR_SCREEN_ICON_HPP
#define XCSOAR_SCREEN_ICON_HPP

#include "Screen/Bitmap.hpp"
#include "Screen/Point.hpp"

class Canvas;

/**
 * An icon with a mask which marks transparent pixels.
 */
class MaskedIcon {
protected:
  Bitmap bitmap;

  PixelSize size;

  RasterPoint origin;

public:
  const PixelSize &GetSize() const {
    return size;
  }

  bool IsDefined() const {
    return bitmap.IsDefined();
  }

  void LoadResource(unsigned id, unsigned big_id = 0, bool center=true);

  void Reset() {
    bitmap.Reset();
  }

  void Draw(Canvas &canvas, PixelScalar x, PixelScalar y) const;
  void Draw(Canvas &canvas, RasterPoint pt) const {
    Draw(canvas, pt.x, pt.y);
  }

protected:
  void CalculateLayout(bool center);
};

#endif
