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

#ifndef XCSOAR_SCREEN_PAINT_WINDOW_HXX
#define XCSOAR_SCREEN_PAINT_WINDOW_HXX

#include "Screen/Canvas.hpp"
#include "Screen/Window.hpp"

class PaintWindow;

/**
 * A #Canvas implementation which allows you to draw directly into a
 * #PaintWindow, outside of the PaintWindow::on_paint().
 */
class WindowCanvas : public Canvas {
#ifdef ENABLE_SDL
public:
  explicit WindowCanvas(Window &window)
    :Canvas(window.canvas.surface) {}

#else /* !ENABLE_SDL */

protected:
  HWND wnd;

public:
  explicit WindowCanvas(PaintWindow &window);

  ~WindowCanvas() {
    ::ReleaseDC(wnd, dc);
  }
#endif /* !ENABLE_SDL */
};

class ContainerWindow;

/**
 * A #Window implementation for custom drawing.  Call get_canvas()
 * whenever you want to draw something.
 */
class PaintWindow : public Window {
private:
  /* hide this method */
  void install_wndproc();

public:
  virtual ~PaintWindow();

  static bool register_class(HINSTANCE hInstance);

  void set(ContainerWindow *parent, const TCHAR *cls,
           int left, int top, unsigned width, unsigned height,
           const WindowStyle style=WindowStyle()) {
    Window::set(parent, cls, NULL,
                left, top, width, height, style);
  }

  void set(ContainerWindow &parent, const TCHAR *cls,
           int left, int top, unsigned width, unsigned height,
           const WindowStyle style=WindowStyle()) {
    set(&parent, cls, left, top, width, height, style);
  }

  void set(ContainerWindow &parent,
           int left, int top, unsigned width, unsigned height,
           const WindowStyle style=WindowStyle()) {
    set(parent, _T("PaintWindow"), left, top, width, height, style);
  }

  unsigned get_width() const {
    return get_size().cx;
  }

  unsigned get_height() const {
    return get_size().cy;
  }

  int get_left() const {
    return 0;
  }

  int get_top() const {
    return 0;
  }

  int get_right() const {
    return get_left() + get_width();
  }

  int get_bottom() const {
    return get_top() + get_height();
  }

  int get_hmiddle() const {
    return (get_left() + get_right()) / 2;
  }

  int get_vmiddle() const {
    return (get_top() + get_bottom()) / 2;
  }

  /**
   * Invalidates the visible area and schedules a repaint (which will
   * occur in the main thread).
   */
  void invalidate() {
#ifdef ENABLE_SDL
    // XXX
    WindowCanvas canvas(*this);
    on_paint(canvas);
    expose();
#else /* !ENABLE_SDL */
    ::InvalidateRect(hWnd, NULL, false);
#endif /* !ENABLE_SDL */
  }

  /**
   * Invalidates a part of the visible area and schedules a repaint
   * (which will occur in the main thread).
   */
  void invalidate(const RECT &rect) {
#ifdef ENABLE_SDL
    invalidate();
#else /* !ENABLE_SDL */
    ::InvalidateRect(hWnd, &rect, false);
#endif /* !ENABLE_SDL */
  }

  void update() {
    //assert_none_locked();

#ifdef ENABLE_SDL
    // XXX
    WindowCanvas canvas(*this);
    on_paint(canvas);
    expose();
#else /* !ENABLE_SDL */
    ::UpdateWindow(hWnd);
    // duplicate in MainWindow
#endif /* !ENABLE_SDL */
  }

#ifndef ENABLE_SDL
  HDC BeginPaint(PAINTSTRUCT *ps) {
    return ::BeginPaint(hWnd, ps);
  }

  void EndPaint(PAINTSTRUCT *ps) {
    ::EndPaint(hWnd, ps);
  }
#endif /* !ENABLE_SDL */

protected:
  virtual bool on_erase(Canvas &canvas);
  virtual void on_paint(Canvas &canvas);

#ifndef ENABLE_SDL
  virtual LRESULT on_message(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);
#endif /* !ENABLE_SDL */

  virtual bool register_class(HINSTANCE hInstance, const TCHAR* szWindowClass);
};

#endif
