// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/VirtualCanvas.hpp"

/**
 * An off-screen #Canvas implementation.  The constructor allocates
 * memory for the specified dimensions.
 */
class BufferCanvas : public VirtualCanvas {
protected:
  HBITMAP bitmap;

public:
  BufferCanvas() noexcept:bitmap(nullptr) {}
  BufferCanvas(const Canvas &canvas, PixelSize new_size) noexcept;
  ~BufferCanvas() noexcept;

  void Create(const Canvas &canvas, PixelSize new_size) noexcept;
  void Create(const Canvas &canvas) noexcept;
  void Destroy() noexcept;

  void Resize(PixelSize new_size) noexcept;

  /**
   * Similar to Resize(), but never shrinks the buffer.
   */
  void Grow(PixelSize new_size) noexcept;
};
