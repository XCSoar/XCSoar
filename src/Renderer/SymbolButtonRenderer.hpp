// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ButtonRenderer.hpp"
#include "util/StaticString.hxx"

/**
 * A #ButtonRenderer instance that renders a regular button frame and
 * a symbol.
 */
class SymbolButtonRenderer : public ButtonRenderer {
  ButtonFrameRenderer frame_renderer;

  const StaticString<16> caption;

  bool menu_scale;

public:
  enum class Style {
    DEFAULT,
    MENU,
  };

  SymbolButtonRenderer(const ButtonLook &_look,
                       StaticString<64>::const_pointer _caption,
                       Style style=Style::DEFAULT) noexcept
    :frame_renderer(_look), caption(_caption),
     menu_scale(style == Style::MENU) {}

  const ButtonLook &GetLook() const noexcept {
    return frame_renderer.GetLook();
  }

  StaticString<64>::const_pointer GetCaption() const noexcept {
    return caption;
  }

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  ButtonState state) const noexcept override;

  [[gnu::pure]]
  static bool IsSymbolCaption(const char *caption) noexcept;

private:
  void DrawSymbol(Canvas &canvas, PixelRect rc,
                  ButtonState state) const noexcept;
};
