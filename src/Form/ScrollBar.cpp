/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Window.hpp"
#include "resource.h"
#include "Asset.hpp"
#include "Util/Macros.hpp"

#include <assert.h>

using std::min;

ScrollBar::ScrollBar()
  :dragging(false)
{
  // Reset the ScrollBar on creation
  Reset();
}

void
ScrollBar::SetSize(const PixelSize size)
{
  UPixelScalar width;

  // if the device has a pointer (mouse/touchscreen/etc.)
  if (HasTouchScreen())
    /* wide scroll bar for touch screen */
    width = Layout::Scale(24);
  else if (HasPointer())
    /* with a mouse, the scroll bar can be smaller */
    width = Layout::SmallScale(16);
  else
    // thin for ALTAIR b/c no touch screen
    width = Layout::SmallScale(12);

  // Update the coordinates of the scrollbar
  rc.left = size.cx - width;
  rc.top = 0;
  rc.right = size.cx;
  rc.bottom = size.cy;
}

void
ScrollBar::Reset()
{
  rc.SetEmpty();
  rc_slider.SetEmpty();
}

void
ScrollBar::SetSlider(unsigned size, unsigned view_size,
                      unsigned origin)
{
  const PixelScalar netto_height = GetNettoHeight();

  // If (no size) slider fills the whole area (no scrolling)
  PixelScalar height = size > 0
    ? (int)(netto_height * view_size / size)
    : netto_height;
  // Prevent the slider from getting to small
  if (height < GetWidth())
    height = GetWidth();

  if (height > netto_height)
    height = netto_height;

  // Calculate highest origin (counted in ListItems)
  unsigned max_origin = size - view_size;

  // Move the slider to the appropriate position
  PixelScalar top = (max_origin > 0) ?
      ((netto_height - height) * origin / max_origin) : 0;

  // Prevent the slider from getting to big
  // TODO: not needed?!
  if (top + height > netto_height)
    height = netto_height - top;

  // Update slider coordinates
  rc_slider.left = rc.left;
  rc_slider.top = rc.top + GetWidth() + top;
  rc_slider.right = rc.right;
  rc_slider.bottom = rc_slider.top + height;
}

unsigned
ScrollBar::ToOrigin(unsigned size, unsigned view_size,
                     PixelScalar y) const
{
  // Calculate highest origin (counted in ListItems)
  unsigned max_origin = size - view_size;
  if (max_origin <= 0)
    return 0;

  y -= rc.top + GetWidth();
  if (y < 0)
    return 0;

  unsigned origin = y * max_origin / GetScrollHeight();
  return min(origin, max_origin);
}

void
ScrollBar::Paint(Canvas &canvas) const
{
  // ###################
  // #### ScrollBar ####
  // ###################

  // draw rectangle around entire scrollbar area
  canvas.SelectBlackPen();
  canvas.SelectHollowBrush();
  canvas.Rectangle(rc.left, rc.top, rc.right, rc.bottom);

  // ###################
  // ####  Buttons  ####
  // ###################

  const int arrow_padding = std::max(GetWidth() / 4, 4);

  PixelRect up_arrow_rect = rc;
  ++up_arrow_rect.left;
  up_arrow_rect.bottom = up_arrow_rect.top + GetWidth();

  PixelRect down_arrow_rect = rc;
  ++down_arrow_rect.left;
  down_arrow_rect.top = down_arrow_rect.bottom - GetWidth();

  canvas.DrawExactLine(up_arrow_rect.left, up_arrow_rect.bottom,
                       up_arrow_rect.right, up_arrow_rect.bottom);
  canvas.DrawExactLine(down_arrow_rect.left, down_arrow_rect.top - 1,
                       down_arrow_rect.right, down_arrow_rect.top - 1);

  canvas.DrawButton(up_arrow_rect, false);
  canvas.DrawButton(down_arrow_rect, false);

  canvas.SelectNullPen();
  canvas.SelectBlackBrush();

  const RasterPoint up_arrow[3] = {
    { (up_arrow_rect.left + rc.right) / 2,
      up_arrow_rect.top + arrow_padding },
    { up_arrow_rect.left + arrow_padding,
      up_arrow_rect.bottom - arrow_padding },
    { rc.right - arrow_padding,
      up_arrow_rect.bottom - arrow_padding },
  };
  canvas.DrawTriangleFan(up_arrow, ARRAY_SIZE(up_arrow));

  const RasterPoint down_arrow[3] = {
    { (down_arrow_rect.left + rc.right) / 2,
      down_arrow_rect.bottom - arrow_padding },
    { down_arrow_rect.left + arrow_padding,
      down_arrow_rect.top + arrow_padding },
    { rc.right - arrow_padding,
      down_arrow_rect.top + arrow_padding },
  };
  canvas.DrawTriangleFan(down_arrow, ARRAY_SIZE(down_arrow));

  // ###################
  // ####  Slider   ####
  // ###################

  if (rc_slider.top + 4 < rc_slider.bottom) {
    canvas.SelectBlackPen();
    canvas.DrawExactLine(rc_slider.left, rc_slider.top,
                         rc_slider.right, rc_slider.top);
    canvas.DrawExactLine(rc_slider.left, rc_slider.bottom,
                         rc_slider.right, rc_slider.bottom);

    PixelRect rc_slider2 = rc_slider;
    ++rc_slider2.left;
    ++rc_slider2.top;
    canvas.DrawButton(rc_slider2, false);
  }

  // fill the rest with darker gray
  if (up_arrow_rect.bottom + 1 < rc_slider.top)
    canvas.DrawFilledRectangle(rc.left + 1, up_arrow_rect.bottom + 1,
                               rc.right, rc_slider.top, COLOR_GRAY);

  if (rc_slider.bottom + 1 < down_arrow_rect.top - 1)
    canvas.DrawFilledRectangle(rc.left + 1, rc_slider.bottom + 1,
                               rc.right, down_arrow_rect.top - 1, COLOR_GRAY);
}

void
ScrollBar::DragBegin(Window *w, UPixelScalar y)
{
  // Make sure that we are not dragging already
  assert(!dragging);

  // Save the offset of the drag
  drag_offset = y - rc_slider.top;
  // ... and remember that we are dragging now
  dragging = true;
  w->SetCapture();
}

void
ScrollBar::DragEnd(Window *w)
{
  // If we are not dragging right now -> nothing to end
  if (!dragging)
    return;

  // Realize that we are not dragging anymore
  dragging = false;
  w->ReleaseCapture();
}

unsigned
ScrollBar::DragMove(unsigned size, unsigned view_size, PixelScalar y) const
{
  assert(dragging);

  return ToOrigin(size, view_size, y - drag_offset);
}
