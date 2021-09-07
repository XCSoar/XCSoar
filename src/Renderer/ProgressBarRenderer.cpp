/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ProgressBarRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/Colors.hpp"
#include "Asset.hpp"

#include <algorithm>

static constexpr unsigned
CalcProgressBarPosition(unsigned current_value,
                        unsigned min_value, unsigned max_value,
                        unsigned width) noexcept
{
  if (min_value >= max_value)
    return 0;

  const unsigned value = std::clamp(current_value, min_value, max_value);
  return (value - min_value) * width / (max_value - min_value);
}

void
DrawSimpleProgressBar(Canvas &canvas, const PixelRect &r,
                      unsigned current_value,
                      unsigned min_value, unsigned max_value) noexcept
{
  const int position =
    CalcProgressBarPosition(current_value, min_value, max_value,
                            r.GetWidth());

  auto a = r, b = r;
  a.right = b.left = a.left + position;

  canvas.DrawFilledRectangle(a, IsDithered() ? COLOR_BLACK : COLOR_GREEN);
  canvas.DrawFilledRectangle(b, COLOR_WHITE);
}

void
DrawRoundProgressBar(Canvas &canvas, const PixelRect &r,
                     unsigned current_value,
                     unsigned min_value, unsigned max_value) noexcept
{
  const unsigned position =
    CalcProgressBarPosition(current_value, min_value, max_value,
                            r.GetWidth());

  canvas.SelectNullPen();
  canvas.SelectWhiteBrush();
  canvas.DrawRoundRectangle(r, PixelSize{r.GetHeight()});

  Brush progress_brush(IsDithered() ? COLOR_BLACK : COLOR_XCSOAR_LIGHT);
  canvas.Select(progress_brush);
  unsigned margin = r.GetHeight() / 9;
  unsigned top, bottom;
  if (position <= r.GetHeight() - 2 * margin) {
    // Use a centered "circle" for small position values. This keeps the progress
    // bar inside the background.
    unsigned center_y = r.GetHeight() / 2;
    top = center_y - position / 2;
    bottom = center_y + position / 2;
  } else {
    top = margin;
    bottom = r.GetHeight() - margin;
  }
  canvas.DrawRoundRectangle(PixelRect(margin, top, margin + position, bottom),
                            PixelSize{r.GetHeight()});
}
