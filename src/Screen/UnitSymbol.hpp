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

#ifndef XCSOAR_SCREEN_UNIT_SYMBOL_HPP
#define XCSOAR_SCREEN_UNIT_SYMBOL_HPP

#include "Screen/Bitmap.hpp"
#include "Screen/Point.hpp"
#include "Compiler.h"

class Canvas;

class UnitSymbol {
public:
  enum Kind {
    NORMAL = 0,
    INVERSE = 1,
    GRAY = 2,
    INVERSE_GRAY = INVERSE | GRAY,
  };

protected:
  Bitmap bitmap;
  PixelSize size;

public:
  void Load(unsigned id) {
    bitmap.Load(id, Bitmap::Type::MONO);
    size = bitmap.GetSize();
  }

  void Reset() {
    bitmap.Reset();
  }

  gcc_pure
  bool IsDefined() const {
    return bitmap.IsDefined();
  }

  const PixelSize GetSize() const {
    return size;
  }

  gcc_pure
  PixelSize GetScreenSize() const;

  void Draw(Canvas &canvas, PixelScalar x, PixelScalar y,
            Kind kind = NORMAL) const;
};

#endif

