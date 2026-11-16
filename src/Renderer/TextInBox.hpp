// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LabelShape.hpp"
#include "ui/canvas/Color.hpp"

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
 * Rounded label box. The corner matches a NOTAM label. OpenGL draws
 * a shadow and, unless @p solid, a translucent fill. Otherwise the
 * fill is opaque and a black outline stands in for the shadow.
 * @p opacity fades the page dots; only OpenGL honours it.
 */
void
DrawPill(Canvas &canvas, const PixelRect &rc,
         uint8_t opacity=0xff,
         Color fill=COLOR_WHITE,
         bool solid=false) noexcept;

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
