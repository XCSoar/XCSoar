// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ScrollBar.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "ui/window/PaintWindow.hpp"
#include "Asset.hpp"
#include "Look/ButtonLook.hpp"
#include "util/Macros.hpp"

#include <cassert>

ScrollBar::ScrollBar(const ButtonLook &_button_look) noexcept
  :button_renderer(_button_look), dragging(false),
   arrow(Arrow::NONE), arrow_pressed(false)
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
  const bool up_pressed = arrow_pressed && arrow == Arrow::UP;
  const bool down_pressed = arrow_pressed && arrow == Arrow::DOWN;
  const ButtonState up_state = up_pressed
    ? ButtonState::PRESSED : ButtonState::ENABLED;
  const ButtonState down_state = down_pressed
    ? ButtonState::PRESSED : ButtonState::ENABLED;

  // draw rectangle around entire scrollbar area
  canvas.SelectBlackPen();
  canvas.SelectHollowBrush();
  canvas.DrawRectangle(rc);

  // draw the up/down arrow buttons
  const int arrow_padding = std::max(GetWidth() / 4, 4);

  PixelRect up_arrow_rect = rc;
  ++up_arrow_rect.left;
  up_arrow_rect.bottom = up_arrow_rect.top + GetWidth();

  PixelRect down_arrow_rect = rc;
  ++down_arrow_rect.left;
  down_arrow_rect.top = down_arrow_rect.bottom - GetWidth();

  canvas.DrawExactLine(up_arrow_rect.GetBottomLeft(),
                       up_arrow_rect.GetBottomRight());
  canvas.DrawExactLine({down_arrow_rect.left, down_arrow_rect.top - 1},
                       {down_arrow_rect.right, down_arrow_rect.top - 1});

  button_renderer.DrawButton(canvas, up_arrow_rect, up_state);
  button_renderer.DrawButton(canvas, down_arrow_rect, down_state);

  const ButtonLook &look = button_renderer.GetLook();
  canvas.SelectNullPen();

  const auto draw_arrow = [&](const PixelRect &arrow_rc, bool up,
                              bool pressed) {
    const int tip_y = up
      ? arrow_rc.top + arrow_padding
      : arrow_rc.bottom - arrow_padding;
    const int base_y = up
      ? arrow_rc.bottom - arrow_padding
      : arrow_rc.top + arrow_padding;
    const BulkPixelPoint tri[3] = {
      { (arrow_rc.left + rc.right) / 2, tip_y },
      { arrow_rc.left + arrow_padding, base_y },
      { rc.right - arrow_padding, base_y },
    };
    canvas.Select(pressed
                  ? look.focused.foreground_brush
                  : look.standard.foreground_brush);
    canvas.DrawTriangleFan(tri, ARRAY_SIZE(tri));
  };

  draw_arrow(up_arrow_rect, true, up_pressed);
  draw_arrow(down_arrow_rect, false, down_pressed);

  if (rc_slider.top + 4 < rc_slider.bottom) {
    canvas.SelectBlackPen();
    canvas.DrawExactLine(rc_slider.GetTopLeft(), rc_slider.GetTopRight());
    canvas.DrawExactLine(rc_slider.GetBottomLeft(),
                         rc_slider.GetBottomRight());

    PixelRect rc_slider2 = rc_slider;
    ++rc_slider2.left;
    ++rc_slider2.top;
    button_renderer.DrawButton(canvas, rc_slider2,
                               dragging ? ButtonState::PRESSED : ButtonState::ENABLED);
  }

  // fill the rest with darker gray
  const Color background_color = IsDithered() ? COLOR_BLACK : COLOR_GRAY;

  if (up_arrow_rect.bottom + 1 < rc_slider.top)
    canvas.DrawFilledRectangle({rc.left + 1, up_arrow_rect.bottom + 1, rc.right, rc_slider.top},
                               background_color);

  if (rc_slider.bottom + 1 < down_arrow_rect.top - 1)
    canvas.DrawFilledRectangle({rc.left + 1, rc_slider.bottom + 1, rc.right, down_arrow_rect.top - 1},
                               background_color);
}

bool
ScrollBar::PointerOnArrow(PixelPoint p) const noexcept
{
  if (arrow == Arrow::NONE || !IsInside(p))
    return false;

  return arrow == Arrow::UP
    ? IsInsideUpArrow(p.y)
    : IsInsideDownArrow(p.y);
}

void
ScrollBar::StartDrag(PaintWindow *w, int offset) noexcept
{
  assert(!dragging);

  drag_offset = offset;
  dragging = true;
  w->SetCapture();
  w->Invalidate(rc_slider);
}

void
ScrollBar::DragBegin(PaintWindow *w, unsigned y) noexcept
{
  StartDrag(w, y - rc_slider.top);
}

void
ScrollBar::DragBeginCentred(PaintWindow *w) noexcept
{
  StartDrag(w, GetSliderHeight() / 2);
}

bool
ScrollBar::ArrowDown(PaintWindow *w, PixelPoint p) noexcept
{
  if (!IsInside(p))
    return false;

  const bool up = IsInsideUpArrow(p.y);
  if (!up && !IsInsideDownArrow(p.y))
    return false;

  assert(!dragging);
  assert(arrow == Arrow::NONE);

  arrow = up ? Arrow::UP : Arrow::DOWN;
  arrow_pressed = true;
  w->SetCapture();
  w->Invalidate(rc);
  return true;
}

bool
ScrollBar::ArrowMove(PaintWindow *w, PixelPoint p) noexcept
{
  if (arrow == Arrow::NONE)
    return false;

  const bool inside = PointerOnArrow(p);
  if (inside != arrow_pressed) {
    arrow_pressed = inside;
    w->Invalidate(rc);
  }
  return true;
}

bool
ScrollBar::ArrowUp(PaintWindow *w, PixelPoint p) noexcept
{
  if (arrow == Arrow::NONE)
    return false;

  const bool fire = PointerOnArrow(p);
  DragEnd(w);
  return fire;
}

void
ScrollBar::DragEnd(PaintWindow *w) noexcept
{
  if (arrow != Arrow::NONE) {
    arrow = Arrow::NONE;
    arrow_pressed = false;
    w->ReleaseCapture();
    w->Invalidate(rc);
    return;
  }

  if (!dragging)
    return;

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
