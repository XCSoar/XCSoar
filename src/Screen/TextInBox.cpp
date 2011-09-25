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
TextInBoxMoveInView(PixelRect &brect, const PixelRect &MapRect)
{
  RasterPoint offset;
  offset.x = 0;
  offset.y = 0;

  // If label is above maprect
  if (MapRect.top > brect.top) {
    // Move label down into maprect
    UPixelScalar d = MapRect.top - brect.top;
    brect.top += d;
    brect.bottom += d;
    offset.y += d;
  }

  // If label is right of maprect
  if (MapRect.right < brect.right) {
    UPixelScalar d = MapRect.right - brect.right;
    brect.right += d;
    brect.left += d;
    offset.x += d;
  }

  // If label is below maprect
  if (MapRect.bottom < brect.bottom) {
    UPixelScalar d = MapRect.bottom - brect.bottom;
    brect.top += d;
    brect.bottom += d;
    offset.y += d;
  }

  // If label is left of maprect
  if (MapRect.left > brect.left) {
    UPixelScalar d = MapRect.left - brect.left;
    brect.right += d;
    brect.left += d;
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
TextInBox(Canvas &canvas, const TCHAR* Value, PixelScalar x, PixelScalar y,
          TextInBoxMode Mode, const PixelRect &MapRect,
          LabelBlock *label_block)
{
  PixelRect brect;

  if ((x < MapRect.left - WPCIRCLESIZE)
      || (x > MapRect.right + (WPCIRCLESIZE * 3))
      || (y < MapRect.top - WPCIRCLESIZE)
      || (y > MapRect.bottom + WPCIRCLESIZE))
    return false; // FIX Not drawn really

  // landable waypoint label inside white box

  canvas.select(Mode.Bold ? Fonts::MapBold : Fonts::Map);

  PixelSize tsize = canvas.text_size(Value);

  if (Mode.Align == Right) {
    x -= tsize.cx;
  } else if (Mode.Align == Center) {
    x -= tsize.cx / 2;
  }

  brect.left = x - Layout::FastScale(2) - 1;
  brect.right = x + tsize.cx + Layout::FastScale(2);
  brect.top = y;
  brect.bottom = y + tsize.cy + 1;

  if (Mode.MoveInView) {
    RasterPoint offset = TextInBoxMoveInView(brect, MapRect);
    x += offset.x;
    y += offset.y;
  }

  if (label_block != NULL && !label_block->check(brect))
    return false;

  if (Mode.Mode == RoundedBlack || Mode.Mode == RoundedWhite) {
    if (Mode.Mode == RoundedBlack)
      canvas.black_pen();
    else
      canvas.white_pen();

    canvas.white_brush();
    canvas.round_rectangle(brect.left, brect.top, brect.right, brect.bottom,
                           Layout::Scale(8), Layout::Scale(8));

    canvas.background_transparent();
    canvas.set_text_color(COLOR_BLACK);
    canvas.text(x, y, Value);
    canvas.background_opaque();
  } else if (Mode.Mode == Filled) {
    canvas.set_background_color(COLOR_WHITE);
    canvas.set_text_color(COLOR_BLACK);
    canvas.text_opaque(x, y, brect, Value);
  } else if (Mode.Mode == Outlined) {
    RenderShadowedText(canvas, Value, x, y, false);
  } else if (Mode.Mode == OutlinedInverted) {
    RenderShadowedText(canvas, Value, x, y, true);
  } else {
    canvas.background_transparent();
    canvas.set_text_color(COLOR_BLACK);
    canvas.text(x, y, Value);
    canvas.background_opaque();
  }

  return true;
}

bool
TextInBox(Canvas &canvas, const TCHAR *Value, PixelScalar x, PixelScalar y,
          TextInBoxMode Mode,
          unsigned screen_width, unsigned screen_height,
          LabelBlock *label_block)
{
  PixelRect rc;
  rc.left = 0;
  rc.top = 0;
  rc.right = screen_width;
  rc.bottom = screen_height;

  return TextInBox(canvas, Value, x, y, Mode, rc, label_block);
}
