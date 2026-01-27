// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ScrollBar.hpp"
#include "ui/window/PaintWindow.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "UIUtil/KineticManager.hpp"

struct DialogLook;
class ContainerWindow;

typedef void (*ListItemRendererFunction)(Canvas &canvas, const PixelRect rc,
                                         unsigned idx);

class ListItemRenderer {
public:
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) noexcept = 0;
};

template<typename C>
class LambdaListItemRenderer : public ListItemRenderer, private C {
public:
  LambdaListItemRenderer(C &&c):C(std::move(c)) {}

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
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

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    function(canvas, rc, idx);
  }
};

class ListCursorHandler {
public:
  virtual void OnCursorMoved([[maybe_unused]] unsigned index) noexcept {}

  [[gnu::pure]]
  virtual bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept {
    return false;
  }

  virtual void OnActivateItem([[maybe_unused]] unsigned index) noexcept {}
};

/**
 * A ListControl implements a scrollable list control based on the
 * WindowControl class.
 */
class ListControl final : public PaintWindow {
  const DialogLook &look;

  /** The ScrollBar object */
  ScrollBar scroll_bar;

  /** The height of one item on the screen, in pixels. */
  unsigned item_height;
  /** The number of items in the list. */
  unsigned length = 0;
  /** The index of the topmost item currently being displayed. */
  unsigned origin = 0;

  /**
   * Which pixel row of the "origin" item is being displayed at the
   * top of the Window?
   */
  unsigned pixel_pan = 0;

  /** The number of items visible at a time. */
  unsigned items_visible;
  /** The index of the selected item on the screen. */
  unsigned cursor = 0;

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

  DragMode drag_mode = DragMode::NONE;

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

  ListItemRenderer *item_renderer = nullptr;
  ListCursorHandler *cursor_handler = nullptr;

  bool activate_on_first_click = false;

  KineticManager kinetic;
  UI::PeriodicTimer kinetic_timer{[this]{ OnKineticTimer(); }};

public:
  explicit ListControl(const DialogLook &_look) noexcept;

  /**
   * @param parent the parent window
   * @param _item_height Height of an item of the ListFrameControl
   */
  ListControl(ContainerWindow &parent, const DialogLook &look,
              PixelRect rc, const WindowStyle style,
              unsigned _item_height) noexcept;

  void Create(ContainerWindow &parent,
              PixelRect rc, const WindowStyle style,
              unsigned _item_height) noexcept;

  const auto &GetLook() const noexcept {
    return look;
  }

  void SetItemRenderer(ListItemRenderer *_item_renderer) noexcept {
    assert(_item_renderer != nullptr);
    assert(item_renderer == nullptr);

    item_renderer = _item_renderer;
  }

  void SetCursorHandler(ListCursorHandler *_cursor_handler) noexcept {
    assert(_cursor_handler != nullptr);
    assert(cursor_handler == nullptr);

    cursor_handler = _cursor_handler;
  }

  void SetActivateOnFirstClick(bool value) noexcept {
    activate_on_first_click = value;
  }

  /**
   * Returns the height of list items
   * @return height of list items in pixel
   */
  unsigned GetItemHeight() const noexcept {
    return item_height;
  }

  void SetItemHeight(unsigned _item_height) noexcept;

  bool IsEmpty() const noexcept {
    return length == 0;
  }

  /**
   * Returns the number of items in the list
   * @return The number of items in the list
   */
  unsigned GetLength() const noexcept {
    return length;
  }

  /** Changes the number of items in the list. */
  void SetLength(unsigned n) noexcept;

  /**
   * Check whether the length of the list is below a certain
   * threshold.  Small lists may have different behaviour on some
   * platforms.
   */
  bool IsShort() const noexcept {
    return length <= 8 || length <= items_visible;
  }

  /**
   * Returns the current cursor position
   * @return The current cursor position
   */
  unsigned GetCursorIndex() const noexcept {
    return cursor;
  }

  /**
   * Moves the cursor to the specified position.
   *
   * @return true if the cursor was moved to the specified position,
   * false if the position was invalid
   */
  bool SetCursorIndex(unsigned i) noexcept;

  /**
   * Move the cursor this many items up (negative delta) or down
   * (positive delta).  Scrolls if necessary.
   */
  void MoveCursor(int delta) noexcept;

  /**
   * Pan the "origin item" to the specified pixel position.
   */
  void SetPixelPan(unsigned _pixel_pan) noexcept;

  /**
   * Scrolls to the specified index.
   */
  void SetOrigin(int i) noexcept;

  unsigned GetPixelOrigin() const noexcept {
    return origin * item_height + pixel_pan;
  }

  void SetPixelOrigin(int pixel_origin) noexcept;

  /**
   * Scrolls a number of items up (negative delta) or down (positive
   * delta).  The cursor is not moved.
   */
  void MoveOrigin(int delta) noexcept;

private:
  [[gnu::pure]]
  bool CanActivateItem() const noexcept;
  void ActivateItem() noexcept;

  /** Checks whether a ScrollBar is needed and shows/hides it */
  void ShowOrHideScrollBar() noexcept;

  /**
   * Scroll to the ListItem defined by i
   * @param i The ListItem array id
   */
  void EnsureVisible(unsigned i) noexcept;

  /**
   * Determine which list item resides at the specified pixel row.
   * Returns -1 if there is no such list item.
   */
  [[gnu::pure]]
  int ItemIndexAt(int y) const noexcept {
    int i = (y + pixel_pan) / item_height + origin;
    return i >= 0 && (unsigned)i < length ? i : -1;
  }

  [[gnu::pure]]
  PixelRect GetItemRect(unsigned i) const noexcept {
    PixelRect rc;
    rc.left = 0;
    rc.top = (int)(i - origin) * item_height - pixel_pan;
    rc.right = scroll_bar.GetLeft(GetSize());
    rc.bottom = rc.top + item_height;
    return rc;
  }

  void InvalidateItem(unsigned i) noexcept {
    Invalidate(GetItemRect(i));
  }

  void drag_end() noexcept;

  void DrawItems(Canvas &canvas, unsigned start, unsigned end) const noexcept;

  /** Draws the ScrollBar */
  void DrawScrollBar(Canvas &canvas) noexcept;

  void OnKineticTimer() noexcept;

  void OnDestroy() noexcept override;

  void OnResize(PixelSize new_size) noexcept override;

  void OnSetFocus() noexcept override;
  void OnKillFocus() noexcept override;

  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseWheel(PixelPoint p, int delta) noexcept override;

  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;

  void OnCancelMode() noexcept override;

  void OnPaint(Canvas &canvas) noexcept override;
  void OnPaint(Canvas &canvas, const PixelRect &dirty) noexcept override;
};
