/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_BUFFER_HPP
#define XCSOAR_SCREEN_BUFFER_HPP

#include "Compiler.h"

/**
 * A reference to an image buffer (or a portion of it) that we can
 * write to.  This class does not allocate or free any memory, it
 * refers to a buffer owned by somebody else.
 */
template<typename PixelTraits>
struct WritableImageBuffer {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::rpointer_type rpointer_type;

  typedef typename PixelTraits::const_pointer_type const_pointer_type;

  rpointer_type data;

  unsigned pitch, width, height;

  static constexpr WritableImageBuffer<PixelTraits> Empty() {
    return { nullptr, 0, 0, 0 };
  }

  void Allocate(unsigned _width, unsigned _height) {
    unsigned i = PixelTraits::CalcIncrement(_width);
    data = new typename PixelTraits::color_type[i * _height];
    pitch = i * sizeof(typename PixelTraits::color_type);
    width = _width;
    height = _height;
  }

  void Free() {
    delete[] data;
    data = nullptr;
  }

  constexpr bool Check(unsigned x, unsigned y) const {
    return x < width && y < height;
  }

  constexpr pointer_type At(unsigned x, unsigned y) {
    return PixelTraits::At(data, pitch, x, y);
  }

#if !GCC_OLDER_THAN(5,0)
  constexpr const_pointer_type At(unsigned x, unsigned y) const {
    return PixelTraits::At(data, pitch, x, y);
  }
#endif
};

/**
 * A reference to an image buffer (or a portion of it) that is
 * read-only.  This class does not allocate or free any memory, it
 * refers to a buffer owned by somebody else.
 */
template<typename PixelTraits>
struct ConstImageBuffer {
  typedef typename PixelTraits::const_pointer_type pointer_type;
  typedef typename PixelTraits::const_rpointer_type rpointer_type;

  rpointer_type data;

  unsigned pitch, width, height;

  ConstImageBuffer() = default;

  constexpr ConstImageBuffer(rpointer_type _data, unsigned _pitch,
                             unsigned _width, unsigned _height)
    :data(_data), pitch(_pitch), width(_width), height(_height) {}

  constexpr ConstImageBuffer(WritableImageBuffer<PixelTraits> other)
    :data(other.data), pitch(other.pitch),
     width(other.width), height(other.height) {}

  static constexpr WritableImageBuffer<PixelTraits> Empty() {
    return { nullptr, 0, 0, 0 };
  }

  constexpr bool Check(unsigned x, unsigned y) const {
    return x < width && y < height;
  }

  constexpr pointer_type At(unsigned x, unsigned y) const {
    return PixelTraits::At(data, pitch, x, y);
  }
};

#endif
