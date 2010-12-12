/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_ANDROID_EVENT_HPP
#define XCSOAR_SCREEN_ANDROID_EVENT_HPP

#include "Util/NonCopyable.hpp"
#include "Thread/Mutex.hpp"
#include "Thread/Cond.hpp"

#include <SDL_version.h>
#include <SDL_events.h>

#include <queue>

class TopWindow;
class Window;
class AndroidTimer;

struct Event {
  enum {
    NOP,
    QUIT,
    TIMER,
    USER,
    KEY_DOWN,
    KEY_UP,
    MOUSE_MOTION,
    MOUSE_DOWN,
    MOUSE_UP,
  } type;

  unsigned param;

  void *ptr;

  int x, y;
};

class EventQueue : private NonCopyable {
  std::queue<Event> events;

  Mutex mutex;
  Cond cond;

  bool running;

public:
  EventQueue():running(true) {}

  void quit() {
    running = false;
  }

  void push(const Event &event);

  bool pop(Event &event);

  bool wait(Event &event);

  void purge(bool (*match)(const Event &event, void *ctx), void *ctx);

  void purge(Window &window);
  void purge(AndroidTimer &timer);
};

class EventLoop : private NonCopyable {
  EventQueue &queue;
  TopWindow &top_window;

  /**
   * True if working on a bulk of events.  At the end of that bulk,
   * TopWindow::validate() gets called.
   */
  bool bulk;

public:
  EventLoop(EventQueue &_queue, TopWindow &_top_window)
    :queue(_queue), top_window(_top_window), bulk(false) {}

  bool get(Event &event);
  void dispatch(const Event &event);
};

#endif
