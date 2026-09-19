// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "../Features.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "util/UTF8.hpp"

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_properties.h>

#if defined(ENABLE_OPENGL) && defined(SOFTWARE_ROTATE_DISPLAY)
#include "ui/event/shared/TransformCoordinates.hpp"
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#ifdef SDL_PLATFORM_MACOS
#import <AppKit/AppKit.h>
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
  return SDL_GetTouchDeviceType(touch_id) == SDL_TOUCH_DEVICE_DIRECT;
}

/**
 * Count the fingers currently touching @p touch_id.  Depending on the
 * SDL version, the finger that triggered a SDL_EVENT_FINGER_UP event may still
 * be listed; pass its id in @p lifted to exclude it.
 *
 * Not [[gnu::pure]]: SDL touch state can change between identical calls.
 */
static unsigned
CountFingers(SDL_TouchID touch_id, const SDL_FingerID *lifted) noexcept
{
  int n = 0;
  SDL_Finger **fingers = SDL_GetTouchFingers(touch_id, &n);
  if (fingers == nullptr)
    return 0;

  unsigned count = unsigned(n);
  if (lifted != nullptr)
    for (int i = 0; i < n; ++i)
      if (fingers[i]->id == *lifted) {
        --count;
        break;
      }

  SDL_free(fingers);
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
  int n = 0;
  SDL_TouchID *devices = SDL_GetTouchDevices(&n);
  if (devices == nullptr)
    return 0;

  for (int i = 0; i < n; ++i)
    if (IsTouchScreen(devices[i]))
      count += CountFingers(devices[i], nullptr);

  SDL_free(devices);
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
  int n = 0;
  SDL_Finger **fingers = SDL_GetTouchFingers(touch_id, &n);
  const bool found = fingers != nullptr && n >= 2;
  if (found) {
    id_a = fingers[0]->id;
    id_b = fingers[1]->id;
  }
  SDL_free(fingers);
  return found;
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

  int w = 0, h = 0;
  if (!SDL_GetWindowSize(window, &w, &h) || w <= 0 || h <= 0)
    return false;

  int n = 0;
  SDL_Finger **fingers = SDL_GetTouchFingers(touch_id, &n);
  if (fingers == nullptr)
    return false;

  bool found = false;
  for (int i = 0; i < n; ++i) {
    const SDL_Finger &f = *fingers[i];
    if (f.id == finger_id) {
      p = {int(f.x * w), int(f.y * h)};
      found = true;
      break;
    }
  }

  SDL_free(fingers);
  return found;
}

#endif

static constexpr SDL_WindowFlags
MakeSDLFlags([[maybe_unused]] bool full_screen, bool resizable) noexcept
{
  SDL_WindowFlags flags = 0;

#ifdef ENABLE_OPENGL
  flags |= SDL_WINDOW_OPENGL;
#endif

#ifndef SDL_PLATFORM_MACOS
  if (full_screen)
    flags |= SDL_WINDOW_FULLSCREEN;
#endif

  if (resizable)
    flags |= SDL_WINDOW_RESIZABLE;

#ifdef HAVE_HIGHDPI_SUPPORT
  flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
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
  const auto flags = MakeSDLFlags(full_screen, resizable);

  window = ::SDL_CreateWindow(text, new_size.width, new_size.height, flags);
  if (window == nullptr)
    throw FmtRuntimeError("SDL_CreateWindow('{}', {}, {}, {:#x}) failed: {}",
                          text, new_size.width, new_size.height, flags,
                          ::SDL_GetError());

  /* SDL3 starts with text input disabled.  Desktop character events
     must remain available even outside the on-screen keyboard dialog. */
  if (!SDL_HasScreenKeyboardSupport())
    SDL_StartTextInput(window);

#ifdef SDL_PLATFORM_MACOS
  NSWindow *native_window = (__bridge NSWindow *)SDL_GetPointerProperty(
    SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER,
    nullptr);
  if (native_window != nil) {
    if (resizable)
      [native_window setCollectionBehavior:
        NSWindowCollectionBehaviorFullScreenPrimary];
    if (full_screen)
      [native_window toggleFullScreen:nil];
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

  case SDL_EVENT_KEY_DOWN:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    if (!w->IsEnabled())
      return false;

    return w->OnKeyDown(event.key.key);

  case SDL_EVENT_TEXT_INPUT:
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

  case SDL_EVENT_KEY_UP:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    if (!w->IsEnabled())
      return false;

    return w->OnKeyUp(event.key.key);

#ifdef HAVE_MULTI_TOUCH
  case SDL_EVENT_FINGER_DOWN:
  case SDL_EVENT_FINGER_MOTION:
  case SDL_EVENT_FINGER_UP:
    {
      if (!IsTouchScreen(event.tfinger.touchID))
        return false;

      /* trust SDL's live finger count instead of an incrementally
         maintained counter, so a dropped event cannot leave the count
         stuck; the finger that triggered SDL_EVENT_FINGER_UP may still be
         listed and is excluded explicitly */
      touch_fingers = event.type == SDL_EVENT_FINGER_UP
        ? CountFingers(event.tfinger.touchID, &event.tfinger.fingerID)
        : CountFingers(event.tfinger.touchID, nullptr);

      if (touch_fingers >= 2)
        touch_multi = true;
      else if (event.type == SDL_EVENT_FINGER_DOWN && touch_fingers <= 1) {
        /* first finger of a new sequence: forget any stale flag from a
           previous gesture that did not tear down cleanly */
        touch_multi = false;
        touch_pair_valid = false;
      }

      if (event.type == SDL_EVENT_FINGER_UP) {
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
        if (event.type != SDL_EVENT_FINGER_DOWN || touch_fingers != 2)
          return false;

        SDL_FingerID id_a, id_b;
        if (!CaptureTwoFingerIds(event.tfinger.touchID, id_a, id_b) ||
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
      if (!GetFingerPoint(window, event.tfinger.touchID,
                          SDL_FingerID(touch_finger_a), a) ||
          !GetFingerPoint(window, event.tfinger.touchID,
                          SDL_FingerID(touch_finger_b), b))
        return false;

      return OnMultiTouchMove(event_to_window(a), event_to_window(b));
    }
#endif

  case SDL_EVENT_MOUSE_MOTION:
    // XXX keys
    {
#ifdef HAVE_MULTI_TOUCH
      if (event.motion.which == SDL_TOUCH_MOUSEID && touch_fingers >= 2)
        /* the pinch handler owns this gesture */
        return true;
#endif

      return OnMouseMove(event_to_window(PixelPoint(int(event.motion.x),
                                                    int(event.motion.y))),
                         0);
    }

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
#ifdef HAVE_MULTI_TOUCH
      /* safety net: never let a postponed release outlive its drag */
      FlushTouchMouseUp();
#endif

      const auto p = event_to_window(PixelPoint(int(event.button.x),
                                                int(event.button.y)));
      return double_click.Check(p)
        ? OnMouseDouble(p)
        : OnMouseDown(p);
    }

  case SDL_EVENT_MOUSE_BUTTON_UP:
    {
      const auto p = event_to_window(PixelPoint(int(event.button.x),
                                                int(event.button.y)));
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

  case SDL_EVENT_QUIT:
    return OnClose();

  case SDL_EVENT_MOUSE_WHEEL:
    {
      const auto p = event_to_window({int(event.wheel.mouse_x),
                                      int(event.wheel.mouse_y)});
      return OnMouseWheel(p, event.wheel.integer_y);
    }

  case SDL_EVENT_WINDOW_RESIZED:
  case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
  case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
  case SDL_EVENT_WINDOW_RESTORED:
  case SDL_EVENT_WINDOW_MOVED:
  case SDL_EVENT_WINDOW_SHOWN:
  case SDL_EVENT_WINDOW_MAXIMIZED:
    {
      int w = 0, h = 0;
      if (!SDL_GetWindowSize(window, &w, &h) || w <= 0 || h <= 0)
        return true;

#ifdef HAVE_HIGHDPI_SUPPORT
      int real_w = 0, real_h = 0;
      if (!SDL_GetWindowSizeInPixels(window, &real_w, &real_h) ||
          real_w <= 0 || real_h <= 0)
        return true;

      point_to_real_x = float(real_w) / float(w);
      point_to_real_y = float(real_h) / float(h);
      w = real_w;
      h = real_h;
#endif
#ifdef ENABLE_OPENGL
      if (screen->CheckResize(PixelSize(w, h)))
        Resize(screen->GetSize());
#else
      Resize({unsigned(w), unsigned(h)});
#endif
      Invalidate();
      return true;
    }

  case SDL_EVENT_WINDOW_EXPOSED:
    invalidated = false;
    Expose();
    return true;
  }

  return false;
}

void
TopWindow::OnResize(PixelSize new_size) noexcept
{
  ContainerWindow::OnResize(new_size);

#ifdef USE_MEMORY_CANVAS
  // Request resize instead of doing it immediately
  // The actual resize will happen in the UI thread (Expose)
  screen->RequestResize(new_size);
#endif
}

} // namespace UI
