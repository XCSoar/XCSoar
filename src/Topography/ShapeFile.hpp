// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
