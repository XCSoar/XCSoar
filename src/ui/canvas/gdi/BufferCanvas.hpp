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

  /**
   * Create an off-screen buffer with the screen pixel format.
   * Matches OpenGL / memory-canvas BufferCanvas::Create(PixelSize).
   */
  void Create(PixelSize new_size) noexcept;

  void Create(const Canvas &canvas, PixelSize new_size) noexcept;
  void Create(const Canvas &canvas) noexcept;
  void Destroy() noexcept;

  void Resize(PixelSize new_size) noexcept;

  /**
   * Similar to Resize(), but never shrinks the buffer.
   */
  void Grow(PixelSize new_size) noexcept;

  /** No-op on GDI (buffer is always drawable). */
  void Begin() noexcept {}

  void Begin([[maybe_unused]] Canvas &other) noexcept {}

  /** No-op on GDI. */
  void End() noexcept {}

  void CopyTo(Canvas &other) noexcept {
    other.Copy(*this);
  }

  void CopyTo(Canvas &dest, PixelRect dest_rc,
              PixelRect src_rc) noexcept {
    dest.Copy(dest_rc.GetTopLeft(), dest_rc.GetSize(),
              *this, src_rc.GetTopLeft());
  }
};
