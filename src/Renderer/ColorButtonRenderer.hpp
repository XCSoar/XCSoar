// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ButtonRenderer.hpp"
#include "ui/canvas/Color.hpp"

/**
 * A #ButtonRenderer instance that renders a regular button frame
 * filled with a color.
 */
class ColorButtonRenderer : public ButtonRenderer {
  ButtonFrameRenderer frame_renderer;

  const Color color;

public:
  ColorButtonRenderer(const ButtonLook &_look, Color _color) noexcept
    :frame_renderer(_look), color(_color) {}

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  ButtonState state) const noexcept override;
};
