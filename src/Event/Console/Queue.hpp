/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_EVENT_CONSOLE_QUEUE_HPP
#define XCSOAR_EVENT_CONSOLE_QUEUE_HPP

#include "Thread/Handle.hpp"
#include "../Shared/TimerQueue.hpp"
#include "../Shared/Event.hpp"
#include "Thread/Mutex.hpp"
#include "OS/EventPipe.hpp"
#include "IO/Async/IOLoop.hpp"
#include "IO/Async/DiscardFileEventHandler.hpp"
#include "../Linux/SignalListener.hpp"

#ifndef NON_INTERACTIVE
#ifdef USE_LIBINPUT
#include "../LibInput/LibInputHandler.hpp"
#else
#include "../Linux/MergeMouse.hpp"
#ifdef KOBO
#include "../Linux/Input.hpp"
#elif defined(USE_LINUX_INPUT)
#include "../Linux/AllInput.hpp"
#else
#include "../Linux/TTYKeyboard.hpp"
#include "../Linux/Mouse.hpp"
#endif
#endif
#endif

#include <stdint.h>

#include <queue>
#include <set>

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

#ifndef NON_INTERACTIVE
#ifdef USE_LIBINPUT
  LibInputHandler libinput_handler;
#else
  MergeMouse merge_mouse;
#ifdef KOBO
  LinuxInputDevice keyboard;
  LinuxInputDevice mouse;
#else
#ifdef USE_LINUX_INPUT
  AllLinuxInputDevices all_input;
#else
  TTYKeyboard keyboard;
  LinuxMouse mouse;
#endif

#endif
#endif
#endif

  Mutex mutex;

  std::queue<Event> events;

  TimerQueue timers;

  EventPipe event_pipe;
  DiscardFileEventHandler discard;

  bool running;

public:
  EventQueue();
  ~EventQueue();

#ifndef NON_INTERACTIVE

  void SetScreenSize(unsigned width, unsigned height) {
  #ifdef USE_LIBINPUT
    libinput_handler.SetScreenSize(width, height);
  #else
    merge_mouse.SetScreenSize(width, height);
  #endif
  }

#ifndef USE_LIBINPUT
  void SetMouseRotation(bool swap, bool invert_x, bool invert_y) {
    merge_mouse.SetSwap(swap);
    merge_mouse.SetInvert(invert_x, invert_y);
  }

  void SetMouseRotation(DisplayOrientation orientation);

  bool HasPointer() const {
    return merge_mouse.HasPointer();
  }
#endif

  RasterPoint GetMousePosition() const {
#ifdef USE_LIBINPUT
    return { int(libinput_handler.GetX()), int(libinput_handler.GetY()) };
#else
    return { int(merge_mouse.GetX()), int(merge_mouse.GetY()) };
#endif
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

  void Quit() {
    running = false;
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
  virtual void OnSignal(int signo) override;
};

#endif
