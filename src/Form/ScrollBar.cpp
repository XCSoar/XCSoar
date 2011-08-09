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

#include "Form/ScrollBar.hpp"
#include "Form/Internal.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Window.hpp"
#include "resource.h"
#include "Asset.hpp"

#include <assert.h>

#include <algorithm>

using std::min;

ScrollBar::ScrollBar()
  :dragging(false)
{
  // Reset the ScrollBar on creation
  reset();
}

void
ScrollBar::set(const PixelSize size)
{
  unsigned width;

  // if the device has a pointer (mouse/touchscreen/etc.)
  if (has_pointer()) {
    width = (unsigned)Layout::Scale(SCROLLBARWIDTH_INITIAL);
  } else {
    // thin for ALTAIR b/c no touch screen
    width = SELECTORWIDTH * 2;
  }

  // Update the coordinates of the scrollbar
  rc.left = size.cx - width;
  rc.top = 0;
  rc.right = size.cx;
  rc.bottom = size.cy;
}

void
ScrollBar::reset()
{
  SetRectEmpty(&rc);
  SetRectEmpty(&rc_slider);
}

void
ScrollBar::set_slider(unsigned size, unsigned view_size,
                                    unsigned origin)
{
  const int netto_height = get_netto_height();

  // If (no size) slider fills the whole area (no scrolling)
  int height = size > 0
    ? (int)(netto_height * view_size / size)
    : netto_height;
  // Prevent the slider from getting to small
  if (height < get_width())
    height = get_width();

  // Calculate highest origin (counted in ListItems)
  int max_origin = size - view_size;

  // Move the slider to the appropriate position
  int top = (max_origin > 0) ?
      ((netto_height - height) * origin / max_origin) : 0;

  // Prevent the slider from getting to big
  // TODO: not needed?!
  if (top + height > netto_height)
    height = netto_height - top;

  // Update slider coordinates
  rc_slider.left = rc.left;
  rc_slider.top = rc.top + get_width() + top;
  rc_slider.right = rc.right;
  rc_slider.bottom = rc_slider.top + height;
}

unsigned
ScrollBar::to_origin(unsigned size, unsigned view_size,
                                   int y) const
{
  // Calculate highest origin (counted in ListItems)
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
ScrollBar::paint(Canvas &canvas) const
{
  // Prepare Pen
  canvas.black_pen();

  // ###################
  // #### ScrollBar ####
  // ###################

  // draw rectangle around entire scrollbar area
  canvas.two_lines(rc.left, rc.top, rc.left, rc.bottom,
                   rc.right, rc.bottom);
  canvas.two_lines(rc.right, rc.bottom, rc.right, rc.top,
                   rc.left, rc.top);

  // ###################
  // ####  Buttons  ####
  // ###################

  unsigned arrow_padding = max(get_width() / 4, 4);
  canvas.black_brush();

  PixelRect up_arrow_rect = rc;
  ++up_arrow_rect.left;
  up_arrow_rect.bottom = up_arrow_rect.top + get_width();
  canvas.line(up_arrow_rect.left, up_arrow_rect.bottom,
              up_arrow_rect.right, up_arrow_rect.bottom);
  canvas.draw_button(up_arrow_rect, false);

  RasterPoint up_arrow[3] = {
    { (up_arrow_rect.left + rc.right) / 2,
      up_arrow_rect.top + arrow_padding },
    { up_arrow_rect.left + arrow_padding,
      up_arrow_rect.bottom - arrow_padding },
    { rc.right - arrow_padding,
      up_arrow_rect.bottom - arrow_padding },
  };
  canvas.TriangleFan(up_arrow, sizeof(up_arrow) / sizeof(up_arrow[0]));

  PixelRect down_arrow_rect = rc;
  ++down_arrow_rect.left;
  down_arrow_rect.top = down_arrow_rect.bottom - get_width();
  canvas.line(down_arrow_rect.left, down_arrow_rect.top - 1,
              down_arrow_rect.right, down_arrow_rect.top - 1);
  canvas.draw_button(down_arrow_rect, false);

  RasterPoint down_arrow[3] = {
    { (down_arrow_rect.left + rc.right) / 2,
      down_arrow_rect.bottom - arrow_padding },
    { down_arrow_rect.left + arrow_padding,
      down_arrow_rect.top + arrow_padding },
    { rc.right - arrow_padding,
      down_arrow_rect.top + arrow_padding },
  };
  canvas.TriangleFan(down_arrow, sizeof(down_arrow) / sizeof(down_arrow[0]));

  // ###################
  // ####  Slider   ####
  // ###################

  canvas.line(rc_slider.left, rc_slider.top,
              rc_slider.right, rc_slider.top);
  canvas.line(rc_slider.left, rc_slider.bottom,
              rc_slider.right, rc_slider.bottom);

  PixelRect rc_slider2 = rc_slider;
  ++rc_slider2.left;
  ++rc_slider2.top;
  canvas.draw_button(rc_slider2, false);

  // fill the rest with darker gray
  canvas.fill_rectangle(rc.left + 1, up_arrow_rect.bottom + 1,
                        rc.right, rc_slider.top, COLOR_GRAY);
  canvas.fill_rectangle(rc.left + 1, rc_slider.bottom,
                        rc.right, down_arrow_rect.top, COLOR_GRAY);
}

void
ScrollBar::drag_begin(Window *w, unsigned y)
{
  // Make sure that we are not dragging already
  assert(!dragging);

  // Save the offset of the drag
  drag_offset = y - rc_slider.top;
  // ... and remember that we are dragging now
  dragging = true;
  w->set_capture();
}

void
ScrollBar::drag_end(Window *w)
{
  // If we are not dragging right now -> nothing to end
  if (!dragging)
    return;

  // Realize that we are not dragging anymore
  dragging = false;
  w->release_capture();
}

unsigned
ScrollBar::drag_move(unsigned size, unsigned view_size,
                                   int y) const
{
  assert(dragging);

  return to_origin(size, view_size, y - drag_offset);
}
