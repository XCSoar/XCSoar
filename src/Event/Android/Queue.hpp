/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_EVENT_ANDROID_QUEUE_HPP
#define XCSOAR_EVENT_ANDROID_QUEUE_HPP

#include "Event.hpp"
#include "Util/NonCopyable.hpp"
#include "Thread/Mutex.hpp"
#include "Thread/Cond.hpp"

#include <queue>

class Window;
class AndroidTimer;
class Notify;

class EventQueue : private NonCopyable {
  std::queue<Event> events;

  Mutex mutex;
  Cond cond;

  bool running;

public:
  EventQueue():running(true) {}

  void Quit() {
    running = false;
  }

  void Push(const Event &event);

  bool Pop(Event &event);

  bool Wait(Event &event);

  void Purge(bool (*match)(const Event &event, void *ctx), void *ctx);

  void Purge(Event::Type type);
  void Purge(Event::Callback callback, void *ctx);
  void Purge(Notify &notify);
  void Purge(Window &window);
  void Purge(AndroidTimer &timer);
};

#endif
