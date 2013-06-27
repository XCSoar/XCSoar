/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Screen/Custom/DoubleClick.hpp"
#endif

#ifdef ANDROID
#include "Thread/Mutex.hpp"
#include "Thread/Cond.hpp"

struct Event;

#elif defined(USE_CONSOLE)
struct Event;
#elif defined(ENABLE_SDL)
union SDL_Event;
#endif

#include <tchar.h>

#ifndef USE_GDI
class TopCanvas;
#endif

class TopWindowStyle : public WindowStyle {
#ifdef ENABLE_SDL
  bool full_screen;
  bool resizable;
#endif

public:
  TopWindowStyle()
#ifdef ENABLE_SDL
    :full_screen(false), resizable(false)
#endif
  {
    Popup();
  }

  TopWindowStyle(const WindowStyle other)
    :WindowStyle(other)
#ifdef ENABLE_SDL
    , full_screen(false), resizable(false)
#endif
  {
    Popup();
  }

  void FullScreen() {
#ifdef ENABLE_SDL
    full_screen = true;
#endif
  }

  bool GetFullScreen() const {
#ifdef ENABLE_SDL
    return full_screen;
#else
    return false;
#endif
  }

  void Resizable() {
#ifdef ENABLE_SDL
    resizable = true;
#elif defined(USE_GDI)
    style &= ~WS_BORDER;
    style |= WS_THICKFRAME;
#endif
  }

  bool GetResizable() const {
#ifdef ENABLE_SDL
    return resizable;
#else
    return false;
#endif
  }
};

/**
 * A top-level full-screen window.
 */
class TopWindow : public ContainerWindow {
#ifndef USE_GDI
  TopCanvas *screen;

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
   * TopWindow::Expose() attempts to reinitialize the OpenGL surface.
   */
  bool resumed;

  /**
   * Was the application view resized while paused?  If true, then
   * new_width and new_height contain the new display dimensions.
   */
  bool resized;

  UPixelScalar new_width, new_height;
#endif

  DoubleClick double_click;

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
#ifdef ANDROID
  TopWindow():screen(nullptr), paused(false), resumed(false), resized(false) {}
#elif !defined(USE_GDI)
  TopWindow():screen(nullptr) {}
#endif

#ifndef USE_GDI
  virtual ~TopWindow();
#endif

#ifdef USE_GDI
  static bool find(const TCHAR *cls, const TCHAR *text);

  void Create(const TCHAR *cls, const TCHAR *text, PixelSize size,
              TopWindowStyle style=TopWindowStyle());
#else
  void Create(const TCHAR *text, PixelSize size,
              TopWindowStyle style=TopWindowStyle());
#endif

#ifdef _WIN32_WCE
  void Destroy();
#endif

#ifndef USE_GDI
#ifdef ANDROID
  void SetCaption(gcc_unused const TCHAR *caption) {}
#else
  void SetCaption(const TCHAR *caption);
#endif
#endif

  /**
   * Triggers an OnCancelMode() call on the focused #Window and/or the
   * #Window currently capturing the mouse.
   */
  void CancelMode();

#if defined(USE_GDI) && !defined(_WIN32_WCE)
  gcc_pure
  const PixelRect GetClientRect() const {
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
        return reinterpret_cast<const PixelRect &>(placement.rcNormalPosition);
      }
    }

    return ContainerWindow::GetClientRect();
  }

  gcc_pure
  const PixelSize GetSize() const {
    /* this is implemented again because Window::get_size() would call
       Window::GetClientRect() (method is not virtual) */
    PixelRect rc = GetClientRect();
    PixelSize s;
    s.cx = rc.right;
    s.cy = rc.bottom;
    return s;
  }
#endif

  void Fullscreen();

#ifndef USE_GDI
  virtual void Invalidate() override;

protected:
  void Expose();

public:
#endif /* !USE_GDI */

  /**
   * Synchronously refresh the screen by handling all pending repaint
   * requests.
   */
  void Refresh();

  void Close() {
    AssertNoneLocked();

#ifndef USE_GDI
    OnClose();
#else
    ::SendMessage(hWnd, WM_CLOSE, 0, 0);
#endif
  }

#if defined(ANDROID) || defined(USE_CONSOLE)
  bool OnEvent(const Event &event);
#elif defined(ENABLE_SDL)
  bool OnEvent(const SDL_Event &event);
#endif

#ifdef ANDROID
  /**
   * The Android OpenGL surface has been resized; notify the TopWindow
   * that this has happened.  The caller should also submit the RESIZE
   * event to the event queue.  This method is thread-safe.
   */
  void AnnounceResize(UPixelScalar width, UPixelScalar height);

  bool ResumeSurface();

  /**
   * Reinitialise the OpenGL surface if the Android Activity has been
   * resumed.
   *
   * @return true if there is a valid OpenGL surface
   */
  bool CheckResumeSurface();

  /**
   * Synchronously update the size of the TopWindow to the new OpenGL
   * surface dimensions.
   */
  void RefreshSize();
#else
  bool CheckResumeSurface() {
    return true;
  }

  void RefreshSize() {}
#endif

protected:
  virtual bool OnActivate();
  virtual bool OnDeactivate();

  virtual bool OnClose();

#ifdef KOBO
  virtual void OnDestroy() override;
#endif

#ifdef DRAW_MOUSE_CURSOR
  virtual void OnPaint(Canvas &canvas) override;
#endif

#ifdef USE_GDI
  virtual LRESULT OnMessage(HWND _hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam) override;
#endif

#ifndef USE_GDI
  virtual void OnResize(PixelSize new_size) override;
#endif

#ifdef ANDROID
  /**
   * @see Event::PAUSE
   */
  virtual void OnPause();

  /**
   * @see Event::RESUME
   */
  virtual void OnResume();

public:
  void Pause();
  void Resume();
#endif

public:
  void PostQuit();

  /**
   * Runs the event loop until the application quits.
   */
  int RunEventLoop();
};

#endif
