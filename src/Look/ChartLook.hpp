/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"

#include <assert.h>

class Font;

struct ChartLook {
  enum Style {
    STYLE_BLUETHIN,
    STYLE_REDTHICK,
    STYLE_DASHGREEN,
    STYLE_MEDIUMBLACK,
    STYLE_THINDASHPAPER,
    STYLE_COUNT
  };

  Pen pens[STYLE_COUNT];

  Brush bar_brush;

  /**
   * Font for miscellaneous labels in the chart.
   */
  const Font *label_font;

  /**
   * Font for the two axis labels.
   */
  const Font *axis_label_font;

  /**
   * Font for tick values along the axis.
   */
  const Font *axis_value_font;

  void Initialise(const Font &label_font,
                  const Font &axis_label_font, const Font &axis_value_font);

  const Pen &GetPen(Style style) const {
    unsigned i = (unsigned)style;
    assert(i < STYLE_COUNT);
    return pens[i];
  }
};

#endif
