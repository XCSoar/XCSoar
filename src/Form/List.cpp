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

#include "Form/List.hpp"
#include "Form/Internal.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Key.h"
#include "Screen/Point.hpp"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/Globals.hpp"
#elif defined(USE_GDI)
#include "Screen/WindowCanvas.hpp"
#endif

#include <assert.h>

#include <algorithm>

using std::min;
using std::max;

WndListFrame::WndListFrame(ContainerWindow &parent, const DialogLook &_look,
                           PixelScalar X, PixelScalar Y,
                           UPixelScalar Width, UPixelScalar Height,
                           const WindowStyle style,
                           UPixelScalar _item_height)
  :look(_look),
  item_height(_item_height),
  length(0), origin(0), items_visible(Height / item_height),
  cursor(0),
  dragging(false),
  ActivateCallback(NULL),
  CursorCallback(NULL),
  PaintItemCallback(NULL)
{
  set(parent, X, Y, Width, Height, style);
}

void
WndListFrame::show_or_hide_scroll_bar()
{
  const PixelSize size = get_size();

  if (length > items_visible)
    // enable the scroll bar
    scroll_bar.set(size);
  else
    // all items are visible
    // -> hide the scroll bar
    scroll_bar.reset();
}

bool
WndListFrame::on_resize(UPixelScalar width, UPixelScalar height)
{
  PaintWindow::on_resize(width, height);
  items_visible = height / item_height;
  show_or_hide_scroll_bar();
  return true;
}

bool
WndListFrame::on_setfocus()
{
  PaintWindow::on_setfocus();
  invalidate_item(cursor);
  return true;
}

bool
WndListFrame::on_killfocus()
{
  PaintWindow::on_killfocus();
  invalidate_item(cursor);
  return true;
}

void
WndListFrame::DrawItems(Canvas &canvas, unsigned start, unsigned end) const
{
  PixelRect rc = item_rect(start);

  canvas.set_text_color(look.list.text_color);
  canvas.set_background_color(look.list.background_color);
  canvas.background_transparent();
  canvas.select(*look.list.font);

#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLScissor scissor(OpenGL::translate_x,
                    OpenGL::screen_height - OpenGL::translate_y - canvas.get_height() - 1,
                    scroll_bar.get_left(get_size()), canvas.get_height());
#endif

  unsigned last_item = min(length, end);

  for (unsigned i = start; i < last_item; i++) {
    if (i == cursor)
      canvas.fill_rectangle(rc, look.list.selected.background_color);
    else
      canvas.fill_rectangle(rc, look.list.background_color);

    PaintItemCallback(canvas, rc, i);

    if (has_focus() && i == cursor)
      canvas.draw_focus(rc);

    ::OffsetRect(&rc, 0, rc.bottom - rc.top);
  }

  /* paint the bottom part below the last item */
  rc.bottom = canvas.get_height();
  if (rc.bottom > rc.top)
    canvas.fill_rectangle(rc, look.list.background_color);
}

void
WndListFrame::on_paint(Canvas &canvas)
{
  if (PaintItemCallback != NULL)
    DrawItems(canvas, origin, origin + items_visible + 1);

  DrawScrollBar(canvas);
}

void
WndListFrame::on_paint(Canvas &canvas, const PixelRect &dirty)
{
  if (PaintItemCallback != NULL)
    DrawItems(canvas, origin + dirty.top / item_height,
              origin + (dirty.bottom + item_height - 1) / item_height);

  DrawScrollBar(canvas);
}

void WndListFrame::DrawScrollBar(Canvas &canvas) {
  if (!scroll_bar.defined())
    return;

  scroll_bar.set_slider(length, items_visible, origin);
  scroll_bar.paint(canvas);
}

void
WndListFrame::SetItemHeight(UPixelScalar _item_height)
{
  item_height = _item_height;
  items_visible = get_size().cy / item_height;

  show_or_hide_scroll_bar();
  invalidate();
}

void
WndListFrame::SetLength(unsigned n)
{
  if (n == length)
    return;

  unsigned cursor = GetCursorIndex();

  length = n;

  if (n == 0)
    cursor = 0;
  else if (cursor >= n)
    cursor = n - 1;

  items_visible = get_size().cy / item_height;

  if (n <= items_visible)
    origin = 0;
  else if (origin + items_visible > n)
    origin = n - items_visible;
  else if (cursor < origin)
    origin = cursor;

  show_or_hide_scroll_bar();
  invalidate();

  SetCursorIndex(cursor);
}

void
WndListFrame::EnsureVisible(unsigned i)
{
  assert(i < length);

  if (origin > i)
    SetOrigin(i);
  else if (origin + items_visible <= i)
    SetOrigin(i - items_visible + 1);
}

bool
WndListFrame::SetCursorIndex(unsigned i)
{
  if (i >= length)
    return false;

  if (i == GetCursorIndex())
    return true;

  EnsureVisible(i);

  invalidate_item(cursor);
  cursor = i;
  invalidate_item(cursor);

  if (CursorCallback != NULL)
    CursorCallback(GetCursorIndex());
  return true;
}

void
WndListFrame::SetOrigin(int i)
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

#ifdef USE_GDI
  if ((unsigned)abs(delta) < items_visible) {
    PixelRect rc = get_client_rect();
    rc.right = scroll_bar.get_left(get_size());
    scroll(0, delta * item_height, rc);

    /* repaint the scrollbar synchronously; we could invalidate its
       area and repaint asynchronously via WM_PAINT, but then the clip
       rect passed to on_paint() would be the whole client area */
    WindowCanvas canvas(*this);
    DrawScrollBar(canvas);
    return;
  }
#endif

  invalidate();
}

bool
WndListFrame::on_key_check(unsigned key_code) const
{
  switch (key_code) {
  case VK_RETURN:
    return ActivateCallback != NULL;

  case VK_UP:
  case VK_LEFT:
    if (!has_pointer() ^ (key_code == VK_LEFT)) {
      if (origin == 0 || length <= items_visible)
        return false;
      return true;
    } else {
      return GetCursorIndex() > 0;
    }

  case VK_DOWN:
  case VK_RIGHT:
    if (!has_pointer() ^ (key_code == VK_RIGHT)) {
      if (cursor >= length || length <= items_visible)
        return false;
      return true;
    } else {
      return GetCursorIndex() + 1 < length;
    }

  default:
    return false;
  }
}

bool
WndListFrame::on_key_down(unsigned key_code)
{
  scroll_bar.drag_end(this);

  switch (key_code) {
#ifdef GNAV
  // JMW added this to make data entry easier
  case VK_F4:
#endif
  case VK_RETURN:
    if (ActivateCallback == NULL)
      break;

    if (GetCursorIndex() < GetLength())
      ActivateCallback(GetCursorIndex());
    return true;

  case VK_UP:
  case VK_LEFT:
    if (!has_pointer() ^ (key_code == VK_LEFT)) {
      // page up
      if (origin == 0 || length <= items_visible)
        break;
      
      SetOrigin(origin - items_visible);
      SetCursorIndex(cursor >= items_visible ? cursor - items_visible : 0);
      return true;
    } else {
      // previous item
      if (GetCursorIndex() <= 0)
        break;
      
      SetCursorIndex(GetCursorIndex() - 1);
      return true;
    }

  case VK_DOWN:
  case VK_RIGHT:
    if (!has_pointer() ^ (key_code == VK_RIGHT)) {
      // page down
      if (cursor >= length || length <= items_visible)
        break;
      
      SetOrigin(origin + items_visible);
      SetCursorIndex(cursor + items_visible < length ?
                     cursor + items_visible : length - 1);
      return true;
    } else {
      // next item
      if (GetCursorIndex() +1 >= length)
        break;
      
      SetCursorIndex(GetCursorIndex() + 1);
      return true;
    }

  case VK_HOME:
    SetCursorIndex(0);
    return true;

  case VK_END:
    if (length > 0) {
      SetCursorIndex(length - 1);
    }
    return true;

  case VK_PRIOR:
    if (origin > 0) {
      SetOrigin(origin - items_visible);
      SetCursorIndex(origin);
    }
    return true;

  case VK_NEXT:
    if (origin + items_visible < length) {
      SetOrigin(origin + items_visible);
      SetCursorIndex(std::min(length - 1, origin + items_visible));
    }
    return true;
  }
  return PaintWindow::on_key_down(key_code);
}

bool
WndListFrame::on_mouse_up(PixelScalar x, PixelScalar y)
{
  if (scroll_bar.is_dragging()) {
    scroll_bar.drag_end(this);
    return true;
  } else if (dragging) {
    drag_end();
    return true;
  } else
    return PaintWindow::on_mouse_up(x, y);
}

void
WndListFrame::drag_end()
{
  if (dragging) {
    dragging = false;
    release_capture();
  }
}

bool
WndListFrame::on_mouse_move(PixelScalar x, PixelScalar y, unsigned keys)
{
  // If we are currently dragging the ScrollBar slider
  if (scroll_bar.is_dragging()) {
    // -> Update ListBox origin
    SetOrigin(scroll_bar.drag_move(length, items_visible, y));
    return true;
  } else if (dragging) {
    int new_origin = drag_line - y / (int)item_height;
    SetOrigin(new_origin);
    return true;
  }

  return PaintWindow::on_mouse_move(x, y, keys);
}

bool
WndListFrame::on_mouse_down(PixelScalar x, PixelScalar y)
{
  // End any previous drag
  scroll_bar.drag_end(this);
  drag_end();

  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  // If possible -> Give focus to the Control
  bool had_focus = has_focus();
  if (!had_focus)
    set_focus();

  if (scroll_bar.in_slider(Pos)) {
    // if click is on scrollbar handle
    // -> start mouse drag
    scroll_bar.drag_begin(this, Pos.y);
  } else if (scroll_bar.in(Pos)) {
    // if click in scroll bar up/down/pgup/pgdn
    if (scroll_bar.in_up_arrow(Pos.y))
      // up
      SetOrigin(origin - 1);
    else if (scroll_bar.in_down_arrow(Pos.y))
      // down
      SetOrigin(origin + 1);
    else if (scroll_bar.above_slider(Pos.y))
      // page up
      SetOrigin(origin - items_visible);
    else if (scroll_bar.below_slider(Pos.y))
      // page down
      SetOrigin(origin + items_visible);
  } else {
    // if click in ListBox area
    // -> select appropriate item

    int index = ItemIndexAt(y);
    // If mouse was clicked outside the list items -> cancel
    if (index < 0)
      return false;

    if (had_focus && ActivateCallback != NULL &&
        (unsigned)index == GetCursorIndex()) {
      // If item was already selected
      // -> call event handler
      ActivateCallback(index);
    } else {
      // If item was not selected before
      // -> select it
      SetCursorIndex(index);

      drag_line = origin + y / item_height;
      dragging = true;
      set_capture();
    }
  }

  return true;
}

bool
WndListFrame::on_mouse_wheel(PixelScalar x, PixelScalar y, int delta)
{
  scroll_bar.drag_end(this);
  drag_end();

  if (delta > 0) {
    // scroll up
    if (origin > 0)
      SetOrigin(origin - 1);
  } else if (delta < 0) {
    // scroll down
    if (origin + items_visible < length)
      SetOrigin(origin + 1);
  }

  return true;
}

bool
WndListFrame::on_cancel_mode()
{
  PaintWindow::on_cancel_mode();

  scroll_bar.drag_end(this);
  drag_end();

  return false;
}
