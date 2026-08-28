// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/Draw.hpp"
#include "ui/dim/Point.hpp"

#include <functional>

struct PixelRect;
class Bitmap;
class ContainerWindow;
class Canvas;
class WindowStyle;

/**
 * Owner-draw window for pan/zoom bitmap viewing (drag and arrow nudge).
 */
class ImageZoomFrame final : public WndOwnerDrawFrame {
  PixelPoint last_mouse_pos, pending_offset;
  DoublePoint2D view_pos{};
  bool is_dragging = false;

  const Bitmap *bitmap = nullptr;
  double *zoom_factor = nullptr;

  std::function<bool(unsigned key_code)> try_key_input;

public:
  void Create(ContainerWindow &parent, PixelRect rc,
              const WindowStyle &style) noexcept;

  void SetContent(const Bitmap *bitmap, double *zoom_factor) noexcept;

  void SetTryKeyInput(std::function<bool(unsigned key_code)> &&f) noexcept;

  void NudgeViewByPixelOffset(PixelPoint o) noexcept;

  DoublePoint2D &GetViewPosition() noexcept {
    return view_pos;
  }

  void ClearPendingOffset() noexcept {
    pending_offset = {};
  }

protected:
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
};
