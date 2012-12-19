/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Screen/GDI/PaintCanvas.hpp"
#include "Event/Idle.hpp"
#include "Asset.hpp"

#include <assert.h>
#include <windowsx.h>

void
Window::Create(ContainerWindow *parent, const TCHAR *cls, const TCHAR *text,
               PixelRect rc, const WindowStyle window_style)
{
  assert(IsScreenInitialized());
  assert(rc.left <= rc.right);
  assert(rc.right - rc.left < 0x1000000);
  assert(rc.top <= rc.bottom);
  assert(rc.bottom - rc.top < 0x1000000);

  double_clicks = window_style.double_clicks;

  DWORD style = window_style.style, ex_style = window_style.ex_style;

  if (window_style.custom_painting)
    EnableCustomPainting();

  hWnd = ::CreateWindowEx(ex_style, cls, text, style,
                          rc.left, rc.top,
                          rc.right - rc.left, rc.bottom - rc.top,
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

bool
Window::IsMaximised() const
{
  const PixelRect this_rc = GetPosition();
  const PixelRect parent_rc = GetParentClientRect();

  return (this_rc.right - this_rc.left) >= (parent_rc.right - parent_rc.left) &&
    (this_rc.bottom - this_rc.top) >= (parent_rc.bottom - parent_rc.top);
}

void
Window::SetEnabled(bool enabled)
{
  AssertThread();

  const bool was_focused = !enabled && HasFocus();

  ::EnableWindow(hWnd, enabled);

  if (was_focused && ::GetFocus() == NULL) {
    /* The window lost its keyboard focus because it got disabled; now
       the focus is in limbo, and can only be recovered by clicking on
       another control, which is impossible for Altair users (no touch
       screen).  This is a major WIN32 API misdesign that is
       documtented here:
       https://blogs.msdn.com/b/oldnewthing/archive/2004/08/04/208005.aspx */

    ContainerWindow *root = GetRootOwner();
    if (root != NULL)
      /* to work around this problem, we pass focus to the main
         window, which will bounce it to the next dialog control; this
         kludge is needed because this Window doesn't know the dialog
         code, and trusts that the main window will do the right
         thing */
      root->SetFocus();
  }
}

void
Window::Created(HWND _hWnd)
{
  assert(hWnd == NULL);
  hWnd = _hWnd;

  AssertThread();
}

void
Window::SetFont(const Font &_font)
{
  AssertNoneLocked();
  AssertThread();

  ::SendMessage(hWnd, WM_SETFONT,
                (WPARAM)_font.Native(), MAKELPARAM(TRUE, 0));
}

LRESULT
Window::OnUnhandledMessage(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam)
{
  return prev_wndproc != NULL
    ? ::CallWindowProc(prev_wndproc, hWnd, message, wParam, lParam)
    : ::DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT
Window::OnMessage(HWND _hWnd, UINT message,
                       WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_CREATE:
    OnCreate();
    return 0;

  case WM_DESTROY:
    OnDestroy();
    return 0;

  case WM_SIZE:
    OnResize(LOWORD(lParam), HIWORD(lParam));
    return 0;

  case WM_MOUSEMOVE:
    if (OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam))
      return 0;
    break;

  case WM_LBUTTONDOWN:
    if (OnMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
      /* true returned: message was handled */
      ResetUserIdle();
      return 0;
    }
    break;

  case WM_LBUTTONUP:
    if (OnMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
      /* true returned: message was handled */
      ResetUserIdle();
      return 0;
    }
    break;

  case WM_LBUTTONDBLCLK:
    if (!double_clicks)
      /* instead of disabling CS_DBLCLKS (which would affect all
         instances of a window class), we just translate
         WM_LBUTTONDBLCLK to WM_LBUTTONDOWN here; this even works for
         built-in window class such as BUTTON */
      return OnMessage(_hWnd, WM_LBUTTONDOWN, wParam, lParam);

    if (OnMouseDouble(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
      /* true returned: message was handled */
      ResetUserIdle();
      return 0;
    }

    break;

#ifdef WM_MOUSEWHEEL
  case WM_MOUSEWHEEL:
    if (OnMouseWheel(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                       GET_WHEEL_DELTA_WPARAM(wParam))) {
      /* true returned: message was handled */
      ResetUserIdle();
      return 0;
    }
    break;
#endif

  case WM_KEYDOWN:
    if (OnKeyDown(wParam)) {
      /* true returned: message was handled */
      ResetUserIdle();
      return 0;
    }
    break;

  case WM_KEYUP:
    if (OnKeyUp(wParam)) {
      /* true returned: message was handled */
      ResetUserIdle();
      return 0;
    }
    break;

  case WM_CHAR:
    if (OnCharacter((TCHAR)wParam))
      /* true returned: message was handled */
      return 0;

    break;

  case WM_COMMAND:
    if (OnCommand(LOWORD(wParam), HIWORD(wParam))) {
      /* true returned: message was handled */
      ResetUserIdle();
      return 0;
    }
    break;

  case WM_CANCELMODE:
    OnCancelMode();
    /* pass on to DefWindowProc(), there may be more to do */
    break;

  case WM_SETFOCUS:
    OnSetFocus();
    /* pass on to DefWindowProc() so the underlying window class knows
       it's focused */
    break;

  case WM_KILLFOCUS:
    OnKillFocus();
    /* pass on to DefWindowProc() so the underlying window class knows
       it's not focused anymore */
    break;

  case WM_TIMER:
    if (OnTimer(*(WindowTimer *)wParam))
      return 0;
    break;

  case WM_PAINT:
    if (custom_painting) {
      PaintCanvas canvas(*this);
      OnPaint(canvas, canvas.get_dirty());
      return 0;
    }
    break;

  case WM_GETDLGCODE:
    if (OnKeyCheck(wParam))
      return DLGC_WANTMESSAGE;
    break;
  }

  if (message >= WM_USER && message <= 0x7FFF && OnUser(message - WM_USER))
    return 0;

  return OnUnhandledMessage(_hWnd, message, wParam, lParam);
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

  AssertNoneLocked();

  if (message == WM_GETMINMAXINFO)
    /* WM_GETMINMAXINFO is called before WM_CREATE, and we havn't set
       a Window pointer yet - let DefWindowProc() handle it */
    return ::DefWindowProc(_hWnd, message, wParam, lParam);

  Window *window;
  if (message == WM_VERY_FIRST) {
    LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;

    window = (Window *)cs->lpCreateParams;
    window->Created(_hWnd);
    window->SetUserData(window);
  } else {
    window = GetUnchecked(_hWnd);
  }

  LRESULT result = window->OnMessage(_hWnd, message, wParam, lParam);
  AssertNoneLocked();

  return result;
}

void
Window::InstallWndProc()
{
  assert(prev_wndproc == NULL);

  SetUserData(this);
  prev_wndproc = SetWndProc(WndProc);
}
