/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Thread/Handle.hpp"
#include "../Shared/TimerQueue.hpp"
#include "../Shared/Event.hpp"
#include "Thread/Mutex.hpp"
#include "OS/EventPipe.hpp"
#include "IO/Async/IOLoop.hpp"
#include "IO/Async/DiscardFileEventHandler.hpp"
#include "Linux/SignalListener.hpp"

#ifdef USE_X11
#include "X11Queue.hpp"
#elif defined(USE_WAYLAND)
#include "WaylandQueue.hpp"
#elif !defined(NON_INTERACTIVE)
#include "InputQueue.hpp"
#endif

#include <stdint.h>

#include <queue>

enum class DisplayOrientation : uint8_t;
class Window;
class Timer;

class EventQueue final : private SignalListener {
  const ThreadHandle thread;

  /**
   * The current time after the event thread returned from sleeping.
   */
  uint64_t now_us;

  IOLoop io_loop;

#ifdef USE_X11
  X11EventQueue input_queue;
#elif defined(USE_WAYLAND)
  WaylandEventQueue input_queue;
#elif !defined(NON_INTERACTIVE)
  InputEventQueue input_queue;
#endif

  Mutex mutex;

  std::queue<Event> events;

  TimerQueue timers;

  EventPipe event_pipe;
  DiscardFileEventHandler discard;

  bool quit;

public:
  EventQueue();
  ~EventQueue();

#ifdef USE_X11
  _XDisplay *GetDisplay() const {
    return input_queue.GetDisplay();
  }

  bool WasCtrlClick() const {
    return input_queue.WasCtrlClick();
  }
#endif

#ifdef USE_WAYLAND
  struct wl_display *GetDisplay() {
    return input_queue.GetDisplay();
  }

  struct wl_compositor *GetCompositor() {
    return input_queue.GetCompositor();
  }

  struct wl_shell *GetShell() {
    return input_queue.GetShell();
  }
#endif

#if defined(USE_X11) || defined(USE_WAYLAND)
  bool IsVisible() const {
    return input_queue.IsVisible();
  }
#endif

#if !defined(NON_INTERACTIVE) && !defined(USE_X11) && !defined(USE_WAYLAND)

  void SetScreenSize(unsigned width, unsigned height) {
    input_queue.SetScreenSize(width, height);
  }

#ifndef USE_LIBINPUT
  void SetMouseRotation(bool swap, bool invert_x, bool invert_y) {
    input_queue.SetMouseRotation(swap, invert_x, invert_y);
  }

  void SetMouseRotation(DisplayOrientation orientation) {
    input_queue.SetMouseRotation(orientation);
  }
#endif

  bool HasPointer() const {
    return input_queue.HasPointer();
  }

#ifdef USE_LIBINPUT
  bool HasTouchScreen() const {
    return input_queue.HasTouchScreen();
  }

  bool HasKeyboard() const {
    return input_queue.HasKeyboard();
  }
#endif

  RasterPoint GetMousePosition() const {
    return input_queue.GetMousePosition();
  }

#endif /* !NON_INTERACTIVE */

  /**
   * Returns the monotonic clock in microseconds.  This method is only
   * available in the main thread.
   */
  gcc_pure
  uint64_t ClockUS() const {
    assert(thread.IsInside());

    return now_us;
  }

  bool IsQuit() const {
    return quit;
  }

  void Quit() {
    quit = true;
  }

  void WakeUp() {
    if (!thread.IsInside())
      event_pipe.Signal();
  }

private:
  gcc_pure
  int GetTimeout() const;

  void Poll();
  bool Generate(Event &event);

public:
  void Push(const Event &event);

  void Push(Event::Callback callback, void *ctx) {
    Push(Event(callback, ctx));
  }

  void PushKeyPress(unsigned key_code);

  bool Pop(Event &event);

  bool Wait(Event &event);

  void Purge(bool (*match)(const Event &event, void *ctx), void *ctx);

  void Purge(enum Event::Type type);
  void Purge(Event::Callback callback, void *ctx);
  void Purge(Window &window);

  void AddTimer(Timer &timer, unsigned ms);
  void CancelTimer(Timer &timer);

private:
  /* virtual methods from SignalListener */
  void OnSignal(int signo) override;
};

#endif
