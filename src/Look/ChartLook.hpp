// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Font.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

#include <cassert>

struct ChartLook {
  enum Style {
    STYLE_BLUETHINDASH,
    STYLE_BLUEDASH,
    STYLE_BLUE,
    STYLE_REDTHICKDASH,
    STYLE_RED,
    STYLE_GREENDASH,
    STYLE_GREEN,
    STYLE_BLACK,
    STYLE_WHITE,
    STYLE_GRID,
    STYLE_GRIDMINOR,
    STYLE_GRIDZERO,
    STYLE_COUNT
  };

  Pen pens[STYLE_COUNT];

  Brush bar_brush;
  Brush neg_brush;
  Brush blank_brush;
  Brush label_blank_brush;
  Brush black_brush;

  /**
   * Font for miscellaneous labels in the chart.
   */
  Font label_font;

  /**
   * Font for the two axis labels.
   */
  Font axis_label_font;

  /**
   * Font for tick values along the axis.
   */
  Font axis_value_font;

  Color color_positive;
  Color color_negative;

  void Initialise();

  const Pen &GetPen(Style style) const {
    unsigned i = (unsigned)style;
    assert(i < STYLE_COUNT);
    return pens[i];
  }
};
