// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TextRenderer.hpp"

#include <tchar.h>

struct DialogLook;
struct PixelRect;
class Canvas;
class MaskedIcon;

/**
 * Render #TabDisplay / #TabMenuDisplay buttons.
 */
class TabRenderer {
  TextRenderer text_renderer;

public:
  constexpr TabRenderer() noexcept {
    text_renderer.SetCenter();
    text_renderer.SetVCenter();
    text_renderer.SetControl();
  }

  void InvalidateLayout() noexcept {
    text_renderer.InvalidateLayout();
  }

  void Draw(Canvas &canvas, const PixelRect &rc,
            const DialogLook &look,
            const TCHAR *caption, const MaskedIcon *icon,
            bool focused, bool pressed, bool selected) const noexcept;
};
