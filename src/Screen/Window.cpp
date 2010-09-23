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

#include "Screen/Window.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Blank.hpp"
#include "Interface.hpp"
#include "Asset.hpp"

#ifndef ENABLE_SDL
#include "Screen/PaintCanvas.hpp"
#endif /* !ENABLE_SDL */

#include <assert.h>

#ifndef ENABLE_SDL
#include <windowsx.h>
#endif

Window::~Window()
{
  reset();
}

#ifndef NDEBUG

void
Window::assert_thread() const
{
#ifndef ENABLE_SDL
  assert(hWnd != NULL);
  assert(!::IsWindow(hWnd) ||
         ::GetWindowThreadProcessId(hWnd, NULL) == ::GetCurrentThreadId());
#endif
}

#endif /* !NDEBUG */

void
Window::set(ContainerWindow *parent, const TCHAR *cls, const TCHAR *text,
            int left, int top, unsigned width, unsigned height,
            const WindowStyle window_style)
{
  double_clicks = window_style.double_clicks;

#ifdef ENABLE_SDL
  this->parent = parent;
  this->left = left;
  this->top = top;

  visible = window_style.visible;

  canvas.set(width, height);

  if (parent != NULL)
    parent->add_child(*this);

  on_create();
  on_resize(width, height);
#else /* !ENABLE_SDL */
  DWORD style = window_style.style, ex_style = window_style.ex_style;

  if (model_is_hp31x() && (style & WS_BORDER) != 0) {
    ex_style |= WS_EX_CLIENTEDGE;
    style |= WS_THICKFRAME;
  }

  if (window_style.custom_painting)
    enable_custom_painting();

  hWnd = ::CreateWindowEx(ex_style, cls, text, style,
                          left, top, width, height,
                          parent != NULL ? parent->hWnd : NULL,
                          NULL, XCSoarInterface::hInst, this);

  /* this isn't good error handling, but this only happens if
     out-of-memory (we can't do anything useful) or if we passed wrong
     arguments - which is a bug */
  assert(hWnd != NULL);
#endif /* !ENABLE_SDL */
}

#ifndef ENABLE_SDL
void
Window::created(HWND _hWnd)
{
  assert(hWnd == NULL);
  hWnd = _hWnd;

  assert_thread();
}
#endif /* !ENABLE_SDL */

void
Window::reset()
{
  if (defined())
    assert_thread();

#ifdef ENABLE_SDL
  on_destroy();
  canvas.reset();
#else /* !ENABLE_SDL */
  if (hWnd != NULL) {
    ::DestroyWindow(hWnd);

    /* the on_destroy() method must have cleared the variable by
       now */
    assert(prev_wndproc == NULL || hWnd == NULL);

    hWnd = NULL;
    prev_wndproc = NULL;
  }
#endif /* !ENABLE_SDL */
}

ContainerWindow *
Window::get_root_owner()
{
#ifdef ENABLE_SDL
  if (parent == NULL)
    /* no parent?  We must be a ContainerWindow instance */
    return (ContainerWindow *)this;

  ContainerWindow *root = parent;
  while (root->parent != NULL)
    root = root->parent;

  return root;
#else /* !ENABLE_SDL */
#ifndef _WIN32_WCE
  HWND hRoot = ::GetAncestor(hWnd, GA_ROOTOWNER);
  if (hRoot == NULL)
    return NULL;
#else
  HWND hRoot = hWnd;
  while (true) {
    HWND hParent = ::GetParent(hRoot);
    if (hParent == NULL)
      break;
    hRoot = hParent;
  }
#endif

  /* can't use the "checked" method get() because hRoot may be a
     dialog, and uses Dialog::DlgProc() */
  return (ContainerWindow *)get_unchecked(hRoot);
#endif /* !ENABLE_SDL */
}

#ifdef ENABLE_SDL

void
Window::to_screen(RECT &rc) const
{
  for (const Window *p = parent; p != NULL; p = p->parent) {
    rc.left += p->left;
    rc.top += p->top;
    rc.right += p->left;
    rc.bottom += p->top;
  }
}

Window *
Window::get_focused_window()
{
  return focused ? this : NULL;
}

void
Window::set_focus()
{
  if (parent != NULL)
    parent->set_active_child(*this);

  if (focused)
    return;

  on_setfocus();
}

void
Window::invalidate()
{
  if (visible && parent != NULL)
    parent->invalidate();
}

void
Window::expose(const RECT &rect)
{
  if (!visible)
    return;

  canvas.expose(rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top);
  if (parent != NULL)
    parent->expose_child(*this);
}

void
Window::expose()
{
  if (!visible)
    return;

  canvas.expose();
  if (parent != NULL)
    parent->expose_child(*this);
}

void
Window::show()
{
  assert_thread();

  if (visible)
    return;

  visible = true;
  expose();
}

void
Window::hide()
{
  assert_thread();

  if (!visible)
    return;

  visible = false;
  parent->invalidate();
}

void
Window::bring_to_top()
{
  assert_none_locked();
  assert_thread();

  parent->bring_child_to_top(*this);
}

#endif /* ENABLE_SDL */

bool
Window::on_create()
{
  return true;
}

bool
Window::on_destroy()
{
#ifdef ENABLE_SDL
  if (parent != NULL) {
    parent->remove_child(*this);
    parent = NULL;
  }
#else /* !ENABLE_SDL */
  assert(hWnd != NULL);

  hWnd = NULL;
#endif /* !ENABLE_SDL */

  return true;
}

bool
Window::on_close()
{
#ifdef ENABLE_SDL
  reset();
  return true;
#else /* !ENABLE_SDL */
  return false;
#endif /* !ENABLE_SDL */
}

bool
Window::on_resize(unsigned width, unsigned height)
{
  return false;
}

bool
Window::on_mouse_move(int x, int y, unsigned keys)
{
  /* not handled here */
  return false;
}

bool
Window::on_mouse_down(int x, int y)
{
  return false;
}

bool
Window::on_mouse_up(int x, int y)
{
  return false;
}

bool
Window::on_mouse_double(int x, int y)
{
  return false;
}

bool
Window::on_mouse_wheel(int delta)
{
  return false;
}

bool
Window::on_key_check(unsigned key_code) const
{
  return false;
}

bool
Window::on_key_down(unsigned key_code)
{
  return false;
}

bool
Window::on_key_up(unsigned key_code)
{
  return false;
}

bool
Window::on_command(unsigned id, unsigned code)
{
  return false;
}

bool
Window::on_cancel_mode()
{
  return false;
}

bool
Window::on_setfocus()
{
#ifdef ENABLE_SDL
  assert(!focused);

  focused = true;
  return true;
#else /* !ENABLE_SDL */
  return false;
#endif /* !ENABLE_SDL */
}

bool
Window::on_killfocus()
{
#ifdef ENABLE_SDL
  assert(focused);

  focused = false;
  return true;
#else /* !ENABLE_SDL */
  return false;
#endif /* !ENABLE_SDL */
}

bool
Window::on_timer(timer_t id)
{
  return false;
}

bool
Window::on_user(unsigned id)
{
  return false;
}

bool
Window::on_erase(Canvas &canvas)
{
  /* if on_paint() is implemented, then don't erase the background;
     on_paint() will paint on top */
#ifdef ENABLE_SDL
  return false;
#else
  return custom_painting;
#endif
}

void
Window::on_paint(Canvas &canvas)
{
}

void
Window::on_paint(Canvas &canvas, const RECT &dirty)
{
  on_paint(canvas);
}

#ifdef ENABLE_SDL

bool
Window::on_event(const SDL_Event &event)
{
  return false;
}

#else /* !ENABLE_SDL */

LRESULT
Window::on_unhandled_message(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam)
{
  return prev_wndproc != NULL
    ? ::CallWindowProc(prev_wndproc, hWnd, message, wParam, lParam)
    : ::DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT
Window::on_message(HWND _hWnd, UINT message,
                       WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_CREATE:
    return on_create() ? 0 : -1;
    break;

  case WM_DESTROY:
    if (on_destroy()) return 0;
    break;

  case WM_CLOSE:
    if (on_close())
      /* true returned: message was handled */
      return 0;
    break;

  case WM_SIZE:
    if (on_resize(LOWORD(lParam), HIWORD(lParam))) return 0;
    break;

  case WM_MOUSEMOVE:
    if (on_mouse_move(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam))
      return 0;
    break;

  case WM_LBUTTONDOWN:
    XCSoarInterface::InterfaceTimeoutReset();
    if (on_mouse_down(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }
    break;

  case WM_LBUTTONUP:
    XCSoarInterface::InterfaceTimeoutReset();
    if (on_mouse_up(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }
    break;

  case WM_LBUTTONDBLCLK:
    if (!double_clicks)
      /* instead of disabling CS_DBLCLKS (which would affect all
         instances of a window class), we just translate
         WM_LBUTTONDBLCLK to WM_LBUTTONDOWN here; this even works for
         built-in window class such as BUTTON */
      return on_message(_hWnd, WM_LBUTTONDOWN, wParam, lParam);

    XCSoarInterface::InterfaceTimeoutReset();
    if (on_mouse_double(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }

    break;

#ifdef WM_MOUSEWHEEL
  case WM_MOUSEWHEEL:
    XCSoarInterface::InterfaceTimeoutReset();
    if (on_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wParam))) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }
    break;
#endif

  case WM_KEYDOWN:
    XCSoarInterface::InterfaceTimeoutReset();
    if (on_key_down(wParam)) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }
    break;

  case WM_KEYUP:
    XCSoarInterface::InterfaceTimeoutReset();
    if (on_key_up(wParam)) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }
    break;

  case WM_COMMAND:
    XCSoarInterface::InterfaceTimeoutReset();
    if (on_command(LOWORD(wParam), HIWORD(wParam))) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }
    break;

  case WM_CANCELMODE:
    if (on_cancel_mode())
      return 0;
    break;

  case WM_SETFOCUS:
    if (on_setfocus())
      return 0;
    break;

  case WM_KILLFOCUS:
    if (on_killfocus())
      return 0;
    break;

  case WM_TIMER:
    if (on_timer(wParam))
      return 0;
    break;

  case WM_ERASEBKGND:
    {
      Canvas canvas((HDC)wParam, get_width(), get_height());
      if (on_erase(canvas))
        return 0;
    }
    break;

  case WM_PAINT:
    if (custom_painting) {
      PaintCanvas canvas(*this);
      on_paint(canvas, canvas.get_dirty());
      return 0;
    }
    break;

  case WM_GETDLGCODE:
    if (on_key_check(wParam))
      return DLGC_WANTMESSAGE;
    break;
  }

  if (message >= WM_USER && message <= 0x7FFF && on_user(message - WM_USER))
    return 0;

  return on_unhandled_message(_hWnd, message, wParam, lParam);
}

LRESULT CALLBACK
Window::WndProc(HWND _hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  enum {
#ifndef _WIN32_WCE
    WM_VERY_FIRST = WM_NCCREATE,
#else
    WM_VERY_FIRST = WM_CREATE,
#endif
  };

  assert_none_locked();

  if (message == WM_GETMINMAXINFO)
    /* WM_GETMINMAXINFO is called before WM_CREATE, and we havn't set
       a Window pointer yet - let DefWindowProc() handle it */
    return ::DefWindowProc(_hWnd, message, wParam, lParam);

  Window *window;
  if (message == WM_VERY_FIRST) {
    LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;

    window = (Window *)cs->lpCreateParams;
    window->created(_hWnd);
    window->set_userdata(window);
  } else {
    window = get_unchecked(_hWnd);
  }

  LRESULT result = window->on_message(_hWnd, message, wParam, lParam);
  assert_none_locked();

  return result;
}

void
Window::install_wndproc()
{
  assert(prev_wndproc == NULL);

  set_userdata(this);
  prev_wndproc = set_wndproc(WndProc);
}

#endif /* !ENABLE_SDL */
