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
#include "Screen/Layout.hpp"

#include <assert.h>

#include <algorithm>

using std::min;
using std::max;

WndListFrame::WndListFrame(WindowControl *Owner, const TCHAR *Name,
                           int X, int Y, int Width, int Height,
                           unsigned _item_height):
  WindowControl(Owner, NULL, Name, X, Y, Width, Height),
  item_height(_item_height),
  length(0), origin(0), items_visible(Height / item_height),
  relative_cursor(0),
  ActivateCallback(NULL),
  CursorCallback(NULL),
  PaintItemCallback(NULL)
{
  SetCanFocus(true);
  PaintSelector(true);

  mCaption[0] = '\0';
  SetForeColor(GetOwner()->GetForeColor());
  SetBackColor(GetOwner()->GetBackColor());
}

void
WndListFrame::show_or_hide_scroll_bar()
{
  const SIZE size = get_size();

  if (length > items_visible)
    /* enable the scroll bar */
    scroll_bar.set(size);
  else
    /* all items are visible - hide the scroll bar */
    scroll_bar.reset();
}

bool
WndListFrame::on_resize(unsigned width, unsigned height)
{
  WindowControl::on_resize(width, height);
  show_or_hide_scroll_bar();
  return true;
}

void
WndListFrame::on_paint(Canvas &canvas)
{
  WindowControl::on_paint(canvas);

  if (PaintItemCallback != NULL) {
    // paint using the PaintItemCallback
    RECT rc;
    rc.left = rc.top = Layout::FastScale(2);
    rc.right = scroll_bar.get_left(get_size()) - rc.left;
    rc.bottom = rc.top + item_height;

    canvas.set_text_color(GetForeColor());
    canvas.set_background_color(GetBackColor());
    canvas.background_transparent();
    canvas.select(*GetFont());

    for (unsigned i = 0; i < items_visible; i++) {
      if (GetFocused() && i == relative_cursor) {
        Brush brush(GetBackColor().highlight());
        canvas.fill_rectangle(rc, brush);
      }

      PaintItemCallback(canvas, rc, origin + i);

      if (i == relative_cursor)
        PaintSelector(canvas, rc);

      ::OffsetRect(&rc, 0, rc.bottom - rc.top);
    }
  }

  DrawScrollBar(canvas);
}

void WndListFrame::DrawScrollBar(Canvas &canvas) {
  if (!scroll_bar.defined())
    return;

  scroll_bar.set_button(length, items_visible, origin);
  scroll_bar.paint(canvas, GetForeColor());
}

void
WndListFrame::SetItemHeight(unsigned _item_height)
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
    origin = i;
  else if (origin + items_visible <= i)
    origin = i - items_visible + 1;
  else
    /* no change, no repaint required */
    return;

  invalidate();
}

bool
WndListFrame::SetCursorIndex(unsigned i)
{
  if (i >= length)
    return false;

  if (i == GetCursorIndex())
    return true;

  EnsureVisible(i);

  relative_cursor = i - origin;
  invalidate();

  if (CursorCallback != NULL)
    CursorCallback(GetCursorIndex());
  return true;
}

void
WndListFrame::SetOrigin(unsigned i)
{
  if (length <= items_visible)
    return;

  if (i + items_visible > length)
    i = length - items_visible;

  if (i == origin)
    return;

  origin = i;

  invalidate();

  if (CursorCallback != NULL)
    CursorCallback(GetCursorIndex());
}

bool
WndListFrame::on_key_down(unsigned key_code)
{
  // XXX SetSourceRectangle(mRc);

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

    //#ifndef GNAV

  case VK_LEFT:
    if (origin == 0 || length <= items_visible)
      break;

    SetOrigin(origin > items_visible
              ? origin - items_visible
              : 0);
    return true;

  case VK_RIGHT:
    if (origin + relative_cursor >= length ||
        length <= items_visible)
      break;

    SetOrigin(origin + items_visible);
    return true;

    //#endif
  case VK_DOWN:
    if (GetCursorIndex() + 1 >= length)
      break;

    SetCursorIndex(GetCursorIndex() + 1);
    return true;

  case VK_UP:
    if (GetCursorIndex() <= 0)
      break;

    SetCursorIndex(GetCursorIndex() - 1);
    return true;
  }

  return WindowControl::on_key_down(key_code);
}

bool
WndListFrame::on_mouse_up(int x, int y)
{
  scroll_bar.drag_end(this);
    return false;
}

void
WndListFrame::SelectItemFromScreen(int xPos, int yPos)
{
  (void)xPos;

  int index = yPos / item_height; // yPos is offset within ListEntry item!

  if (index >= 0 && index + relative_cursor < length) {
    if ((unsigned)index == relative_cursor) {
      if (ActivateCallback != NULL)
        ActivateCallback(GetCursorIndex());

      invalidate();
    } else {
      SetCursorIndex(origin + index);
    }
  }
}


bool
WndListFrame::on_mouse_move(int x, int y, unsigned keys)
{
  static bool bMoving = false;

  if (!bMoving)
  {
    bMoving=true;

    if (scroll_bar.is_dragging()) {
      SetOrigin(scroll_bar.drag_move(length, items_visible, y));
    }

    bMoving=false;
  } // Tickcount
  return false;
}

bool
WndListFrame::on_mouse_down(int x, int y)
{
  scroll_bar.drag_end(this);

  POINT Pos;
  Pos.x = x;
  Pos.y = y;

  if (!GetFocused())
    set_focus();

  if (scroll_bar.in_button(Pos)) // see if click is on scrollbar handle
  {
    // start mouse drag
    scroll_bar.drag_begin(this, Pos.y);
  }
  else if (scroll_bar.in(Pos)) // clicked in scroll bar up/down/pgup/pgdn
  {
    if (scroll_bar.in_up_arrow(Pos.y))
      origin = max(0U, origin - 1);
    else if (scroll_bar.in_down_arrow(Pos.y))
      origin = max(0U, min(length - items_visible, origin + 1));
    else if (scroll_bar.above_button(Pos.y)) // page up
      origin = max(0U, origin - items_visible);
    else if (scroll_bar.below_button(Pos.y)) // page up
      if (length > origin + items_visible)
          origin = min(length - items_visible,
                       origin + items_visible);

    invalidate();
  }
  else
  {
    SelectItemFromScreen(x, y);
  }

  return false;
}

bool
WndListFrame::on_mouse_wheel(int delta)
{
  if (delta > 0) {
    // scroll up
    if (origin > 0) {
      --origin;
      invalidate();
    }
  } else if (delta < 0) {
    // scroll down
    if (origin + items_visible < length) {
      ++origin;
      invalidate();
    }
  }

  return true;
}
