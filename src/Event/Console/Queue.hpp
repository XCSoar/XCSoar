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

#ifndef XCSOAR_EVENT_CONSOLE_QUEUE_HPP
#define XCSOAR_EVENT_CONSOLE_QUEUE_HPP

#include "../Shared/TimerQueue.hpp"
#include "../Shared/Event.hpp"
#include "Thread/Mutex.hpp"
#include "OS/EventPipe.hpp"
#include "IO/Async/IOLoop.hpp"

#ifdef KOBO
#include "../Linux/Input.hpp"
#else
#include "../Linux/TTYKeyboard.hpp"
#include "../Linux/Mouse.hpp"
#endif

#include <queue>
#include <set>

class Window;
class Timer;

class EventQueue final : private FileEventHandler {
  IOLoop io_loop;

#ifdef KOBO
  LinuxInputDevice mouse;
#else
  TTYKeyboard keyboard;
  LinuxMouse mouse;
#endif

  Mutex mutex;

  std::queue<Event> events;

  TimerQueue timers;

  EventPipe event_pipe;

  bool running;

public:
  EventQueue();
  ~EventQueue();

  void SetScreenSize(unsigned width, unsigned height) {
#ifndef KOBO
    mouse.SetScreenSize(width, height);
#endif
  }

#ifndef KOBO
  RasterPoint GetMousePosition() const {
    return { int(mouse.GetX()), int(mouse.GetY()) };
  }
#endif

  void Quit() {
    running = false;
  }

  void WakeUp() {
    event_pipe.Signal();
  }

private:
  gcc_pure
  int GetTimeout() const;

  void Poll();
  bool Generate(Event &event);

public:
  void Push(const Event &event);
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
  /* virtual methods from FileEventHandler */
  virtual bool OnFileEvent(int fd, unsigned mask) override {
    event_pipe.Read();
    return true;
  }
};

#endif
