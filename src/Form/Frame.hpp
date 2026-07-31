// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "ui/canvas/Color.hpp"
#include "Renderer/TextRenderer.hpp"

#include <optional>
#include <string>

struct DialogLook;

class WndFrame : public PaintWindow {
  const DialogLook &look;

  Color text_color;

  std::optional<Color> background_color;

  TextRenderer text_renderer;

  std::string text;

  bool top_separator = false;

public:
  explicit WndFrame(const DialogLook &look) noexcept;

  WndFrame(ContainerWindow &parent, const DialogLook &look,
           PixelRect rc,
           const WindowStyle style=WindowStyle()) noexcept;

  const DialogLook &GetLook() const noexcept {
    return look;
  }

  void SetAlignCenter() noexcept;
  void SetVAlignCenter() noexcept;

  const char *GetText() const noexcept {
    return text.c_str();
  }

  void SetText(const char *_text) noexcept;

  void SetTextColor(const Color &color) noexcept {
    text_color = color;
  }

  /**
   * Fill the frame with this colour instead of the look's background,
   * e.g. to mark a message as a warning.
   */
  void SetBackgroundColor(const Color &color) noexcept {
    background_color = color;
  }

  void SetTopSeparator(bool value = true) noexcept {
    top_separator = value;
  }

  [[gnu::pure]]
  unsigned GetTextHeight() const noexcept;

protected:
  /** from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};
