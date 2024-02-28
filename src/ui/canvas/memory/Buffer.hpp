// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Concepts.hpp"
#include "ui/dim/Size.hpp"

#include <cstddef>

/**
 * A reference to an image buffer (or a portion of it) that we can
 * write to.  This class does not allocate or free any memory, it
 * refers to a buffer owned by somebody else.
 */
template<AnyPixelTraits PixelTraits>
struct WritableImageBuffer {
  typedef typename PixelTraits::pointer pointer;
  typedef typename PixelTraits::rpointer rpointer;

  typedef typename PixelTraits::const_pointer const_pointer;

  rpointer data;

  std::size_t pitch;

  PixelSize size;

  static constexpr WritableImageBuffer<PixelTraits> Empty() noexcept {
    return { nullptr, 0, {} };
  }

  void Allocate(PixelSize _size) noexcept {
    const std::size_t i = PixelTraits::CalcIncrement(_size.width);
    data = new typename PixelTraits::color_type[i * _size.height];
    pitch = i * sizeof(typename PixelTraits::color_type);
    size = _size;
  }

  void Free() noexcept {
    delete[] data;
    data = nullptr;
  }

  constexpr bool Check(unsigned x, unsigned y) const noexcept {
    return x < size.width && y < size.height;
  }

  constexpr pointer At(unsigned x, unsigned y) noexcept {
    return PixelTraits::At(data, pitch, x, y);
  }

  constexpr const_pointer At(unsigned x, unsigned y) const noexcept {
    return PixelTraits::At(data, pitch, x, y);
  }
};

/**
 * A reference to an image buffer (or a portion of it) that is
 * read-only.  This class does not allocate or free any memory, it
 * refers to a buffer owned by somebody else.
 */
template<AnyPixelTraits PixelTraits>
struct ConstImageBuffer {
  typedef typename PixelTraits::const_pointer pointer;
  typedef typename PixelTraits::const_rpointer rpointer;

  rpointer data;

  std::size_t pitch;
  PixelSize size;

  ConstImageBuffer() noexcept = default;

  constexpr ConstImageBuffer(rpointer _data, std::size_t _pitch,
                             PixelSize _size) noexcept
    :data(_data), pitch(_pitch), size(_size) {}

  constexpr ConstImageBuffer(WritableImageBuffer<PixelTraits> other) noexcept
    :data(other.data), pitch(other.pitch), size(other.size) {}

  static constexpr WritableImageBuffer<PixelTraits> Empty() noexcept {
    return { nullptr, 0, 0, 0 };
  }

  constexpr bool Check(unsigned x, unsigned y) const noexcept {
    return x < size.width && y < size.height;
  }

  constexpr pointer At(unsigned x, unsigned y) const noexcept {
    return PixelTraits::At(data, pitch, x, y);
  }
};
