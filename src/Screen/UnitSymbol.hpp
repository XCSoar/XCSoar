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

#include "Screen/Point.hpp"
#include "Compiler.h"

#if defined(USE_GDI) || defined(ENABLE_OPENGL)
#include "Screen/Bitmap.hpp"
#else
#include "Screen/Custom/Buffer.hpp"
#include "Screen/Custom/PixelTraits.hpp"
#endif

class Canvas;
class Color;

class UnitSymbol {
#if defined(USE_GDI) || defined(ENABLE_OPENGL)
  Bitmap bitmap;
  PixelSize size;
#else
  WritableImageBuffer<GreyscalePixelTraits> buffer;
#endif

public:
#if defined(USE_GDI) || defined(ENABLE_OPENGL)
  void Load(unsigned id) {
    bitmap.Load(id, Bitmap::Type::MONO);
    size = bitmap.GetSize();
  }
#else
  UnitSymbol() {
    buffer.data = nullptr;
  }

  ~UnitSymbol() {
    delete[] buffer.data;
  }

  void Load(unsigned id);
#endif

  void Reset() {
#if defined(USE_GDI) || defined(ENABLE_OPENGL)
    bitmap.Reset();
#else
    delete[] buffer.data;
    buffer.data = nullptr;
#endif
  }

  gcc_pure
  bool IsDefined() const {
#if defined(USE_GDI) || defined(ENABLE_OPENGL)
    return bitmap.IsDefined();
#else
    return buffer.data != nullptr;
#endif
  }

  const PixelSize GetSize() const {
#if defined(USE_GDI) || defined(ENABLE_OPENGL)
    return size;
#else
    return { buffer.width, buffer.height };
#endif
  }

  gcc_pure
  PixelSize GetScreenSize() const;

  void Draw(Canvas &canvas, PixelScalar x, PixelScalar y,
            Color bg_color, Color text_color) const;
};

#endif

