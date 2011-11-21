/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Thread/Thread.hpp"
#include "Thread/Notify.hpp"
#include "Screen/TopWindow.hpp"
#include "Screen/Init.hpp"
#include "TestUtil.hpp"

#ifdef ANDROID
#include "Screen/Android/Event.hpp"
#elif defined(ENABLE_SDL)
#include "Screen/SDL/Event.hpp"
#else
#include "Screen/GDI/Event.hpp"
#endif

static bool quit;

class TestThread : public Thread {
  Notify &notify;

public:
  TestThread(Notify &_notify):notify(_notify) {}

protected:
  virtual void Run() {
    notify.SendNotification();
  }
};

class TestNotify : public Notify {
protected:
  virtual void OnNotification() {
    quit = true;
  }
};

int main(int argc, char **argv)
{
  plan_tests(1);

  ScreenGlobalInit screen;

#ifdef ANDROID
  TopWindow main_window;
  EventLoop loop(*event_queue, main_window);
  Event event;
#elif defined(ENABLE_SDL)
  TopWindow main_window;
  EventLoop loop(main_window);
  SDL_Event event;
#else
  EventLoop loop;
  MSG event;
#endif

  TestNotify notify;
  TestThread thread(notify);
  thread.Start();

#ifndef USE_GDI
  while (!quit && loop.get(event))
    loop.dispatch(event);
#else
  while (!quit && loop.get(event))
    loop.dispatch(event);
#endif

  ok1(quit);

  return exit_status();
}
