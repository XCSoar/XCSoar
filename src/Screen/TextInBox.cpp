/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Screen/TextInBox.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/LabelBlock.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Sizes.h" /* for WPCIRCLESIZE */

static RasterPoint
TextInBoxMoveInView(PixelRect &rc, const PixelRect &map_rc)
{
  RasterPoint offset;
  offset.x = 0;
  offset.y = 0;

  // If label is above maprect
  if (map_rc.top > rc.top) {
    // Move label down into maprect
    UPixelScalar d = map_rc.top - rc.top;
    rc.top += d;
    rc.bottom += d;
    offset.y += d;
  }

  // If label is right of maprect
  if (map_rc.right < rc.right) {
    UPixelScalar d = map_rc.right - rc.right;
    rc.right += d;
    rc.left += d;
    offset.x += d;
  }

  // If label is below maprect
  if (map_rc.bottom < rc.bottom) {
    UPixelScalar d = map_rc.bottom - rc.bottom;
    rc.top += d;
    rc.bottom += d;
    offset.y += d;
  }

  // If label is left of maprect
  if (map_rc.left > rc.left) {
    UPixelScalar d = map_rc.left - rc.left;
    rc.right += d;
    rc.left += d;
    offset.x += d;
  }

  return offset;
}

static void
RenderShadowedText(Canvas &canvas, const TCHAR* text,
                   PixelScalar x, PixelScalar y,
                   bool inverted)
{
  canvas.background_transparent();

  canvas.set_text_color(inverted ? COLOR_BLACK : COLOR_WHITE);
  canvas.text(x + Layout::FastScale(1), y, text);
  canvas.text(x - Layout::FastScale(1), y, text);
  canvas.text(x, y + 1, text);
  canvas.text(x, y - 1, text);

  canvas.set_text_color(inverted ? COLOR_WHITE : COLOR_BLACK);
  canvas.text(x, y, text);
}

// returns true if really wrote something
bool
TextInBox(Canvas &canvas, const TCHAR* text, PixelScalar x, PixelScalar y,
          TextInBoxMode mode, const PixelRect &map_rc, LabelBlock *label_block)
{
  PixelRect rc;

  if ((x < map_rc.left - WPCIRCLESIZE)
      || (x > map_rc.right + (WPCIRCLESIZE * 3))
      || (y < map_rc.top - WPCIRCLESIZE)
      || (y > map_rc.bottom + WPCIRCLESIZE))
    return false; // FIX Not drawn really

  // landable waypoint label inside white box

  canvas.select(mode.bold ? Fonts::MapBold : Fonts::Map);

  PixelSize tsize = canvas.text_size(text);

  if (mode.align == A_RIGHT) {
    x -= tsize.cx;
  } else if (mode.align == A_CENTER) {
    x -= tsize.cx / 2;
  }

  rc.left = x - Layout::FastScale(2) - 1;
  rc.right = x + tsize.cx + Layout::FastScale(2);
  rc.top = y;
  rc.bottom = y + tsize.cy + 1;

  if (mode.move_in_view) {
    RasterPoint offset = TextInBoxMoveInView(rc, map_rc);
    x += offset.x;
    y += offset.y;
  }

  if (label_block != NULL && !label_block->check(rc))
    return false;

  if (mode.mode == RM_ROUNDED_BLACK || mode.mode == RM_ROUNDED_WHITE) {
    if (mode.mode == RM_ROUNDED_BLACK)
      canvas.black_pen();
    else
      canvas.white_pen();

    canvas.white_brush();
    canvas.round_rectangle(rc.left, rc.top, rc.right, rc.bottom,
                           Layout::Scale(8), Layout::Scale(8));

    canvas.background_transparent();
    canvas.set_text_color(COLOR_BLACK);
    canvas.text(x, y, text);
    canvas.background_opaque();
  } else if (mode.mode == RM_FILLED) {
    canvas.set_background_color(COLOR_WHITE);
    canvas.set_text_color(COLOR_BLACK);
    canvas.text_opaque(x, y, rc, text);
  } else if (mode.mode == RM_OUTLINED) {
    RenderShadowedText(canvas, text, x, y, false);
  } else if (mode.mode == RM_OUTLINED_INVERTED) {
    RenderShadowedText(canvas, text, x, y, true);
  } else {
    canvas.background_transparent();
    canvas.set_text_color(COLOR_BLACK);
    canvas.text(x, y, text);
    canvas.background_opaque();
  }

  return true;
}

bool
TextInBox(Canvas &canvas, const TCHAR *text, PixelScalar x, PixelScalar y,
          TextInBoxMode mode,
          UPixelScalar screen_width, UPixelScalar screen_height,
          LabelBlock *label_block)
{
  PixelRect rc;
  rc.left = 0;
  rc.top = 0;
  rc.right = screen_width;
  rc.bottom = screen_height;

  return TextInBox(canvas, text, x, y, mode, rc, label_block);
}
