/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Thread/Debug.hpp"
#include "Compiler.h"

#include <assert.h>

#ifdef USE_GDI
#include <windows.h>
#endif /* GDI */

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
#ifndef USE_GDI
protected:
  bool visible;
  bool enabled;
  bool tab_stop, control_parent;
  bool double_clicks;
  bool has_border;
  unsigned text_style;

public:
  constexpr
  WindowStyle()
    :visible(true), enabled(true),
     tab_stop(false), control_parent(false),
     double_clicks(false), has_border(false),
     text_style(0) {}

#else /* USE_GDI */
protected:
  DWORD style, ex_style;
  bool double_clicks;
  bool custom_painting;

#ifdef _WIN32_WCE
  /* workaround for gcc optimization bug on ARM/XScale */
  bool dummy0, dummy1;
#endif

public:
  constexpr
  WindowStyle()
    :style(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS),
     ex_style(0), double_clicks(false), custom_painting(false)
#ifdef _WIN32_WCE
    , dummy0(0), dummy1(0)
#endif
  {}
#endif /* USE_GDI */

  /** The window is initially not visible. */
  void Hide() {
#ifndef USE_GDI
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
#ifndef USE_GDI
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
#ifndef USE_GDI
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
#ifndef USE_GDI
    control_parent = true;
#else
    ex_style |= WS_EX_CONTROLPARENT;
#endif
  }

  /** The window has a thin-line border. */
  void Border() {
#ifdef USE_GDI
    style |= WS_BORDER;
#else
    has_border = true;
#endif
  }

  /** The window has a sunken 3D border. */
  void SunkenEdge() {
    Border();
#ifdef USE_GDI
    ex_style |= WS_EX_CLIENTEDGE;
#endif
  }

  /** The window has a vertical scroll bar. */
  void VerticalScroll() {
#ifdef USE_GDI
    style |= WS_VSCROLL;
#endif
  }

  void Popup() {
#ifdef USE_GDI
    style &= ~WS_CHILD;
    style |= WS_SYSMENU;
#endif
  }

  void EnableCustomPainting() {
#ifdef USE_GDI
    custom_painting = true;
#endif
  }

  void EnableDoubleClicks() {
    double_clicks = true;
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

protected:
#ifndef USE_GDI
  ContainerWindow *parent;

private:
  RasterPoint position;
  PixelSize size;

private:
  const Font *font;
  unsigned text_style;

  bool tab_stop, control_parent;

  bool visible;
  bool enabled;
  bool focused;
  bool capture;
  bool has_border;
#else
  HWND hWnd;

private:
  WNDPROC prev_wndproc;
#endif

private:
  bool double_clicks;
  bool custom_painting;

public:
#ifndef USE_GDI
  Window()
    :parent(nullptr), size(0, 0),
     font(nullptr),
     visible(true), focused(false), capture(false), has_border(false),
     double_clicks(false) {}
#else
  Window():hWnd(nullptr), prev_wndproc(nullptr),
           double_clicks(false), custom_painting(false) {}
#endif
  virtual ~Window();

  Window(const Window &other) = delete;
  Window &operator=(const Window &other) = delete;

  /**
   * Activates the OnPaint() method.  It is disabled by default
   * because its preparation would needlessly allocate resources.
   */
  void EnableCustomPainting() {
#ifdef USE_GDI
    custom_painting = true;
#endif
  }

#ifndef USE_GDI
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

#ifdef USE_GDI
  bool GetCustomPainting() const {
    return custom_painting;
  }
#endif

#ifndef USE_GDI
  bool HasBorder() const {
    return has_border;
  }
#endif

public:
  bool IsDefined() const {
#ifndef USE_GDI
    return size.cx > 0;
#else
    return hWnd != nullptr;
#endif
  }

#ifndef USE_GDI
  PixelScalar GetTop() const {
    assert(IsDefined());

    return position.y;
  }

  PixelScalar GetLeft() const {
    assert(IsDefined());

    return position.x;
  }

  UPixelScalar GetWidth() const {
    assert(IsDefined());

    return size.cx;
  }

  UPixelScalar GetHeight() const {
    assert(IsDefined());

    return size.cy;
  }

  PixelScalar GetRight() const {
    return GetLeft() + GetWidth();
  }

  PixelScalar GetBottom() const {
    return GetTop() + GetHeight();
  }

  unsigned GetTextStyle() const {
    return text_style;
  }
#else /* USE_GDI */
  UPixelScalar GetWidth() const {
    return GetSize().cx;
  }

  UPixelScalar GetHeight() const {
    return GetSize().cy;
  }
#endif

#ifndef USE_GDI
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

#ifdef USE_GDI
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

  void Move(PixelScalar left, PixelScalar top) {
    AssertNoneLocked();
    AssertThread();

#ifndef USE_GDI
    position = { left, top };
    Invalidate();
#else
    ::SetWindowPos(hWnd, nullptr, left, top, 0, 0,
                   SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
  }

  void Move(PixelScalar left, PixelScalar top,
            UPixelScalar width, UPixelScalar height) {
    AssertNoneLocked();
    AssertThread();

#ifndef USE_GDI
    Move(left, top);
    Resize(width, height);
#else /* USE_GDI */
    ::SetWindowPos(hWnd, nullptr, left, top, width, height,
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    // XXX store new size?
#endif
  }

  void Move(const PixelRect rc) {
    assert(rc.left < rc.right);
    assert(rc.top < rc.bottom);

    Move(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
  }

  void MoveToCenter() {
    const PixelSize window_size = GetSize();
    const PixelSize parent_size = GetParentClientRect().GetSize();
    PixelScalar dialog_x = (parent_size.cx - window_size.cx) / 2;
    PixelScalar dialog_y = (parent_size.cy - window_size.cy) / 2;
    Move(dialog_x, dialog_y);
  }

  /**
   * Like move(), but does not trigger a synchronous redraw.  The
   * caller is responsible for redrawing.
   */
  void FastMove(PixelScalar left, PixelScalar top,
                UPixelScalar width, UPixelScalar height) {
    AssertNoneLocked();
    AssertThread();

#ifndef USE_GDI
    Move(left, top, width, height);
#else /* USE_GDI */
    ::SetWindowPos(hWnd, nullptr, left, top, width, height,
                   SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_DEFERERASE |
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
  }

  void FastMove(const PixelRect rc) {
    FastMove(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
  }

  /**
   * Move the Window to the specified position within the parent
   * ContainerWindow and make it visible.
   */
  void MoveAndShow(const PixelRect rc) {
    AssertNoneLocked();
    AssertThread();

#ifdef USE_GDI
    ::SetWindowPos(hWnd, nullptr,
                   rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                   SWP_SHOWWINDOW | SWP_NOACTIVATE |
                   SWP_NOZORDER | SWP_NOOWNERZORDER);
#else
    Move(rc);
    Show();
#endif
  }

  void Resize(UPixelScalar width, UPixelScalar height) {
    AssertNoneLocked();
    AssertThread();

#ifndef USE_GDI
    if (width == GetWidth() && height == GetHeight())
      return;

    size = { width, height };

    Invalidate();
    OnResize(size);
#else /* USE_GDI */
    ::SetWindowPos(hWnd, nullptr, 0, 0, width, height,
                   SWP_NOMOVE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    // XXX store new size?
#endif
  }

  void Resize(PixelSize size) {
    Resize(size.cx, size.cy);
  }

#ifndef USE_GDI
  void BringToTop();
  void BringToBottom();
#else
  void BringToTop() {
    AssertNoneLocked();
    AssertThread();

    /* not using BringWindowToTop() because it activates the
       winddow */
    ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                   SWP_NOMOVE|SWP_NOSIZE|
                   SWP_NOACTIVATE|SWP_NOOWNERZORDER);
  }

  void BringToBottom() {
    AssertNoneLocked();
    AssertThread();

    ::SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0,
                   SWP_NOMOVE|SWP_NOSIZE|
                   SWP_NOACTIVATE|SWP_NOOWNERZORDER);
  }
#endif

  void ShowOnTop() {
    AssertNoneLocked();
    AssertThread();

#ifndef USE_GDI
    BringToTop();
    Show();
#else
    ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                   SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|
                   SWP_NOACTIVATE|SWP_NOOWNERZORDER);
#endif
  }

#ifdef USE_GDI
  void SetFont(const Font &_font);
#else
  void SetFont(const Font &_font) {
    AssertNoneLocked();
    AssertThread();

    font = &_font;
    Invalidate();
  }

  const Font &GetFont() const {
    AssertThread();
    assert(font != nullptr);

    return *font;
  }
#endif

  /**
   * Determine whether this Window is visible.  This method disregards
   * the visibility of parent windows, it just checks if the "visible"
   * flag is set for this Window.
   */
  gcc_pure
  bool IsVisible() const {
    assert(IsDefined());

#ifndef USE_GDI
    return visible;
#else
    return (GetStyle() & WS_VISIBLE) != 0;
#endif
  }

#ifndef USE_GDI
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

#ifndef USE_GDI
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

  gcc_pure
  bool IsTabStop() const {
    assert(IsDefined());

#ifdef USE_GDI
    return (GetStyle() & WS_VISIBLE) != 0;
#else
    return tab_stop;
#endif
  }

  gcc_pure
  bool IsControlParent() const {
    assert(IsDefined());

#ifdef USE_GDI
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

#ifndef USE_GDI
    return enabled;
#else
    return ::IsWindowEnabled(hWnd);
#endif
  }

  /**
   * Specifies whether this window can get user input.
   */
  void SetEnabled(bool enabled);

#ifndef USE_GDI

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

#else /* USE_GDI */

  void SetFocus() {
    AssertNoneLocked();
    AssertThread();

    ::SetFocus(hWnd);
  }

  void FocusParent() {
    AssertNoneLocked();
    AssertThread();

    ::SetFocus(::GetParent(hWnd));
  }

#endif /* USE_GDI */

  gcc_pure
  bool HasFocus() const {
    assert(IsDefined());

#ifndef USE_GDI
    return focused;
#else
    return hWnd == ::GetFocus();
#endif
  }

#ifndef USE_GDI
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

#else /* USE_GDI */

  void SetCapture() {
    AssertNoneLocked();
    AssertThread();

    ::SetCapture(hWnd);
  }

  void ReleaseCapture() {
    AssertNoneLocked();
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
#endif /* USE_GDI */

#ifndef USE_GDI
  void ToScreen(PixelRect &rc) const;
#endif

  /**
   * Returns the position on the screen.
   */
  gcc_pure
  const PixelRect GetScreenPosition() const
  {
    assert(IsDefined());

    PixelRect rc;
#ifndef USE_GDI
    rc = GetPosition();
    ToScreen(rc);
#else
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

#ifndef USE_GDI
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

#ifndef USE_GDI
    return PixelRect(size);
#else
    PixelRect rc;
    ::GetClientRect(hWnd, &rc);
    return rc;
#endif
  }

  gcc_pure
  const PixelSize GetSize() const
  {
    assert(IsDefined());

#ifdef USE_GDI
    PixelRect rc = GetClientRect();
    PixelSize s;
    s.cx = rc.right;
    s.cy = rc.bottom;
    return s;
#else
    return size;
#endif
  }

  /**
   * Checks whether the specified coordinates are inside the Window's
   * client area.
   */
  gcc_pure
  bool IsInside(int x, int y) const {
    assert(IsDefined());

    const PixelSize size = GetSize();
    return unsigned(x) < unsigned(size.cx) && unsigned(y) < unsigned(size.cy);
  }

  gcc_pure
  bool IsInside(RasterPoint pt) const {
    return IsInside(pt.x, pt.y);
  }

  /**
   * Returns the parent's client area rectangle.
   */
#ifdef USE_GDI
  gcc_pure
  PixelRect GetParentClientRect() const {
    assert(IsDefined());

    HWND hParent = ::GetParent(hWnd);
    assert(hParent != nullptr);

    PixelRect rc;
    ::GetClientRect(hParent, &rc);
    return rc;
  }
#else
  gcc_pure
  PixelRect GetParentClientRect() const;
#endif

#ifndef USE_GDI
  void Setup(Canvas &canvas);

  virtual void Invalidate();
#else /* USE_GDI */
  HDC BeginPaint(PAINTSTRUCT *ps) {
    AssertThread();

    return ::BeginPaint(hWnd, ps);
  }

  void EndPaint(PAINTSTRUCT *ps) {
    AssertThread();

    ::EndPaint(hWnd, ps);
  }

  void Scroll(PixelScalar dx, PixelScalar dy, const PixelRect &rc) {
    assert(IsDefined());

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
   * have called InstallWndProc().
   */
  gcc_const
  static Window *GetChecked(HWND hWnd) {
    WNDPROC wndproc = (WNDPROC)::GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    return wndproc == WndProc
#ifdef _WIN32_WCE
      /* Windows CE seems to put WNDPROC pointers into some other
         segment (0x22000000 added); this is a dirty workaround which
         will be implemented properly once we understand what this
         really means */
      || ((DWORD)wndproc & 0xffffff) == (DWORD)WndProc
#endif
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

#ifndef USE_GDI
  void SendUser(unsigned id);
#else
  void SendUser(unsigned id) {
    assert(IsDefined());

    ::PostMessage(hWnd, WM_USER + id, (WPARAM)0, (LPARAM)0);
  }
#endif

protected:
#ifndef USE_GDI
public:
#endif /* !USE_GDI */
  /**
   * @return true on success, false if the window should not be
   * created
   */
  virtual void OnCreate();
  virtual void OnDestroy();
  virtual void OnResize(PixelSize new_size);
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys);
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y);
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y);
  virtual bool OnMouseDouble(PixelScalar x, PixelScalar y);
  virtual bool OnMouseWheel(PixelScalar x, PixelScalar y, int delta);

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

#ifdef USE_GDI
  virtual bool OnCommand(unsigned id, unsigned code);
#endif

  virtual void OnCancelMode();
  virtual void OnSetFocus();
  virtual void OnKillFocus();
  virtual bool OnTimer(WindowTimer &timer);
  virtual bool OnUser(unsigned id);

  virtual void OnPaint(Canvas &canvas);
  virtual void OnPaint(Canvas &canvas, const PixelRect &dirty);

#ifdef USE_GDI
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
#endif /* USE_GDI */

public:
#ifndef USE_GDI
  void InstallWndProc() {
    // XXX
  }
#else /* USE_GDI */
  /**
   * This static method reads the Window* object from GWL_USERDATA and
   * calls OnMessage().
   */
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
                                  WPARAM wParam, LPARAM lParam);

  /**
   * Installs Window::WndProc() has the WNDPROC.  This enables the
   * methods on_*() methods, which may be implemented by sub classes.
   */
  void InstallWndProc();
#endif /* USE_GDI */
};

#endif
