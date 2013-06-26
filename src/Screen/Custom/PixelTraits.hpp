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

#include "Screen/PortableColor.hpp"
#include "Compiler.h"

#include <algorithm>

#include <string.h>

struct GreyscalePixelTraits {
  typedef Luminosity8 color_type;
  typedef uint8_t channel_type;
  typedef uint8_t integer_type;
  typedef color_type *pointer_type;
  typedef color_type *gcc_restrict rpointer_type;
  typedef const color_type *const_pointer_type;
  typedef const color_type *gcc_restrict const_rpointer_type;

  template<typename F>
  static color_type TransformInteger(color_type c, F f) {
    return f(c.GetLuminosity());
  }

  template<typename F>
  static color_type TransformInteger(color_type a, color_type b, F f) {
    return f(a.GetLuminosity(), b.GetLuminosity());
  }

  template<typename F>
  static color_type TransformChannels(color_type c, F f) {
    return f(c.GetLuminosity());
  }

  template<typename F>
  static color_type TransformChannels(color_type a, color_type b, F f) {
    return f(a.GetLuminosity(), b.GetLuminosity());
  }

  static constexpr bool IsBlack(color_type c) {
    return c.GetLuminosity() == 0;
  }

  static constexpr int CalcIncrement(int delta) {
    return delta;
  }

  static constexpr pointer_type Next(pointer_type p, int delta) {
    return p + CalcIncrement(delta);
  }

  static constexpr const_pointer_type Next(const_pointer_type p, int delta) {
    return p + CalcIncrement(delta);
  }

  static constexpr pointer_type NextByte(pointer_type p, int delta) {
    return pointer_type((uint8_t *)p + delta);
  }

  static constexpr const_pointer_type NextByte(const_pointer_type p,
                                               int delta) {
    return const_pointer_type((const uint8_t *)p + delta);
  }

  static constexpr pointer_type NextRow(pointer_type p,
                                        unsigned pitch, int delta) {
    return NextByte(p, int(pitch) * delta);
  }

  static constexpr const_pointer_type NextRow(const_pointer_type p,
                                              unsigned pitch, int delta) {
    return NextByte(p, int(pitch) * delta);
  }

  static constexpr pointer_type At(pointer_type p, unsigned pitch,
                                   int x, int y) {
    return Next(NextRow(p, pitch, y), x);
  }

  static constexpr const_pointer_type At(const_pointer_type p, unsigned pitch,
                                         int x, int y) {
    return Next(NextRow(p, pitch, y), x);
  }

  static color_type ReadPixel(const_pointer_type p) {
    return *p;
  }

  static void WritePixel(pointer_type p, color_type c) {
    *p = c;
  }

  static void FillPixels(pointer_type p, unsigned n, color_type c) {
    memset(p, c.GetLuminosity(), n);
  }

  static void CopyPixels(rpointer_type p, const_rpointer_type src,
                         unsigned n) {
    memcpy(p, src, n);
  }

  template<typename F>
  gcc_hot
  static void ForHorizontal(pointer_type p, unsigned n, F f) {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i));
  }

  template<typename F>
  gcc_hot
  static void ForHorizontal(rpointer_type p, const_rpointer_type q,
                            unsigned n, F f) {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i), Next(q, i));
  }

  template<typename F>
  gcc_hot
  static void ForVertical(pointer_type p, unsigned pitch, unsigned n, F f) {
    for (; n > 0; --n, p = NextByte(p, pitch))
      f(p);
  }

  template<typename SPT>
  struct Mixed {
    template<typename F>
    gcc_hot
    static void ForHorizontal(pointer_type p,
                              typename SPT::const_pointer_type q,
                              unsigned n, F f) {
      for (unsigned i = 0; i < n; ++i)
        f(Next(p, i), SPT::Next(q, i));
    }
  };
};

#ifndef GREYSCALE

struct BGRAPixelTraits {
  typedef BGRA8Color color_type;
  typedef uint8_t channel_type;
  typedef uint32_t integer_type;
  typedef color_type *pointer_type;
  typedef color_type *gcc_restrict rpointer_type;
  typedef const color_type *const_pointer_type;
  typedef const color_type *gcc_restrict const_rpointer_type;

  static constexpr integer_type ToInteger(color_type c) {
    return *reinterpret_cast<const integer_type *>(&c);
  }

  static constexpr color_type FromInteger(integer_type i) {
    return *reinterpret_cast<const color_type *>(&i);
  }

  template<typename F>
  static color_type TransformInteger(color_type c, F f) {
    return FromInteger(f(ToInteger(c)));
  }

  template<typename F>
  static color_type TransformInteger(color_type a, color_type b, F f) {
    return FromInteger(f(ToInteger(a), ToInteger(b)));
  }

  template<typename F>
  static color_type TransformChannels(color_type c, F f) {
    return BGRA8Color(f(c.Red()), f(c.Green()), f(c.Blue()), c.Alpha());
  }

  template<typename F>
  static color_type TransformChannels(color_type a, color_type b, F f) {
    return BGRA8Color(f(a.Red(), b.Red()),
                      f(a.Green(), b.Green()),
                      f(a.Blue(), b.Blue()),
                      a.Alpha());
  }

  static constexpr bool IsBlack(color_type c) {
    return c.Red() == 0 && c.Green() == 0 && c.Blue() == 0;
  }

  static constexpr int CalcIncrement(int delta) {
    return delta;
  }

  static constexpr pointer_type Next(pointer_type p, int delta) {
    return p + CalcIncrement(delta);
  }

  static constexpr const_pointer_type Next(const_pointer_type p, int delta) {
    return p + CalcIncrement(delta);
  }

  static constexpr pointer_type NextByte(pointer_type p, int delta) {
    return pointer_type((uint8_t *)p + delta);
  }

  static constexpr const_pointer_type NextByte(const_pointer_type p,
                                               int delta) {
    return const_pointer_type((const uint8_t *)p + delta);
  }

  static constexpr pointer_type NextRow(pointer_type p,
                                        unsigned pitch, int delta) {
    return NextByte(p, int(pitch) * delta);
  }

  static constexpr const_pointer_type NextRow(const_pointer_type p,
                                              unsigned pitch, int delta) {
    return NextByte(p, int(pitch) * delta);
  }

  static constexpr pointer_type At(pointer_type p, unsigned pitch,
                                   int x, int y) {
    return Next(NextRow(p, pitch, y), x);
  }

  static constexpr const_pointer_type At(const_pointer_type p, unsigned pitch,
                                         int x, int y) {
    return Next(NextRow(p, pitch, y), x);
  }

  static color_type ReadPixel(const_pointer_type p) {
    return *p;
  }

  static void WritePixel(pointer_type p, color_type c) {
    *p = c;
  }

  static void FillPixels(pointer_type p, unsigned n, color_type c) {
    std::fill_n(p, n, c);
  }

  static void CopyPixels(rpointer_type p,
                         const_rpointer_type src, unsigned n) {
    std::copy_n(src, n, p);
  }

  template<typename F>
  static void ForHorizontal(pointer_type p, unsigned n, F f) {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i));
  }

  template<typename F>
  static void ForHorizontal(rpointer_type p, const_rpointer_type q,
                            unsigned n, F f) {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i), Next(q, i));
  }

  template<typename F>
  static void ForVertical(pointer_type p, unsigned pitch, unsigned n, F f) {
    for (; n > 0; --n, p = NextByte(p, pitch))
      f(p);
  }
};

#endif

#endif
