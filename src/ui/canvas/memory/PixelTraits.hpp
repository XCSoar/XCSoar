/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ui/canvas/PortableColor.hpp"
#include "util/ByteOrder.hxx"
#include "util/Cast.hxx"
#include "util/Compiler.h"
#include "util/OffsetPointer.hxx"

#include <algorithm>

#include <string.h>

/**
 * Description for the 8 bit greyscale pixel format.
 *
 * About PixelTraits: this class contains only typedefs and static
 * methods.  It provides operations to manipulate image buffers.
 */
struct GreyscalePixelTraits {
  /**
   * One color for passing as parameter or return type.
   */
  typedef Luminosity8 color_type;

  /**
   * An integer type that holds the value of one channel.  It is used
   * by TransformChannels().
   */
  typedef uint8_t channel_type;

  /**
   * An integer type that may hold more than one channel.  It is used
   * by TransformInteger().
   */
  typedef uint8_t integer_type;

  /**
   * A pointer/iterator to a writable image buffer.
   *
   * Note that some pixel formats may decide not to use #color_type
   * here.  Make no assumptions on the nature of this type, don't
   * dereference.  This class provides functions for all of this, use
   * them.
   */
  typedef color_type *pointer;

  /**
   * Same as #pointer, but with "restrict".  This guarantees the
   * compiler that there will be no aliasing, and allows the compiler
   * to apply more optimisations, e.g. auto vectorisation.
   */
  typedef color_type *gcc_restrict rpointer;

  /**
   * A pointer/iterator to a read-only image buffer.
   */
  typedef const color_type *const_pointer;

  /**
   * Like #rpointer, but read-only.
   */
  typedef const color_type *gcc_restrict const_rpointer;

  /**
   * Transform a color by passing integers to the given functions.
   * The function must be able to deal with more than channel at a
   * time.  A classic example would be bit-wise OR/AND.  It does not
   * work with arithmetics such as "add", because it may overflow one
   * channel and bleed to the next.
   */
  template<typename F>
  static color_type TransformInteger(color_type c, F f) {
    return f(c.GetLuminosity());
  }

  template<typename F>
  static color_type TransformInteger(color_type a, color_type b, F f) {
    return f(a.GetLuminosity(), b.GetLuminosity());
  }

  /**
   * Transform a color by calling the given function for each channel.
   */
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

  static constexpr bool IsWhite(color_type c) {
    return c.GetLuminosity() == 0xff;
  }

  /**
   * How much to add to this pointer to go #delta pixels ahead?
   */
  static constexpr int CalcIncrement(int delta) {
    return delta;
  }

  /**
   * Calculate a pointer to the pixel with the given offset.
   */
  static constexpr pointer Next(pointer p, int delta) {
    return p + CalcIncrement(delta);
  }

  static constexpr const_pointer Next(const_pointer p, int delta) {
    return p + CalcIncrement(delta);
  }

  static constexpr pointer NextByte(pointer p, int delta) {
    return OffsetCast<color_type>(p, delta);
  }

  static constexpr const_pointer NextByte(const_pointer p,
                                               int delta) {
    return OffsetCast<const color_type>(p, delta);
  }

  /**
   * Calculate a pointer to the pixel with the given row offset.
   *
   * @param pitch the number of bytes per row
   */
  static constexpr pointer NextRow(pointer p,
                                        unsigned pitch, int delta) {
    return NextByte(p, int(pitch) * delta);
  }

  static constexpr const_pointer NextRow(const_pointer p,
                                              unsigned pitch, int delta) {
    return NextByte(p, int(pitch) * delta);
  }

  /**
   * Calculate a pointer to the pixel with the given 2D offset.
   *
   * @param pitch the number of bytes per row
   */
  static constexpr pointer At(pointer p, unsigned pitch,
                                   int x, int y) {
    return Next(NextRow(p, pitch, y), x);
  }

  static constexpr const_pointer At(const_pointer p, unsigned pitch,
                                         int x, int y) {
    return Next(NextRow(p, pitch, y), x);
  }

  /**
   * Read the pixel at the location pointed to.
   */
  static color_type ReadPixel(const_pointer p) {
    return *p;
  }

  /**
   * Write the pixel at the location pointed to.
   */
  static void WritePixel(pointer p, color_type c) {
    *p = c;
  }

  /**
   * Fill #n horizontal pixels with the given color.
   */
  static void FillPixels(pointer p, unsigned n, color_type c) {
    std::fill_n(p, n, c.GetLuminosity());
  }

  /**
   * Copy #n horizontal pixels from #src.
   */
  static void CopyPixels(rpointer p, const_rpointer src,
                         unsigned n) {
    std::copy_n(src, n, p);
  }

  /**
   * Call the given function for the next #n pixels in the current
   * row.  Pass the pointer to each pixel.
   */
  template<typename F>
  gcc_hot
  static void ForHorizontal(pointer p, unsigned n, F f) {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i));
  }

  template<typename F>
  gcc_hot
  static void ForHorizontal(rpointer p, const_rpointer q,
                            unsigned n, F f) {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i), Next(q, i));
  }

  /**
   * Call the given function for the next #n pixels in the current
   * column.  Pass the pointer to each pixel.
   */
  template<typename F>
  gcc_hot
  static void ForVertical(pointer p, unsigned pitch, unsigned n, F f) {
    for (; n > 0; --n, p = NextByte(p, pitch))
      f(p);
  }

  /**
   * Container for mixed operations, to allow operations with pointers
   * to two different image buffers with two different pixel formats.
   *
   * @param SPT the source pixel format
   */
  template<typename SPT>
  struct Mixed {
    template<typename F>
    gcc_hot
    static void ForHorizontal(pointer p,
                              typename SPT::const_pointer q,
                              unsigned n, F f) {
      for (unsigned i = 0; i < n; ++i)
        f(Next(p, i), SPT::Next(q, i));
    }
  };
};

#ifndef GREYSCALE

/**
 * Description for the 32 bit pixel format with four channels: blue,
 * green, red and alpha.
 *
 * See #GreyscalePixelTraits for documentation about PixelTraits.
 */
struct BGRAPixelTraits {
  typedef BGRA8Color color_type;
  typedef uint8_t channel_type;
  typedef uint32_t integer_type;
  typedef color_type *pointer;
  typedef color_type *gcc_restrict rpointer;
  typedef const color_type *const_pointer;
  typedef const color_type *gcc_restrict const_rpointer;

  static_assert(sizeof(color_type) == sizeof(integer_type),
                "Wrong integer_type");

  union U {
    color_type c;
    integer_type i;

    constexpr U(color_type _c):c(_c) {}
    constexpr U(integer_type _i):i(_i) {}
  };

  static constexpr integer_type ToInteger(color_type c) {
    return U(c).i;
  }

  static constexpr color_type FromInteger(integer_type i) {
    return U(i).c;
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

  static constexpr bool IsWhite(color_type c) {
    return IsLittleEndian()
      ? (ToInteger(c) & 0xffffff) == 0xffffff
      : (ToInteger(c) & 0xffffff00) == 0xffffff00;
  }

  static constexpr int CalcIncrement(int delta) {
    return delta;
  }

  static constexpr pointer Next(pointer p, int delta) {
    return p + CalcIncrement(delta);
  }

  static constexpr const_pointer Next(const_pointer p, int delta) {
    return p + CalcIncrement(delta);
  }

  static constexpr pointer NextByte(pointer p, int delta) {
    return (pointer)OffsetPointer(p, delta);
  }

  static constexpr const_pointer NextByte(const_pointer p,
                                               int delta) {
    return (const_pointer)OffsetPointer(p, delta);
  }

  static constexpr pointer NextRow(pointer p,
                                        unsigned pitch, int delta) {
    return NextByte(p, int(pitch) * delta);
  }

  static constexpr const_pointer NextRow(const_pointer p,
                                              unsigned pitch, int delta) {
    return NextByte(p, int(pitch) * delta);
  }

  static constexpr pointer At(pointer p, unsigned pitch,
                                   int x, int y) {
    return Next(NextRow(p, pitch, y), x);
  }

  static constexpr const_pointer At(const_pointer p, unsigned pitch,
                                         int x, int y) {
    return Next(NextRow(p, pitch, y), x);
  }

  static color_type ReadPixel(const_pointer p) {
    const integer_type *const pi = reinterpret_cast<const integer_type *>(p);
    return FromInteger(*pi);
  }

  static void WritePixel(pointer p, color_type c) {
    integer_type *const pi = reinterpret_cast<integer_type *>(p);
    *pi = ToInteger(c);
  }

  static void FillPixels(pointer p, unsigned n, color_type c) {
    /* gcc is pretty bad at optimising BGRA8Color assignment; the
       following switches to 32 bit integer operations */
    integer_type *const pi = reinterpret_cast<integer_type *>(p);
    const integer_type ci = ToInteger(c);

#if defined(__GNUC__) && defined(__x86_64__)
    const uint64_t cl = (uint64_t(ci) << 32) | uint64_t(ci);

    gcc_unused size_t dummy0, dummy1;
    asm volatile("cld\n"

                 /* n /= 2 (remainder is moved to the "Carry Flag") */
                 "shr %3\n"

                 /* fill two pixels at a time */
                 "rep stosq\n"

                 /* fill the remaining pixel (if any) */
                 "jnc 0f\n"
                 "stosl\n"
                 "0:\n"

                 : "=&c"(dummy0), "=&D"(dummy1)
                 : "a"(cl), "0"(size_t(n)), "1"(pi)
                 : "memory"
                 );
#else
    std::fill_n(pi, n, ci);
#endif
  }

  static void CopyPixels(rpointer p,
                         const_rpointer src, unsigned n) {
    std::copy_n(src, n, p);
  }

  template<typename F>
  static void ForHorizontal(pointer p, unsigned n, F f) {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i));
  }

  template<typename F>
  static void ForHorizontal(rpointer p, const_rpointer q,
                            unsigned n, F f) {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i), Next(q, i));
  }

  template<typename F>
  static void ForVertical(pointer p, unsigned pitch, unsigned n, F f) {
    for (; n > 0; --n, p = NextByte(p, pitch))
      f(p);
  }
};

#endif

#endif
