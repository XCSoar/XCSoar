// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"

#include <functional>

class ContainerWindow;

/**
 * This class is used for creating custom drawn content.
 * It is based on the WindowControl class.
 */
class WndOwnerDrawFrame : public PaintWindow {
public:
  template<typename CB>
  void Create(ContainerWindow &parent,
              PixelRect rc, const WindowStyle style,
              CB &&_paint) {
    mOnPaintCallback = std::move(_paint);
    PaintWindow::Create(parent, rc, style);
  }

protected:
  /**
   * The callback function for painting the content of the control
   * @see SetOnPaintNotify()
   */
  std::function<void(Canvas &canvas, const PixelRect &rc)> mOnPaintCallback;

  /** from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};
