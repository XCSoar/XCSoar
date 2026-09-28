// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LabelShape.hpp"

#include <cstdint>

struct PixelPoint;
struct PixelSize;
struct PixelRect;
class Canvas;
class LabelBlock;
void
RenderShadowedText(Canvas &canvas, const char *text,
                   PixelPoint p,
                   bool inverted) noexcept;

/**
 * Draw the box of a #LabelShape::PILL and its shadow, e.g. as the
 * background of something which is not text.
 *
 * @param opacity scales the pill's opacity, for fading it out; only
 * OpenGL honours it
 */
void
DrawPill(Canvas &canvas, const PixelRect &rc,
         uint8_t opacity=0xff) noexcept;

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
