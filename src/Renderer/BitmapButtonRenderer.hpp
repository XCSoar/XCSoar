// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ButtonRenderer.hpp"

class Bitmap;

/**
 * A #ButtonRenderer that draws a #Bitmap.
 */
class BitmapButtonRenderer : public ButtonRenderer {
  const Bitmap &bitmap;

public:
  explicit BitmapButtonRenderer(const Bitmap &_bitmap) noexcept
    :bitmap(_bitmap) {}

  [[gnu::pure]]
  unsigned GetMinimumButtonWidth() const noexcept override;

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  ButtonState state) const noexcept override;
};
