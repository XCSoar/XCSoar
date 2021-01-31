/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ContainerWindow.hpp"

#ifndef USE_WINUSER
#include "custom/DoubleClick.hpp"
#endif

#ifdef ENABLE_OPENGL
#include "Screen/Features.hpp"
#endif

#ifdef ANDROID
#include "thread/Mutex.hxx"
#include "thread/Cond.hxx"

namespace UI { struct Event; }

#elif defined(USE_POLL_EVENT)
namespace UI { struct Event; }
#elif defined(ENABLE_SDL)
union SDL_Event;
struct SDL_Window;
#endif

#include <tchar.h>

#ifdef SOFTWARE_ROTATE_DISPLAY
#include "DisplayOrientation.hpp"
#endif

#ifndef USE_WINUSER
class TopCanvas;
#endif

#ifdef USE_X11
#define Font X11Font
#define Window X11Window
#define Display X11Display
#include <X11/X.h>
#ifdef USE_GLX
#include <GL/glx.h>
#undef NoValue
#endif
#undef Font
#undef Window
#undef Display
#undef Expose
#undef KeyPress
struct _XDisplay;
#endif

#ifdef USE_WAYLAND
struct wl_display;
struct wl_egl_window;
#endif

namespace UI {

class TopWindowStyle : public WindowStyle {
#if defined(ENABLE_SDL) || defined(USE_X11)
  bool full_screen = false;
#endif
#ifdef ENABLE_SDL
  bool resizable = false;
#endif
#ifdef SOFTWARE_ROTATE_DISPLAY
  DisplayOrientation initial_orientation = DisplayOrientation::DEFAULT;
#endif

public:
  TopWindowStyle()
  {
    Popup();
  }

  TopWindowStyle(const WindowStyle other)
    :WindowStyle(other)
  {
    Popup();
  }

  void FullScreen() {
#if defined(ENABLE_SDL) || defined(USE_X11)
    full_screen = true;
#endif
  }

  bool GetFullScreen() const {
#if defined(ENABLE_SDL) || defined(USE_X11)
    return full_screen;
#else
    return false;
#endif
  }

  void Resizable() {
#ifdef ENABLE_SDL
    resizable = true;
#elif defined(USE_WINUSER)
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
#ifdef SOFTWARE_ROTATE_DISPLAY
  void InitialOrientation(DisplayOrientation orientation) {
    initial_orientation = orientation;
  }

  DisplayOrientation GetInitialOrientation() const {
    return initial_orientation;
  }
#endif
};

/**
 * A top-level full-screen window.
 */
class TopWindow : public ContainerWindow {
#ifdef USE_X11
  _XDisplay *x_display;
  X11Window x_window;
#ifdef USE_GLX
  GLXFBConfig *fb_cfg;
#endif
#elif defined(USE_WAYLAND)
  struct wl_display *native_display;
  struct wl_egl_window *native_window;
#elif defined(ENABLE_SDL)
  SDL_Window *window;
#endif
#ifdef DRAW_MOUSE_CURSOR
  uint8_t cursor_size = 1;
  bool invert_cursor_colors = false;
#endif

#ifndef USE_WINUSER
  TopCanvas *screen = nullptr;

  bool invalidated;

#ifdef ANDROID
  Mutex paused_mutex;
  Cond paused_cond;

  /**
   * Is the main (UI) thread currently inside RunEventLoop()?  If not,
   * then the Android Activity thread should not wait for
   * #paused_cond, to avoid deadlocks.
   */
  bool running = false;

  /**
   * Is the application currently paused?  While this flag is set, no
   * OpenGL operations are allowed, because the OpenGL surface does
   * not exist.
   */
  bool paused = false;

  /**
   * Has the application been resumed?  When this flag is set,
   * TopWindow::Expose() attempts to reinitialize the OpenGL surface.
   */
  bool resumed = false;

  /**
   * Was the application view resized while paused?  If true, then
   * new_size contains the new display dimensions.
   */
  bool resized = false;

  PixelSize new_size;
#endif

  DoubleClick double_click;

#else /* USE_WINUSER */

  /**
   * On WM_ACTIVATE, the focus is returned to this window.
   */
  HWND hSavedFocus;

#endif /* USE_WINUSER */

#ifdef HAVE_HIGHDPI_SUPPORT
  float point_to_real_x = 1, point_to_real_y = 1;
#endif

public:
#ifndef USE_WINUSER
  ~TopWindow() noexcept override;
#endif

  /**
   * Throws on error.
   */
#ifdef USE_WINUSER
  void Create(const TCHAR *cls, const TCHAR *text, PixelSize size,
              TopWindowStyle style=TopWindowStyle());
#else
  void Create(const TCHAR *text, PixelSize size,
              TopWindowStyle style=TopWindowStyle());
#endif

#if defined(USE_X11) || defined(USE_WAYLAND) || defined(ENABLE_SDL)
private:
  /**
   * Throws on error.
   */
  void CreateNative(const TCHAR *text, PixelSize size,
                    TopWindowStyle style);

public:
#endif

  /**
   * Check if the screen has been resized.
   */
#ifdef USE_FB
  void CheckResize() noexcept;
#else
  void CheckResize() noexcept {}
#endif

#if !defined(USE_WINUSER) && !defined(ENABLE_SDL)
#if defined(ANDROID) || defined(USE_FB) || defined(USE_EGL) || defined(USE_GLX) || defined(USE_VFB)
  void SetCaption(gcc_unused const TCHAR *caption) noexcept {}
#else
  void SetCaption(const TCHAR *caption) noexcept;
#endif
#endif

  /**
   * Triggers an OnCancelMode() call on the focused #Window and/or the
   * #Window currently capturing the mouse.
   */
  void CancelMode() noexcept;

#if defined(USE_WINUSER)
  gcc_pure
  const PixelRect GetClientRect() const noexcept {
    if (::IsIconic(hWnd)) {
      /* for a minimized window, GetClientRect() returns the
         dimensions of the icon, which is not what we want */
      WINDOWPLACEMENT placement;
      if (::GetWindowPlacement(hWnd, &placement) &&
          (placement.showCmd == SW_MINIMIZE ||
           placement.showCmd == SW_SHOWMINIMIZED)) {
        const auto &r = placement.rcNormalPosition;
        return PixelRect(0, 0, r.right - r.left, r.bottom - r.top);
      }
    }

    return ContainerWindow::GetClientRect();
  }

  gcc_pure
  const PixelSize GetSize() const noexcept {
    /* this is implemented again because Window::get_size() would call
       Window::GetClientRect() (method is not virtual) */
    PixelRect rc = GetClientRect();
    return {rc.right, rc.bottom};
  }
#endif

#ifndef USE_WINUSER
  void Invalidate() noexcept override;

protected:
  void Expose() noexcept;

#if defined(USE_X11) || defined(USE_WAYLAND)
  void EnableCapture() noexcept override;
  void DisableCapture() noexcept override;
#endif

public:
#endif /* !USE_WINUSER */

  /**
   * Synchronously refresh the screen by handling all pending repaint
   * requests.
   */
  void Refresh() noexcept;

  void Close() noexcept {
#ifndef USE_WINUSER
    OnClose();
#else
    ::SendMessage(hWnd, WM_CLOSE, 0, 0);
#endif
  }

#if defined(ANDROID) || defined(USE_POLL_EVENT)
  bool OnEvent(const Event &event);
#elif defined(ENABLE_SDL)
  bool OnEvent(const SDL_Event &event);
#endif

#if defined(USE_X11) || defined(USE_WAYLAND)
  gcc_pure
  bool IsVisible() const noexcept;
#endif

#ifdef ANDROID
  /**
   * The Android OpenGL surface has been resized; notify the TopWindow
   * that this has happened.  The caller should also submit the RESIZE
   * event to the event queue.  This method is thread-safe.
   */
  void AnnounceResize(PixelSize _new_size) noexcept;

  bool ResumeSurface() noexcept;

  /**
   * Reinitialise the OpenGL surface if the Android Activity has been
   * resumed.
   *
   * @return true if there is a valid OpenGL surface
   */
  bool CheckResumeSurface() noexcept;

  /**
   * Synchronously update the size of the TopWindow to the new OpenGL
   * surface dimensions.
   */
  void RefreshSize() noexcept;
#else
  bool CheckResumeSurface() noexcept {
    return true;
  }

  void RefreshSize() noexcept {}
#endif

#ifdef SOFTWARE_ROTATE_DISPLAY
  void SetDisplayOrientation(DisplayOrientation orientation) noexcept;
#endif

#ifdef DRAW_MOUSE_CURSOR
  void SetCursorSize(const uint8_t &cursorSize) noexcept {
    cursor_size = cursorSize;
  }

  void SetCursorColorsInverted(bool inverted) {
    invert_cursor_colors = inverted;
  }
#endif


protected:
  PixelPoint PointToReal(PixelPoint p) const noexcept {
#ifdef HAVE_HIGHDPI_SUPPORT
    p.x = int(static_cast<float>(p.x) * point_to_real_x);
    p.y = int(static_cast<float>(p.y) * point_to_real_y);
#endif
    return p;
  }

protected:
  virtual bool OnActivate() noexcept;
  virtual bool OnDeactivate() noexcept;

  virtual bool OnClose() noexcept;

#ifdef KOBO
  void OnDestroy() override;
#endif

#ifdef DRAW_MOUSE_CURSOR
  void OnPaint(Canvas &canvas) override;
#endif

#ifdef USE_WINUSER
  LRESULT OnMessage(HWND _hWnd, UINT message,
                    WPARAM wParam, LPARAM lParam) noexcept override;
#endif

#ifndef USE_WINUSER
  void OnResize(PixelSize new_size) override;
#endif

#ifdef ANDROID
  /**
   * @see Event::PAUSE
   */
  virtual void OnPause() noexcept;

  /**
   * @see Event::RESUME
   */
  virtual void OnResume() noexcept;

public:
  void Pause() noexcept;
  void Resume() noexcept;
#endif

public:
  void PostQuit() noexcept;

  /**
   * Runs the event loop until the application quits.
   */
  int RunEventLoop() noexcept;
};

} // namespace UI

#endif
