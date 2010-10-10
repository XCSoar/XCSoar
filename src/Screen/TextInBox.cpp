/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, .
}
*/

#include "Screen/TextInBox.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/LabelBlock.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Appearance.hpp"
#include "Sizes.h" /* for WPCIRCLESIZE */

static bool
TextInBoxMoveInView(POINT *offset, RECT *brect, const RECT &MapRect)
{
  bool res = false;

  int LabelMargin = 4;

  offset->x = 0;
  offset->y = 0;

  if (MapRect.top > brect->top) {
    int d = MapRect.top - brect->top;
    brect->top += d;
    brect->bottom += d;
    offset->y += d;
    brect->bottom -= d;
    brect->left -= d;
    offset->x -= d;
    res = true;
  }

  if (MapRect.right < brect->right) {
    int d = MapRect.right - brect->right;

    if (offset->y < LabelMargin) {
      int dy;

      if (d > -LabelMargin) {
        dy = LabelMargin - offset->y;
        if (d > -dy)
          dy = -d;
      } else {
        int x = d + (brect->right - brect->left) + 10;

        dy = x - offset->y;

        if (dy < 0)
          dy = 0;

        if (dy > LabelMargin)
          dy = LabelMargin;
      }

      brect->top += dy;
      brect->bottom += dy;
      offset->y += dy;
    }

    brect->right += d;
    brect->left += d;
    offset->x += d;

    res = true;
  }

  if (MapRect.bottom < brect->bottom) {
    if (offset->x == 0) {
      int d = MapRect.bottom - brect->bottom;
      brect->top += d;
      brect->bottom += d;
      offset->y += d;
    } else {
      if (offset->x < -LabelMargin) {
        int d = -(brect->bottom - brect->top) - 10;
        brect->top += d;
        brect->bottom += d;
        offset->y += d;
      } else {
        int d = -(2 * offset->x + (brect->bottom - brect->top));
        brect->top += d;
        brect->bottom += d;
        offset->y += d;
      }
    }

    res = true;
  }

  if (MapRect.left > brect->left) {
    int d = MapRect.left - brect->left;
    brect->right += d;
    brect->left += d;
    offset->x += d;
    res = true;
  }

  return res;
}

// returns true if really wrote something
bool
TextInBox(Canvas &canvas, const TCHAR* Value, int x, int y,
          TextInBoxMode_t Mode, const RECT &MapRect, LabelBlock *label_block)
{
  RECT brect;
  POINT org;

  bool drawn = false;

  if ((x < MapRect.left - WPCIRCLESIZE)
      || (x > MapRect.right + (WPCIRCLESIZE * 3))
      || (y < MapRect.top - WPCIRCLESIZE)
      || (y > MapRect.bottom + WPCIRCLESIZE))
    return drawn; // FIX Not drawn really

  org.x = x;
  org.y = y;

  canvas.white_brush();

  if (Mode.AsFlag.Reachable && Appearance.IndLandable == wpLandableDefault)
    // make space for the green circle
    x += 5;

  // landable waypoint label inside white box
  if (!Mode.AsFlag.NoSetFont)
    // VENTA5 predefined font from calling function
    canvas.select(Mode.AsFlag.Border ? Fonts::MapBold : Fonts::Map);

  SIZE tsize = canvas.text_size(Value);

  if (Mode.AsFlag.AlignRight) {
    x -= tsize.cx;
  } else if (Mode.AsFlag.AlignCenter) {
    x -= tsize.cx / 2;
    y -= tsize.cy / 2;
  }

  if (Mode.AsFlag.Border || Mode.AsFlag.WhiteBorder) {
    POINT offset;

    brect.left = x - 2;
    brect.right = brect.left + tsize.cx + 4;
    brect.top = y + ((tsize.cy + 4) >> 3) - 2;
    brect.bottom = brect.top + 3 + tsize.cy - ((tsize.cy + 4) >> 3);

    if (Mode.AsFlag.AlignRight)
      x -= 3;

    if (TextInBoxMoveInView(&offset, &brect, MapRect)) {
      x += offset.x;
      y += offset.y;
    }

    if (label_block ? label_block->check(brect) : true) {
      if (Mode.AsFlag.Border)
        canvas.select(Graphics::hpMapScale);
      else
        canvas.white_pen();


      canvas.round_rectangle(brect.left, brect.top, brect.right, brect.bottom,
                             Layout::Scale(8), Layout::Scale(8));

      canvas.background_transparent();
      canvas.text(x, y, Value);
      canvas.background_opaque();

      drawn = true;
    }
  } else if (Mode.AsFlag.FillBackground) {
    POINT offset;

    brect.left = x - 1;
    brect.right = brect.left + tsize.cx + 1;
    brect.top = y + ((tsize.cy + 4) >> 3);
    brect.bottom = brect.top + tsize.cy - ((tsize.cy + 4) >> 3);

    if (Mode.AsFlag.AlignRight)
      x -= 2;

    if (TextInBoxMoveInView(&offset, &brect, MapRect)) {
      x += offset.x;
      y += offset.y;
    }

    if (label_block ? label_block->check(brect) : true) {
      canvas.set_background_color(Color::WHITE);
      canvas.text_opaque(x, y, brect, Value);
      drawn = true;
    }
  } else if (Mode.AsFlag.WhiteBold) {
    brect.left = x - 2;
    brect.right = brect.left + tsize.cx + 4;
    brect.top = y + ((tsize.cy + 4) >> 3) - 2;
    brect.bottom = brect.top + 3 + tsize.cy - ((tsize.cy + 4) >> 3);

    if (label_block ? label_block->check(brect) : true) {
      canvas.set_text_color(Color::WHITE);

      canvas.text(x + 1, y, Value);
      canvas.text(x + 2, y, Value);
      canvas.text(x - 1, y, Value);
      canvas.text(x - 2, y, Value);
      canvas.text(x, y + 1, Value);
      canvas.text(x, y - 1, Value);

      canvas.background_transparent();
      canvas.set_text_color(Color::BLACK);
      canvas.text(x, y, Value);
      canvas.background_opaque();

      drawn = true;
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

      drawn = true;
    }
  }

  return drawn;
}
