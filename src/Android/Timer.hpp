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

#ifndef XCSOAR_SCREEN_SDL_TIMER_HXX
#define XCSOAR_SCREEN_SDL_TIMER_HXX

#include "Java/Object.hpp"

#include <jni.h>

class Timer;

class AndroidTimer {
  class Bridge : protected Java::Object {
  public:
    Bridge(JNIEnv *env, jlong ptr, jint period);

    void install() {
      CallVoid("install");
    }

    void uninstall() {
      CallVoid("uninstall");
    }
  };

private:
  Timer &timer;
  Bridge bridge;

  bool disabled, running;

public:
  AndroidTimer(Timer &timer, unsigned ms);

  void disable();

  void run();
};

#endif
