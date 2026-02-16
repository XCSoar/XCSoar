// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "ui/canvas/Color.hpp"
#include "Renderer/TextRenderer.hpp"
#include "util/tstring.hpp"

#include <tchar.h>

struct DialogLook;

class WndFrame : public PaintWindow {
  const DialogLook &look;

  Color text_color;

  TextRenderer text_renderer;

  tstring text;

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

  [[gnu::pure]]
  unsigned GetTextHeight() const noexcept;

protected:
  /** from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};
