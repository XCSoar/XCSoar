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

#ifndef XCSOAR_SCREEN_TOP_WINDOW_HXX
#define XCSOAR_SCREEN_TOP_WINDOW_HXX

#include "Screen/ContainerWindow.hpp"

#ifdef HAVE_AYGSHELL_DLL
#include "OS/AYGShellDLL.hpp"
#endif

#ifndef USE_GDI
#include "Thread/Mutex.hpp"
#include "Screen/SDL/TopCanvas.hpp"
#endif

#ifdef ANDROID
#include "Thread/Mutex.hpp"
#include "Thread/Cond.hpp"

struct Event;
#endif

/**
 * A top-level full-screen window.
 */
class TopWindow : public ContainerWindow {
#ifndef USE_GDI
  TopCanvas screen;

  Mutex invalidated_lock;
  bool invalidated;

#ifdef ANDROID
  Mutex paused_mutex;
  Cond paused_cond;

  /**
   * Is the application currently paused?  While this flag is set, no
   * OpenGL operations are allowed, because the OpenGL surface does
   * not exist.
   */
  bool paused;

  /**
   * Has the application been resumed?  When this flag is set,
   * TopWindow::expose() attempts to reinitialize the OpenGL surface.
   */
  bool resumed;

  /**
   * Was the application view resized while paused?  If true, then
   * new_width and new_height contain the new display dimensions.
   */
  bool resized;

  unsigned new_width, new_height;
#endif

#else /* USE_GDI */

#ifdef _WIN32_WCE
  /**
   * A handle to the task bar that was manually hidden.  This is a
   * hack when aygshell.dll is not available (Windows CE Core).
   */
  HWND task_bar;
#endif

  /**
   * On WM_ACTIVATE, the focus is returned to this window.
   */
  HWND hSavedFocus;

#ifdef HAVE_AYGSHELL_DLL
  SHACTIVATEINFO s_sai;
#endif
#endif /* USE_GDI */

public:
#ifdef HAVE_AYGSHELL_DLL
  const AYGShellDLL ayg_shell_dll;
#endif

public:
  TopWindow();

  static bool find(const TCHAR *cls, const TCHAR *text);

  void set(const TCHAR *cls, const TCHAR *text,
           int left, int top, unsigned width, unsigned height);

#ifdef _WIN32_WCE
  void reset();
#endif

#if defined(USE_GDI) && !defined(_WIN32_WCE)
  gcc_pure
  const PixelRect get_client_rect() const {
    if (::IsIconic(hWnd)) {
      /* for a minimized window, GetClientRect() returns the
         dimensions of the icon, which is not what we want */
      WINDOWPLACEMENT placement;
      if (::GetWindowPlacement(hWnd, &placement) &&
          (placement.showCmd == SW_MINIMIZE ||
           placement.showCmd == SW_SHOWMINIMIZED)) {
        placement.rcNormalPosition.right -= placement.rcNormalPosition.left;
        placement.rcNormalPosition.bottom -= placement.rcNormalPosition.top;
        placement.rcNormalPosition.left = 0;
        placement.rcNormalPosition.top = 0;
        return placement.rcNormalPosition;
      }
    }

    return ContainerWindow::get_client_rect();
  }

  gcc_pure
  const PixelSize get_size() const {
    /* this is implemented again because Window::get_size() would call
       Window::get_client_rect() (method is not virtual) */
    PixelRect rc = get_client_rect();
    PixelSize s;
    s.cx = rc.right;
    s.cy = rc.bottom;
    return s;
  }
#endif

  void full_screen();

#ifndef USE_GDI
  virtual void invalidate();

  virtual void expose();
#endif /* !USE_GDI */

  /**
   * Synchronously refresh the screen by handling all pending repaint
   * requests.
   */
  void refresh();

  void close() {
    assert_none_locked();

#ifndef USE_GDI
    on_close();
#else /* ENABLE_SDL */
    ::SendMessage(hWnd, WM_CLOSE, 0, 0);
#endif
  }

#ifdef ANDROID
  bool on_event(const Event &event);
#elif defined(ENABLE_SDL)
  bool on_event(const SDL_Event &event);
#endif

protected:
  virtual bool on_activate();
  virtual bool on_deactivate();

#ifdef ENABLE_SDL
  virtual bool on_close();
#else
  virtual LRESULT on_message(HWND _hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);
#endif /* !ENABLE_SDL */

#ifdef ANDROID
  virtual bool on_resize(unsigned width, unsigned height);

  /**
   * @see Event::PAUSE
   */
  virtual void on_pause();

  /**
   * @see Event::RESUME
   */
  virtual void on_resume();

public:
  void pause();
  void resume();
#endif

public:
  void post_quit();

  /**
   * Runs the event loop until the application quits.
   */
  int event_loop();
};

#endif
