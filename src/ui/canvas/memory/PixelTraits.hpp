// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Concepts.hpp"
#include "ui/canvas/PortableColor.hpp"
#include "util/ByteOrder.hxx"
#include "util/Cast.hxx"
#include "util/Compiler.h"
#include "util/OffsetPointer.hxx"

#include <algorithm>
#include <concepts>
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
  using color_type = Luminosity8;

  /**
   * An integer type that holds the value of one channel.  It is used
   * by TransformChannels().
   */
  using channel_type = uint8_t;

  /**
   * An integer type that may hold more than one channel.  It is used
   * by TransformInteger().
   */
  using integer_type = uint8_t;

  /**
   * A pointer/iterator to a writable image buffer.
   *
   * Note that some pixel formats may decide not to use #color_type
   * here.  Make no assumptions on the nature of this type, don't
   * dereference.  This class provides functions for all of this, use
   * them.
   */
  using pointer = color_type *;

  /**
   * Same as #pointer, but with "restrict".  This guarantees the
   * compiler that there will be no aliasing, and allows the compiler
   * to apply more optimisations, e.g. auto vectorisation.
   */
  using rpointer = color_type *gcc_restrict;

  /**
   * A pointer/iterator to a read-only image buffer.
   */
  using const_pointer = const color_type *;

  /**
   * Like #rpointer, but read-only.
   */
  using const_rpointer = const color_type *gcc_restrict;

  /**
   * Transform a color by passing integers to the given functions.
   * The function must be able to deal with more than channel at a
   * time.  A classic example would be bit-wise OR/AND.  It does not
   * work with arithmetics such as "add", because it may overflow one
   * channel and bleed to the next.
   */
  static constexpr color_type TransformInteger(color_type c,
                                               std::invocable<integer_type> auto f) noexcept {
    return f(c.GetLuminosity());
  }

  static constexpr color_type TransformInteger(color_type a, color_type b,
                                               std::invocable<integer_type, integer_type> auto f) noexcept {
    return f(a.GetLuminosity(), b.GetLuminosity());
  }

  /**
   * Transform a color by calling the given function for each channel.
   */
  static constexpr color_type TransformChannels(color_type c,
                                                std::invocable<channel_type> auto f) noexcept {
    return f(c.GetLuminosity());
  }

  static constexpr color_type TransformChannels(color_type a, color_type b,
                                                std::invocable<channel_type, channel_type> auto f) noexcept {
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
  static constexpr std::ptrdiff_t CalcIncrement(std::ptrdiff_t delta) noexcept {
    return delta;
  }

  /**
   * Calculate a pointer to the pixel with the given offset.
   */
  static constexpr pointer Next(pointer p, std::ptrdiff_t delta) noexcept {
    return p + CalcIncrement(delta);
  }

  static constexpr const_pointer Next(const_pointer p, std::ptrdiff_t delta) noexcept {
    return p + CalcIncrement(delta);
  }

  static constexpr pointer NextByte(pointer p, std::ptrdiff_t delta) noexcept {
    return OffsetCast<color_type>(p, delta);
  }

  static constexpr const_pointer NextByte(const_pointer p,
                                          std::ptrdiff_t delta) noexcept {
    return OffsetCast<const color_type>(p, delta);
  }

  /**
   * Calculate a pointer to the pixel with the given row offset.
   *
   * @param pitch the number of bytes per row
   */
  static constexpr pointer NextRow(pointer p, std::size_t pitch,
                                   std::ptrdiff_t delta) noexcept {
    return NextByte(p, int(pitch) * delta);
  }

  static constexpr const_pointer NextRow(const_pointer p, std::size_t pitch,
                                         std::ptrdiff_t delta) noexcept {
    return NextByte(p, int(pitch) * delta);
  }

  /**
   * Calculate a pointer to the pixel with the given 2D offset.
   *
   * @param pitch the number of bytes per row
   */
  static constexpr pointer At(pointer p, std::size_t pitch,
                              int x, int y) noexcept {
    return Next(NextRow(p, pitch, y), x);
  }

  static constexpr const_pointer At(const_pointer p, std::size_t pitch,
                                    int x, int y) noexcept {
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
  [[gnu::hot]]
  static constexpr void ForHorizontal(pointer p, unsigned n,
                                      std::invocable<pointer> auto f) noexcept {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i));
  }

  [[gnu::hot]]
  static constexpr void ForHorizontal(rpointer p, const_rpointer q, unsigned n,
                                      std::invocable<pointer> auto f) noexcept {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i), Next(q, i));
  }

  /**
   * Call the given function for the next #n pixels in the current
   * column.  Pass the pointer to each pixel.
   */
  [[gnu::hot]]
  static constexpr void ForVertical(pointer p, std::size_t pitch, unsigned n,
                                    std::invocable<pointer> auto f) noexcept {
    for (; n > 0; --n, p = NextByte(p, pitch))
      f(p);
  }

  /**
   * Container for mixed operations, to allow operations with pointers
   * to two different image buffers with two different pixel formats.
   *
   * @param SPT the source pixel format
   */
  template<AnyPixelTraits SPT>
  struct Mixed {
    [[gnu::hot]]
    static constexpr void ForHorizontal(pointer p,
                                        typename SPT::const_pointer q,
                                        unsigned n,
                                        std::invocable<pointer, typename SPT::const_pointer> auto f) noexcept {
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
  using color_type = BGRA8Color;
  using channel_type = uint8_t;
  using integer_type = uint32_t;
  using pointer = color_type *;
  using rpointer = color_type *gcc_restrict;
  using const_pointer = const color_type *;
  using const_rpointer = const color_type *gcc_restrict;

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

  static constexpr color_type TransformInteger(color_type c,
                                               std::invocable<integer_type> auto f) noexcept {
    return FromInteger(f(ToInteger(c)));
  }

  static constexpr color_type TransformInteger(color_type a, color_type b,
                                               std::invocable<integer_type, integer_type> auto f) noexcept {
    return FromInteger(f(ToInteger(a), ToInteger(b)));
  }

  static constexpr color_type TransformChannels(color_type c,
                                                std::invocable<channel_type> auto f) noexcept {
    return BGRA8Color(f(c.Red()), f(c.Green()), f(c.Blue()), c.Alpha());
  }

  static constexpr color_type TransformChannels(color_type a, color_type b,
                                                std::invocable<channel_type, channel_type> auto f) noexcept {
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

  static constexpr std::ptrdiff_t CalcIncrement(std::ptrdiff_t delta) noexcept {
    return delta;
  }

  static constexpr pointer Next(pointer p, std::ptrdiff_t delta) noexcept {
    return p + CalcIncrement(delta);
  }

  static constexpr const_pointer Next(const_pointer p,
                                      std::ptrdiff_t delta) noexcept {
    return p + CalcIncrement(delta);
  }

  static constexpr pointer NextByte(pointer p, std::ptrdiff_t delta) noexcept {
    return (pointer)OffsetPointer(p, delta);
  }

  static constexpr const_pointer NextByte(const_pointer p,
                                          std::ptrdiff_t delta) noexcept {
    return (const_pointer)OffsetPointer(p, delta);
  }

  static constexpr pointer NextRow(pointer p, std::size_t pitch,
                                   std::ptrdiff_t delta) noexcept {
    return NextByte(p, int(pitch) * delta);
  }

  static constexpr const_pointer NextRow(const_pointer p, std::size_t pitch,
                                         std::ptrdiff_t delta) noexcept {
    return NextByte(p, int(pitch) * delta);
  }

  static constexpr pointer At(pointer p, std::size_t pitch,
                              int x, int y) noexcept {
    return Next(NextRow(p, pitch, y), x);
  }

  static constexpr const_pointer At(const_pointer p, std::size_t pitch,
                                    int x, int y) noexcept {
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

    [[maybe_unused]] size_t dummy0, dummy1;
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

  [[gnu::hot]]
  static constexpr void ForHorizontal(pointer p, unsigned n,
                                      std::invocable<pointer> auto f) noexcept {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i));
  }

  [[gnu::hot]]
  static constexpr void ForHorizontal(rpointer p, const_rpointer q, unsigned n,
                                      std::invocable<pointer> auto f) noexcept {
    for (unsigned i = 0; i < n; ++i)
      f(Next(p, i), Next(q, i));
  }

  [[gnu::hot]]
  static constexpr void ForVertical(pointer p, std::size_t pitch, unsigned n,
                                    std::invocable<pointer> auto f) noexcept {
    for (; n > 0; --n, p = NextByte(p, pitch))
      f(p);
  }
};

#endif
