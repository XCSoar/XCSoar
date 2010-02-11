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

#ifndef XCSOAR_FORM_LIST_HPP
#define XCSOAR_FORM_LIST_HPP

#include "Form/Control.hpp"
#include "Screen/Bitmap.hpp"

/**
 * A WndListFrame implements a scrollable list control based on the
 * WindowControl class.
 */
class WndListFrame : public WindowControl {
  class ScrollBar {
    /** Top button bitmap */
    Bitmap hScrollBarBitmapTop;
    /** Slider bitmap */
    Bitmap hScrollBarBitmapMid;
    /** Bottom button bitmap */
    Bitmap hScrollBarBitmapBot;
    /** Scrollbar background bitmap */
    Bitmap hScrollBarBitmapFill;

  protected:
    /** Whether the slider is currently being dragged */
    bool dragging;
    int drag_offset;
    /** Coordinates of the ScrollBar */
    RECT rc;
    /** Coordinates of the Slider */
    RECT rc_slider;

  public:
    enum {
      SCROLLBARWIDTH_INITIAL = 32,
    };

    /** Constructor of the ScrollBar class */
    ScrollBar();

    /** Returns the width of the ScrollBar */
    int get_width() const {
      return rc.right - rc.left;
    }

    /** Returns the height of the ScrollBar */
    int get_height() const {
      return rc.bottom - rc.top;
    }

    /** Returns the height of the slider */
    int get_slider_height() const {
      return rc_slider.bottom - rc_slider.top;
    }

    /** Returns the height of the scrollable area of the ScrollBar */
    int get_netto_height() const {
      return get_height() - 2 * get_width();
    }

    /**
     * Returns the height of the visible scroll area of the ScrollBar
     * (the area thats not covered with the slider)
     */
    int get_scroll_height() const {
      return get_netto_height() - get_slider_height();
    }

    /**
     * Returns whether the ScrollBar is defined or has to be set up first
     * @return True if the ScrollBar is defined,
     * False if it has to be set up first
     */
    bool defined() const {
      return get_width() > 0;
    }

    /**
     * Returns the x-Coordinate of the ScrollBar
     * (remaining client area aside the ScrollBar)
     * @param size Size of the client area including the ScrollBar
     * @return The x-Coordinate of the ScrollBar
     */
    unsigned get_left(const SIZE size) const {
      return defined() ? rc.left : size.cx;
    }

    /**
     * Returns whether the given POINT is in the ScrollBar area
     * @param pt POINT to check
     * @return True if the given POINT is in the ScrollBar area, False otherwise
     */
    bool in(const POINT pt) const {
      return ::PtInRect(&rc, pt);
    }

    /**
     * Returns whether the given POINT is in the slider area
     * @param pt POINT to check
     * @return True if the given POINT is in the slider area, False otherwise
     */
    bool in_slider(const POINT pt) const {
      return ::PtInRect(&rc_slider, pt);
    }

    /**
     * Returns whether the given y-Coordinate is on the up arrow
     * @param y y-Coordinate to check
     * @return True if the given y-Coordinate is on the up arrow,
     * False otherwise
     */
    bool in_up_arrow(int y) const {
      return y < rc.top + get_width();
    }

    /**
     * Returns whether the given y-Coordinate is on the down arrow
     * @param y y-Coordinate to check
     * @return True if the given y-Coordinate is on the down arrow,
     * False otherwise
     */
    bool in_down_arrow(int y) const {
      return y >= rc.bottom - get_width();
    }

    /**
     * Returns whether the given y-Coordinate is above the slider area
     * @param y y-Coordinate to check
     * @return True if the given y-Coordinate is above the slider area,
     * False otherwise
     */
    bool above_slider(int y) const {
      return y < rc_slider.top;
    }

    /**
     * Returns whether the given y-Coordinate is below the slider area
     * @param y y-Coordinate to check
     * @return True if the given y-Coordinate is below the slider area,
     * False otherwise
     */
    bool below_slider(int y) const {
      return y >= rc_slider.bottom;
    }

    /**
     * Sets the size of the ScrollBar
     * (actually just the height, width is automatically set)
     * @param size Size of the Control the ScrollBar is used with
     */
    void set(const SIZE size);
    /** Resets the ScrollBar (undefines it) */
    void reset();

    /** Calculates the size and position of the slider */
    void set_slider(unsigned size, unsigned view_size, unsigned origin);
    /** Calculates the new origin out of the given y-Coordinate of the drag */
    unsigned to_origin(unsigned size, unsigned view_size, int y) const;

    /** Paints the ScollBar */
    void paint(Canvas &canvas, Color fore_color) const;

    /**
     * Returns whether the slider is currently being dragged
     * @return True if the slider is currently being dragged, False otherwise
     */
    bool is_dragging() const { return dragging; }
    /**
     * Should be called when beginning to drag
     * (Called by WndListFrame::on_mouse_down)
     * @param w The Window object the ScrollBar is belonging to
     * @param y y-Coordinate
     */
    void drag_begin(Window *w, unsigned y);
    /**
     * Should be called when stopping to drag
     * (Called by WndListFrame::on_mouse_up)
     * @param w The Window object the ScrollBar is belonging to
     */
    void drag_end(Window *w);
    /**
     * Should be called while dragging
     * @param size Size of the Scrollbar (not pixelwise)
     * @param view_size Visible size of the Scrollbar (not pixelwise)
     * @param y y-Coordinate
     * @return "Value" of the ScrollBar
     */
    unsigned drag_move(unsigned size, unsigned view_size, int y) const;
  };

public:
  typedef void (*ActivateCallback_t)(unsigned idx);
  typedef void (*CursorCallback_t)(unsigned idx);
  typedef void (*PaintItemCallback_t)(Canvas &canvas, const RECT rc, unsigned idx);

protected:
  /** The ScrollBar object */
  ScrollBar scroll_bar;

  /** The height of one item on the screen, in pixels. */
  unsigned item_height;
  /** The number of items in the list. */
  unsigned length;
  /** The index of the topmost item currently being displayed. */
  unsigned origin;
  /** The number of items visible at a time. */
  unsigned items_visible;
  /** The index of the selected item on the screen. */
  unsigned relative_cursor;

  ActivateCallback_t ActivateCallback;
  CursorCallback_t CursorCallback;
  PaintItemCallback_t PaintItemCallback;

public:
  /**
   * Constructor of the WndListFrame class
   * @param Owner Parent ContainerControl
   * @param X x-Coordinate of the ListFrameControl
   * @param Y y-Coordinate of the ListFrameControl
   * @param Width Width of the ListFrameControl
   * @param Height Height of the ListFrameControl
   * @param _item_height Height of an item of the ListFrameControl
   */
  WndListFrame(ContainerControl *Owner,
               int X, int Y, int Width, int Height,
               const WindowStyle style,
               unsigned _item_height);

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

  void SetItemHeight(unsigned _item_height);

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
    return origin + relative_cursor;
  }

  /**
   * Moves the cursor to the specified position.
   *
   * @return true if the cursor was moved to the specified position,
   * false if the position was invalid
   */
  bool SetCursorIndex(unsigned i);

  /**
   * Scrolls to the specified index.
   */
  void SetOrigin(unsigned i);

protected:
  /** Checks whether a ScrollBar is needed and shows/hides it */
  void show_or_hide_scroll_bar();

  /**
   * Scroll to the ListItem defined by i
   * @param i The ListItem array id
   */
  void EnsureVisible(unsigned i);

  /**
   * Selects the ListItem below the given coordinates
   * @param xPos x-Coordinate
   * @param yPos y-Coordinate
   */
  void SelectItemFromScreen(int xPos, int yPos);

  /** Draws the ScrollBar */
  void DrawScrollBar(Canvas &canvas);

  /**
   * The on_resize event is called when the Control is resized
   * (derived from Window)
   */
  virtual bool on_resize(unsigned width, unsigned height);

  /**
   * The on_mouse_down event is called when the mouse is pressed over the button
   * (derived from Window)
   */
  virtual bool on_mouse_down(int x, int y);
  /**
   * The on_mouse_up event is called when the mouse is released over the button
   * (derived from Window)
   */
  virtual bool on_mouse_up(int x, int y);
  /**
   * The on_mouse_move event is called when the mouse is moved over the button
   * (derived from Window)
   */
  virtual bool on_mouse_move(int x, int y, unsigned keys);
  /**
   * The on_mouse_wheel event is called when the mouse wheel is turned
   * (derived from Window)
   */
  virtual bool on_mouse_wheel(int delta);

  /**
   * The on_key_down event is called when a key is pressed while the
   * button is focused
   * (derived from Window)
   */
  virtual bool on_key_down(unsigned key_code);

  /**
   * The on_paint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  virtual void on_paint(Canvas &canvas);
};

#endif
