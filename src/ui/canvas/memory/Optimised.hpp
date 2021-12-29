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

#ifndef XCSOAR_SCREEN_MEMORY_OPTIMISED_HPP
#define XCSOAR_SCREEN_MEMORY_OPTIMISED_HPP

#include "PixelOperations.hpp"
#include "PixelTraits.hpp"
#include "util/Compiler.h"

#ifdef __ARM_NEON__
#include "NEON.hpp"
#endif

#ifdef __MMX__
#include "MMX.hpp"
#endif

#include <type_traits>

/**
 * This class hosts two base classes: one that is optimised (e.g. via
 * SIMD) and one that is portable (but slow).  The optimised one will
 * be used as much as possible, and for the odd remainder, we use the
 * portable version.
 */
template<AnyBulkPixelOperation Optimised, unsigned N, AnyBulkPixelOperation Portable>
class SelectOptimisedPixelOperations
  : protected Optimised, protected Portable {

  static_assert(std::is_same_v<typename Optimised::PixelTraits, typename Portable::PixelTraits>);
  static_assert(std::is_same_v<typename Optimised::SourcePixelTraits, typename Portable::SourcePixelTraits>);

public:
  using typename Portable::PixelTraits;
  using typename Portable::SourcePixelTraits;

  using color_type = typename PixelTraits::color_type;
  using rpointer = typename PixelTraits::rpointer;
  using const_rpointer = typename PixelTraits::const_rpointer;

  static constexpr unsigned PORTABLE_MASK = N - 1;
  static constexpr unsigned OPTIMISED_MASK = ~PORTABLE_MASK;

  constexpr SelectOptimisedPixelOperations(const SelectOptimisedPixelOperations<Optimised, N, Portable> &other)
    :Optimised((const Optimised &)other), Portable((const Portable &)other) {}

  template<typename... Args>
  explicit constexpr SelectOptimisedPixelOperations(Args... args)
    :Optimised(args...), Portable(args...) {}

  using Portable::WritePixel;

  gcc_flatten gcc_nonnull_all
  void FillPixels(rpointer p, unsigned n, color_type c) const {
    const unsigned no = n & OPTIMISED_MASK;
    const unsigned np = n & PORTABLE_MASK;

    Optimised::FillPixels(p, no, c);
    Portable::FillPixels(PixelTraits::Next(p, no), np, c);
  }

  gcc_flatten gcc_nonnull_all
  void CopyPixels(rpointer p, const_rpointer q, unsigned n) const {
    const unsigned no = n & OPTIMISED_MASK;
    const unsigned np = n & PORTABLE_MASK;

    Optimised::CopyPixels(p, q, no);
    Portable::CopyPixels(PixelTraits::Next(p, no),
                         PixelTraits::Next(q, no), np);
  }
};

template<AnyPixelTraits PixelTraits>
struct BitOrPixelOperations
  : PortableBitOrPixelOperations<PixelTraits> {
};

template<AnyPixelTraits PixelTraits>
struct TransparentPixelOperations
  : PortableTransparentPixelOperations<PixelTraits> {
  using color_type = typename PixelTraits::color_type;

  explicit constexpr TransparentPixelOperations(const color_type key)
    :PortableTransparentPixelOperations<PixelTraits>(key) {}
};

#ifdef __ARM_NEON__

template<>
struct BitOrPixelOperations<GreyscalePixelTraits>
  : SelectOptimisedPixelOperations<NEONBitOrPixelOperations, 16,
                                   PortableBitOrPixelOperations<GreyscalePixelTraits>> {
};

template<>
struct TransparentPixelOperations<GreyscalePixelTraits>
  : public SelectOptimisedPixelOperations<NEONTransparentPixelOperations, 32,
                                          PortableTransparentPixelOperations<GreyscalePixelTraits>> {
  typedef typename PixelTraits::color_type color_type;

  explicit constexpr TransparentPixelOperations(const color_type key)
    :SelectOptimisedPixelOperations(key) {}
};

#endif

template<AnyPixelTraits PixelTraits>
class AlphaPixelOperations
  : public PortableAlphaPixelOperations<PixelTraits> {
public:
  explicit constexpr AlphaPixelOperations(const uint8_t alpha)
    :PortableAlphaPixelOperations<PixelTraits>(alpha) {}
};

#ifdef __ARM_NEON__

template<>
class AlphaPixelOperations<GreyscalePixelTraits>
  : public SelectOptimisedPixelOperations<NEONAlphaPixelOperations, 16,
                                          PortableAlphaPixelOperations<GreyscalePixelTraits>> {
public:
  explicit constexpr AlphaPixelOperations(const uint8_t alpha)
    :SelectOptimisedPixelOperations(alpha) {}
};

#endif

#ifdef __MMX__

template<>
class AlphaPixelOperations<GreyscalePixelTraits>
  : public SelectOptimisedPixelOperations<MMXAlpha8PixelOperations, 8,
                                          PortableAlphaPixelOperations<GreyscalePixelTraits>> {
public:
  explicit constexpr AlphaPixelOperations(const uint8_t alpha)
    :SelectOptimisedPixelOperations(alpha) {}
};

#ifndef GREYSCALE

template<>
class AlphaPixelOperations<BGRAPixelTraits>
  : public SelectOptimisedPixelOperations<MMXAlpha32PixelOperations, 2,
                                          PortableAlphaPixelOperations<BGRAPixelTraits>> {
public:
  using typename SelectOptimisedPixelOperations::PixelTraits;
  using typename SelectOptimisedPixelOperations::SourcePixelTraits;

  explicit constexpr AlphaPixelOperations(const uint8_t alpha)
    :SelectOptimisedPixelOperations(alpha) {}
};

#endif /* !GREYSCALE */

#endif

#endif
