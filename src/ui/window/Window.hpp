// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"
#include "ui/dim/Rect.hpp"

#include <cassert>

#ifdef USE_WINUSER
#include <windef.h> // for HWND (needed by winuser.h)
#include <winuser.h>
#else
#include "util/IntrusiveList.hxx"
#endif

class Font;
class Canvas;
class ContainerWindow;

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
  void Hide() noexcept {
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
  void Disable() noexcept {
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
  void TabStop() noexcept {
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
  void ControlParent() noexcept {
#ifndef USE_WINUSER
    control_parent = true;
#else
    ex_style |= WS_EX_CONTROLPARENT;
#endif
  }

  /** The window has a thin-line border. */
  void Border() noexcept {
#ifdef USE_WINUSER
    style |= WS_BORDER;
#else
    has_border = true;
#endif
  }

  /** The window has a sunken 3D border. */
  void SunkenEdge() noexcept {
    Border();
#ifdef USE_WINUSER
    ex_style |= WS_EX_CLIENTEDGE;
#endif
  }

  /** The window has a vertical scroll bar. */
  void VerticalScroll() noexcept {
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
  IntrusiveListHook<IntrusiveHookMode::NORMAL> siblings;
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
  virtual ~Window() noexcept;

  Window(const Window &other) = delete;
  Window &operator=(const Window &other) = delete;

#ifndef USE_WINUSER
  ContainerWindow *GetParent() const noexcept {
    assert(IsDefined());

    return parent;
  }
#else
  operator HWND() const noexcept {
    assert(IsDefined());

    return hWnd;
  };

  /**
   * Is it this window?
   */
  [[gnu::pure]]
  bool Identify(HWND h) const noexcept {
    assert(IsDefined());

    return h == hWnd;
  }

  /**
   * Is it this window or one of its descendants?
   */
  [[gnu::pure]]
  bool IdentifyDescendant(HWND h) const noexcept {
    assert(IsDefined());

    return h == hWnd || ::IsChild(hWnd, h);
  }

  [[gnu::pure]]
  ContainerWindow *GetParent() const noexcept;
#endif

protected:
  /**
   * Assert that the current thread is the one which created this
   * window.
   */
#ifdef NDEBUG
  void AssertThread() const noexcept {}
  void AssertThreadOrUndefined() const noexcept {}
#else
  void AssertThread() const noexcept;
  void AssertThreadOrUndefined() const noexcept;
#endif

#ifndef USE_WINUSER
  bool HasBorder() const noexcept {
    return has_border;
  }
#endif

public:
  bool IsDefined() const noexcept {
#ifndef USE_WINUSER
    return size.width > 0;
#else
    return hWnd != nullptr;
#endif
  }

#ifndef USE_WINUSER
  PixelPoint GetTopLeft() const noexcept {
    assert(IsDefined());

    return position;
  }
#endif

#ifndef USE_WINUSER
  void Create(ContainerWindow *parent, const PixelRect rc,
              const WindowStyle window_style=WindowStyle()) noexcept;
#else
  void Create(ContainerWindow *parent, const TCHAR *cls, const TCHAR *text,
              const PixelRect rc,
              const WindowStyle window_style=WindowStyle()) noexcept;

  /**
   * Create a message-only window.
   */
  void CreateMessageWindow() noexcept;
#endif

#ifdef USE_WINUSER
  void Created(HWND _hWnd) noexcept;
#endif

  void Destroy() noexcept;

  /**
   * Determines the root owner window of this Window.  This is
   * probably a pointer to the #MainWindow instance.
   */
  [[gnu::pure]]
  ContainerWindow *GetRootOwner() noexcept;

  /**
   * Checks whether the window is "maximised" within its parent
   * window, i.e. whether its dimensions are not smaller than its
   * parent's dimensions.
   */
  [[gnu::pure]]
  bool IsMaximised() const noexcept;

  void Move(PixelPoint _position) noexcept {
    AssertThread();

#ifndef USE_WINUSER
    position = _position;
    Invalidate();
#else
    ::SetWindowPos(hWnd, nullptr, _position.x, _position.y, 0, 0,
                   SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
  }

  void Move(PixelPoint _position, PixelSize _size) noexcept {
    AssertThread();

#ifndef USE_WINUSER
    Move(_position);
    Resize(_size);
#else /* USE_WINUSER */
    ::SetWindowPos(hWnd, nullptr, _position.x, _position.y,
                   _size.width, _size.height,
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    // XXX store new size?
#endif
  }

  void Move(const PixelRect rc) noexcept {
    assert(rc.left < rc.right);
    assert(rc.top < rc.bottom);

    Move(rc.GetTopLeft(), rc.GetSize());
  }

  void MoveToCenter() noexcept {
    const PixelSize window_size = GetSize();
    const PixelSize parent_size = GetParentClientRect().GetSize();
    int dialog_x = (int(parent_size.width) - int(window_size.width)) / 2;
    int dialog_y = (int(parent_size.height) - int(window_size.height)) / 2;
    Move({dialog_x, dialog_y});
  }

  /**
   * Like Move(), but does not trigger a synchronous redraw.  The
   * caller is responsible for redrawing.
   */
  void FastMove(PixelPoint _position, PixelSize _size) noexcept {
    AssertThread();

#ifndef USE_WINUSER
    Move(_position, _size);
#else /* USE_WINUSER */
    ::SetWindowPos(hWnd, nullptr, _position.x, _position.y,
                   _size.width, _size.height,
                   SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_DEFERERASE |
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
  }

  void FastMove(const PixelRect rc) noexcept {
    FastMove(rc.GetTopLeft(), rc.GetSize());
  }

  /**
   * Move the Window to the specified position within the parent
   * ContainerWindow and make it visible.
   */
  void MoveAndShow(const PixelRect rc) noexcept {
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

  void Resize(PixelSize _size) noexcept {
    AssertThread();

#ifndef USE_WINUSER
    if (_size == size)
      return;

    size = _size;

    Invalidate();
    OnResize(size);
#else /* USE_WINUSER */
    ::SetWindowPos(hWnd, nullptr, 0, 0, _size.width, _size.height,
                   SWP_NOMOVE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    // XXX store new size?
#endif
  }

#ifndef USE_WINUSER
  void BringToTop() noexcept;
  void BringToBottom() noexcept;
#else
  void BringToTop() noexcept {
    AssertThread();

    /* not using BringWindowToTop() because it activates the
       winddow */
    ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                   SWP_NOMOVE|SWP_NOSIZE|
                   SWP_NOACTIVATE|SWP_NOOWNERZORDER);
  }

  void BringToBottom() noexcept {
    AssertThread();

    ::SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0,
                   SWP_NOMOVE|SWP_NOSIZE|
                   SWP_NOACTIVATE|SWP_NOOWNERZORDER);
  }
#endif

  void ShowOnTop() noexcept {
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
  void SetFont(const Font &_font) noexcept;
#endif

  /**
   * Determine whether this Window is visible.  This method disregards
   * the visibility of parent windows, it just checks if the "visible"
   * flag is set for this Window.
   */
  [[gnu::pure]]
  bool IsVisible() const noexcept {
    assert(IsDefined());

#ifndef USE_WINUSER
    return visible;
#else
    return (GetStyle() & WS_VISIBLE) != 0;
#endif
  }

#ifndef USE_WINUSER
  void Show() noexcept;
  void Hide() noexcept;
#else
  void Show() noexcept {
    AssertThread();

    ::ShowWindow(hWnd, SW_SHOW);
  }

  void Hide() noexcept {
    AssertThread();

    ::ShowWindow(hWnd, SW_HIDE);
  }
#endif

  /**
   * Like Hide(), but does not trigger a synchronous redraw of the
   * parent window's background.
   */
  void FastHide() noexcept {
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

  void SetVisible(bool visible) noexcept {
    if (visible)
      Show();
    else
      Hide();
  }

  /**
   * Attempt to make this window visible by scrolling the parent (via
   * ContainerWindow::ScrollTo()).
   */
  void ScrollParentTo() noexcept;

#ifndef USE_WINUSER
  bool IsTransparent() const noexcept {
    return transparent;
  }

  /**
   * Declare this window "transparent".  This means that portions of
   * the windows below it may be visible, and it will not be
   * considered "covering" windows behind it completely.  This flag is
   * evaluated by WindowList::IsCovered().
   */
  void SetTransparent() noexcept {
    assert(!transparent);

    transparent = true;
  }
#endif

  [[gnu::pure]]
  bool IsTabStop() const noexcept {
    assert(IsDefined());

#ifdef USE_WINUSER
    return (GetStyle() & WS_VISIBLE) != 0;
#else
    return tab_stop;
#endif
  }

  [[gnu::pure]]
  bool IsControlParent() const noexcept {
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
  [[gnu::pure]]
  bool IsEnabled() const noexcept {
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
  void SetEnabled(bool enabled) noexcept;

#ifndef USE_WINUSER

  virtual Window *GetFocusedWindow() noexcept;
  virtual void SetFocus() noexcept;

  /**
   * Called by the parent window when this window loses focus, or when
   * one of its (indirect) child windows loses focus.  This method is
   * responsible for invoking OnKillFocus().
   */
  virtual void ClearFocus() noexcept;

  /**
   * Send keyboard focus to this window's parent.  This should usually
   * only be called when this window owns the keyboard focus, and
   * doesn't want it anymore.
   */
  void FocusParent() noexcept;

#else /* USE_WINUSER */

  void SetFocus() noexcept {
    AssertThread();

    ::SetFocus(hWnd);
  }

  void FocusParent() noexcept {
    AssertThread();

    ::SetFocus(::GetParent(hWnd));
  }

#endif /* USE_WINUSER */

  [[gnu::pure]]
  bool HasFocus() const noexcept {
    assert(IsDefined());

#ifndef USE_WINUSER
    return focused;
#else
    return hWnd == ::GetFocus();
#endif
  }

#ifndef USE_WINUSER
  void SetCapture() noexcept;
  void ReleaseCapture() noexcept;
  virtual void ClearCapture() noexcept;

protected:
#if defined(USE_X11) || defined(USE_WAYLAND)
  virtual void EnableCapture() noexcept {}
  virtual void DisableCapture() noexcept {}
#else
  void EnableCapture() noexcept {}
  void DisableCapture() noexcept {}
#endif

public:

#else /* USE_WINUSER */

  void SetCapture() noexcept {
    AssertThread();

    ::SetCapture(hWnd);
  }

  void ReleaseCapture() noexcept {
    AssertThread();

    ::ReleaseCapture();
  }

  WNDPROC SetWndProc(WNDPROC wndproc) noexcept
  {
    AssertThread();

    return (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)wndproc);
  }

  [[gnu::const]]
  static void *GetUserData(HWND hWnd) noexcept {
    return (void *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
  }

  void SetUserData(void *value) noexcept
  {
    AssertThread();

    ::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)value);
  }
#endif /* USE_WINUSER */

#ifdef USE_WINUSER
  /**
   * Returns the position on the screen.
   */
  [[gnu::pure]]
  const PixelRect GetScreenPosition() const noexcept
  {
    assert(IsDefined());

    RECT rc;
    ::GetWindowRect(hWnd, &rc);
    return rc;
  }
#endif

  /**
   * Returns the position within the parent window.
   */
#ifndef USE_WINUSER
  [[gnu::pure]]
  const PixelRect GetPosition() const noexcept
  {
    assert(IsDefined());

    return { position, size };
  }
#else
  [[gnu::pure]]
  const PixelRect GetPosition() const noexcept;
#endif

  /**
   * Translate coordinates relative to this window to coordinates
   * relative to the parent window.
   */
  [[gnu::pure]]
  PixelPoint ToParentCoordinates(const PixelPoint p) const noexcept {
    return GetPosition().GetTopLeft() + p;
  }

  [[gnu::pure]]
  PixelRect ToParentCoordinates(const PixelRect &r) const noexcept {
    return {ToParentCoordinates(r.GetTopLeft()), r.GetSize()};
  }

  [[gnu::pure]]
  virtual const PixelRect GetClientRect() const noexcept
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

  [[gnu::pure]]
  const PixelSize GetSize() const noexcept
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
  [[gnu::pure]]
  bool IsInside(PixelPoint p) const noexcept {
    assert(IsDefined());

    const PixelSize size = GetSize();
    return unsigned(p.x) < size.width &&
        unsigned(p.y) < size.height;
  }

  /**
   * Returns the parent's client area rectangle.
   */
#ifdef USE_WINUSER
  [[gnu::pure]]
  PixelRect GetParentClientRect() const noexcept {
    assert(IsDefined());

    HWND hParent = ::GetParent(hWnd);
    assert(hParent != nullptr);

    RECT rc;
    ::GetClientRect(hParent, &rc);
    return rc;
  }
#else
  [[gnu::pure]]
  PixelRect GetParentClientRect() const noexcept;
#endif

#ifndef USE_WINUSER
  virtual void Invalidate() noexcept;
#else /* USE_WINUSER */
  HDC BeginPaint(PAINTSTRUCT *ps) noexcept {
    AssertThread();

    return ::BeginPaint(hWnd, ps);
  }

  void EndPaint(PAINTSTRUCT *ps) noexcept {
    AssertThread();

    ::EndPaint(hWnd, ps);
  }

  void Scroll(int dx, int dy, const PixelRect &_rc) noexcept {
    assert(IsDefined());

    const RECT rc = _rc;
    ::ScrollWindowEx(hWnd, dx, dy, &rc, nullptr, nullptr, nullptr,
                     SW_INVALIDATE);
  }

  /**
   * Converts a #HWND into a #Window pointer, without checking if that
   * is legal.
   */
  [[gnu::const]]
  static Window *GetUnchecked(HWND hWnd) noexcept {
    return (Window *)GetUserData(hWnd);
  }

  /**
   * Converts a #HWND into a #Window pointer.  Returns nullptr if the
   * HWND is not a Window peer.  This only works for windows which
   * use our WndProc.
   */
  [[gnu::const]]
  static Window *GetChecked(HWND hWnd) noexcept {
    WNDPROC wndproc = (WNDPROC)::GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    return wndproc == WndProc
      ? GetUnchecked(hWnd)
      : nullptr;
  }

  [[gnu::pure]]
  LONG GetWindowLong(int nIndex) const noexcept {
    assert(IsDefined());

    return ::GetWindowLong(hWnd, nIndex);
  }

  void SetWindowLong(int nIndex, LONG value) noexcept {
    assert(IsDefined());

    ::SetWindowLong(hWnd, nIndex, value);
  }

  LONG GetStyle() const noexcept {
    return GetWindowLong(GWL_STYLE);
  }

  void SetStyle(LONG value) noexcept {
    SetWindowLong(GWL_STYLE, value);
  }

  LONG GetExStyle() const noexcept {
    return GetWindowLong(GWL_EXSTYLE);
  }

  void SetExStyle(LONG value) noexcept {
    SetWindowLong(GWL_EXSTYLE, value);
  }
#endif

#ifdef USE_WINUSER
  void SendUser(unsigned id) noexcept {
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
  virtual void OnDestroy() noexcept;
  virtual void OnResize(PixelSize new_size) noexcept;
  virtual bool OnMouseMove(PixelPoint p, unsigned keys) noexcept;
  virtual bool OnMouseDown(PixelPoint p) noexcept;
  virtual bool OnMouseUp(PixelPoint p) noexcept;
  virtual bool OnMouseDouble(PixelPoint p) noexcept;
  virtual bool OnMouseWheel(PixelPoint p, int delta) noexcept;

#ifdef HAVE_MULTI_TOUCH
  /**
   * A secondary pointer is being pressed.
   */
  virtual bool OnMultiTouchDown() noexcept;

  /**
   * A secondary pointer is being released.
   */
  virtual bool OnMultiTouchUp() noexcept;
#endif

  /**
   * Checks if the window wishes to handle a special key, like cursor
   * keys and tab.  This wraps the WIN32 message WM_GETDLGCODE.
   *
   * @return true if the window will handle they key, false if the
   * dialog manager may use it
   */
  [[gnu::pure]]
  virtual bool OnKeyCheck(unsigned key_code) const noexcept;

  virtual bool OnKeyDown(unsigned key_code) noexcept;
  virtual bool OnKeyUp(unsigned key_code) noexcept;

  /**
   * A character was entered with the (virtual) keyboard.  This will
   * not be called if OnKeydown() has been handled already.
   *
   * @param ch the unicode character
   * @return true if the event was handled
   */
  virtual bool OnCharacter(unsigned ch) noexcept;

#ifdef USE_WINUSER
  virtual bool OnCommand(unsigned id, unsigned code) noexcept;
#endif

  virtual void OnCancelMode() noexcept;
  virtual void OnSetFocus() noexcept;
  virtual void OnKillFocus() noexcept;

#ifdef USE_WINUSER
  virtual bool OnUser(unsigned id) noexcept;

  virtual LRESULT OnMessage(HWND hWnd, UINT message,
                            WPARAM wParam, LPARAM lParam) noexcept;
#endif /* USE_WINUSER */

public:
#ifdef USE_WINUSER
  /**
   * This static method reads the Window* object from GWL_USERDATA and
   * calls OnMessage().
   */
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
                                  WPARAM wParam, LPARAM lParam) noexcept;
#endif /* USE_WINUSER */
};
