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

#include "Form/SymbolButton.hpp"
#include "Form/Control.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Layout.hpp"
#include "resource.h"

void
WndSymbolButton::on_paint(Canvas &canvas)
{
#ifdef HAVE_CLIPPING
  /* background and selector */
  canvas.clear(background_brush);
#endif

  // Get button PixelRect and shrink it to make room for the selector/focus
  PixelRect rc = get_client_rect();

  // Draw button to the background
  canvas.draw_button(rc, is_down());

  // Draw focus rectangle
  if (has_focus()) {
    PixelRect focus_rc = rc;
    InflateRect(&focus_rc, -3, -3);
    canvas.draw_focus(focus_rc);
  }

  // If button has text on it
  tstring caption = get_text();
  if (caption.empty())
    return;

  // If button is pressed, offset the text for 3D effect
  if (is_down())
    OffsetRect(&rc, 1, 1);

  canvas.null_pen();
  if (is_enabled())
    canvas.black_brush();
  else
    canvas.select(disabled_brush);

  const char ch = (char)caption[0];

  // Draw arrow symbols instead of < and >
  if (ch == '<' || ch == '>') {
    int size = min(rc.right - rc.left, rc.bottom - rc.top) / 5;

    static RasterPoint Arrow[4];
    Arrow[0].x = (rc.left + rc.right) / 2 + (ch == '<' ? size : -size);
    Arrow[0].y = (rc.top + rc.bottom) / 2 + size;
    Arrow[1].x = (rc.left + rc.right) / 2 + (ch == '<' ? -size : size);
    Arrow[1].y = (rc.top + rc.bottom) / 2;
    Arrow[2].x = (rc.left + rc.right) / 2 + (ch == '<' ? size : -size);
    Arrow[2].y = (rc.top + rc.bottom) / 2 - size;
    Arrow[3].x = Arrow[0].x;
    Arrow[3].y = Arrow[0].y;

    canvas.polygon(Arrow, 4);
  }

  // Draw arrow symbols instead of v and ^
  if (ch == '^' || ch == 'v') {
    int size = min(rc.right - rc.left, rc.bottom - rc.top) / 5;

    RasterPoint Arrow[3];
    Arrow[0].x = (rc.left + rc.right) / 2 +
                 size;
    Arrow[0].y = (rc.top + rc.bottom) / 2 +
                 (ch == '^' ? size : -size);
    Arrow[1].x = (rc.left + rc.right) / 2;
    Arrow[1].y = (rc.top + rc.bottom) / 2 +
                 (ch == '^' ? -size : size);
    Arrow[2].x = (rc.left + rc.right) / 2 - size;
    Arrow[2].y = (rc.top + rc.bottom) / 2 +
                 (ch == '^' ? size : -size);

    canvas.polygon(Arrow, 3);
  }

  // Draw symbols instead of + and -
  if (ch == '+' || ch == '-') {
    int size = min(rc.right - rc.left, rc.bottom - rc.top) / 5;

    canvas.rectangle((rc.left + rc.right) / 2 - size,
                     (rc.top + rc.bottom) / 2 - size / 3,
                     (rc.left + rc.right) / 2 + size,
                     (rc.top + rc.bottom) / 2 + size / 3);

    if (ch == '+')
      canvas.rectangle((rc.left + rc.right) / 2 - size / 3,
                       (rc.top + rc.bottom) / 2 - size,
                       (rc.left + rc.right) / 2 + size / 3,
                       (rc.top + rc.bottom) / 2 + size);
  }

  // Draw Fly bitmap
  if (caption == _T("Fly")) {
    Bitmap launcher1_bitmap(IDB_LAUNCHER1);
    canvas.clear_white();
    if (is_down())
      canvas.invert_stretch_transparent(launcher1_bitmap, COLOR_YELLOW);
    else
      canvas.stretch_transparent(launcher1_bitmap, COLOR_BLUE);
  }

  // Draw Simulator bitmap
  if (caption == _T("Simulator")) {
    Bitmap launcher2_bitmap(IDB_LAUNCHER2);
    canvas.clear_white();
    if (is_down())
      canvas.invert_stretch_transparent(launcher2_bitmap, COLOR_YELLOW);
    else
      canvas.stretch_transparent(launcher2_bitmap, COLOR_BLUE);
  }

}
