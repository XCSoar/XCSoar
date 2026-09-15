// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ScrollBar.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Brush.hpp"
#include "Screen/Layout.hpp"
#include "ui/window/PaintWindow.hpp"
#include "Asset.hpp"
#include "Look/ButtonLook.hpp"
#include "util/Compiler.h"
#include "util/Macros.hpp"

#include <cassert>

ScrollBar::ScrollBar(const ButtonLook &_button_look) noexcept
  :button_renderer(_button_look), dragging(false)
{
  // Reset the ScrollBar on creation
  Reset();
}

void
ScrollBar::SetSize(const PixelSize size) noexcept
{
  unsigned width;

  // if the device has a pointer (mouse/touchscreen/etc.)
  if (HasPointer())
    /* with a mouse, the scroll bar can be smaller */
    width = Layout::GetMinimumControlHeight();
  else
    // thin for devices without touch screen
    width = Layout::VptScale(10);

  // Update the coordinates of the scrollbar
  rc.left = size.width - width;
  rc.top = 0;
  rc.right = size.width;
  rc.bottom = size.height;
}

void
ScrollBar::Reset() noexcept
{
  rc.SetEmpty();
  rc_slider.SetEmpty();
}

void
ScrollBar::SetSlider(unsigned size, unsigned view_size,
                     unsigned origin) noexcept
{
  const int netto_height = GetNettoHeight();

  // If (no size) slider fills the whole area (no scrolling)
  int height = size > 0
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
  int top = (max_origin > 0) ?
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
ScrollBar::ToOrigin(unsigned size, unsigned view_size, int y) const noexcept
{
  // Calculate highest origin (counted in ListItems)
  unsigned max_origin = size - view_size;
  if (max_origin <= 0)
    return 0;

  y -= rc.top + GetWidth();
  if (y < 0)
    return 0;

  unsigned origin = y * max_origin / GetScrollHeight();
  return std::min(origin, max_origin);
}

void
ScrollBar::Paint(Canvas &canvas) const noexcept
{
  Paint(canvas, ButtonState::ENABLED, ButtonState::ENABLED);
}

void
ScrollBar::Paint(Canvas &canvas, ButtonState up_state,
                 ButtonState down_state) const noexcept
{
  const ButtonLook &look = button_renderer.GetLook();
  const int width = GetWidth();

  /* the track is the quiet step between the page and a button face,
     so the bar reads as a channel in the surface instead of as a
     frame around its parts */
  canvas.DrawFilledRectangle(rc, IsDithered()
                             ? COLOR_WHITE
                             : look.disabled.background_color);

  /* the arrows keep their button faces: they are tap targets like
     any other button */
  PixelRect up_arrow_rect = rc;
  up_arrow_rect.bottom = up_arrow_rect.top + width;

  PixelRect down_arrow_rect = rc;
  down_arrow_rect.top = down_arrow_rect.bottom - width;

  button_renderer.DrawButton(canvas, up_arrow_rect, up_state);
  button_renderer.DrawButton(canvas, down_arrow_rect, down_state);

  canvas.SelectNullPen();

  const auto select_foreground = [&canvas, &look](ButtonState state) {
    switch (state) {
    case ButtonState::DISABLED:
      canvas.Select(look.disabled.brush);
      break;

    case ButtonState::FOCUSED:
      canvas.Select(look.focused.foreground_brush);
      break;

    case ButtonState::PRESSED:
      canvas.Select(look.focused.pressed_foreground_brush);
      break;

    case ButtonState::SELECTED:
      canvas.Select(look.focused.pressed_foreground_brush);
      break;

    case ButtonState::ENABLED:
      canvas.Select(look.standard.foreground_brush);
      break;
    }
  };

  const int arrow_padding = std::max(width / 4, 4);

  const BulkPixelPoint up_arrow[3] = {
    { (up_arrow_rect.left + rc.right) / 2,
      up_arrow_rect.top + arrow_padding },
    { up_arrow_rect.left + arrow_padding,
      up_arrow_rect.bottom - arrow_padding },
    { rc.right - arrow_padding,
      up_arrow_rect.bottom - arrow_padding },
  };
  select_foreground(up_state);
  canvas.DrawTriangleFan(up_arrow, ARRAY_SIZE(up_arrow));

  const BulkPixelPoint down_arrow[3] = {
    { (down_arrow_rect.left + rc.right) / 2,
      down_arrow_rect.bottom - arrow_padding },
    { down_arrow_rect.left + arrow_padding,
      down_arrow_rect.top + arrow_padding },
    { rc.right - arrow_padding,
      down_arrow_rect.top + arrow_padding },
  };
  select_foreground(down_state);
  canvas.DrawTriangleFan(down_arrow, ARRAY_SIZE(down_arrow));

  canvas.SelectHollowBrush();

  /* the slider is a pill lying on the track, not a button: it has no
     face and no border of its own */
  if (rc_slider.top + 4 < rc_slider.bottom) {
    PixelRect thumb = rc_slider;
    const int inset = std::max(1, width / 5);
    if ((int)thumb.GetWidth() > 2 * inset)
      thumb.Grow(-inset, 0);

    /* the slider darkens while it is dragged; it takes that from its
       own color rather than from a button state, which would tie the
       track to a face it has nothing to do with */
    const Brush brush{IsDithered()
        ? COLOR_BLACK
        : (dragging
           ? DarkColor(look.standard.ring_color)
           : look.standard.ring_color)};
    canvas.Select(brush);

    if (IsDithered())
      canvas.DrawFilledRectangle(thumb, COLOR_BLACK);
    else
      canvas.DrawRoundRectangle(thumb, PixelSize{(unsigned)thumb.GetWidth()});

    canvas.SelectHollowBrush();
  }
}

void
ScrollBar::DragBegin(PaintWindow *w, unsigned y) noexcept
{
  // Make sure that we are not dragging already
  assert(!dragging);

  // Save the offset of the drag
  drag_offset = y - rc_slider.top;
  // ... and remember that we are dragging now
  dragging = true;
  w->SetCapture();
  w->Invalidate(rc_slider);
}

void
ScrollBar::DragBeginCentred(PaintWindow *w) noexcept
{
  // Make sure that we are not dragging already
  assert(!dragging);

  // Pick the slider up by its middle
  drag_offset = GetSliderHeight() / 2;
  // ... and remember that we are dragging now
  dragging = true;
  w->SetCapture();
  w->Invalidate(rc_slider);
}

void
ScrollBar::DragEnd(PaintWindow *w) noexcept
{
  // If we are not dragging right now -> nothing to end
  if (!dragging)
    return;

  // Realize that we are not dragging anymore
  dragging = false;
  w->ReleaseCapture();
  w->Invalidate(rc_slider);
}

unsigned
ScrollBar::DragMove(unsigned size, unsigned view_size, int y) const noexcept
{
  assert(dragging);

  return ToOrigin(size, view_size, y - drag_offset);
}
