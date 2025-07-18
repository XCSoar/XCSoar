// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContainerWindow.hpp"

#ifndef USE_WINUSER
#include "custom/DoubleClick.hpp"
#endif

#ifdef ENABLE_OPENGL
#include "ui/opengl/Features.hpp"
#endif

#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR

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
#undef Font
#undef Window
#undef Display
#undef Expose
#undef KeyPress
#undef Below
#endif

#ifdef USE_WAYLAND
struct wl_egl_window;
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif

namespace UI {

class Display;

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
  UI::Display &display;

#ifdef USE_X11
  X11Window x_window;
#elif defined(USE_WAYLAND)
  struct wl_egl_window *native_window;
#elif defined(ENABLE_SDL)
  SDL_Window *window;
#endif
#ifdef DRAW_MOUSE_CURSOR
  uint8_t cursor_size = 1;
  bool invert_cursor_colors = false;
  std::chrono::steady_clock::time_point cursor_visible_until;
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
   * Does the Java #NativeView class have a surface?
   *
   * Protected by #paused_mutex.
   */
  bool have_java_surface = true;

  /**
   * Does the C++ #TopCanvas class have a surface?
   *
   * Protected by #paused_mutex.
   */
  bool have_native_surface = false;

  /**
   * Shall we destroy our EGL surface?  This will be done by the
   * #SURFACE_DESTROYED event.
   *
   * Protected by #paused_mutex.
   */
  bool should_release_surface = false;

  /**
   * Shall we acquire our EGL surface?  This will be done by the
   * #SURFACE_DESTROYED event.
   *
   * Protected by #paused_mutex.
   */
  bool should_acquire_surface = false;

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
#ifdef ANDROID
  explicit TopWindow(UI::Display &_display) noexcept;
#else
  explicit TopWindow(UI::Display &_display) noexcept
    :display(_display) {}
#endif

#ifndef USE_WINUSER
  ~TopWindow() noexcept override;
#endif

  auto &GetDisplay() const noexcept {
    return display;
  }

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
  void SetCaption(const TCHAR *) noexcept {}
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
  [[gnu::pure]]
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

  [[gnu::pure]]
  const PixelSize GetSize() const noexcept {
    /* this is implemented again because Window::get_size() would call
       Window::GetClientRect() (method is not virtual) */
    PixelRect rc = GetClientRect();
    return {rc.right, rc.bottom};
  }
#endif
    
#if defined(__APPLE__) && TARGET_OS_IPHONE
  [[gnu::pure]]
  const PixelRect GetClientRect() const noexcept override {
    assert(IsDefined());
    // Get safe area insets in points
    UIWindow *window = UIApplication.sharedApplication.windows.firstObject;
    UIEdgeInsets insets = window.safeAreaInsets;
    // Get screen scale factor (e.g. 2.0 or 3.0 depending on resolution)
    CGFloat scale = [UIScreen mainScreen].scale;
    insets.top *= scale;
    insets.left *= scale;
    insets.bottom *= scale;
    insets.right *= scale;

    PixelSize size = GetSize();
    return PixelRect(
        insets.left,
        insets.top,
        size.width - insets.left,
        size.height - insets.top
    );
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
  [[gnu::pure]]
  bool IsVisible() const noexcept;
#endif

#ifdef ANDROID
  /**
   * Set the "running" flag, which is used by Pause().
   */
  void BeginRunning() noexcept;

  /**
   * Clear the "running" flag, which is used by Pause().
   */
  void EndRunning() noexcept;

  /**
   * The Android OpenGL surface has been resized; notify the TopWindow
   * that this has happened.  The caller should also submit the RESIZE
   * event to the event queue.  This method is thread-safe.
   */
  void AnnounceResize(PixelSize _new_size) noexcept;

  bool ResumeSurface() noexcept;

  /**
   * Synchronously update the size of the TopWindow to the new OpenGL
   * surface dimensions.
   */
  void RefreshSize() noexcept;
#else
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

#ifdef DRAW_MOUSE_CURSOR
private:
  void DrawMouseCursor(Canvas &canvas) noexcept;
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
  void OnDestroy() noexcept override;
#endif

#ifdef USE_WINUSER
  LRESULT OnMessage(HWND _hWnd, UINT message,
                    WPARAM wParam, LPARAM lParam) noexcept override;
#endif

#ifndef USE_WINUSER
  void OnResize(PixelSize new_size) noexcept override;
#endif

#ifdef ANDROID
  virtual void OnLook() noexcept {}

  /**
   * @see Event::SURFACE
   */
  void OnSurface() noexcept;

  virtual void OnTaskReceived() noexcept {}

  /**
   * @see Event::PAUSE
   */
  void OnPause() noexcept;

  /**
   * @see Event::RESUME
   */
  void OnResume() noexcept;

public:
  void InvokeSurfaceDestroyed() noexcept;
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
