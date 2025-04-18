// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../Window.hpp"
#include "../ContainerWindow.hpp"
#include "ui/canvas/Font.hpp"
#include "Screen/Debug.hpp"
#include "ui/event/Idle.hpp"
#include "Asset.hpp"

#include <cassert>
#include <windowsx.h>

void
Window::Create(ContainerWindow *parent, const TCHAR *cls, const TCHAR *text,
               PixelRect rc, const WindowStyle window_style) noexcept
{
  assert(IsScreenInitialized());
  assert(rc.left <= rc.right);
  assert(rc.GetWidth() < 0x1000000);
  assert(rc.top <= rc.bottom);
  assert(rc.GetHeight() < 0x1000000);

  DWORD style = window_style.style, ex_style = window_style.ex_style;

  hWnd = ::CreateWindowEx(ex_style, cls, text, style,
                          rc.left, rc.top,
                          rc.GetWidth(), rc.GetHeight(),
                          parent != nullptr ? parent->hWnd : nullptr,
                          nullptr, nullptr, this);

  /* this isn't good error handling, but this only happens if
     out-of-memory (we can't do anything useful) or if we passed wrong
     arguments - which is a bug */
  assert(hWnd != nullptr);
}

void
Window::CreateMessageWindow() noexcept
{
  hWnd = ::CreateWindowEx(0, _T("PaintWindow"), nullptr, 0, 0, 0, 0, 0,
                          HWND_MESSAGE,
                          nullptr, nullptr, this);
  assert(hWnd != nullptr);
}

ContainerWindow *
Window::GetParent() const noexcept
{
  assert(IsDefined());

  HWND h = ::GetParent(hWnd);
  if (h == nullptr)
    return nullptr;

  return dynamic_cast<ContainerWindow *>(GetChecked(h));
}

bool
Window::IsMaximised() const noexcept
{
  const PixelRect this_rc = GetPosition();
  const PixelRect parent_rc = GetParentClientRect();

  return this_rc.GetWidth() >= parent_rc.GetWidth() &&
    this_rc.GetHeight() >= parent_rc.GetHeight();
}

[[gnu::pure]]
static PixelPoint
ScreenToClient(HWND h, PixelPoint p) noexcept
{
  POINT q{p.x, p.y};
  ScreenToClient(h, &q);
  return {q.x, q.y};
}

[[gnu::pure]]
static PixelRect
ScreenToClient(HWND h, PixelRect r) noexcept
{
  return {
    ScreenToClient(h, r.GetTopLeft()),
    ScreenToClient(h, r.GetBottomRight()),
  };
}

const PixelRect
Window::GetPosition() const noexcept
{
  assert(IsDefined());

  PixelRect rc = GetScreenPosition();

  HWND parent = ::GetParent(hWnd);
  if (parent != nullptr)
    return ::ScreenToClient(parent, rc);

  return rc;
}

void
Window::SetEnabled(bool enabled) noexcept
{
  AssertThread();

  const bool was_focused = !enabled && HasFocus();

  ::EnableWindow(hWnd, enabled);

  if (was_focused && ::GetFocus() == nullptr) {
    /* The window lost its keyboard focus because it got disabled; now
       the focus is in limbo, and can only be recovered by clicking on
       another control.  This is a major WIN32 API misdesign that is
       documtented here:
       https://blogs.msdn.com/b/oldnewthing/archive/2004/08/04/208005.aspx */

    ContainerWindow *root = GetRootOwner();
    if (root != nullptr)
      /* to work around this problem, we pass focus to the main
         window, which will bounce it to the next dialog control; this
         kludge is needed because this Window doesn't know the dialog
         code, and trusts that the main window will do the right
         thing */
      root->SetFocus();
  }
}

void
Window::Created(HWND _hWnd) noexcept
{
  assert(hWnd == nullptr);
  hWnd = _hWnd;

  AssertThread();
}

void
Window::SetFont(const Font &_font) noexcept
{
  AssertThread();

  ::SendMessage(hWnd, WM_SETFONT,
                (WPARAM)_font.Native(), MAKELPARAM(TRUE, 0));
}

bool
Window::OnCommand([[maybe_unused]] unsigned id,
                  [[maybe_unused]] unsigned code) noexcept
{
  return false;
}

bool
Window::OnUser([[maybe_unused]] unsigned id) noexcept
{
  return false;
}

LRESULT
Window::OnMessage([[maybe_unused]] HWND _hWnd, UINT message,
                  WPARAM wParam, LPARAM lParam) noexcept
{
  switch (message) {
  case WM_CREATE:
    OnCreate();
    return 0;

  case WM_DESTROY:
    OnDestroy();
    return 0;

  case WM_SIZE:
    OnResize({LOWORD(lParam), HIWORD(lParam)});
    return 0;

  case WM_MOUSEMOVE:
    if (OnMouseMove(PixelPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)),
                    wParam))
      return 0;
    break;

  case WM_LBUTTONDOWN:
    if (OnMouseDown(PixelPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))) {
      /* true returned: message was handled */
      ResetUserIdle();
      return 0;
    }
    break;

  case WM_LBUTTONUP:
    if (OnMouseUp(PixelPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))) {
      /* true returned: message was handled */
      ResetUserIdle();
      return 0;
    }
    break;

  case WM_LBUTTONDBLCLK:
    if (OnMouseDouble(PixelPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))) {
      /* true returned: message was handled */
      ResetUserIdle();
      return 0;
    }

    break;

#ifdef WM_MOUSEWHEEL
  case WM_MOUSEWHEEL:
    if (OnMouseWheel(PixelPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)),
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

  case WM_GETDLGCODE:
    if (OnKeyCheck(wParam))
      return DLGC_WANTMESSAGE;
    break;
  }

  if (message >= WM_USER && message <= 0x7FFF && OnUser(message - WM_USER))
    return 0;

  return ::DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK
Window::WndProc(HWND _hWnd, UINT message,
                WPARAM wParam, LPARAM lParam) noexcept
{
  if (message == WM_GETMINMAXINFO)
    /* WM_GETMINMAXINFO is called before WM_CREATE, and we havn't set
       a Window pointer yet - let DefWindowProc() handle it */
    return ::DefWindowProc(_hWnd, message, wParam, lParam);

  Window *window;
  if (message == WM_NCCREATE) {
    LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;

    window = (Window *)cs->lpCreateParams;
    window->Created(_hWnd);
    window->SetUserData(window);
  } else {
    window = GetUnchecked(_hWnd);
  }

  return window->OnMessage(_hWnd, message, wParam, lParam);
}
