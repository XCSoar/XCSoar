/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_UNIT_SYMBOL_HPP
#define XCSOAR_SCREEN_UNIT_SYMBOL_HPP

#include "Units/Units.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Point.hpp"

class Canvas;

class UnitSymbol {
public:
  enum kind {
    NORMAL = 0,
    INVERSE = 1,
    GRAY = 2,
    INVERSE_GRAY = INVERSE | GRAY,
  };

protected:
  Bitmap bitmap;
  PixelSize size;

public:
  void load(unsigned id, UPixelScalar width, UPixelScalar height) {
    bitmap.load(id);
    size.cx = width;
    size.cy = height;
  }

  void reset() {
    bitmap.reset();
  }

  gcc_pure
  bool defined() const {
    return bitmap.defined();
  }

  const Bitmap &get_bitmap() const {
    return bitmap;
  }

  operator const Bitmap &() const {
    return bitmap;
  }

  gcc_pure
  const RasterPoint get_origin(enum kind kind) const;

  const PixelSize get_size() const {
    return size;
  }

  void draw(Canvas& canvas, PixelScalar x, PixelScalar y) const;
};

void
LoadUnitSymbols();

void
DeinitialiseUnitSymbols();

const UnitSymbol *
GetUnitSymbol(Units_t unit);

#endif

