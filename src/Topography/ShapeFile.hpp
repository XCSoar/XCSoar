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

#pragma once

#include "shapelib/mapserver.h"

#include <cstddef>

struct zzip_dir;

/**
 * C++ wrapper for #shapefileObj;
 */
class ShapeFile {
  shapefileObj obj;

public:
  /**
   * Throws on error.
   */
  ShapeFile(zzip_dir *dir, const char *filename);

  ~ShapeFile() noexcept {
    msShapefileClose(&obj);
  }

  ShapeFile(const ShapeFile &) = delete;
  ShapeFile &operator=(const ShapeFile &) = delete;

  std::size_t size() const noexcept {
    return obj.numshapes;
  }

  const auto &GetBounds() const noexcept {
    return obj.bounds;
  }

  int WhichShapes(struct zzip_dir *dir, rectObj rect) noexcept {
    return msShapefileWhichShapes(&obj, dir, rect, 0);
  }

  ms_const_bitarray GetStatus() const noexcept {
    return obj.status;
  }

  /**
   * Throws on error.
   */
  void ReadShape(shapeObj &shape, std::size_t i);

  [[gnu::pure]]
  const char *ReadLabel(std::size_t i, unsigned field) noexcept;
};
