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

#ifndef XCSOAR_FORM_LIST_HPP
#define XCSOAR_FORM_LIST_HPP

#include "Screen/PaintWindow.hpp"
#include "Screen/Timer.hpp"
#include "Form/ScrollBar.hpp"
#include "UIUtil/KineticManager.hpp"
#include "Compiler.h"

struct DialogLook;
class ContainerWindow;

typedef void (*ListItemRendererFunction)(Canvas &canvas, const PixelRect rc,
                                         unsigned idx);

class ListItemRenderer {
public:
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) = 0;
};

template<typename C>
class LambdaListItemRenderer : public ListItemRenderer, private C {
public:
  LambdaListItemRenderer(C &&c):C(std::move(c)) {}

  void OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned idx) override {
    C::operator()(canvas, rc, idx);
  }
};

/**
 * Convert a lambda expression (a closure object) to ListItemRenderer.
 */
template<typename C>
LambdaListItemRenderer<C>
MakeListItemRenderer(C &&c)
{
  return LambdaListItemRenderer<C>(std::move(c));
}

class FunctionListItemRenderer : public ListItemRenderer {
  const ListItemRendererFunction function;

public:
  FunctionListItemRenderer(ListItemRendererFunction _function)
    :function(_function) {}

  void OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned idx) override {
    function(canvas, rc, idx);
  }
};

class ListCursorHandler {
public:
  virtual void OnCursorMoved(gcc_unused unsigned index) {}

  gcc_pure
  virtual bool CanActivateItem(gcc_unused unsigned index) const {
    return false;
  }

  virtual void OnActivateItem(gcc_unused unsigned index) {}
};

/**
 * A ListControl implements a scrollable list control based on the
 * WindowControl class.
 */
class ListControl : public PaintWindow {
protected:
  const DialogLook &look;

  /** The ScrollBar object */
  ScrollBar scroll_bar;

  /** The height of one item on the screen, in pixels. */
  unsigned item_height;
  /** The number of items in the list. */
  unsigned length;
  /** The index of the topmost item currently being displayed. */
  unsigned origin;

  /**
   * Which pixel row of the "origin" item is being displayed at the
   * top of the Window?
   */
  unsigned pixel_pan;

  /** The number of items visible at a time. */
  unsigned items_visible;
  /** The index of the selected item on the screen. */
  unsigned cursor;

  /**
   * Tracks the state of the mouse dragging over the list items
   */
  enum class DragMode {

    /**
     * No dragging in progress
     */
    NONE,

    /**
     * The user is currently scrolling the map.
     */
    SCROLL,

    /**
     * The user is dragging the selected item.
     */
    CURSOR,
  };

  DragMode drag_mode;

  /**
   * the vertical distance from the start of the drag relative to the
   * top of the list (not the top of the screen)
   */
  int drag_y;

  /**
   * The vertical distance from the start of the drag relative to the
   * top of the window
   */
  int drag_y_window;

  ListItemRenderer *item_renderer;
  ListCursorHandler *cursor_handler;

  KineticManager kinetic;
  WindowTimer kinetic_timer;

public:
  ListControl(const DialogLook &_look);

  /**
   * @param parent the parent window
   * @param _item_height Height of an item of the ListFrameControl
   */
  ListControl(ContainerWindow &parent, const DialogLook &look,
              PixelRect rc, const WindowStyle style,
              unsigned _item_height);

  virtual ~ListControl();

  void Create(ContainerWindow &parent,
              PixelRect rc, const WindowStyle style,
              unsigned _item_height);

  void SetItemRenderer(ListItemRenderer *_item_renderer) {
    assert(_item_renderer != nullptr);
    assert(item_renderer == nullptr);

    item_renderer = _item_renderer;
  }

  void SetCursorHandler(ListCursorHandler *_cursor_handler) {
    assert(_cursor_handler != nullptr);
    assert(cursor_handler == nullptr);

    cursor_handler = _cursor_handler;
  }

  /**
   * Returns the height of list items
   * @return height of list items in pixel
   */
  unsigned GetItemHeight() const {
    return item_height;
  }

  void SetItemHeight(unsigned _item_height);

  bool IsEmpty() const {
    return length == 0;
  }

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
   * Check whether the length of the list is below a certain
   * threshold.  Small lists may have different behaviour on some
   * platforms.
   */
  bool IsShort() const {
    return length <= 8 || length <= items_visible;
  }

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
   * Pan the "origin item" to the specified pixel position.
   */
  void SetPixelPan(unsigned _pixel_pan);

  /**
   * Scrolls to the specified index.
   */
  void SetOrigin(int i);

  unsigned GetPixelOrigin() const {
    return origin * item_height + pixel_pan;
  }

  void SetPixelOrigin(int pixel_origin);

  /**
   * Scrolls a number of items up (negative delta) or down (positive
   * delta).  The cursor is not moved.
   */
  void MoveOrigin(int delta);

protected:
  gcc_pure
  bool CanActivateItem() const;
  void ActivateItem();

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
    int i = (y + pixel_pan) / item_height + origin;
    return i >= 0 && (unsigned)i < length ? i : -1;
  }

  gcc_pure
  PixelRect item_rect(unsigned i) const {
    PixelRect rc;
    rc.left = 0;
    rc.top = (int)(i - origin) * item_height - pixel_pan;
    rc.right = scroll_bar.GetLeft(GetSize());
    rc.bottom = rc.top + item_height;
    return rc;
  }

  void Invalidate_item(unsigned i) {
    Invalidate(item_rect(i));
  }

  void drag_end();

  void DrawItems(Canvas &canvas, unsigned start, unsigned end) const;

  /** Draws the ScrollBar */
  void DrawScrollBar(Canvas &canvas);

  bool OnTimer(WindowTimer &timer) override;
  void OnDestroy() override;

  void OnResize(PixelSize new_size) override;

  void OnSetFocus() override;
  void OnKillFocus() override;

  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseUp(PixelPoint p) override;
  bool OnMouseMove(PixelPoint p, unsigned keys) override;
  bool OnMouseWheel(PixelPoint p, int delta) override;

  bool OnKeyCheck(unsigned key_code) const override;
  bool OnKeyDown(unsigned key_code) override;

  void OnCancelMode() override;

  void OnPaint(Canvas &canvas) override;
  void OnPaint(Canvas &canvas, const PixelRect &dirty) override;
};

#endif
