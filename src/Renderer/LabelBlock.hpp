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

#ifndef SCREEN_LABELBLOCK_HPP
#define SCREEN_LABELBLOCK_HPP

#include "Screen/Point.hpp"
#include "Util/StaticArray.hxx"
#include "Compiler.h"

/**
 * Simple code to prevent text writing over map city names.
 */
class LabelBlock {
#if defined(HAVE_GLES)
  /* embedded (Android or Windows CE) */
  static constexpr unsigned SCREEN_HEIGHT = 2048;
  static constexpr unsigned BUCKET_SIZE = 64;
#else
  /* desktop, screen may be huge, lots of memory */
  static constexpr unsigned SCREEN_HEIGHT = 4096;
  static constexpr unsigned BUCKET_SIZE = 64;
#endif
  static constexpr unsigned BUCKET_SHIFT = 7;
  static constexpr unsigned BUCKET_HEIGHT = 1 << BUCKET_SHIFT;
  static constexpr unsigned BUCKET_COUNT = SCREEN_HEIGHT / BUCKET_HEIGHT;

  /**
   * A bucket is responsible for hit tests in one horizontal section
   * of the screen.
   */
  class Bucket {
    typedef StaticArray<PixelRect, BUCKET_SIZE> BlockArray;
    BlockArray blocks;

  public:
    void Clear();

    gcc_pure
    bool Check(const PixelRect rc) const;

    void Add(const PixelRect rc) {
      if (!blocks.full())
        blocks.append(rc);
    }
  };

  Bucket buckets[BUCKET_COUNT];

public:
  bool check(const PixelRect rc);
  void reset();
};

#endif
