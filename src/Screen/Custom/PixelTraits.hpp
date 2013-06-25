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

#ifndef XCSOAR_SCREEN_PIXEL_TRAITS_HPP
#define XCSOAR_SCREEN_PIXEL_TRAITS_HPP

#include "Compiler.h"

#include <string.h>

struct GreyscalePixelTraits {
  typedef uint8_t *pointer_type;
  typedef const uint8_t *const_pointer_type;
  typedef uint8_t color_type;

  static int CalcIncrement(int delta) {
    return delta;
  }

  static pointer_type Next(pointer_type p, int delta) {
    return p + CalcIncrement(delta);
  }

  static const_pointer_type Next(const_pointer_type p, int delta) {
    return p + CalcIncrement(delta);
  }

  static color_type ReadPixel(const_pointer_type p) {
    return *p;
  }

  static void WritePixel(pointer_type p, color_type c) {
    *p = c;
  }

  static void FillPixels(pointer_type p, unsigned n, color_type c) {
    memset(p, c, n);
  }

  static void CopyPixels(pointer_type gcc_restrict p,
                         const_pointer_type gcc_restrict src, unsigned n) {
    memcpy(p, src, n);
  }

  template<typename F>
  static void ForHorizontal(pointer_type p, unsigned n, F f) {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i));
  }

  template<typename F>
  static void ForHorizontal(pointer_type p, const_pointer_type q,
                            unsigned n, F f) {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i), Next(q, i));
  }

  template<typename F>
  static void ForVertical(pointer_type p, unsigned pitch, unsigned n, F f) {
    for (; n > 0; --n, p = Next(p, pitch))
      f(p);
  }
};

#endif
