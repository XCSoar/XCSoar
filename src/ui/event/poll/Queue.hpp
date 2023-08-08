// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "../shared/Event.hpp"
#include "thread/Mutex.hxx"
#include "event/InjectEvent.hxx"
#include "event/Loop.hxx"

#ifdef USE_X11
#include "X11Queue.hpp"
#elif defined(USE_WAYLAND)
#include "WaylandQueue.hpp"
#elif !defined(NON_INTERACTIVE)
#include "InputQueue.hpp"
#endif

#include <cstdint>

#include <queue>

enum class DisplayOrientation : uint8_t;
struct PixelSize;

namespace UI {

class Display;

class EventQueue final {
  ::EventLoop event_loop;

#if defined(USE_X11) || defined(USE_WAYLAND) || defined(MESA_KMS)
  Display &display;
#endif

#ifdef USE_X11
  X11EventQueue input_queue{display, *this};
#elif defined(USE_WAYLAND)
  WaylandEventQueue input_queue{display, *this};
#elif !defined(NON_INTERACTIVE)
  InputEventQueue input_queue{*this};
#endif

  Mutex mutex;

  std::queue<Event> events;

  InjectEvent wake_event{event_loop, BIND_THIS_METHOD(OnWakeUp)};

  bool quit = false;

public:
  /**
   * Throws on error.
   */
  explicit EventQueue(Display &display);
  ~EventQueue() noexcept;

  auto &GetEventLoop() noexcept {
    return event_loop;
  }

  void Suspend() noexcept {
#if !defined(NON_INTERACTIVE) && !defined(USE_X11) && !defined(USE_WAYLAND)
    input_queue.Suspend();
#endif
  }

  void Resume() noexcept {
#if !defined(NON_INTERACTIVE) && !defined(USE_X11) && !defined(USE_WAYLAND)
    input_queue.Resume();
#endif
  }

#ifdef USE_X11
  bool WasCtrlClick() const noexcept {
    return input_queue.WasCtrlClick();
  }
#endif

#ifdef USE_WAYLAND
  struct wl_compositor *GetCompositor() noexcept {
    return input_queue.GetCompositor();
  }

  struct wl_shell *GetShell() noexcept {
    return input_queue.GetShell();
  }

  auto GetWmBase() noexcept {
    return input_queue.GetWmBase();
  }
#endif

#if defined(USE_X11) || defined(USE_WAYLAND)
  bool IsVisible() const noexcept {
    return input_queue.IsVisible();
  }
#endif

  void SetScreenSize([[maybe_unused]] const PixelSize &screen_size) noexcept {
#if !defined(NON_INTERACTIVE) && !defined(USE_X11) && !defined(USE_WAYLAND)
    input_queue.SetScreenSize(screen_size);
#endif
  }

  void SetDisplayOrientation([[maybe_unused]] DisplayOrientation orientation) noexcept {
#if !defined(NON_INTERACTIVE) && !defined(USE_X11) && !defined(USE_WAYLAND) && !defined(USE_LIBINPUT)
    input_queue.SetDisplayOrientation(orientation);
#endif
  }

#if !defined(NON_INTERACTIVE) && !defined(USE_X11)
  bool HasPointer() const noexcept {
    return input_queue.HasPointer();
  }

#if defined(USE_LIBINPUT) || defined(USE_WAYLAND)
  bool HasTouchScreen() const noexcept {
    return input_queue.HasTouchScreen();
  }

  bool HasKeyboard() const noexcept {
    return input_queue.HasKeyboard();
  }
#endif

#ifndef USE_WAYLAND
  PixelPoint GetMousePosition() const noexcept {
    return input_queue.GetMousePosition();
  }
#endif

#endif /* !NON_INTERACTIVE */

  bool IsQuit() const noexcept {
    return quit;
  }

  void Quit() noexcept {
    quit = true;
  }

  void WakeUp() noexcept {
    wake_event.Schedule();
  }

private:
  void Poll() noexcept;
  bool Generate(Event &event) noexcept;

public:
  /**
   * Add an event to the queue; thread-unsafe version which must be
   * called from the UI thread.
   */
  void Push(const Event &event) noexcept;

  /**
   * Ensure that the next Wait() call finishes by injecting a NOP
   * event.  This method is not thread-safe.
   */
  void Interrupt() noexcept;

  /**
   * Add an event to the queue; thread-safe version which may be
   * called from any thread.
   */
  void Inject(const Event &event) noexcept;

  void InjectCall(Event::Callback callback, void *ctx) {
    Inject(Event{callback, ctx});
  }

  bool Pop(Event &event) noexcept;

  bool Wait(Event &event) noexcept;

  void Purge(bool (*match)(const Event &event, void *ctx) noexcept,
             void *ctx) noexcept;

  void Purge(enum Event::Type type) noexcept;
  void Purge(Event::Callback callback, void *ctx) noexcept;

private:
  void OnQuitSignal() noexcept {
    Quit();
    WakeUp();
  }

  void OnWakeUp() noexcept {
    event_loop.Finish();
  }
};

} // namespace UI
