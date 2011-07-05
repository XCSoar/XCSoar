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

#include "Screen/Window.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Debug.hpp"
#include "Screen/Blank.hpp"
#include "Screen/GDI/Event.hpp"
#include "Screen/GDI/PaintCanvas.hpp"
#include "Asset.hpp"

#include <assert.h>
#include <windowsx.h>

void
Window::set(ContainerWindow *parent, const TCHAR *cls, const TCHAR *text,
            int left, int top, unsigned width, unsigned height,
            const WindowStyle window_style)
{
  assert(IsScreenInitialized());
  assert(width > 0);
  assert(width < 0x1000000);
  assert(height > 0);
  assert(height < 0x1000000);

  double_clicks = window_style.double_clicks;

  DWORD style = window_style.style, ex_style = window_style.ex_style;

  if (window_style.custom_painting)
    enable_custom_painting();

  hWnd = ::CreateWindowEx(ex_style, cls, text, style,
                          left, top, width, height,
                          parent != NULL ? parent->hWnd : NULL,
                          NULL, NULL, this);

  /* this isn't good error handling, but this only happens if
     out-of-memory (we can't do anything useful) or if we passed wrong
     arguments - which is a bug */
  assert(hWnd != NULL);
}

void
Window::CreateMessageWindow()
{
  hWnd = ::CreateWindowEx(0, _T("PaintWindow"), NULL, 0, 0, 0, 0, 0,
#ifdef _WIN32_WCE
                          NULL,
#else
                          HWND_MESSAGE,
#endif
                          NULL, NULL, this);
  assert(hWnd != NULL);
}

void
Window::created(HWND _hWnd)
{
  assert(hWnd == NULL);
  hWnd = _hWnd;

  assert_thread();
}

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
  if (is_embedded() && !is_altair()) {
    /* some older iPaqs such as the H3900 send only WM_KEYUP for
       VK_APP*, but never VK_KEYDOWN; the hx4700 has an additional set
       of undocumented key codes (0xca..0xcd) for the APP keys, but
       sends WM_KEYUP/VK_APP* additionally; the following rules
       hopefully catch all of these obscurities */
    if (message == WM_KEYUP && wParam >= 0x80)
      /* convert to WM_KEYDOWN to make all handlers catch it */
      message = WM_KEYDOWN;
    else if (message == WM_KEYDOWN && wParam >= 0x80)
      /* ignore the real WM_KEYDOWN, just in case it really happens */
      return 0;
  }

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
    if (on_mouse_down(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }
    break;

  case WM_LBUTTONUP:
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

    if (on_mouse_double(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }

    break;

#ifdef WM_MOUSEWHEEL
  case WM_MOUSEWHEEL:
    if (on_mouse_wheel(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                       GET_WHEEL_DELTA_WPARAM(wParam))) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }
    break;
#endif

  case WM_KEYDOWN:
    if (on_key_down(::TranscodeKey(wParam))) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }
    break;

  case WM_KEYUP:
    if (on_key_up(::TranscodeKey(wParam))) {
      /* true returned: message was handled */
      ResetDisplayTimeOut();
      return 0;
    }
    break;

  case WM_COMMAND:
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
