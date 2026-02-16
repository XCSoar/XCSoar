// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct DialogLook;
struct PixelRect;
class Canvas;
class MaskedIcon;

/**
 * Render #TabDisplay / #TabMenuDisplay buttons.
 */
class TabRenderer {
public:
  void InvalidateLayout() noexcept {}

  void Draw(Canvas &canvas, const PixelRect &rc,
            const DialogLook &look,
            const char *caption, const MaskedIcon *icon,
            bool focused, bool pressed, bool selected) const noexcept;
};
