/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef CHART_LOOK_HPP
#define CHART_LOOK_HPP

#include "Screen/Font.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"

#include <assert.h>

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

#endif
