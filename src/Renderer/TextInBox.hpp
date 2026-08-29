// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LabelShape.hpp"

struct PixelPoint;
struct PixelSize;
struct PixelRect;
class Canvas;
class Color;
class LabelBlock;
void
RenderShadowedText(Canvas &canvas, const char *text,
                   PixelPoint p,
                   bool inverted) noexcept;

/**
 * Like above, but with an explicit pair of colours, for text that is
 * neither black nor white.
 */
void
RenderShadowedText(Canvas &canvas, const char *text,
                   PixelPoint p,
                   Color text_color, Color outline_color) noexcept;

struct TextInBoxMode {
  enum Alignment : uint8_t {
    LEFT,
    CENTER,
    RIGHT,
  };

  enum VerticalPosition : uint8_t {
    ABOVE,
    CENTERED,
    BELOW,
  };

  LabelShape shape = LabelShape::SIMPLE;
  Alignment align = Alignment::LEFT;
  VerticalPosition vertical_position = VerticalPosition::BELOW;
  bool move_in_view = false;
};

bool
TextInBox(Canvas &canvas, const char *value, PixelPoint p,
          TextInBoxMode mode, const PixelRect &map_rc,
          LabelBlock *label_block=nullptr) noexcept;

bool
TextInBox(Canvas &canvas, const char *value, PixelPoint p,
          TextInBoxMode mode,
          PixelSize screen_size,
          LabelBlock *label_block=nullptr) noexcept;
