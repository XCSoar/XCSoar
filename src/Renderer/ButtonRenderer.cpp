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

#include "ButtonRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/ButtonLook.hpp"
#include "Util/Macros.hpp"

unsigned
ButtonFrameRenderer::GetMargin()
{
  return Layout::VptScale(2);
}

void
ButtonFrameRenderer::DrawButton(Canvas &canvas, PixelRect rc,
                                bool focused, bool pressed) const
{
  const ButtonLook::StateLook &_look = focused ? look.focused : look.standard;

  canvas.DrawFilledRectangle(rc, _look.background_color);

  const unsigned margin = GetMargin();

  if (margin < 4) {
    /* draw 1-pixel lines */

    canvas.Select(pressed ? _look.dark_border_pen : _look.light_border_pen);
    for (unsigned i = 0; i < margin; ++i)
      canvas.DrawTwoLinesExact(rc.left + i, rc.bottom - 2 - i,
                               rc.left + i, rc.top + i,
                               rc.right - 2 - i, rc.top + i);

    canvas.Select(pressed ? _look.light_border_pen : _look.dark_border_pen);
    for (unsigned i = 0; i < margin; ++i)
      canvas.DrawTwoLinesExact(rc.left + 1 + i, rc.bottom - 1 - i,
                               rc.right - 1 - i, rc.bottom - 1 - i,
                               rc.right - 1 - i, rc.top + 1 + i);
  } else {
    /* at 4 pixels or more, it's more efficient to draw a filled
       polygon */

    const BulkPixelPoint p1[] = {
      BulkPixelPoint(rc.left, rc.top),
      BulkPixelPoint(rc.right, rc.top),
      BulkPixelPoint(rc.right - margin, rc.top + margin),
      BulkPixelPoint(rc.left + margin, rc.top + margin),
      BulkPixelPoint(rc.left + margin, rc.bottom - margin),
      BulkPixelPoint(rc.left, rc.bottom),
    };

    canvas.SelectNullPen();
    canvas.Select(pressed
                  ? _look.dark_border_brush
                  : _look.light_border_brush);
    canvas.DrawTriangleFan(p1, ARRAY_SIZE(p1));

    const BulkPixelPoint p2[] = {
      BulkPixelPoint(rc.right, rc.bottom),
      BulkPixelPoint(rc.right, rc.top),
      BulkPixelPoint(rc.right - margin, rc.top + margin),
      BulkPixelPoint(rc.right - margin, rc.bottom - margin),
      BulkPixelPoint(rc.left + margin, rc.bottom - margin),
      BulkPixelPoint(rc.left, rc.bottom),
    };

    canvas.Select(pressed
                  ? _look.light_border_brush
                  : _look.dark_border_brush);
    canvas.DrawTriangleFan(p2, ARRAY_SIZE(p2));
}
}

PixelRect
ButtonFrameRenderer::GetDrawingRect(PixelRect rc, bool pressed) const
{
  rc.Grow(-2);
  if (pressed)
    rc.Offset(1, 1);

  return rc;
}

unsigned
ButtonRenderer::GetMinimumButtonWidth() const
{
  return Layout::GetMaximumControlHeight();
}
