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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Form/List.hpp"
#include "Form/Internal.hpp"
#include "Screen/BitmapCanvas.hpp"
#include "Screen/Layout.hpp"
#include "resource.h"
#include "Asset.hpp"

#include <assert.h>

#include <algorithm>

using std::min;

WndListFrame::ScrollBar::ScrollBar()
  :dragging(false)
{
  reset();
}

void
WndListFrame::ScrollBar::set(const SIZE size)
{
  unsigned width;

  // if the device has a pointer (mouse/touchscreen/etc.)
  if (has_pointer()) {
    // shrink width factor.  Range .1 to 1 where 1 is very "fat"
    double ShrinkFactor = is_pna() ? 1.0 : 0.75;
    width = (unsigned)Layout::Scale(SCROLLBARWIDTH_INITIAL * ShrinkFactor);
  } else {
    // thin for ALTAIR b/c no touch screen
    width = SELECTORWIDTH * 2;
  }

  // Update the coordinates of the scrollbar
  rc.left = size.cx - width;
  rc.top = 0;
  rc.right = size.cx;
  rc.bottom = size.cy;

  // Load bitmaps
  if (!hScrollBarBitmapTop.defined())
    hScrollBarBitmapTop.load(IDB_SCROLLBARTOP);
  if (!hScrollBarBitmapMid.defined())
    hScrollBarBitmapMid.load(IDB_SCROLLBARMID);
  if (!hScrollBarBitmapBot.defined())
    hScrollBarBitmapBot.load(IDB_SCROLLBARBOT);
  if (!hScrollBarBitmapFill.defined())
    hScrollBarBitmapFill.load(IDB_SCROLLBARFILL);
}

void
WndListFrame::ScrollBar::reset()
{
  SetRectEmpty(&rc);
  SetRectEmpty(&rc_slider);
}

void
WndListFrame::ScrollBar::set_slider(unsigned size, unsigned view_size,
                                    unsigned origin)
{
  const int netto_height = get_netto_height();

  int height = size > 0 ? netto_height * view_size / size : netto_height;
  if (height < get_width())
    height = get_width();

  int max_origin = size - view_size;
  int top = max_origin > 0
    ? (netto_height - height) * origin / max_origin
    : 0;

  if (top + height > netto_height)
    height = netto_height - top;

  rc_slider.left = rc.left;
  rc_slider.top = rc.top + get_width() + top - 1;
  rc_slider.right = rc.right - 1; // -2 if use 3x pen.  -1 if 2x pen
  rc_slider.bottom = rc_slider.top + height + 2; // +2 for 3x pen, +1 for 2x pen
}

unsigned
WndListFrame::ScrollBar::to_origin(unsigned size, unsigned view_size,
                                   int y) const
{
  int max_origin = size - view_size;
  if (max_origin <= 0)
    return 0;

  y -= rc.top + get_width();
  if (y < 0)
    return 0;

  unsigned origin = y * max_origin / get_scroll_height();
  return min(origin, (unsigned)max_origin);
}

void
WndListFrame::ScrollBar::paint(Canvas &canvas, Color fore_color) const
{
  Brush brush(Color(0xff, 0xff, 0xff));
  Pen pen(DEFAULTBORDERPENWIDTH, fore_color);
  canvas.select(pen);

  // ###################
  // #### ScrollBar ####
  // ###################

  // draw rectangle around entire scrollbar area
  canvas.two_lines(rc.left, rc.top, rc.left, rc.bottom,
                   rc.right, rc.bottom);
  canvas.two_lines(rc.right, rc.bottom, rc.right, rc.top,
                   rc.left, rc.top);

  // TODO: ScrollBar Fill Bitmap is unused

  // ###################
  // ####  Buttons  ####
  // ###################

  bool bTransparentUpDown = true;

  // Create a canvas for drawing the bitmaps
  BitmapCanvas bitmap_canvas(canvas);

  if (get_width() == SCROLLBARWIDTH_INITIAL) {
    // If the Scrollbar size hasn't been changed
    // -> just copy the button bitmaps to the canvas

    bitmap_canvas.select(hScrollBarBitmapTop);
    canvas.copy(rc.left, rc.top,
                SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                bitmap_canvas, 0, 0,
                bTransparentUpDown);

    bitmap_canvas.select(hScrollBarBitmapBot);
    canvas.copy(rc.left, rc.bottom - SCROLLBARWIDTH_INITIAL,
                SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                bitmap_canvas, 0, 0,
                bTransparentUpDown);
  } else {
    // If the Scrollbar size has been changed
    // -> stretch-copy the button bitmaps to the canvas

    bitmap_canvas.select(hScrollBarBitmapTop);
    canvas.stretch(rc.left, rc.top, get_width(), get_width(),
                   bitmap_canvas,
                   0, 0, SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                   bTransparentUpDown);

    bitmap_canvas.select(hScrollBarBitmapBot);
    canvas.stretch(rc.left, rc.bottom - get_width(), get_width(), get_width(),
                   bitmap_canvas,
                   0, 0, SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                   bTransparentUpDown);
  }

  // ###################
  // ####  Slider   ####
  // ###################

  bitmap_canvas.select(hScrollBarBitmapMid);
  // always SRCAND b/c on top of scrollbutton texture
  canvas.stretch_and(rc_slider.left + 1, rc_slider.top + 1,
                     rc_slider.right - rc_slider.left - 2,
                     rc_slider.bottom - rc_slider.top - 2,
                     bitmap_canvas, 0, 0, 30, 28);

  // box around slider rect
  pen.set(DEFAULTBORDERPENWIDTH * 2, fore_color);
  canvas.select(pen);

  int iBorderOffset = 1;  // set to 1 if BORDERWIDTH >2, else 0

  canvas.two_lines(rc_slider.left + iBorderOffset, rc_slider.top,
                   rc_slider.left + iBorderOffset, rc_slider.bottom,
                   rc_slider.right, rc_slider.bottom); // just left line of scrollbar
  canvas.two_lines(rc_slider.right, rc_slider.bottom,
                   rc_slider.right, rc_slider.top,
                   rc_slider.left + iBorderOffset, rc_slider.top); // just left line of scrollbar
}

void
WndListFrame::ScrollBar::drag_begin(Window *w, unsigned y)
{
  assert(!dragging);

  drag_offset = y - rc_slider.top;
  dragging = true;
  w->set_capture();
}

void
WndListFrame::ScrollBar::drag_end(Window *w)
{
  if (!dragging)
    return;

  dragging = false;
  w->release_capture();
}

unsigned
WndListFrame::ScrollBar::drag_move(unsigned size, unsigned view_size,
                                   int y) const
{
  assert(dragging);

  return to_origin(size, view_size, y - drag_offset);
}
