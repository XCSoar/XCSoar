// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "../Features.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "util/UTF8.hpp"

#include <SDL_video.h>
#include <SDL_events.h>
#include <SDL_version.h>

#if defined(ENABLE_OPENGL) && defined(SOFTWARE_ROTATE_DISPLAY)
#include "ui/event/shared/TransformCoordinates.hpp"
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__MACOSX__) && __MACOSX__
#include <SDL_syswm.h>
#import <AppKit/AppKit.h>
#include <alloca.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
#include "UtilsSystem.hpp"
#endif

namespace UI {

#ifdef HAVE_MULTI_TOUCH

/**
 * Does this touch device report window coordinates?  SDL also emits
 * finger events for touchpads, whose normalised coordinates describe a
 * position on the pad, not on the screen; two-finger scrolling on a
 * touchpad must not be mistaken for a pinch gesture.
 */
[[gnu::pure]]
static bool
IsTouchScreen([[maybe_unused]] SDL_TouchID touch_id) noexcept
{
#if SDL_VERSION_ATLEAST(2, 0, 10)
  return SDL_GetTouchDeviceType(touch_id) == SDL_TOUCH_DEVICE_DIRECT;
#else
  return true;
#endif
}

/**
 * Count the fingers currently touching @p touch_id.  Depending on the
 * SDL version, the finger that triggered a SDL_FINGERUP event may still
 * be listed; pass its id in @p lifted to exclude it.
 *
 * Not [[gnu::pure]]: SDL touch state can change between identical calls.
 */
static unsigned
CountFingers(SDL_TouchID touch_id, const SDL_FingerID *lifted) noexcept
{
  const int n = SDL_GetNumTouchFingers(touch_id);
  if (n <= 0)
    return 0;

  unsigned count = unsigned(n);

  if (lifted != nullptr)
    for (int i = 0; i < n; ++i) {
      const SDL_Finger *f = SDL_GetTouchFinger(touch_id, i);
      if (f != nullptr && f->id == *lifted) {
        --count;
        break;
      }
    }

  return count;
}

/**
 * Count the fingers currently touching any direct (touch-screen)
 * device.  The emulated mouse events do not carry a touch id, so the
 * total across devices is used to decide when the last finger has been
 * lifted.
 */
static unsigned
CountAllFingers() noexcept
{
  unsigned count = 0;
  const int devices = SDL_GetNumTouchDevices();
  for (int i = 0; i < devices; ++i) {
    const SDL_TouchID id = SDL_GetTouchDevice(i);
    if (IsTouchScreen(id))
      count += CountFingers(id, nullptr);
  }

  return count;
}

/**
 * Capture the two active finger ids for a new two-finger gesture.
 * Prefer stable #SDL_FingerID values over array indices; SDL may
 * compact its finger list when another finger is added or removed.
 */
static bool
CaptureTwoFingerIds(SDL_TouchID touch_id,
                    SDL_FingerID &id_a, SDL_FingerID &id_b) noexcept
{
  if (SDL_GetNumTouchFingers(touch_id) < 2)
    return false;

  const SDL_Finger *f0 = SDL_GetTouchFinger(touch_id, 0);
  const SDL_Finger *f1 = SDL_GetTouchFinger(touch_id, 1);
  if (f0 == nullptr || f1 == nullptr)
    return false;

  id_a = f0->id;
  id_b = f1->id;
  return true;
}

/**
 * Look up one finger by id into window-pixel coordinates (not yet
 * HiDPI-scaled).
 */
static bool
GetFingerPoint(SDL_Window *window, SDL_TouchID touch_id,
               SDL_FingerID finger_id, PixelPoint &p) noexcept
{
  if (window == nullptr)
    return false;

  const int n = SDL_GetNumTouchFingers(touch_id);
  for (int i = 0; i < n; ++i) {
    const SDL_Finger *f = SDL_GetTouchFinger(touch_id, i);
    if (f == nullptr || f->id != finger_id)
      continue;

    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    if (w <= 0 || h <= 0)
      return false;

    p = {int(f->x * w), int(f->y * h)};
    return true;
  }

  return false;
}

#endif

static constexpr Uint32
MakeSDLFlags([[maybe_unused]] bool full_screen, bool resizable) noexcept
{
  Uint32 flags = 0;

#ifdef ENABLE_OPENGL
  flags |= SDL_WINDOW_OPENGL;
#else /* !ENABLE_OPENGL */
  flags |= SDL_SWSURFACE;
#endif /* !ENABLE_OPENGL */

#if !defined(__MACOSX__) || !(__MACOSX__)
  if (full_screen)
    flags |= SDL_WINDOW_FULLSCREEN;
#endif

  if (resizable)
    flags |= SDL_WINDOW_RESIZABLE;

#ifdef HAVE_HIGHDPI_SUPPORT
  flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif

  return flags;
}

void
TopWindow::CreateNative(const char *_text, PixelSize new_size,
                        TopWindowStyle style)
{
  const char *text = _text;

  const bool full_screen = style.GetFullScreen();
  const bool resizable = style.GetResizable();
  const Uint32 flags = MakeSDLFlags(full_screen, resizable);

  window = ::SDL_CreateWindow(text, SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, new_size.width,
                              new_size.height, flags);
  if (window == nullptr)
    throw FmtRuntimeError("SDL_CreateWindow('{}', {}, {}, {}, {}, {:#x}) has failed: {}",
                          text, SDL_WINDOWPOS_UNDEFINED,
                          SDL_WINDOWPOS_UNDEFINED, new_size.width,
                          new_size.height, flags,
                          ::SDL_GetError());

#if defined(__MACOSX__) && __MACOSX__
  SDL_SysWMinfo *wm_info =
      reinterpret_cast<SDL_SysWMinfo *>(alloca(sizeof(SDL_SysWMinfo)));
  SDL_VERSION(&wm_info->version);
  if ((SDL_GetWindowWMInfo(window, wm_info)) &&
      (wm_info->subsystem == SDL_SYSWM_COCOA)) {
    if (resizable) {
      [wm_info->info.cocoa.window
          setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];
    }
    if (full_screen) {
      [wm_info->info.cocoa.window toggleFullScreen: nil];
    }
  }
#endif

}

#ifdef HAVE_MULTI_TOUCH

bool
TopWindow::FlushTouchMouseUp() noexcept
{
  if (!touch_mouse_up_pending)
    return false;

  touch_mouse_up_pending = false;
  double_click.Moved(touch_mouse_up_point);
  return OnMouseUp(touch_mouse_up_point);
}

#endif

bool
TopWindow::OnEvent(const SDL_Event &event)
{
  const auto event_to_window = [this](PixelPoint p) noexcept {
    p = PointToReal(p);
#if defined(ENABLE_OPENGL) && defined(SOFTWARE_ROTATE_DISPLAY)
    if (OpenGL::window_size.x > 0 && OpenGL::window_size.y > 0)
      p = TransformCoordinates(p,
                               PixelSize{OpenGL::window_size.x,
                                         OpenGL::window_size.y});
#endif
    return p;
  };

  switch (event.type) {
    Window *w;

  case SDL_KEYDOWN:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    if (!w->IsEnabled())
      return false;

    return w->OnKeyDown(event.key.keysym.sym);

  case SDL_TEXTINPUT:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    if (!w->IsEnabled())
      return false;

    if (*event.text.text) {
      std::pair<unsigned, const char *> next = NextUTF8(event.text.text);
      bool handled = w->OnCharacter(next.first);
      while (next.second) {
        next = NextUTF8(next.second);
        handled = w->OnCharacter(next.first) || handled;
      }
      return handled;
    } else
      return false;

  case SDL_KEYUP:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    if (!w->IsEnabled())
      return false;

    return w->OnKeyUp(event.key.keysym.sym);

#ifdef HAVE_MULTI_TOUCH
  case SDL_FINGERDOWN:
  case SDL_FINGERMOTION:
  case SDL_FINGERUP:
    {
      if (!IsTouchScreen(event.tfinger.touchId))
        return false;

      /* trust SDL's live finger count instead of an incrementally
         maintained counter, so a dropped event cannot leave the count
         stuck; the finger that triggered SDL_FINGERUP may still be
         listed and is excluded explicitly */
      touch_fingers = event.type == SDL_FINGERUP
        ? CountFingers(event.tfinger.touchId, &event.tfinger.fingerId)
        : CountFingers(event.tfinger.touchId, nullptr);

      if (touch_fingers >= 2)
        touch_multi = true;
      else if (event.type == SDL_FINGERDOWN && touch_fingers <= 1) {
        /* first finger of a new sequence: forget any stale flag from a
           previous gesture that did not tear down cleanly */
        touch_multi = false;
        touch_pair_valid = false;
      }

      if (event.type == SDL_FINGERUP) {
        bool result = false;

        if (touch_fingers == 1 && touch_pair_valid) {
          result = OnMultiTouchUp();
          touch_pair_valid = false;
        }

        if (touch_fingers == 0) {
          touch_multi = false;
          touch_pair_valid = false;
          result |= FlushTouchMouseUp();
        }

        return result;
      }

      if (touch_fingers < 2) {
        touch_pair_valid = false;
        return false;
      }

      if (!touch_pair_valid) {
        if (event.type != SDL_FINGERDOWN || touch_fingers != 2)
          return false;

        SDL_FingerID id_a, id_b;
        if (!CaptureTwoFingerIds(event.tfinger.touchId, id_a, id_b) ||
            !OnMultiTouchDown()) {
          /* rejected start: do not treat the sequence as multi-touch */
          touch_multi = false;
          return false;
        }

        touch_finger_a = id_a;
        touch_finger_b = id_b;
        touch_pair_valid = true;
      }

      PixelPoint a, b;
      if (!GetFingerPoint(window, event.tfinger.touchId,
                          SDL_FingerID(touch_finger_a), a) ||
          !GetFingerPoint(window, event.tfinger.touchId,
                          SDL_FingerID(touch_finger_b), b))
        return false;

      return OnMultiTouchMove(event_to_window(a), event_to_window(b));
    }
#endif

  case SDL_MOUSEMOTION:
    // XXX keys
    {
#ifdef HAVE_MULTI_TOUCH
      if (event.motion.which == SDL_TOUCH_MOUSEID && touch_fingers >= 2)
        /* the pinch handler owns this gesture */
        return true;
#endif

      return OnMouseMove(event_to_window(PixelPoint(event.motion.x,
                                                    event.motion.y)),
                         0);
    }

  case SDL_MOUSEBUTTONDOWN:
    {
#ifdef HAVE_MULTI_TOUCH
      /* safety net: never let a postponed release outlive its drag */
      FlushTouchMouseUp();
#endif

      const auto p = event_to_window(PixelPoint(event.button.x,
                                                event.button.y));
      return double_click.Check(p)
        ? OnMouseDouble(p)
        : OnMouseDown(p);
    }

  case SDL_MOUSEBUTTONUP:
    {
      const auto p = event_to_window(PixelPoint(event.button.x,
                                                event.button.y));
#ifdef HAVE_MULTI_TOUCH
      if (event.button.which == SDL_TOUCH_MOUSEID) {
        /* SDL emulates the mouse with the first finger only.  Base the
           decision on the live finger total (the emulated event has no
           touch id): while other fingers remain on a multi-touch
           gesture, postpone the release so the drag does not end with
           fingers still down; otherwise release capture now. */
        if (touch_multi && CountAllFingers() > 0) {
          touch_mouse_up_point = p;
          touch_mouse_up_pending = true;
          return true;
        }

        touch_multi = false;
      }
#endif

      double_click.Moved(p);
      return OnMouseUp(p);
    }

  case SDL_QUIT:
    return OnClose();

  case SDL_MOUSEWHEEL:
    {
      PixelPoint p;
      SDL_GetMouseState(&p.x, &p.y);
#ifdef HAVE_HIGHDPI_SUPPORT
      p = PointToReal(p);
#endif
      return OnMouseWheel(p, event.wheel.y);
    }

  case SDL_WINDOWEVENT:
    switch (event.window.event) {

    case SDL_WINDOWEVENT_RESIZED:
#if defined(HAVE_HIGHDPI_SUPPORT) && defined(_WIN32)
      {
        int w = static_cast<int>(event.window.data1 *
                                 point_to_real_x);
        int h = static_cast<int>(event.window.data2 *
                                 point_to_real_y);

        if (screen->CheckResize(PixelSize(w, h)))
          Resize(screen->GetSize());
        Refresh();

        return true;
      }
#elif !defined(HAVE_HIGHDPI_SUPPORT)
#ifdef ENABLE_OPENGL
      if (screen->CheckResize(PixelSize(event.window.data1, event.window.data2)))
        Resize(screen->GetSize());
#else
      Resize({event.window.data1, event.window.data2});
#endif
      return true;
#endif
    case SDL_WINDOWEVENT_RESTORED:
    case SDL_WINDOWEVENT_MOVED:
    case SDL_WINDOWEVENT_SHOWN:
    case SDL_WINDOWEVENT_MAXIMIZED:
      if (auto *event_window = SDL_GetWindowFromID(event.window.windowID)) {
        int w, h;
        SDL_GetWindowSize(event_window, &w, &h);
        if ((w >= 0) && (h >= 0)) {
#ifdef HAVE_HIGHDPI_SUPPORT
          int real_w, real_h;
          SDL_GL_GetDrawableSize(event_window, &real_w, &real_h);
          point_to_real_x = static_cast<float>(real_w) /
            static_cast<float>(w);
          point_to_real_y = static_cast<float>(real_h) /
            static_cast<float>(h);
          w = real_w;
          h = real_h;
#endif
#ifdef ENABLE_OPENGL
#if defined(__APPLE__) && TARGET_OS_IPHONE
          PixelSize size = SystemWindowSize();
          if (screen->CheckResize(size))
            Resize(size);
#else
          if (screen->CheckResize(PixelSize(w, h)))
            Resize(screen->GetSize());
#endif
#else
          Resize({w, h});
#endif
        }

#if defined(__MACOSX__) && __MACOSX__
        SDL_SysWMinfo *wm_info =
          reinterpret_cast<SDL_SysWMinfo *>(alloca(sizeof(SDL_SysWMinfo)));
        SDL_VERSION(&wm_info->version);
        if ((SDL_GetWindowWMInfo(event_window, wm_info)) &&
            (wm_info->subsystem == SDL_SYSWM_COCOA)) {
          [wm_info->info.cocoa.window
           setCollectionBehavior:
           NSWindowCollectionBehaviorFullScreenPrimary];
        }
        Invalidate();
#endif
#ifdef _WIN32
        Refresh();
#endif
      }
      return true;

    case SDL_WINDOWEVENT_EXPOSED:
      invalidated = false;
      Expose();
      return true;
    }
  }

  return false;
}

void
TopWindow::OnResize(PixelSize new_size) noexcept
{
  ContainerWindow::OnResize(new_size);

#ifdef USE_MEMORY_CANVAS
  // Request resize instead of doing it immediately
  // The actual resize will happen in the draw thread (Expose)
  screen->RequestResize(new_size);
#endif
}

} // namespace UI
