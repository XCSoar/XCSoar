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

#include "TextInBox.hpp"
#include "LabelBlock.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

static PixelPoint
TextInBoxMoveInView(PixelRect &rc, const PixelRect &map_rc)
{
  PixelPoint offset(0, 0);

  // If label is above maprect
  if (map_rc.top > rc.top) {
    // Move label down into maprect
    unsigned d = map_rc.top - rc.top;
    rc.top += d;
    rc.bottom += d;
    offset.y += d;
  }

  // If label is right of maprect
  if (map_rc.right < rc.right) {
    unsigned d = map_rc.right - rc.right;
    rc.right += d;
    rc.left += d;
    offset.x += d;
  }

  // If label is below maprect
  if (map_rc.bottom < rc.bottom) {
    unsigned d = map_rc.bottom - rc.bottom;
    rc.top += d;
    rc.bottom += d;
    offset.y += d;
  }

  // If label is left of maprect
  if (map_rc.left > rc.left) {
    unsigned d = map_rc.left - rc.left;
    rc.right += d;
    rc.left += d;
    offset.x += d;
  }

  return offset;
}

static void
RenderShadowedText(Canvas &canvas, const TCHAR *text,
                   int x, int y,
                   bool inverted)
{
  canvas.SetBackgroundTransparent();

  canvas.SetTextColor(inverted ? COLOR_BLACK : COLOR_WHITE);
  const int offset = canvas.GetFontHeight() / 12u;
  canvas.DrawText(x + offset, y + offset, text);
  canvas.DrawText(x - offset, y + offset, text);
  canvas.DrawText(x + offset, y - offset, text);
  canvas.DrawText(x - offset, y - offset, text);

  canvas.SetTextColor(inverted ? COLOR_WHITE : COLOR_BLACK);
  canvas.DrawText(x, y, text);
}

// returns true if really wrote something
bool
TextInBox(Canvas &canvas, const TCHAR *text, int x, int y,
          TextInBoxMode mode, const PixelRect &map_rc, LabelBlock *label_block)
{
  // landable waypoint label inside white box

  PixelSize tsize = canvas.CalcTextSize(text);

  if (mode.align == TextInBoxMode::Alignment::RIGHT)
    x -= tsize.cx;
  else if (mode.align == TextInBoxMode::Alignment::CENTER)
    x -= tsize.cx / 2;

  if (mode.vertical_position == TextInBoxMode::VerticalPosition::ABOVE)
    y -= tsize.cy;
  else if (mode.vertical_position == TextInBoxMode::VerticalPosition::CENTERED)
    y -= tsize.cy / 2;

  const unsigned padding = Layout::GetTextPadding();
  PixelRect rc;
  rc.left = x - padding - 1;
  rc.right = x + tsize.cx + padding;
  rc.top = y;
  rc.bottom = y + tsize.cy + 1;

  if (mode.move_in_view) {
    auto offset = TextInBoxMoveInView(rc, map_rc);
    x += offset.x;
    y += offset.y;
  }

  if (label_block != nullptr && !label_block->check(rc))
    return false;

  if (mode.shape == LabelShape::ROUNDED_BLACK ||
      mode.shape == LabelShape::ROUNDED_WHITE) {
    if (mode.shape == LabelShape::ROUNDED_BLACK)
      canvas.SelectBlackPen();
    else
      canvas.SelectWhitePen();

    {
#ifdef ENABLE_OPENGL
      const ScopeAlphaBlend alpha_blend;
      canvas.Select(Brush(COLOR_WHITE.WithAlpha(0xa0)));
#else
      canvas.SelectWhiteBrush();
#endif

      canvas.DrawRoundRectangle(rc.left, rc.top, rc.right, rc.bottom,
                                Layout::VptScale(8), Layout::VptScale(8));
    }

    canvas.SetBackgroundTransparent();
    canvas.SetTextColor(COLOR_BLACK);
    canvas.DrawText(x, y, text);
  } else if (mode.shape == LabelShape::FILLED) {
    canvas.SetBackgroundColor(COLOR_WHITE);
    canvas.SetTextColor(COLOR_BLACK);
    canvas.DrawOpaqueText(x, y, rc, text);
  } else if (mode.shape == LabelShape::OUTLINED) {
    RenderShadowedText(canvas, text, x, y, false);
  } else if (mode.shape == LabelShape::OUTLINED_INVERTED) {
    RenderShadowedText(canvas, text, x, y, true);
  } else {
    canvas.SetBackgroundTransparent();
    canvas.SetTextColor(COLOR_BLACK);
    canvas.DrawText(x, y, text);
  }

  return true;
}

bool
TextInBox(Canvas &canvas, const TCHAR *text, int x, int y,
          TextInBoxMode mode,
          unsigned screen_width, unsigned screen_height,
          LabelBlock *label_block)
{
  PixelRect rc;
  rc.left = 0;
  rc.top = 0;
  rc.right = screen_width;
  rc.bottom = screen_height;

  return TextInBox(canvas, text, x, y, mode, rc, label_block);
}
