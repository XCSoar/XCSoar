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

#ifndef XCSOAR_SCREEN_WINDOW_HPP
#define XCSOAR_SCREEN_WINDOW_HPP

#include "Screen/Point.hpp"
#include "Screen/Features.hpp"
#include "Compiler.h"

#include <assert.h>

#ifdef USE_WINUSER
#include <windows.h>
#else
#include <boost/intrusive/list.hpp>
#endif

#ifdef ANDROID
struct Event;
#endif

class Font;
class Canvas;
class ContainerWindow;
class WindowTimer;

/**
 * A portable wrapper for describing a window's style settings on
 * creation.
 */
class WindowStyle {
protected:
#ifndef USE_WINUSER
  bool visible = true;
  bool enabled = true;
  bool tab_stop = false, control_parent = false;
  bool has_border = false;

#else /* USE_WINUSER */
  DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
  DWORD ex_style = 0;

#endif /* USE_WINUSER */

public:
  /** The window is initially not visible. */
  void Hide() {
#ifndef USE_WINUSER
    visible = false;
#else
    style &= ~WS_VISIBLE;
#endif
  }

  /**
   * The window is initially disabled.
   * A disabled window cannot receive input from the user.
   */
  void Disable() {
#ifndef USE_WINUSER
    enabled = false;
#else
    style |= WS_DISABLED;
#endif
  }

  /**
   * The window is a control that can receive the keyboard focus when the
   * user presses the TAB key. Pressing the TAB key changes the keyboard
   * focus to the next control with the WS_TABSTOP style.
   */
  void TabStop() {
#ifndef USE_WINUSER
    tab_stop = true;
#else
    style |= WS_TABSTOP;
#endif
  }

  /**
   * If the search for the next control with the WS_TABSTOP style encounters
   * a window with the WS_EX_CONTROLPARENT style, the system recursively
   * searches the window's children.
   */
  void ControlParent() {
#ifndef USE_WINUSER
    control_parent = true;
#else
    ex_style |= WS_EX_CONTROLPARENT;
#endif
  }

  /** The window has a thin-line border. */
  void Border() {
#ifdef USE_WINUSER
    style |= WS_BORDER;
#else
    has_border = true;
#endif
  }

  /** The window has a sunken 3D border. */
  void SunkenEdge() {
    Border();
#ifdef USE_WINUSER
    ex_style |= WS_EX_CLIENTEDGE;
#endif
  }

  /** The window has a vertical scroll bar. */
  void VerticalScroll() {
#ifdef USE_WINUSER
    style |= WS_VSCROLL;
#endif
  }

  void Popup() {
#ifdef USE_WINUSER
    style &= ~WS_CHILD;
    style |= WS_SYSMENU;
#endif
  }

  friend class Window;
};

/**
 * A Window is a portion on the screen which displays something, and
 * which optionally interacts with the user.  To draw custom graphics
 * into a Window, derive your class from #PaintWindow.
 */
class Window {
  friend class ContainerWindow;

#ifndef USE_WINUSER
  friend class WindowList;
  typedef boost::intrusive::list_member_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>> SiblingsHook;
  SiblingsHook siblings;
#endif

protected:
#ifndef USE_WINUSER
  ContainerWindow *parent = nullptr;

private:
  PixelPoint position;
  PixelSize size = {0, 0};

private:
  bool tab_stop, control_parent;

  bool visible = true;
  bool transparent = false;
  bool enabled;
  bool focused = false;
  bool capture = false;
  bool has_border = false;
#else
  HWND hWnd = nullptr;
#endif

public:
  Window() = default;
  virtual ~Window();

  Window(const Window &other) = delete;
  Window &operator=(const Window &other) = delete;

#ifndef USE_WINUSER
  const ContainerWindow *GetParent() const {
    assert(IsDefined());

    return parent;
  }
#else
  operator HWND() const {
    assert(IsDefined());

    return hWnd;
  };

  /**
   * Is it this window?
   */
  gcc_pure
  bool Identify(HWND h) const {
    assert(IsDefined());

    return h == hWnd;
  }

  /**
   * Is it this window or one of its descendants?
   */
  gcc_pure
  bool IdentifyDescendant(HWND h) const {
    assert(IsDefined());

    return h == hWnd || ::IsChild(hWnd, h);
  }
#endif

protected:
  /**
   * Assert that the current thread is the one which created this
   * window.
   */
#ifdef NDEBUG
  void AssertThread() const {}
  void AssertThreadOrUndefined() const {}
#else
  void AssertThread() const;
  void AssertThreadOrUndefined() const;
#endif

#ifndef USE_WINUSER
  bool HasBorder() const {
    return has_border;
  }
#endif

public:
  bool IsDefined() const {
#ifndef USE_WINUSER
    return size.cx > 0;
#else
    return hWnd != nullptr;
#endif
  }

#ifndef USE_WINUSER
  PixelPoint GetTopLeft() const {
    assert(IsDefined());

    return position;
  }

  int GetTop() const {
    assert(IsDefined());

    return position.y;
  }

  int GetLeft() const {
    assert(IsDefined());

    return position.x;
  }

  unsigned GetWidth() const {
    assert(IsDefined());

    return size.cx;
  }

  unsigned GetHeight() const {
    assert(IsDefined());

    return size.cy;
  }

  int GetRight() const {
    return GetLeft() + GetWidth();
  }

  int GetBottom() const {
    return GetTop() + GetHeight();
  }
#else /* USE_WINUSER */
  unsigned GetWidth() const {
    return GetSize().cx;
  }

  unsigned GetHeight() const {
    return GetSize().cy;
  }
#endif

#ifndef USE_WINUSER
  void Create(ContainerWindow *parent, const PixelRect rc,
              const WindowStyle window_style=WindowStyle());
#else
  void Create(ContainerWindow *parent, const TCHAR *cls, const TCHAR *text,
              const PixelRect rc,
              const WindowStyle window_style=WindowStyle());

  /**
   * Create a message-only window.
   */
  void CreateMessageWindow();
#endif

#ifdef USE_WINUSER
  void Created(HWND _hWnd);
#endif

  void Destroy();

  /**
   * Determines the root owner window of this Window.  This is
   * probably a pointer to the #MainWindow instance.
   */
  gcc_pure
  ContainerWindow *GetRootOwner();

  /**
   * Checks whether the window is "maximised" within its parent
   * window, i.e. whether its dimensions are not smaller than its
   * parent's dimensions.
   */
  gcc_pure
  bool IsMaximised() const;

  void Move(int left, int top) {
    AssertThread();

#ifndef USE_WINUSER
    position = { left, top };
    Invalidate();
#else
    ::SetWindowPos(hWnd, nullptr, left, top, 0, 0,
                   SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
  }

  void Move(int left, int top,
            unsigned width, unsigned height) {
    AssertThread();

#ifndef USE_WINUSER
    Move(left, top);
    Resize(width, height);
#else /* USE_WINUSER */
    ::SetWindowPos(hWnd, nullptr, left, top, width, height,
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    // XXX store new size?
#endif
  }

  void Move(const PixelRect rc) {
    assert(rc.left < rc.right);
    assert(rc.top < rc.bottom);

    Move(rc.left, rc.top, rc.GetWidth(), rc.GetHeight());
  }

  void MoveToCenter() {
    const PixelSize window_size = GetSize();
    const PixelSize parent_size = GetParentClientRect().GetSize();
    int dialog_x = (parent_size.cx - window_size.cx) / 2;
    int dialog_y = (parent_size.cy - window_size.cy) / 2;
    Move(dialog_x, dialog_y);
  }

  /**
   * Like move(), but does not trigger a synchronous redraw.  The
   * caller is responsible for redrawing.
   */
  void FastMove(int left, int top,
                unsigned width, unsigned height) {
    AssertThread();

#ifndef USE_WINUSER
    Move(left, top, width, height);
#else /* USE_WINUSER */
    ::SetWindowPos(hWnd, nullptr, left, top, width, height,
                   SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_DEFERERASE |
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
  }

  void FastMove(const PixelRect rc) {
    FastMove(rc.left, rc.top, rc.GetWidth(), rc.GetHeight());
  }

  /**
   * Move the Window to the specified position within the parent
   * ContainerWindow and make it visible.
   */
  void MoveAndShow(const PixelRect rc) {
    AssertThread();

#ifdef USE_WINUSER
    ::SetWindowPos(hWnd, nullptr,
                   rc.left, rc.top, rc.GetWidth(), rc.GetHeight(),
                   SWP_SHOWWINDOW | SWP_NOACTIVATE |
                   SWP_NOZORDER | SWP_NOOWNERZORDER);
#else
    Move(rc);
    Show();
#endif
  }

  void Resize(unsigned width, unsigned height) {
    AssertThread();

#ifndef USE_WINUSER
    if (width == GetWidth() && height == GetHeight())
      return;

    size = { width, height };

    Invalidate();
    OnResize(size);
#else /* USE_WINUSER */
    ::SetWindowPos(hWnd, nullptr, 0, 0, width, height,
                   SWP_NOMOVE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    // XXX store new size?
#endif
  }

  void Resize(PixelSize size) {
    Resize(size.cx, size.cy);
  }

#ifndef USE_WINUSER
  void BringToTop();
  void BringToBottom();
#else
  void BringToTop() {
    AssertThread();

    /* not using BringWindowToTop() because it activates the
       winddow */
    ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                   SWP_NOMOVE|SWP_NOSIZE|
                   SWP_NOACTIVATE|SWP_NOOWNERZORDER);
  }

  void BringToBottom() {
    AssertThread();

    ::SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0,
                   SWP_NOMOVE|SWP_NOSIZE|
                   SWP_NOACTIVATE|SWP_NOOWNERZORDER);
  }
#endif

  void ShowOnTop() {
    AssertThread();

#ifndef USE_WINUSER
    BringToTop();
    Show();
#else
    ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                   SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|
                   SWP_NOACTIVATE|SWP_NOOWNERZORDER);
#endif
  }

#ifdef USE_WINUSER
  void SetFont(const Font &_font);
#endif

  /**
   * Determine whether this Window is visible.  This method disregards
   * the visibility of parent windows, it just checks if the "visible"
   * flag is set for this Window.
   */
  gcc_pure
  bool IsVisible() const {
    assert(IsDefined());

#ifndef USE_WINUSER
    return visible;
#else
    return (GetStyle() & WS_VISIBLE) != 0;
#endif
  }

#ifndef USE_WINUSER
  void Show();
  void Hide();
#else
  void Show() {
    AssertThread();

    ::ShowWindow(hWnd, SW_SHOW);
  }

  void Hide() {
    AssertThread();

    ::ShowWindow(hWnd, SW_HIDE);
  }
#endif

  /**
   * Like Hide(), but does not trigger a synchronous redraw of the
   * parent window's background.
   */
  void FastHide() {
    AssertThread();

#ifndef USE_WINUSER
    Hide();
#else
    ::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0,
                   SWP_HIDEWINDOW |
                   SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_DEFERERASE |
                   SWP_NOMOVE | SWP_NOSIZE |
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
  }

  void SetVisible(bool visible) {
    if (visible)
      Show();
    else
      Hide();
  }

#ifndef USE_WINUSER
  bool IsTransparent() const {
    return transparent;
  }

  /**
   * Declare this window "transparent".  This means that portions of
   * the windows below it may be visible, and it will not be
   * considered "covering" windows behind it completely.  This flag is
   * evaluated by WindowList::IsCovered().
   */
  void SetTransparent() {
    assert(!transparent);

    transparent = true;
  }
#endif

  gcc_pure
  bool IsTabStop() const {
    assert(IsDefined());

#ifdef USE_WINUSER
    return (GetStyle() & WS_VISIBLE) != 0;
#else
    return tab_stop;
#endif
  }

  gcc_pure
  bool IsControlParent() const {
    assert(IsDefined());

#ifdef USE_WINUSER
    return (GetExStyle() & WS_EX_CONTROLPARENT) != 0;
#else
    return control_parent;
#endif
  }

  /**
   * Can this window get user input?
   */
  gcc_pure
  bool IsEnabled() const {
    assert(IsDefined());

#ifndef USE_WINUSER
    return enabled;
#else
    return ::IsWindowEnabled(hWnd);
#endif
  }

  /**
   * Specifies whether this window can get user input.
   */
  void SetEnabled(bool enabled);

#ifndef USE_WINUSER

  virtual Window *GetFocusedWindow();
  virtual void SetFocus();

  /**
   * Called by the parent window when this window loses focus, or when
   * one of its (indirect) child windows loses focus.  This method is
   * responsible for invoking OnKillFocus().
   */
  virtual void ClearFocus();

  /**
   * Send keyboard focus to this window's parent.  This should usually
   * only be called when this window owns the keyboard focus, and
   * doesn't want it anymore.
   */
  void FocusParent();

#else /* USE_WINUSER */

  void SetFocus() {
    AssertThread();

    ::SetFocus(hWnd);
  }

  void FocusParent() {
    AssertThread();

    ::SetFocus(::GetParent(hWnd));
  }

#endif /* USE_WINUSER */

  gcc_pure
  bool HasFocus() const {
    assert(IsDefined());

#ifndef USE_WINUSER
    return focused;
#else
    return hWnd == ::GetFocus();
#endif
  }

#ifndef USE_WINUSER
  void SetCapture();
  void ReleaseCapture();
  virtual void ClearCapture();

protected:
#if defined(USE_X11) || defined(USE_WAYLAND)
  virtual void EnableCapture() {}
  virtual void DisableCapture() {}
#else
  void EnableCapture() {}
  void DisableCapture() {}
#endif

public:

#else /* USE_WINUSER */

  void SetCapture() {
    AssertThread();

    ::SetCapture(hWnd);
  }

  void ReleaseCapture() {
    AssertThread();

    ::ReleaseCapture();
  }

  WNDPROC SetWndProc(WNDPROC wndproc)
  {
    AssertThread();

    return (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)wndproc);
  }

  gcc_const
  static void *GetUserData(HWND hWnd) {
    return (void *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
  }

  void SetUserData(void *value)
  {
    AssertThread();

    ::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)value);
  }
#endif /* USE_WINUSER */

#ifndef USE_WINUSER
  void ToScreen(PixelRect &rc) const;
#endif

  /**
   * Returns the position on the screen.
   */
  gcc_pure
  const PixelRect GetScreenPosition() const
  {
    assert(IsDefined());

#ifndef USE_WINUSER
    PixelRect rc = GetPosition();
    ToScreen(rc);
#else
    RECT rc;
    ::GetWindowRect(hWnd, &rc);
#endif
    return rc;
  }

  /**
   * Returns the position within the parent window.
   */
  gcc_pure
  const PixelRect GetPosition() const
  {
    assert(IsDefined());

#ifndef USE_WINUSER
    return { GetLeft(), GetTop(), GetRight(), GetBottom() };
#else
    PixelRect rc = GetScreenPosition();

    HWND parent = ::GetParent(hWnd);
    if (parent != nullptr) {
      POINT pt;

      pt.x = rc.left;
      pt.y = rc.top;
      ::ScreenToClient(parent, &pt);
      rc.left = pt.x;
      rc.top = pt.y;

      pt.x = rc.right;
      pt.y = rc.bottom;
      ::ScreenToClient(parent, &pt);
      rc.right = pt.x;
      rc.bottom = pt.y;
    }
    return rc;
#endif
  }

  gcc_pure
  const PixelRect GetClientRect() const
  {
    assert(IsDefined());

#ifndef USE_WINUSER
    return PixelRect(size);
#else
    RECT rc;
    ::GetClientRect(hWnd, &rc);
    return rc;
#endif
  }

  gcc_pure
  const PixelSize GetSize() const
  {
    assert(IsDefined());

#ifdef USE_WINUSER
    const auto rc = GetClientRect();
    return PixelSize(rc.right, rc.bottom);
#else
    return size;
#endif
  }

  /**
   * Checks whether the specified coordinates are inside the Window's
   * client area.
   */
  gcc_pure
  bool IsInside(PixelPoint p) const {
    assert(IsDefined());

    const PixelSize size = GetSize();
    return unsigned(p.x) < unsigned(size.cx) &&
        unsigned(p.y) < unsigned(size.cy);
  }

  /**
   * Returns the parent's client area rectangle.
   */
#ifdef USE_WINUSER
  gcc_pure
  PixelRect GetParentClientRect() const {
    assert(IsDefined());

    HWND hParent = ::GetParent(hWnd);
    assert(hParent != nullptr);

    RECT rc;
    ::GetClientRect(hParent, &rc);
    return rc;
  }
#else
  gcc_pure
  PixelRect GetParentClientRect() const;
#endif

#ifndef USE_WINUSER
  virtual void Invalidate();
#else /* USE_WINUSER */
  HDC BeginPaint(PAINTSTRUCT *ps) {
    AssertThread();

    return ::BeginPaint(hWnd, ps);
  }

  void EndPaint(PAINTSTRUCT *ps) {
    AssertThread();

    ::EndPaint(hWnd, ps);
  }

  void Scroll(int dx, int dy, const PixelRect &_rc) {
    assert(IsDefined());

    const RECT rc = _rc;
    ::ScrollWindowEx(hWnd, dx, dy, &rc, nullptr, nullptr, nullptr,
                     SW_INVALIDATE);
  }

  /**
   * Converts a #HWND into a #Window pointer, without checking if that
   * is legal.
   */
  gcc_const
  static Window *GetUnchecked(HWND hWnd) {
    return (Window *)GetUserData(hWnd);
  }

  /**
   * Converts a #HWND into a #Window pointer.  Returns nullptr if the
   * HWND is not a Window peer.  This only works for windows which
   * use our WndProc.
   */
  gcc_const
  static Window *GetChecked(HWND hWnd) {
    WNDPROC wndproc = (WNDPROC)::GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    return wndproc == WndProc
      ? GetUnchecked(hWnd)
      : nullptr;
  }

  gcc_pure
  LONG GetWindowLong(int nIndex) const {
    assert(IsDefined());

    return ::GetWindowLong(hWnd, nIndex);
  }

  void SetWindowLong(int nIndex, LONG value) {
    assert(IsDefined());

    ::SetWindowLong(hWnd, nIndex, value);
  }

  LONG GetStyle() const {
    return GetWindowLong(GWL_STYLE);
  }

  void SetStyle(LONG value) {
    SetWindowLong(GWL_STYLE, value);
  }

  LONG GetExStyle() const {
    return GetWindowLong(GWL_EXSTYLE);
  }

  void SetExStyle(LONG value) {
    SetWindowLong(GWL_EXSTYLE, value);
  }
#endif

#ifndef USE_WINUSER
  void SendUser(unsigned id);
#else
  void SendUser(unsigned id) {
    assert(IsDefined());

    ::PostMessage(hWnd, WM_USER + id, (WPARAM)0, (LPARAM)0);
  }
#endif

protected:
#ifndef USE_WINUSER
public:
#endif /* !USE_WINUSER */
  /**
   * @return true on success, false if the window should not be
   * created
   */
  virtual void OnCreate();
  virtual void OnDestroy();
  virtual void OnResize(PixelSize new_size);
  virtual bool OnMouseMove(PixelPoint p, unsigned keys);
  virtual bool OnMouseDown(PixelPoint p);
  virtual bool OnMouseUp(PixelPoint p);
  virtual bool OnMouseDouble(PixelPoint p);
  virtual bool OnMouseWheel(PixelPoint p, int delta);

#ifdef HAVE_MULTI_TOUCH
  /**
   * A secondary pointer is being pressed.
   */
  virtual bool OnMultiTouchDown();

  /**
   * A secondary pointer is being released.
   */
  virtual bool OnMultiTouchUp();
#endif

  /**
   * Checks if the window wishes to handle a special key, like cursor
   * keys and tab.  This wraps the WIN32 message WM_GETDLGCODE.
   *
   * @return true if the window will handle they key, false if the
   * dialog manager may use it
   */
  gcc_pure
  virtual bool OnKeyCheck(unsigned key_code) const;

  virtual bool OnKeyDown(unsigned key_code);
  virtual bool OnKeyUp(unsigned key_code);

  /**
   * A character was entered with the (virtual) keyboard.  This will
   * not be called if OnKeydown() has been handled already.
   *
   * @param ch the unicode character
   * @return true if the event was handled
   */
  virtual bool OnCharacter(unsigned ch);

#ifdef USE_WINUSER
  virtual bool OnCommand(unsigned id, unsigned code);
#endif

  virtual void OnCancelMode();
  virtual void OnSetFocus();
  virtual void OnKillFocus();
  virtual bool OnTimer(WindowTimer &timer);
  virtual bool OnUser(unsigned id);

#ifdef USE_WINUSER
  /**
   * Called by OnMessage() when the message was not handled by any
   * virtual method.  Calls the default handler.  This function is
   * virtual, because the Dialog class will have to override it -
   * dialogs have slightly different semantics.
   */
  virtual LRESULT OnUnhandledMessage(HWND hWnd, UINT message,
                                     WPARAM wParam, LPARAM lParam);

  virtual LRESULT OnMessage(HWND hWnd, UINT message,
                            WPARAM wParam, LPARAM lParam);
#endif /* USE_WINUSER */

public:
#ifdef USE_WINUSER
  /**
   * This static method reads the Window* object from GWL_USERDATA and
   * calls OnMessage().
   */
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
                                  WPARAM wParam, LPARAM lParam);
#endif /* USE_WINUSER */
};

#endif
