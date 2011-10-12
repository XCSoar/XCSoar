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

#ifndef XCSOAR_FORM_LIST_HPP
#define XCSOAR_FORM_LIST_HPP

#include "Screen/PaintWindow.hpp"
#include "Form/ScrollBar.hpp"
#include "Compiler.h"

struct DialogLook;
class ContainerWindow;

/**
 * A WndListFrame implements a scrollable list control based on the
 * WindowControl class.
 */
class WndListFrame : public PaintWindow {
public:
  typedef void (*ActivateCallback_t)(unsigned idx);
  typedef void (*CursorCallback_t)(unsigned idx);
  typedef void (*PaintItemCallback_t)(Canvas &canvas, const PixelRect rc,
                                      unsigned idx);

protected:
  const DialogLook &look;

  /** The ScrollBar object */
  ScrollBar scroll_bar;

  /** The height of one item on the screen, in pixels. */
  UPixelScalar item_height;
  /** The number of items in the list. */
  unsigned length;
  /** The index of the topmost item currently being displayed. */
  unsigned origin;
  /** The number of items visible at a time. */
  unsigned items_visible;
  /** The index of the selected item on the screen. */
  unsigned cursor;

  bool dragging;
  int drag_line;

  ActivateCallback_t ActivateCallback;
  CursorCallback_t CursorCallback;
  PaintItemCallback_t PaintItemCallback;

public:
  /**
   * Constructor of the WndListFrame class
   * @param parent the parent window
   * @param X x-Coordinate of the ListFrameControl
   * @param Y y-Coordinate of the ListFrameControl
   * @param Width Width of the ListFrameControl
   * @param Height Height of the ListFrameControl
   * @param _item_height Height of an item of the ListFrameControl
   */
  WndListFrame(ContainerWindow &parent, const DialogLook &look,
               PixelScalar x, PixelScalar y,
               UPixelScalar Width, UPixelScalar Height,
               const WindowStyle style,
               UPixelScalar _item_height);

  /** Sets the function to call when a ListItem is chosen */
  void SetActivateCallback(ActivateCallback_t cb) {
    ActivateCallback = cb;
  }

  /** Sets the function to call when cursor has changed */
  void SetCursorCallback(CursorCallback_t cb) {
    CursorCallback = cb;
  }

  /** Sets the function to call when painting an item */
  void SetPaintItemCallback(PaintItemCallback_t cb) {
    PaintItemCallback = cb;
  }

  void SetItemHeight(UPixelScalar _item_height);

  /**
   * Returns the number of items in the list
   * @return The number of items in the list
   */
  unsigned GetLength() const {
    return length;
  }

  /** Changes the number of items in the list. */
  void SetLength(unsigned n);

  /**
   * Returns the current cursor position
   * @return The current cursor position
   */
  unsigned GetCursorIndex() const {
    return cursor;
  }

  /**
   * Moves the cursor to the specified position.
   *
   * @return true if the cursor was moved to the specified position,
   * false if the position was invalid
   */
  bool SetCursorIndex(unsigned i);

  /**
   * Move the cursor this many items up (negative delta) or down
   * (positive delta).  Scrolls if necessary.
   */
  void MoveCursor(int delta);

  /**
   * Scrolls to the specified index.
   */
  void SetOrigin(int i);

  /**
   * Scrolls a number of items up (negative delta) or down (positive
   * delta).  The cursor is not moved.
   */
  void MoveOrigin(int delta);

protected:
  /** Checks whether a ScrollBar is needed and shows/hides it */
  void show_or_hide_scroll_bar();

  /**
   * Scroll to the ListItem defined by i
   * @param i The ListItem array id
   */
  void EnsureVisible(unsigned i);

  /**
   * Determine which list item resides at the specified pixel row.
   * Returns -1 if there is no such list item.
   */
  gcc_pure
  int ItemIndexAt(int y) const {
    int i = y / item_height + origin;
    return i >= 0 && (unsigned)i < length ? i : -1;
  }

  gcc_const
  PixelRect item_rect(unsigned i) const {
    PixelRect rc;
    rc.left = 0;
    rc.top = (int)(i - origin) * item_height;
    rc.right = scroll_bar.get_left(get_size());
    rc.bottom = rc.top + item_height;
    return rc;
  }

  void invalidate_item(unsigned i) {
    invalidate(item_rect(i));
  }

  void drag_end();

  void DrawItems(Canvas &canvas, unsigned start, unsigned end) const;

  /** Draws the ScrollBar */
  void DrawScrollBar(Canvas &canvas);

  /**
   * The on_resize event is called when the Control is resized
   * (derived from Window)
   */
  virtual bool on_resize(UPixelScalar width, UPixelScalar height);

  virtual bool on_setfocus();
  virtual bool on_killfocus();

  /**
   * The on_mouse_down event is called when the mouse is pressed over the button
   * (derived from Window)
   */
  virtual bool on_mouse_down(PixelScalar x, PixelScalar y);
  /**
   * The on_mouse_up event is called when the mouse is released over the button
   * (derived from Window)
   */
  virtual bool on_mouse_up(PixelScalar x, PixelScalar y);
  /**
   * The on_mouse_move event is called when the mouse is moved over the button
   * (derived from Window)
   */
  virtual bool on_mouse_move(PixelScalar x, PixelScalar y, unsigned keys);
  /**
   * The on_mouse_wheel event is called when the mouse wheel is turned
   * (derived from Window)
   */
  virtual bool on_mouse_wheel(PixelScalar x, PixelScalar y, int delta);

  virtual bool on_key_check(unsigned key_code) const;

  /**
   * The on_key_down event is called when a key is pressed while the
   * button is focused
   * (derived from Window)
   */
  virtual bool on_key_down(unsigned key_code);

  virtual bool on_cancel_mode();

  /**
   * The on_paint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  virtual void on_paint(Canvas &canvas);
  virtual void on_paint(Canvas &canvas, const PixelRect &dirty);
};

#endif
