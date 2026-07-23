// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "ui/canvas/Color.hpp"
#include "Renderer/TextRenderer.hpp"

#include <string>

struct DialogLook;
class Font;

class WndFrame : public PaintWindow {
  const DialogLook &look;

  Color text_color;

  /**
   * The font used to render the text.  Defaults to the dialog look's
   * regular text font; override with SetFont() (e.g. for a bold heading).
   */
  const Font *font;

  TextRenderer text_renderer;

  std::string text;

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

  void SetFont(const Font &_font) noexcept {
    font = &_font;
    text_renderer.InvalidateLayout();
    Invalidate();
  }

  [[gnu::pure]]
  unsigned GetTextHeight() const noexcept;

protected:
  /** from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};
