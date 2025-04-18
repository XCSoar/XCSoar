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
  VirtualCanvas() noexcept = default;
  VirtualCanvas(PixelSize new_size) noexcept;
  VirtualCanvas(const Canvas &canvas, PixelSize new_size) noexcept;

  ~VirtualCanvas() noexcept {
    Destroy();
  }

  void Create(PixelSize new_size) noexcept;

  void Create(const Canvas &canvas, PixelSize new_size) noexcept;

  void Create(const Canvas &canvas) noexcept {
    Create(canvas, canvas.GetSize());
  }

  void Destroy() noexcept;

#ifdef USE_MEMORY_CANVAS
  void Resize(PixelSize new_size) noexcept {
    if (new_size != GetSize())
      Create(*this, new_size);
  }

  void Grow(PixelSize new_size) noexcept;
#endif
};
