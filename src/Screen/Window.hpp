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

#ifndef XCSOAR_SCREEN_WINDOW_HPP
#define XCSOAR_SCREEN_WINDOW_HPP

#include "Util/NonCopyable.hpp"
#include "Screen/Font.hpp"
#include "Screen/Point.hpp"
#include "Thread/Debug.hpp"
#include "Compiler.h"

#ifdef ANDROID
#include "Android/Timer.hpp"
#elif defined(ENABLE_SDL)
#include "Screen/SDL/Timer.hpp"
#endif /* ENABLE_SDL */

#ifdef ANDROID
struct Event;
#elif defined(ENABLE_SDL)
#include <SDL_events.h>
#endif

class Canvas;
class ContainerWindow;

/**
 * A portable wrapper for describing a window's style settings on
 * creation.
 */
class WindowStyle {
#ifdef ENABLE_SDL
protected:
  bool visible;
  bool enabled;
  bool m_tab_stop, m_control_parent;
  bool double_clicks;
  int text_style;

public:
  WindowStyle()
    :visible(true), enabled(true),
     m_tab_stop(false), m_control_parent(false),
     double_clicks(false),
     text_style(0) {}

#else /* !ENABLE_SDL */
protected:
  DWORD style, ex_style;
  bool double_clicks;
  bool custom_painting;

#ifdef _WIN32_WCE
  /* workaround for gcc optimization bug on ARM/XScale */
  bool dummy0, dummy1;
#endif

public:
  WindowStyle()
    :style(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS),
     ex_style(0), double_clicks(false), custom_painting(false) {}
#endif /* !ENABLE_SDL */

  void hide() {
#ifdef ENABLE_SDL
    visible = false;
#else
    style &= ~WS_VISIBLE;
#endif
  }

  void disable() {
#ifdef ENABLE_SDL
    enabled = false;
#else
    style |= WS_DISABLED;
#endif
  }

  void tab_stop() {
#ifdef ENABLE_SDL
    m_tab_stop = true;
#else
    style |= WS_TABSTOP;
#endif
  }

  void control_parent() {
#ifdef ENABLE_SDL
    m_control_parent = true;
#else
    ex_style |= WS_EX_CONTROLPARENT;
#endif
  }

  void border() {
#ifndef ENABLE_SDL
    style |= WS_BORDER;
#endif
  }

  void sunken_edge() {
    border();
#ifndef ENABLE_SDL
    ex_style |= WS_EX_CLIENTEDGE;
#endif
  }

  void vscroll() {
#ifndef ENABLE_SDL
    style |= WS_VSCROLL;
#endif
  }

  void popup() {
#ifndef ENABLE_SDL
    style &= ~WS_CHILD;
    style |= WS_SYSMENU;
#endif
  }

  void enable_custom_painting() {
#ifndef ENABLE_SDL
    custom_painting = true;
#endif
  }

  void enable_double_clicks() {
    double_clicks = true;
  }

  friend class Window;
};

/**
 * A Window is a portion on the screen which displays something, and
 * which optionally interacts with the user.  To draw custom graphics
 * into a Window, derive your class from #PaintWindow.
 */
class Window : private NonCopyable {
  friend class ContainerWindow;

public:
#ifdef ANDROID
  typedef AndroidTimer *timer_t;
#elif defined(ENABLE_SDL)
  enum {
    EVENT_USER = SDL_USEREVENT,
    EVENT_TIMER,
  };

  friend class SDLTimer;
  typedef SDLTimer *timer_t;
#else
  typedef UINT_PTR timer_t;
#endif

protected:
#ifdef ENABLE_SDL
  ContainerWindow *parent;

private:
  int left, top;
  unsigned width, height;

private:
  const Font *font;
  int text_style;

  bool tab_stop, control_parent;

  bool visible;
  bool enabled;
  bool focused;
  bool capture;
#else
  HWND hWnd;

private:
  WNDPROC prev_wndproc;
#endif

private:
  bool double_clicks;
  bool custom_painting;

public:
#ifdef ENABLE_SDL
  Window()
    :parent(NULL), width(0), height(0),
     font(NULL),
     visible(true), focused(false), capture(false),
     double_clicks(false) {}
#else
  Window():hWnd(NULL), prev_wndproc(NULL),
           double_clicks(false), custom_painting(false) {}
#endif
  virtual ~Window();

  /**
   * Activates the on_paint() method.  It is disabled by default
   * because its preparation would needlessly allocate resources.
   */
  void enable_custom_painting() {
#ifndef ENABLE_SDL
    custom_painting = true;
#endif
  }

#ifdef ENABLE_SDL
  const ContainerWindow *GetParent() const {
    return parent;
  }
#else
  operator HWND() const {
    return hWnd;
  };

  /**
   * Is it this window?
   */
  gcc_pure
  bool identify(HWND h) const {
    return h == hWnd;
  }

  /**
   * Is it this window or one of its descendants?
   */
  gcc_pure
  bool identify_descendant(HWND h) const {
    return h == hWnd || ::IsChild(hWnd, h);
  }
#endif

protected:
  /**
   * Assert that the current thread is the one which created this
   * window.
   */
#ifdef NDEBUG
  void assert_thread() const {}
#else
  void assert_thread() const;
#endif

#ifndef ENABLE_SDL
  bool get_custom_painting() const {
    return custom_painting;
  }
#endif

public:
  bool defined() const {
#ifdef ENABLE_SDL
    return width > 0;
#else
    return hWnd != NULL;
#endif
  }

#ifdef ENABLE_SDL
  void clear_parent() {
    parent = NULL;
  }

  int get_top() const {
    return top;
  }

  int get_left() const {
    return left;
  }

  unsigned get_width() const {
    return width;
  }

  unsigned get_height() const {
    return height;
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

  int get_text_style() const {
    return text_style;
  }
#else /* !ENABLE_SDL */
  unsigned get_width() const {
    return get_size().cx;
  }

  unsigned get_height() const {
    return get_size().cy;
  }
#endif

#ifdef ENABLE_SDL
  void set(ContainerWindow *parent,
           int left, int top, unsigned width, unsigned height,
           const WindowStyle window_style=WindowStyle());
#else
  void set(ContainerWindow *parent, const TCHAR *cls, const TCHAR *text,
           int left, int top, unsigned width, unsigned height,
           const WindowStyle window_style=WindowStyle());
#endif

#ifndef ENABLE_SDL
  void created(HWND _hWnd);
#endif

  void reset();

  /**
   * Determines the root owner window of this Window.  This is
   * probably a pointer to the #MainWindow instance.
   */
  gcc_pure
  ContainerWindow *get_root_owner();

  void move(int left, int top) {
    assert_none_locked();
    assert_thread();

#ifdef ENABLE_SDL
    this->left = left;
    this->top = top;
    invalidate();
#else
    ::SetWindowPos(hWnd, NULL, left, top, 0, 0,
                   SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
  }

  void move(int left, int top, unsigned width, unsigned height) {
    assert_none_locked();
    assert_thread();

#ifdef ENABLE_SDL
    move(left, top);
    resize(width, height);
#else /* !ENABLE_SDL */
    ::SetWindowPos(hWnd, NULL, left, top, width, height,
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    // XXX store new size?
#endif
  }

  /**
   * Like move(), but does not trigger a synchronous redraw.  The
   * caller is responsible for redrawing.
   */
  void fast_move(int left, int top, unsigned width, unsigned height) {
    assert_none_locked();
    assert_thread();

#ifdef ENABLE_SDL
    move(left, top, width, height);
#else /* !ENABLE_SDL */
    ::SetWindowPos(hWnd, NULL, left, top, width, height,
                   SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_DEFERERASE |
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
  }

  void resize(unsigned width, unsigned height) {
    assert_none_locked();
    assert_thread();

#ifdef ENABLE_SDL
    if (width == this->width && height == this->height)
      return;

    this->width = width;
    this->height = height;

    invalidate();
    on_resize(width, height);
#else /* !ENABLE_SDL */
    ::SetWindowPos(hWnd, NULL, 0, 0, width, height,
                   SWP_NOMOVE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    // XXX store new size?
#endif
  }

#ifdef ENABLE_SDL
  void bring_to_top();
  void BringToBottom();
#else
  void bring_to_top() {
    assert_none_locked();
    assert_thread();

    /* not using BringWindowToTop() because it activates the
       winddow */
    ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                   SWP_NOMOVE|SWP_NOSIZE|
                   SWP_NOACTIVATE|SWP_NOOWNERZORDER);
  }

  void BringToBottom() {
    assert_none_locked();
    assert_thread();

    ::SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0,
                   SWP_NOMOVE|SWP_NOSIZE|
                   SWP_NOACTIVATE|SWP_NOOWNERZORDER);
  }
#endif

  void show_on_top() {
    assert_none_locked();
    assert_thread();

#ifdef ENABLE_SDL
    bring_to_top();
    show();
#else
    ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                   SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|
                   SWP_NOACTIVATE|SWP_NOOWNERZORDER);
#endif
  }

  void set_font(const Font &_font) {
    assert_none_locked();
    assert_thread();

#ifdef ENABLE_SDL
    font = &_font;
    invalidate();
#else
    ::SendMessage(hWnd, WM_SETFONT,
                  (WPARAM)_font.native(), MAKELPARAM(TRUE,0));
#endif
  }

  gcc_pure
  bool is_visible() const {
#ifdef ENABLE_SDL
    return visible;
#else
    return ::IsWindowVisible(hWnd);
#endif
  }

#ifdef ENABLE_SDL
  void show();
  void hide();
#else
  void show() {
    assert_thread();

    ::ShowWindow(hWnd, SW_SHOW);
  }

  void hide() {
    assert_thread();

    ::ShowWindow(hWnd, SW_HIDE);
  }
#endif

  /**
   * Like hide(), but does not trigger a synchronous redraw of the
   * parent window's background.
   */
  void fast_hide() {
    assert_thread();

#ifdef ENABLE_SDL
    hide();
#else
    ::SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                   SWP_HIDEWINDOW |
                   SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_DEFERERASE |
                   SWP_NOMOVE | SWP_NOSIZE |
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
  }

  void set_visible(bool visible) {
    if (visible)
      show();
    else
      hide();
  }

#ifdef ENABLE_SDL
  bool is_tab_stop() const {
    return tab_stop;
  }

  bool is_control_parent() const {
    return control_parent;
  }
#endif

  /**
   * Can this window get user input?
   */
  gcc_pure
  bool is_enabled() const {
#ifdef ENABLE_SDL
    return enabled;
#else
    return ::IsWindowEnabled(hWnd);
#endif
  }

  /**
   * Specifies whether this window can get user input.
   */
  void set_enabled(bool enabled) {
    assert_thread();

#ifdef ENABLE_SDL
    if (enabled == this->enabled)
      return;

    this->enabled = enabled;
    invalidate();
#else
    ::EnableWindow(hWnd, enabled);
#endif
  }

#ifdef ENABLE_SDL

  virtual Window *get_focused_window();
  void set_focus();

  /**
   * Called by the parent window when this window loses focus, or when
   * one of its (indirect) child windows loses focus.  This method is
   * responsible for invoking on_killfocus().
   */
  virtual void ClearFocus();

#else /* !ENABLE_SDL */

  void set_focus() {
    assert_none_locked();
    assert_thread();

    ::SetFocus(hWnd);
  }

#endif /* !ENABLE_SDL */

  gcc_pure
  bool has_focus() const {
#ifdef ENABLE_SDL
    return focused;
#else
    return hWnd == ::GetFocus();
#endif
  }

#ifdef ENABLE_SDL
  void set_capture();
  void release_capture();
  virtual void clear_capture();
#else /* !ENABLE_SDL */

  void set_capture() {
    assert_none_locked();
    assert_thread();

    ::SetCapture(hWnd);
  }

  void release_capture() {
    assert_none_locked();
    assert_thread();

    ::ReleaseCapture();
  }

  WNDPROC set_wndproc(WNDPROC wndproc)
  {
    assert_thread();

    return (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)wndproc);
  }

  void set_userdata(void *value)
  {
    assert_thread();

    ::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)value);
  }
#endif /* !ENABLE_SDL */

  timer_t set_timer(unsigned id, unsigned ms)
  {
    assert_thread();

#ifdef ANDROID
    return new AndroidTimer(*this, ms);
#elif defined(ENABLE_SDL)
    return new SDLTimer(*this, ms);
#else
    ::SetTimer(hWnd, id, ms, NULL);
    return id;
#endif
  }

  void kill_timer(timer_t id)
  {
    assert_thread();

#ifdef ANDROID
    id->disable();
#elif defined(ENABLE_SDL)
    delete id;
#else
    ::KillTimer(hWnd, id);
#endif
  }

#ifdef ENABLE_SDL
  void to_screen(PixelRect &rc) const;
#endif

  /**
   * Returns the position on the screen.
   */
  gcc_pure
  const PixelRect get_screen_position() const
  {
    PixelRect rc;
#ifdef ENABLE_SDL
    rc = get_position();
    to_screen(rc);
#else
    ::GetWindowRect(hWnd, &rc);
#endif
    return rc;
  }

  /**
   * Returns the position within the parent window.
   */
  gcc_pure
  const PixelRect get_position() const
  {
    PixelRect rc;
#ifdef ENABLE_SDL
    rc.left = get_left();
    rc.top = get_top();
    rc.right = get_right();
    rc.bottom = get_bottom();
#else
    rc = get_screen_position();

    HWND parent = ::GetParent(hWnd);
    if (parent != NULL) {
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
#endif
    return rc;
  }

  gcc_pure
  const PixelRect get_client_rect() const
  {
    PixelRect rc;
#ifdef ENABLE_SDL
    rc.left = 0;
    rc.top = 0;
    rc.right = get_width();
    rc.bottom = get_height();
#else
    ::GetClientRect(hWnd, &rc);
#endif
    return rc;
  }

  gcc_pure
  bool in_client_rect(int x, int y) const {
    if (x < 0 || y < 0)
      return false;

    PixelRect rc = get_client_rect();
    return x < rc.right && y < rc.bottom;
  }

  gcc_pure
  const PixelSize get_size() const
  {
    PixelRect rc = get_client_rect();
    PixelSize s;
    s.cx = rc.right;
    s.cy = rc.bottom;
    return s;
  }

#ifdef ENABLE_SDL
  void setup(Canvas &canvas);

  virtual void invalidate();

  /**
   * Ensures that the window is updated on the physical screen.
   */
  virtual void expose();
#else /* !ENABLE_SDL */
  HDC BeginPaint(PAINTSTRUCT *ps) {
    assert_thread();

    return ::BeginPaint(hWnd, ps);
  }

  void EndPaint(PAINTSTRUCT *ps) {
    assert_thread();

    ::EndPaint(hWnd, ps);
  }

  void scroll(int dx, int dy, const PixelRect &rc) {
    ::ScrollWindowEx(hWnd, dx, dy, &rc, NULL, NULL, NULL, SW_INVALIDATE);
  }
#endif /* !ENABLE_SDL */

#ifndef ENABLE_SDL
  gcc_const
  static void *get_userdata(HWND hWnd) {
    return (void *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
  }

  /**
   * Converts a #HWND into a #Window pointer, without checking if that
   * is legal.
   */
  gcc_const
  static Window *get_unchecked(HWND hWnd) {
    return (Window *)get_userdata(hWnd);
  }

  /**
   * Converts a #HWND into a #Window pointer.  Returns NULL if the
   * HWND is not a Window peer.  This only works for windows which
   * have called install_wndproc().
   */
  gcc_const
  static Window *get(HWND hWnd) {
    WNDPROC wndproc = (WNDPROC)::GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    return wndproc == WndProc
#ifdef _WIN32_WCE
      /* Windows CE seems to put WNDPROC pointers into some other
         segment (0x22000000 added); this is a dirty workaround which
         will be implemented properly once we understand what this
         really means */
      || ((DWORD)wndproc & 0xffffff) == (DWORD)WndProc
#endif
      ? get_unchecked(hWnd)
      : NULL;
  }

  gcc_pure
  LONG get_window_long(int nIndex) const {
    return ::GetWindowLong(hWnd, nIndex);
  }

  LONG get_window_style() const {
    return get_window_long(GWL_STYLE);
  }

  LONG get_window_ex_style() const {
    return get_window_long(GWL_EXSTYLE);
  }
#endif

#ifdef ENABLE_SDL
  void send_user(unsigned id);
#else
  void send_user(unsigned id) {
    ::PostMessage(hWnd, WM_USER + id, (WPARAM)0, (LPARAM)0);
  }
#endif

#if defined(ENABLE_SDL) && !defined(ANDROID)
  void send_timer(SDLTimer *timer);
#endif

protected:
#ifdef ENABLE_SDL
public:
#endif /* ENABLE_SDL */
  /**
   * @return true on success, false if the window should not be
   * created
   */
  virtual bool on_create();

  virtual bool on_destroy();
  virtual bool on_close();
  virtual bool on_resize(unsigned width, unsigned height);
  virtual bool on_mouse_move(int x, int y, unsigned keys);
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);
  virtual bool on_mouse_double(int x, int y);
  virtual bool on_mouse_wheel(int delta);

  /**
   * Checks if the window wishes to handle a special key, like cursor
   * keys and tab.  This wraps the WIN32 message WM_GETDLGCODE.
   *
   * @return true if the window will handle they key, false if the
   * dialog manager may use it
   */
  gcc_pure
  virtual bool on_key_check(unsigned key_code) const;

  virtual bool on_key_down(unsigned key_code);
  virtual bool on_key_up(unsigned key_code);
  virtual bool on_command(unsigned id, unsigned code);
  virtual bool on_cancel_mode();
  virtual bool on_setfocus();
  virtual bool on_killfocus();
  virtual bool on_timer(timer_t id);
  virtual bool on_user(unsigned id);

  virtual bool on_erase(Canvas &canvas);
  virtual void on_paint(Canvas &canvas);
  virtual void on_paint(Canvas &canvas, const PixelRect &dirty);

#ifndef ENABLE_SDL
  /**
   * Called by on_message() when the message was not handled by any
   * virtual method.  Calls the default handler.  This function is
   * virtual, because the Dialog class will have to override it -
   * dialogs have slightly different semantics.
   */
  virtual LRESULT on_unhandled_message(HWND hWnd, UINT message,
                                       WPARAM wParam, LPARAM lParam);

  virtual LRESULT on_message(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);
#endif /* !ENABLE_SDL */

public:
#ifdef ENABLE_SDL
  void install_wndproc() {
    // XXX
  }
#else /* !ENABLE_SDL */
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
  void install_wndproc();
#endif /* !ENABLE_SDL */
};

#endif
