// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ButtonRenderer.hpp"
#include "TextRenderer.hpp"
#include "util/StaticString.hxx"

/**
 * A #ButtonRenderer instance that renders a regular button frame and
 * some text.
 */
class TextButtonRenderer : public ButtonRenderer {
  ButtonFrameRenderer frame_renderer;

  TextRenderer text_renderer;

  StaticString<64> caption;

public:
  explicit TextButtonRenderer(const ButtonLook &_look) noexcept
    :frame_renderer(_look) {
    text_renderer.SetCenter();
    text_renderer.SetVCenter();
    text_renderer.SetControl();
  }

  TextButtonRenderer(const ButtonLook &_look,
                     StaticString<64>::const_pointer _caption) noexcept
    :frame_renderer(_look), caption(_caption) {
    text_renderer.SetCenter();
    text_renderer.SetVCenter();
    text_renderer.SetControl();
  }

  [[gnu::pure]]
  static unsigned GetMinimumButtonWidth(const ButtonLook &look,
                                        std::string_view caption) noexcept;

  const ButtonLook &GetLook() const noexcept {
    return frame_renderer.GetLook();
  }

  StaticString<64>::const_pointer GetCaption() const noexcept {
    return caption;
  }

  void SetCaption(StaticString<64>::const_pointer _caption) noexcept {
    caption = _caption;
    text_renderer.InvalidateLayout();
  }

  [[gnu::pure]]
  unsigned GetMinimumButtonWidth() const noexcept override;

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  ButtonState state) const noexcept override;

private:
  void DrawCaption(Canvas &canvas, const PixelRect &rc,
                   ButtonState state) const noexcept;
};
