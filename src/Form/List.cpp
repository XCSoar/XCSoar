/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Form/List.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Canvas.hpp"
#include "Event/KeyCode.hpp"
#include "Screen/Point.hpp"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#elif defined(USE_GDI)
#include "Screen/WindowCanvas.hpp"
#endif

#include <assert.h>

#include <algorithm>

/**
 * Can the user scroll with pixel precision?  This is used on fast
 * displays to give more instant feedback, which feels more slick.  On
 * slow e-paper screens, this is not a good idea.
 */
gcc_const
static bool
UsePixelPan()
{
  return !HasEPaper();
}

ListControl::ListControl(const DialogLook &_look)
  :look(_look),
   scroll_bar(look.button),
   length(0),
   origin(0), pixel_pan(0),
   cursor(0),
   drag_mode(DragMode::NONE),
   item_renderer(nullptr), cursor_handler(nullptr),
   kinetic_timer(*this)
{
}

ListControl::ListControl(ContainerWindow &parent, const DialogLook &_look,
                         PixelRect rc, const WindowStyle style,
                         unsigned _item_height)
  :look(_look),
   scroll_bar(look.button),
   length(0),
   origin(0), pixel_pan(0),
   cursor(0),
   drag_mode(DragMode::NONE),
   item_renderer(nullptr), cursor_handler(nullptr),
   kinetic_timer(*this)
{
  Create(parent, rc, style, _item_height);
}

ListControl::~ListControl() {
  /* we must override ~Window(), because in ~Window(), our own
     OnDestroy() method won't be called (during object destruction,
     this object loses its identity) */
  Destroy();
}

void
ListControl::Create(ContainerWindow &parent,
                    PixelRect rc, const WindowStyle style,
                    unsigned _item_height)
{
  item_height = _item_height;
  PaintWindow::Create(parent, rc, style);
}

bool
ListControl::CanActivateItem() const
{
  if (IsEmpty())
    return false;

  return cursor_handler != nullptr &&
    cursor_handler->CanActivateItem(GetCursorIndex());
}

void
ListControl::ActivateItem()
{
  assert(CanActivateItem());

  unsigned index = GetCursorIndex();
  assert(index < GetLength());
  if (cursor_handler != nullptr)
    cursor_handler->OnActivateItem(index);
}

void
ListControl::show_or_hide_scroll_bar()
{
  const PixelSize size = GetSize();

  if (length > items_visible)
    // enable the scroll bar
    scroll_bar.SetSize(size);
  else
    // all items are visible
    // -> hide the scroll bar
    scroll_bar.Reset();
}

void
ListControl::OnResize(PixelSize new_size)
{
  PaintWindow::OnResize(new_size);

  items_visible = new_size.cy / item_height;

  if (unsigned(new_size.cy) >= length * item_height) {
    /* after the resize, there is enough room for all list items -
       scroll back to the top */
    origin = pixel_pan = 0;
  }

  if (length > 0)
    /* make sure the cursor is still visible */
    EnsureVisible(GetCursorIndex());

  show_or_hide_scroll_bar();
}

void
ListControl::OnSetFocus()
{
  PaintWindow::OnSetFocus();
  Invalidate_item(cursor);
}

void
ListControl::OnKillFocus()
{
  PaintWindow::OnKillFocus();
  Invalidate_item(cursor);
}

void
ListControl::DrawItems(Canvas &canvas, unsigned start, unsigned end) const
{
  PixelRect rc = item_rect(start);

  canvas.SetBackgroundColor(look.list.background_color);
  canvas.SetBackgroundTransparent();
  canvas.Select(*look.list.font);

#ifdef ENABLE_OPENGL
  /* enable clipping */
  const PixelRect scissor_rc(0, 0, scroll_bar.GetLeft(GetSize()),
                             canvas.GetHeight());
  GLCanvasScissor scissor(scissor_rc);
#endif

  unsigned last_item = std::min(length, end);

  const bool focused = !HasCursorKeys() || HasFocus();

  for (unsigned i = start; i < last_item; i++) {
    const bool selected = i == cursor;
    const bool pressed = selected && drag_mode == DragMode::CURSOR;

    canvas.DrawFilledRectangle(rc,
                               look.list.GetBackgroundColor(selected,
                                                            focused,
                                                            pressed));

    canvas.SetTextColor(look.list.GetTextColor(selected, focused, pressed));

    if (item_renderer != nullptr)
      item_renderer->OnPaintItem(canvas, rc, i);

    if (focused && selected)
      canvas.DrawFocusRectangle(rc);

    rc.Offset(0, rc.GetHeight());
  }

  /* paint the bottom part below the last item */
  rc.bottom = canvas.GetHeight();
  if (rc.bottom > rc.top)
    canvas.DrawFilledRectangle(rc, look.list.background_color);
}

void
ListControl::OnPaint(Canvas &canvas)
{
  if (item_renderer != nullptr)
    DrawItems(canvas, origin, origin + items_visible + 2);

  DrawScrollBar(canvas);
}

void
ListControl::OnPaint(Canvas &canvas, const PixelRect &dirty)
{
  if (item_renderer != nullptr)
    DrawItems(canvas, origin + (dirty.top + pixel_pan) / item_height,
              origin + (dirty.bottom + pixel_pan + item_height - 1) / item_height);

  DrawScrollBar(canvas);
}

void
ListControl::DrawScrollBar(Canvas &canvas) {
  if (!scroll_bar.IsDefined())
    return;

  if (UsePixelPan())
    scroll_bar.SetSlider(length * item_height, GetHeight(), GetPixelOrigin());
  else
    scroll_bar.SetSlider(length, items_visible, origin);

  scroll_bar.Paint(canvas);
}

void
ListControl::SetItemHeight(unsigned _item_height)
{
  item_height = _item_height;
  items_visible = GetHeight() / item_height;

  show_or_hide_scroll_bar();
  Invalidate();
}

void
ListControl::SetLength(unsigned n)
{
  if (n == length)
    return;

  unsigned cursor = GetCursorIndex();

  length = n;

  if (n == 0)
    cursor = 0;
  else if (cursor >= n)
    cursor = n - 1;

  items_visible = GetHeight() / item_height;

  if (n <= items_visible)
    origin = 0;
  else if (origin + items_visible > n)
    origin = n - items_visible;
  else if (cursor < origin)
    origin = cursor;

  show_or_hide_scroll_bar();
  Invalidate();

  SetCursorIndex(cursor);
}

void
ListControl::EnsureVisible(unsigned i)
{
  assert(i < length);

  if (origin > i || (origin == i && pixel_pan > 0)) {
    SetOrigin(i);
    SetPixelPan(0);
  } else if (origin + items_visible <= i) {
    if (UsePixelPan()) {
      SetOrigin(i - items_visible);

      if (origin > 0 || i >= items_visible)
        SetPixelPan(((items_visible + 1) * item_height - GetHeight()) % item_height);
    } else {
      /* no pixel panning on e-paper screens to avoid tearing */
      SetOrigin(i + 1 - items_visible);
    }
  }
}

bool
ListControl::SetCursorIndex(unsigned i)
{
  if (i >= length)
    return false;

  if (i == GetCursorIndex())
    return true;

  EnsureVisible(i);

  Invalidate_item(cursor);
  cursor = i;
  Invalidate_item(cursor);

  if (cursor_handler != nullptr)
    cursor_handler->OnCursorMoved(i);
  return true;
}

void
ListControl::MoveCursor(int delta)
{
  if (length == 0)
    return;

  int new_cursor = cursor + delta;
  if (new_cursor < 0)
    new_cursor = 0;
  else if ((unsigned)new_cursor >= length)
    new_cursor = length - 1;

  SetCursorIndex(new_cursor);
}

void
ListControl::SetPixelPan(unsigned _pixel_pan)
{
  if (pixel_pan == _pixel_pan)
    return;

  pixel_pan = _pixel_pan;
  Invalidate();
}

void
ListControl::SetOrigin(int i)
{
  if (length <= items_visible)
    return;

  if (i < 0)
    i = 0;
  else if ((unsigned)i + items_visible > length)
    i = length - items_visible;

  if ((unsigned)i == origin)
    return;

#ifdef USE_GDI
  int delta = origin - i;
#endif

  origin = i;

#ifdef USE_WINUSER
  if ((unsigned)abs(delta) < items_visible) {
    PixelRect rc = GetClientRect();
    rc.right = scroll_bar.GetLeft(GetSize());
    Scroll(0, delta * item_height, rc);

    /* repaint the scrollbar synchronously; we could Invalidate its
       area and repaint asynchronously via WM_PAINT, but then the clip
       rect passed to OnPaint() would be the whole client area */
    WindowCanvas canvas(*this);
    DrawScrollBar(canvas);
    return;
  }
#endif

  Invalidate();
}

void
ListControl::SetPixelOrigin(int pixel_origin)
{
  int max = length * item_height - GetHeight();
  if (pixel_origin > max)
    pixel_origin = max;

  if (pixel_origin < 0)
    pixel_origin = 0;

  SetOrigin(pixel_origin / item_height);

  /* no pixel panning on e-paper screens to avoid tearing */
  SetPixelPan(UsePixelPan() ? pixel_origin % item_height : 0);
}

void
ListControl::MoveOrigin(int delta)
{
  if (UsePixelPan()) {
    int pixel_origin = (int)GetPixelOrigin();
    SetPixelOrigin(pixel_origin + delta * (int)item_height);
  } else {
    SetOrigin(origin + delta);
  }
}

bool
ListControl::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {
  case KEY_RETURN:
    return CanActivateItem();

  case KEY_UP:
    return GetCursorIndex() > 0;

  case KEY_DOWN:
    return GetCursorIndex() + 1 < length;

  default:
    return false;
  }
}

bool
ListControl::OnKeyDown(unsigned key_code)
{
  scroll_bar.DragEnd(this);
  kinetic_timer.Cancel();

  switch (key_code) {
  case KEY_RETURN:
    if (CanActivateItem())
      ActivateItem();
    return true;

  case KEY_UP:
    // previous item
    if (GetCursorIndex() <= 0)
      break;

    MoveCursor(-1);
    return true;

  case KEY_DOWN:
    // next item
    if (GetCursorIndex() +1 >= length)
      break;

    MoveCursor(1);
    return true;

  case KEY_LEFT:
    // page up
    MoveCursor(-(int)items_visible);
    return true;

  case KEY_RIGHT:
    // page down
    MoveCursor(items_visible);
    return true;

  case KEY_HOME:
    SetCursorIndex(0);
    return true;

  case KEY_END:
    if (length > 0) {
      SetCursorIndex(length - 1);
    }
    return true;

  case KEY_PRIOR:
    MoveCursor(-(int)items_visible);
    return true;

  case KEY_NEXT:
    MoveCursor(items_visible);
    return true;
  }
  return PaintWindow::OnKeyDown(key_code);
}

bool
ListControl::OnMouseUp(PixelPoint p)
{
  if (scroll_bar.IsDragging()) {
    scroll_bar.DragEnd(this);
    return true;
  }

  if (drag_mode == DragMode::CURSOR &&
      p.x >= 0 && p.x <= ((int)GetWidth() - scroll_bar.GetWidth())) {
    drag_end();
    ActivateItem();
    return true;
  }

  if (drag_mode == DragMode::SCROLL || drag_mode == DragMode::CURSOR) {
    const bool enable_kinetic = UsePixelPan() && drag_mode == DragMode::SCROLL;

    drag_end();

    if (enable_kinetic) {
      kinetic.MouseUp(GetPixelOrigin());
      kinetic_timer.Schedule(30);
    }

    return true;
  } else
    return PaintWindow::OnMouseUp(p);
}

void
ListControl::drag_end()
{
  if (drag_mode != DragMode::NONE) {
    if (drag_mode == DragMode::CURSOR)
      Invalidate_item(cursor);

    drag_mode = DragMode::NONE;
    ReleaseCapture();
  }
}

bool
ListControl::OnMouseMove(PixelPoint p, unsigned keys)
{
  // If we are currently dragging the ScrollBar slider
  if (scroll_bar.IsDragging()) {
    // -> Update ListBox origin
    if (UsePixelPan())
      SetPixelOrigin(scroll_bar.DragMove(length * item_height, GetHeight(),
                                         p.y));
    else
      SetOrigin(scroll_bar.DragMove(length, items_visible, p.y));

    return true;
  } else if (drag_mode == DragMode::CURSOR) {
    if (abs(p.y - drag_y_window) > ((int)item_height / 5)) {
      drag_mode = DragMode::SCROLL;
      Invalidate_item(cursor);
    } else
      return true;
  }

  if (drag_mode == DragMode::SCROLL) {
    int new_origin = drag_y - p.y;
    SetPixelOrigin(new_origin);
    if (UsePixelPan())
      kinetic.MouseMove(GetPixelOrigin());
    return true;
  }

  return PaintWindow::OnMouseMove(p, keys);
}

bool
ListControl::OnMouseDown(PixelPoint Pos)
{
  // End any previous drag
  scroll_bar.DragEnd(this);
  drag_end();

  kinetic_timer.Cancel();

  // If possible -> Give focus to the Control
  const bool had_focus = !HasCursorKeys() || HasFocus();
  if (!had_focus)
    SetFocus();

  if (scroll_bar.IsInsideSlider(Pos)) {
    // if click is on scrollbar handle
    // -> start mouse drag
    scroll_bar.DragBegin(this, Pos.y);
  } else if (scroll_bar.IsInside(Pos)) {
    // if click in scroll bar up/down/pgup/pgdn
    if (scroll_bar.IsInsideUpArrow(Pos.y))
      // up
      MoveOrigin(-1);
    else if (scroll_bar.IsInsideDownArrow(Pos.y))
      // down
      MoveOrigin(1);
    else if (scroll_bar.IsAboveSlider(Pos.y))
      // page up
      MoveOrigin(-(int)items_visible);
    else if (scroll_bar.IsBelowSlider(Pos.y))
      // page down
      MoveOrigin(items_visible);
  } else {
    // if click in ListBox area
    // -> select appropriate item

    int index = ItemIndexAt(Pos.y);
    // If mouse was clicked outside the list items -> cancel
    if (index < 0)
      return false;

    drag_y = GetPixelOrigin() + Pos.y;
    drag_y_window = Pos.y;

    if (had_focus && (unsigned)index == GetCursorIndex() &&
        CanActivateItem()) {
      drag_mode = DragMode::CURSOR;
      Invalidate_item(cursor);
    } else {
      // If item was not selected before
      // -> select it
      SetCursorIndex(index);
      drag_mode = DragMode::SCROLL;
    }
    if (UsePixelPan())
      kinetic.MouseDown(GetPixelOrigin());
    SetCapture();
  }

  return true;
}

bool
ListControl::OnMouseWheel(PixelPoint p, int delta)
{
  scroll_bar.DragEnd(this);
  drag_end();

  kinetic_timer.Cancel();

  if (delta > 0) {
    // scroll up
    MoveOrigin(-1);
  } else if (delta < 0) {
    // scroll down
    MoveOrigin(1);
  }

  return true;
}

void
ListControl::OnCancelMode()
{
  PaintWindow::OnCancelMode();

  scroll_bar.DragEnd(this);
  drag_end();

  kinetic_timer.Cancel();
}

bool
ListControl::OnTimer(WindowTimer &timer)
{
  if (timer == kinetic_timer) {
    assert(UsePixelPan());

    if (kinetic.IsSteady()) {
      kinetic_timer.Cancel();
    } else
      SetPixelOrigin(kinetic.GetPosition());

    return true;
  }

  return PaintWindow::OnTimer(timer);
}

void
ListControl::OnDestroy()
{
  kinetic_timer.Cancel();

  PaintWindow::OnDestroy();
}
