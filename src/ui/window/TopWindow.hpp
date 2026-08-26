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

#if defined(ENABLE_OPENGL) || defined(ENABLE_SDL) || \
  defined(DRAW_REDRAW_COUNTER)
#include <cstdint>
#endif

#ifdef DRAW_REDRAW_COUNTER
#include <chrono>
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
struct wl_surface;
struct xdg_surface;
struct xdg_toplevel;
struct zxdg_toplevel_decoration_v1;
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
#import <UIKit/UIKit.h>

#include <algorithm>
#include <cmath>
#endif

namespace UI {

class Display;

class TopWindowStyle : public WindowStyle {
#if defined(ENABLE_SDL) || defined(USE_X11) || defined(USE_WAYLAND)
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
#if defined(ENABLE_SDL) || defined(USE_X11) || defined(USE_WAYLAND)
    full_screen = true;
#endif
  }

  bool GetFullScreen() const {
#if defined(ENABLE_SDL) || defined(USE_X11) || defined(USE_WAYLAND)
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
  struct wl_surface *wl_surface = nullptr;
  struct wl_egl_window *native_window;
  struct xdg_surface *xdg_surface = nullptr;
  struct xdg_toplevel *xdg_toplevel = nullptr;
  struct zxdg_toplevel_decoration_v1 *xdg_decoration = nullptr;
  PixelSize initial_requested_size{0, 0};
  std::chrono::steady_clock::time_point last_resize_flush_time;

private:
  bool received_first_configure = false;

public:
  void MarkFirstConfigureReceived() noexcept {
    received_first_configure = true;
  }

  void OnNativeConfigure(PixelSize new_native_size) noexcept;
#elif defined(ENABLE_SDL)
  SDL_Window *window;
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
  /**
   * Shall XCSoar use the whole screen, including the areas covered by
   * the status bar, the display cutout ("notch") and the home
   * indicator?  This is the iOS equivalent of Android's immersive
   * mode.
   *
   * The initial value must match the default of
   * DisplaySettings::full_screen, because the window is created before
   * the profile is loaded.  On iOS, full-screen mode is opt-in.
   *
   * @see SetFullScreenMode()
   */
  bool full_screen_mode = false;

  /**
   * Shall the iOS status bar be hidden?  This is independent of
   * #full_screen_mode: the window covers the whole screen either way,
   * and the status bar merely overlays its top edge.
   *
   * The initial value must match what DisplaySettings::SetDefaults()
   * resolves to, because the window is created before the profile has
   * been loaded.
   *
   * @see SetStatusBarHidden()
   */
  bool status_bar_hidden = false;

  /**
   * Pass the current values of #full_screen_mode and
   * #status_bar_hidden on to iOS.
   */
  void ApplyFullScreenMode() noexcept;
#endif

#ifdef DRAW_MOUSE_CURSOR
  uint8_t cursor_size = 1;
  bool invert_cursor_colors = false;
  std::chrono::steady_clock::time_point cursor_visible_until;
#endif

#ifdef DRAW_REDRAW_COUNTER
  uint64_t redraw_count = 0;
  unsigned hz_window_frames = 0;
  double redraw_hz = 0;
  std::chrono::steady_clock::time_point hz_window_start{};
#endif

#ifndef USE_WINUSER
  TopCanvas *screen = nullptr;

  bool invalidated;

#ifdef ENABLE_OPENGL
  uint32_t render_state_token = 0;
#endif

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

  struct SafeAreaInsets {
    unsigned left = 0, top = 0, right = 0, bottom = 0;
  };

  /**
   * The area covered by system UI (status bar, navigation bar,
   * display cutout), as reported by Java via
   * NativeView.resizedNative().
   *
   * Protected by #paused_mutex.
   *
   * @see PublishSafeAreaInsets()
   */
  SafeAreaInsets pending_safe_area_insets;

  /**
   * The insets the main thread works with.  Only #PublishSafeAreaInsets()
   * writes it, so readers need no lock.
   *
   * @see GetSafeAreaRect()
   */
  SafeAreaInsets safe_area_insets;
#endif

  DoubleClick double_click;

#if defined(ENABLE_SDL) && defined(HAVE_MULTI_TOUCH)
  /**
   * Number of fingers currently touching the screen.
   */
  unsigned touch_fingers = 0;

  /**
   * Were two or more fingers down during the current touch sequence?
   */
  bool touch_multi = false;

  /**
   * Stable SDL finger ids for the active two-finger gesture.  Indices
   * into SDL's finger array are not stable across up/down events.
   */
  bool touch_pair_valid = false;
  std::int64_t touch_finger_a = 0;
  std::int64_t touch_finger_b = 0;

  /**
   * Is an emulated mouse button release waiting for the last finger to
   * be lifted?
   */
  bool touch_mouse_up_pending = false;

  PixelPoint touch_mouse_up_point{0, 0};

  /**
   * Deliver a postponed emulated mouse button release, if any.
   */
  bool FlushTouchMouseUp() noexcept;
#endif

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
#ifdef ENABLE_OPENGL
  [[gnu::pure]]
  uint32_t GetRenderStateToken() const noexcept {
    return render_state_token;
  }
#endif

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
  void Create(const char *cls, const char *text, PixelSize size,
              TopWindowStyle style=TopWindowStyle());
#else
  void Create(const char *text, PixelSize size,
              TopWindowStyle style=TopWindowStyle());
#endif

#if defined(USE_X11) || defined(USE_WAYLAND) || defined(ENABLE_SDL)
private:
  /**
   * Throws on error.
   */
  void CreateNative(const char *text, PixelSize size,
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
  void SetCaption(const char *) noexcept {}
#else
  void SetCaption(const char *caption) noexcept;
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
  /**
   * Enable or disable full-screen mode: hide the status bar, defer the
   * system edge gestures, and let XCSoar draw into the whole screen,
   * including the areas behind the display cutout ("notch") and the
   * home indicator.  This is the iOS equivalent of Android's immersive
   * mode.
   *
   * Unlike on Android, the window keeps its size; only the area
   * returned by GetClientRect() changes, and no resize event is
   * generated.
   *
   * @see DisplaySettings::full_screen
   */
  void SetFullScreenMode(bool _full_screen) noexcept;

  /**
   * Show or hide the iOS status bar.  This is independent of full
   * screen mode.
   */
  void SetStatusBarHidden(bool _hidden) noexcept;

  bool GetStatusBarHidden() const noexcept {
    return status_bar_hidden;
  }

  bool GetFullScreenMode() const noexcept {
    return full_screen_mode;
  }

  /**
   * The whole screen, including the areas covered by the status bar,
   * the display cutout ("notch") and the home indicator.
   */
  [[gnu::pure]]
  const PixelRect GetScreenRect() const noexcept {
    /* Start from this window's real size (which was derived from the
       OpenGL drawable) instead of computing the screen size from
       UIScreen again: two independent conversions from points to
       pixels can be rounded differently, and a client rect which is
       one pixel smaller than the framebuffer makes full-screen dialogs
       look non-maximised. */
    return ContainerWindow::GetClientRect();
  }

  /**
   * Like GetSafeAreaRect(), but for a window size that has not been
   * applied yet, e.g. from inside OnResize().  @p size is the size of
   * the whole window, not of its client area.
   */
  [[gnu::pure]]
  const PixelRect GetSafeAreaRect(PixelSize size) const noexcept {
    const PixelRect safe = GetSafeAreaRect();
    const PixelRect rc{size};

    return PixelRect(std::max(rc.left, safe.left),
                     std::max(rc.top, safe.top),
                     std::min(rc.right, safe.right),
                     std::min(rc.bottom, safe.bottom));
  }

  [[gnu::pure]]
  const PixelRect GetSafeAreaRect() const noexcept {
    PixelRect rc = GetScreenRect();

    UIWindow *window = UIApplication.sharedApplication.windows.firstObject;
    if (window == nullptr)
      /* the window is not available yet */
      return rc;

    /* Get the scale factor of the screen this window is on.  We need
       nativeScale instead of scale to correctly account for
       downsampling on mini and Plus devices; it is also the scale the
       window size was derived from. */
    const CGFloat scale = window.screen.nativeScale;

    /* The safe area is expressed in points.  Round the scaled insets
       instead of truncating them, which would bias them towards being
       too small. */
    const UIEdgeInsets insets = window.safeAreaInsets;

    rc.left += (int)std::lround(insets.left * scale);
    rc.top += (int)std::lround(insets.top * scale);
    rc.right -= (int)std::lround(insets.right * scale);
    rc.bottom -= (int)std::lround(insets.bottom * scale);

    return rc;
  }

  [[gnu::pure]]
  const PixelRect GetClientRect() const noexcept override {
    assert(IsDefined());

    /* full-screen mode draws edge-to-edge, i.e. also behind the
       status bar, the display cutout and the home indicator */
    return full_screen_mode ? GetScreenRect() : GetSafeAreaRect();
  }
#endif

#if !(defined(__APPLE__) && TARGET_OS_IPHONE)
  /**
   * The part of the window that is not covered by system UI such as
   * the status bar, the navigation bar, the display cutout ("notch")
   * or the home indicator.  Equals GetClientRect() on platforms and
   * devices without such areas.
   */
  [[gnu::pure]]
  PixelRect GetSafeAreaRect() const noexcept {
    PixelRect rc = GetClientRect();

#ifdef ANDROID
    rc.left += int(safe_area_insets.left);
    rc.top += int(safe_area_insets.top);
    rc.right -= int(safe_area_insets.right);
    rc.bottom -= int(safe_area_insets.bottom);
#endif

    return rc;
  }

  /**
   * Like GetSafeAreaRect(), but for a window size that has not been
   * applied yet, e.g. from inside OnResize().
   */
  [[gnu::pure]]
  PixelRect GetSafeAreaRect(PixelSize size) const noexcept {
    PixelRect rc{size};

#ifdef ANDROID
    rc.left += int(safe_area_insets.left);
    rc.top += int(safe_area_insets.top);
    rc.right -= int(safe_area_insets.right);
    rc.bottom -= int(safe_area_insets.bottom);
#endif

    return rc;
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

  /**
   * The area covered by system UI has changed.  Called from the
   * Android UI thread right before the RESIZE event is submitted;
   * #PublishSafeAreaInsets() then hands the values to the main
   * thread.  This method is thread-safe.
   */
  void AnnounceSafeAreaInsets(unsigned left, unsigned top,
                              unsigned right, unsigned bottom) noexcept;

  /**
   * Copy the insets announced by the Android UI thread to the main
   * thread.  Called while handling the RESIZE event, before anything
   * is laid out or drawn with them.
   */
  void PublishSafeAreaInsets() noexcept;

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

#ifdef DRAW_REDRAW_COUNTER
private:
  void DrawRedrawCounter(Canvas &canvas) noexcept;
#endif

protected:
#ifdef ENABLE_OPENGL
  void BumpRenderStateToken() noexcept {
    ++render_state_token;
  }
#endif

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
