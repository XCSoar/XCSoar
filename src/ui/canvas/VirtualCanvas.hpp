// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Canvas.hpp"

/**
 * A #Canvas implementation which draws to an off-screen surface.
 * This is an abstract class; see #BufferCanvas for
 * a concrete implementation.
 */
class VirtualCanvas : public Canvas {
public:
  VirtualCanvas() = default;
  VirtualCanvas(PixelSize new_size);
  VirtualCanvas(const Canvas &canvas, PixelSize new_size);

  ~VirtualCanvas() {
    Destroy();
  }

  void Create(PixelSize new_size);

  void Create(const Canvas &canvas, PixelSize new_size);

  void Create(const Canvas &canvas) {
    Create(canvas, canvas.GetSize());
  }

  void Destroy();

#ifdef USE_MEMORY_CANVAS
  void Resize(PixelSize new_size) {
    if (new_size != GetSize())
      Create(*this, new_size);
  }

  void Grow(PixelSize new_size);
#endif
};
