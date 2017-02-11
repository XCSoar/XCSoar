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

#include "ProgressBar.hpp"
#include "Canvas.hpp"
#include "Features.hpp"
#include "Look/Colors.hpp"
#include "Thread/Debug.hpp"
#include "Asset.hpp"

void
ProgressBar::SetRange(unsigned min_value, unsigned max_value)
{
  AssertThread();

  this->min_value = min_value;
  this->max_value = max_value;
  value = 0;
  step_size = 1;
  Invalidate();
}

void
ProgressBar::SetValue(unsigned value)
{
  AssertThread();

  if (value == this->value)
    return;

  this->value = value;
  Invalidate();
}

void
ProgressBar::SetStep(unsigned size)
{
  AssertThread();

  step_size = size;
  Invalidate();
}

void
ProgressBar::Step()
{
  AssertThread();

  value += step_size;
  Invalidate();
}

#if defined(EYE_CANDY) && !defined(HAVE_CLIPPING)
/* when the Canvas is clipped, we can't render rounded corners,
   because the parent's background would not be left visible then */
#define ROUND_PROGRESS_BAR
#endif

void
ProgressBar::OnPaint(Canvas &canvas)
{
  unsigned position = 0;
  if (min_value < max_value) {
    unsigned value = this->value;
    if (value < min_value)
      value = min_value;
    else if (value > max_value)
      value = max_value;
#ifdef ROUND_PROGRESS_BAR
    position = (value - min_value) * (GetWidth() - GetHeight()) /
               (max_value - min_value);
#else
    position = (value - min_value) * GetWidth() / (max_value - min_value);
#endif
  }

#ifdef ROUND_PROGRESS_BAR
  canvas.SelectNullPen();
  canvas.SelectWhiteBrush();
  canvas.DrawRoundRectangle(0, 0, GetWidth(), GetHeight(),
                            GetHeight(), GetHeight());

  Brush progress_brush(IsDithered() ? COLOR_BLACK : COLOR_XCSOAR_LIGHT);
  canvas.Select(progress_brush);
  unsigned margin = GetHeight() / 9;
  unsigned top, bottom;
  if (position <= GetHeight() - 2 * margin) {
    // Use a centered "circle" for small position values. This keeps the progress
    // bar inside the background.
    unsigned center_y = GetHeight() / 2;
    top = center_y - position / 2;
    bottom = center_y + position / 2;
  } else {
    top = margin;
    bottom = GetHeight() - margin;
  }
  canvas.DrawRoundRectangle(margin, top, margin + position, bottom,
                            GetHeight(), GetHeight());
#else
  canvas.DrawFilledRectangle(0, 0, position, GetHeight(),
                             IsDithered() ? COLOR_BLACK : COLOR_GREEN);
  canvas.DrawFilledRectangle(position, 0, GetWidth(), GetHeight(), COLOR_WHITE);
#endif
}
