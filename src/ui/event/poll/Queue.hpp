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

#ifndef XCSOAR_EVENT_POLL_QUEUE_HPP
#define XCSOAR_EVENT_POLL_QUEUE_HPP

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

namespace UI {

class EventQueue final {
  ::EventLoop event_loop;

#ifdef USE_X11
  X11EventQueue input_queue{*this};
#elif defined(USE_WAYLAND)
  WaylandEventQueue input_queue{*this};
#elif !defined(NON_INTERACTIVE)
  InputEventQueue input_queue{*this};
#endif

  Mutex mutex;

  std::queue<Event> events;

  InjectEvent wake_event{event_loop, BIND_THIS_METHOD(OnWakeUp)};

  bool quit = false;

public:
  EventQueue() noexcept;
  ~EventQueue() noexcept;

  auto &GetEventLoop() noexcept {
    return event_loop;
  }

#ifdef USE_X11
  _XDisplay *GetDisplay() const noexcept {
    return input_queue.GetDisplay();
  }

  bool WasCtrlClick() const noexcept {
    return input_queue.WasCtrlClick();
  }
#endif

#ifdef USE_WAYLAND
  struct wl_display *GetDisplay() noexcept {
    return input_queue.GetDisplay();
  }

  struct wl_compositor *GetCompositor() noexcept {
    return input_queue.GetCompositor();
  }

  struct wl_shell *GetShell() noexcept {
    return input_queue.GetShell();
  }
#endif

#if defined(USE_X11) || defined(USE_WAYLAND)
  bool IsVisible() const noexcept {
    return input_queue.IsVisible();
  }
#endif

  void SetScreenSize(unsigned width, unsigned height) noexcept {
#if !defined(NON_INTERACTIVE) && !defined(USE_X11) && !defined(USE_WAYLAND)
    input_queue.SetScreenSize(width, height);
#endif
  }

  void SetDisplayOrientation(DisplayOrientation orientation) noexcept {
#if !defined(NON_INTERACTIVE) && !defined(USE_X11) && !defined(USE_WAYLAND) && !defined(USE_LIBINPUT)
    input_queue.SetDisplayOrientation(orientation);
#endif
  }

#if !defined(NON_INTERACTIVE) && !defined(USE_X11) && !defined(USE_WAYLAND)
  bool HasPointer() const noexcept {
    return input_queue.HasPointer();
  }

#ifdef USE_LIBINPUT
  bool HasTouchScreen() const noexcept {
    return input_queue.HasTouchScreen();
  }

  bool HasKeyboard() const noexcept {
    return input_queue.HasKeyboard();
  }
#endif

  PixelPoint GetMousePosition() const noexcept {
    return input_queue.GetMousePosition();
  }

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
  void Push(const Event &event) noexcept;

  void Push(Event::Callback callback, void *ctx) noexcept {
    Push(Event(callback, ctx));
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
    event_loop.Break();
  }
};

} // namespace UI

#endif
