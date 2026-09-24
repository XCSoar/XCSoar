// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/VirtualCanvas.hpp"

/**
 * Off-screen canvas for the memory backend (#VirtualCanvas with
 * Begin/End/CopyTo helpers matching OpenGL).
 *
 * #Grow is inherited from #VirtualCanvas (defined in
 * ui/canvas/BufferCanvas.cpp for this backend).
 */
class BufferCanvas : public VirtualCanvas {
public:
  using VirtualCanvas::VirtualCanvas;

  /** No-op on memory canvas (buffer is always drawable). */
  void Begin() noexcept {}

  void Begin([[maybe_unused]] Canvas &other) noexcept {}

  /** No-op on memory canvas. */
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
