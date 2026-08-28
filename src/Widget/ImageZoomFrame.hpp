// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/Draw.hpp"
#include "ui/dim/Point.hpp"
#include "ui/window/Features.hpp"

#include <functional>

struct PixelRect;
class Bitmap;
class ContainerWindow;
class Canvas;
class WindowStyle;

/**
 * Owner-draw window for pan/zoom bitmap viewing (drag, arrow nudge and
 * pinch-to-zoom).
 */
class ImageZoomFrame final : public WndOwnerDrawFrame {
  PixelPoint last_mouse_pos, pending_offset;
  DoublePoint2D view_pos{};
  bool is_dragging = false;

#ifdef HAVE_MULTI_TOUCH
  /**
   * The bitmap position which the pinch gesture keeps below the centre
   * between the two fingers.
   */
  DoublePoint2D pinch_anchor;

  PixelPoint pinch_last_a, pinch_last_b;

  double pinch_distance = 0, pinch_zoom_factor = 0;
  bool is_pinching = false;
#endif

  const Bitmap *bitmap = nullptr;
  double *zoom_factor = nullptr;

  std::function<bool(unsigned key_code)> try_key_input;
  std::function<void()> on_zoom_changed;

public:
  void Create(ContainerWindow &parent, PixelRect rc,
              const WindowStyle &style) noexcept;

  void SetContent(const Bitmap *bitmap, double *zoom_factor) noexcept;

  void SetTryKeyInput(std::function<bool(unsigned key_code)> &&f) noexcept;

  /**
   * Set a callback which is invoked after this window has changed the
   * zoom factor itself (pinch-to-zoom).
   */
  void SetOnZoomChanged(std::function<void()> &&f) noexcept;

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
#ifdef HAVE_MULTI_TOUCH
  bool OnMultiTouchDown() noexcept override;
  bool OnMultiTouchMove(PixelPoint a, PixelPoint b) noexcept override;
  bool OnMultiTouchUp() noexcept override;
#endif
  void OnCancelMode() noexcept override;
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
};
