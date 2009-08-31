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

#ifndef XCSOAR_SCREEN_WINDOW_HPP
#define XCSOAR_SCREEN_WINDOW_HPP

#include "Screen/Font.hpp"

class ContainerWindow;

/**
 * A Window is a portion on the screen which displays something, and
 * which optionally interacts with the user.  To draw custom graphics
 * into a Window, derive your class from #PaintWindow.
 */
class Window {
protected:
  HWND hWnd;

public:
  Window():hWnd(NULL) {}

  operator HWND() const {
    return hWnd;
  };

public:
  bool defined() const {
    return hWnd != NULL;
  }

  void set(ContainerWindow *parent, LPCTSTR cls, LPCTSTR text,
           unsigned left, unsigned top,
           unsigned width, unsigned height,
           DWORD style, DWORD ex_style=0);

  void set(ContainerWindow *parent, LPCTSTR cls, LPCTSTR text,
           unsigned left, unsigned top,
           unsigned width, unsigned height,
           bool center = false, bool notify = false, bool show = true,
           bool tabstop = false, bool border = false);

  void created(HWND _hWnd);

  void reset();

  void move(int left, int top) {
    ::SetWindowPos(hWnd, NULL, left, top, 0, 0,
                   SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_NOOWNERZORDER);
  }

  void move(int left, int top, unsigned width, unsigned height) {
    ::SetWindowPos(hWnd, NULL, left, top, width, height,
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    // XXX store new size?
  }

  void insert_after(HWND hWnd2, bool show=true) {
    ::SetWindowPos(hWnd, hWnd2, 0, 0, 0, 0,
                   SWP_NOMOVE|SWP_NOSIZE|(show?SWP_SHOWWINDOW:SWP_HIDEWINDOW));
  }

  void bring_to_top() {
    ::BringWindowToTop(hWnd);
  }

  void set_active() {
    ::SetActiveWindow(hWnd);
  }

  void set_font(const Font &font) {
    ::SendMessage(hWnd, WM_SETFONT,
                  (WPARAM)font.native(), MAKELPARAM(TRUE,0));
  }

  void show() {
    ::ShowWindow(hWnd, SW_SHOW);
  }

  void hide() {
    ::ShowWindow(hWnd, SW_HIDE);
  }

  void set_focus() {
    ::SetFocus(hWnd);
  }

  void set_capture() {
    ::SetCapture(hWnd);
  }

  void release_capture() {
    ::ReleaseCapture();
  }

  WNDPROC set_wndproc(WNDPROC wndproc)
  {
    return (WNDPROC)::SetWindowLong(hWnd, GWL_WNDPROC, (LONG)wndproc);
  }

  void set_userdata(LONG value)
  {
    ::SetWindowLong(hWnd, GWL_USERDATA, value);
  }

  void set_userdata(void *value)
  {
    // XXX on 64 bit machines?
    set_userdata((LONG)(size_t)value);
  }

  LONG get_userdata() const
  {
    return ::GetWindowLong(hWnd, GWL_USERDATA);
  }

  void *get_userdata_pointer() const
  {
    // XXX on 64 bit machines?
    return (void *)get_userdata();
  }

  UINT_PTR set_timer(UINT_PTR nIDEvent, UINT uElapse)
  {
    return ::SetTimer(hWnd, nIDEvent, uElapse, NULL);
  }

  void kill_timer(UINT_PTR uIDEvent)
  {
    ::KillTimer(hWnd, uIDEvent);
  }

  const RECT get_position() const
  {
    RECT rc;
    ::GetWindowRect(hWnd, &rc);
    return rc;
  }

  const RECT get_client_rect() const
  {
    RECT rc;
    ::GetClientRect(hWnd, &rc);
    return rc;
  }

  static LONG get_userdata(HWND hWnd) {
    return ::GetWindowLong(hWnd, GWL_USERDATA);
  }

  static void *get_userdata_pointer(HWND hWnd) {
    // XXX on 64 bit machines?
    return (void *)get_userdata(hWnd);
  }

  void send_command(const Window &from) {
    ::SendMessage(hWnd, WM_COMMAND, (WPARAM)0, (LPARAM)from.hWnd);
  }

protected:
  virtual bool on_create();
  virtual bool on_destroy();
  virtual bool on_close();
  virtual bool on_resize(unsigned width, unsigned height);
  virtual bool on_mouse_move(unsigned x, unsigned y, unsigned keys);
  virtual bool on_mouse_down(unsigned x, unsigned y);
  virtual bool on_mouse_up(unsigned x, unsigned y);
  virtual bool on_mouse_double(unsigned x, unsigned y);
  virtual bool on_key_down(unsigned key_code);
  virtual bool on_key_up(unsigned key_code);
  virtual bool on_command(HWND hWnd, unsigned id, unsigned code);

  virtual LRESULT on_message(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);

public:
  /**
   * This static method reads the Window* object from GWL_USERDATA and
   * calls on_message().
   */
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
                                  WPARAM wParam, LPARAM lParam);

  /**
   * Installs Window::WndProc() has the WNDPROC.  This enables the
   * methods on_*() methods, which may be implemented by sub classes.
   */
  void install_wndproc() {
    set_userdata(this);
    set_wndproc(WndProc);
  }
};

#endif
