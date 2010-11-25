/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

static POINT
TextInBoxMoveInView(RECT &brect, const RECT &MapRect)
{
  const int LabelMargin = 4;

  POINT offset;
  offset.x = 0;
  offset.y = 0;

  if (MapRect.top > brect.top) {
    int d = MapRect.top - brect.top;
    brect.top += d;
    offset.y += d;
    brect.left -= d;
    offset.x -= d;
  }

  if (MapRect.right < brect.right) {
    int d = MapRect.right - brect.right;

    if (offset.y < LabelMargin) {
      int dy;

      if (d > -LabelMargin) {
        dy = LabelMargin - offset.y;
        if (d > -dy)
          dy = -d;
      } else {
        int x = d + (brect.right - brect.left) + 10;

        dy = x - offset.y;

        if (dy < 0)
          dy = 0;

        if (dy > LabelMargin)
          dy = LabelMargin;
      }

      brect.top += dy;
      brect.bottom += dy;
      offset.y += dy;
    }

    brect.right += d;
    brect.left += d;
    offset.x += d;
  }

  if (MapRect.bottom < brect.bottom) {
    if (offset.x == 0) {
      int d = MapRect.bottom - brect.bottom;
      brect.top += d;
      brect.bottom += d;
      offset.y += d;
    } else {
      if (offset.x < -LabelMargin) {
        int d = -(brect.bottom - brect.top) - 10;
        brect.top += d;
        brect.bottom += d;
        offset.y += d;
      } else {
        int d = -(2 * offset.x + (brect.bottom - brect.top));
        brect.top += d;
        brect.bottom += d;
        offset.y += d;
      }
    }
  }

  if (MapRect.left > brect.left) {
    int d = MapRect.left - brect.left;
    brect.right += d;
    brect.left += d;
    offset.x += d;
  }

  return offset;
}

static void
RenderShadowedText(Canvas &canvas, const TCHAR* text, int x, int y)
{
  canvas.background_transparent();

  canvas.set_text_color(Color::WHITE);
  canvas.text(x + 1, y, text);
  canvas.text(x + 2, y, text);
  canvas.text(x - 1, y, text);
  canvas.text(x - 2, y, text);
  canvas.text(x, y + 1, text);
  canvas.text(x, y - 1, text);

  canvas.set_text_color(Color::BLACK);
  canvas.text(x, y, text);
}

// returns true if really wrote something
bool
TextInBox(Canvas &canvas, const TCHAR* Value, int x, int y,
          TextInBoxMode_t Mode, const RECT &MapRect, LabelBlock *label_block)
{
  RECT brect;

  if ((x < MapRect.left - WPCIRCLESIZE)
      || (x > MapRect.right + (WPCIRCLESIZE * 3))
      || (y < MapRect.top - WPCIRCLESIZE)
      || (y > MapRect.bottom + WPCIRCLESIZE))
    return false; // FIX Not drawn really

  // landable waypoint label inside white box

  canvas.select(Mode.Mode == RoundedBlack ? Fonts::MapBold : Fonts::Map);

  SIZE tsize = canvas.text_size(Value);

  if (Mode.Align == Right) {
    x -= tsize.cx;
  } else if (Mode.Align == Center) {
    x -= tsize.cx / 2;
  }

  if (Mode.Mode == RoundedBlack || Mode.Mode == RoundedWhite) {
    brect.left = x - 2;
    brect.right = brect.left + tsize.cx + 4;
    brect.top = y + ((tsize.cy + 4) >> 3) - 2;
    brect.bottom = brect.top + 3 + tsize.cy - ((tsize.cy + 4) >> 3);

    if (Mode.Align == Right)
      x -= 3;

    POINT offset = TextInBoxMoveInView(brect, MapRect);
    x += offset.x;
    y += offset.y;

    if (label_block ? label_block->check(brect) : true) {
      if (Mode.Mode == RoundedBlack)
        canvas.black_pen();
      else
        canvas.white_pen();

      canvas.white_brush();
      canvas.round_rectangle(brect.left, brect.top, brect.right, brect.bottom,
                             Layout::Scale(8), Layout::Scale(8));

      canvas.background_transparent();
      canvas.text(x, y, Value);
      canvas.background_opaque();
      return true;
    }
  } else if (Mode.Mode == Filled) {
    brect.left = x - 1;
    brect.right = brect.left + tsize.cx + 1;
    brect.top = y + ((tsize.cy + 4) >> 3);
    brect.bottom = brect.top + tsize.cy - ((tsize.cy + 4) >> 3);

    if (Mode.Align == Right)
      x -= 2;

    POINT offset = TextInBoxMoveInView(brect, MapRect);
    x += offset.x;
    y += offset.y;

    if (label_block ? label_block->check(brect) : true) {
      canvas.set_background_color(Color::WHITE);
      canvas.text_opaque(x, y, brect, Value);
      return true;
    }
  } else if (Mode.Mode == Outlined) {
    brect.left = x - 2;
    brect.right = brect.left + tsize.cx + 4;
    brect.top = y + ((tsize.cy + 4) >> 3) - 2;
    brect.bottom = brect.top + 3 + tsize.cy - ((tsize.cy + 4) >> 3);

    if (label_block ? label_block->check(brect) : true) {
      RenderShadowedText(canvas, Value, x, y);
      return true;
    }
  } else {
    brect.left = x - 2;
    brect.right = brect.left + tsize.cx + 4;
    brect.top = y + ((tsize.cy + 4) >> 3) - 2;
    brect.bottom = brect.top + 3 + tsize.cy - ((tsize.cy + 4) >> 3);

    if (label_block ? label_block->check(brect) : true) {
      canvas.background_transparent();
      canvas.text(x, y, Value);
      canvas.background_opaque();
      return true;
    }
  }

  return false;
}
