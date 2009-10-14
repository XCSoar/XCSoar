/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#ifdef ENABLE_SDL
#define WindowCanvas BufferCanvas
#else /* !ENABLE_SDL */

/**
 * A #Canvas implementation which allows you to draw into WIN32 window
 * handle (HWND).  Use #PaintCanvas instead to implement WM_PAINT.
 */
class WindowCanvas : public Canvas {
protected:
  HWND wnd;

public:
  WindowCanvas() {}
  WindowCanvas(HWND _wnd, unsigned width, unsigned height);
  ~WindowCanvas();

  void set(HWND _wnd, unsigned _width, unsigned _height);
  void reset();
};

#endif /* !ENABLE_SDL */

class ContainerWindow;

/**
 * A #Window implementation for custom drawing.  Call get_canvas()
 * whenever you want to draw something.
 */
class PaintWindow : public Window {
private:
#ifndef ENABLE_SDL
  WindowCanvas canvas;
#endif

public:
  static bool register_class(HINSTANCE hInstance);

  void set(ContainerWindow &parent, LPCTSTR cls,
           int left, int top, unsigned width, unsigned height) {
    Window::set(&parent, cls, NULL, left, top, width, height);
  }

  void set(ContainerWindow *parent,
           int left, int top, unsigned width, unsigned height,
           bool center = false, bool notify = false, bool show = true,
           bool tabstop = false, bool border = false);

  void set(ContainerWindow &parent,
           int left, int top, unsigned width, unsigned height,
           bool center = false, bool notify = false, bool show = true,
           bool tabstop = false, bool border = false);

  void reset();

#ifndef ENABLE_SDL
  void resize(unsigned width, unsigned height) {
    canvas.resize(width, height);
  }
#endif /* !ENABLE_SDL */

  Canvas &get_canvas() {
    return canvas;
  }

  const Canvas &get_canvas() const {
    return canvas;
  }

  unsigned get_width() const {
    return canvas.get_width();
  }

  unsigned get_height() const {
    return canvas.get_height();
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
#ifdef ENABLE_SDL
    // XXX
    on_paint(get_canvas());
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
#ifndef ENABLE_SDL
  virtual bool on_create();

  virtual bool on_resize(unsigned width, unsigned height);
#endif /* !ENABLE_SDL */

  virtual bool on_erase(Canvas &canvas);
  virtual void on_paint(Canvas &canvas);

#ifndef ENABLE_SDL
  virtual LRESULT on_message(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);
#endif /* !ENABLE_SDL */

  virtual bool register_class(HINSTANCE hInstance, const TCHAR* szWindowClass);
};

#endif
