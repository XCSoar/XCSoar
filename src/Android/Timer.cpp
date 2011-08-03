/*
Copyright_License {

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

#include "Android/Timer.hpp"
#include "Android/Main.hpp"
#include "Java/Global.hpp"
#include "Java/Class.hpp"
#include "Screen/Window.hpp"
#include "Screen/Android/Event.hpp"
#include "org_xcsoar_Timer.h"

#include <assert.h>

AndroidTimer::Bridge::Bridge(JNIEnv *env, jlong ptr, jint period)
{
  Java::Class cls(env, "org/xcsoar/Timer");
  jmethodID cid = env->GetMethodID(cls, "<init>", "(JI)V");
  assert(cid != NULL);

  jobject obj = env->NewObject(cls, cid, ptr, period);

  set(env, obj);
  env->DeleteLocalRef(obj);
}

AndroidTimer::AndroidTimer(Window &_window, unsigned ms)
  :window(_window), bridge(Java::GetEnv(), (jlong)this, ms),
   disabled(false), running(false)
{
}

static bool
match_timer(const Event &event, void *ctx)
{
  return event.type == Event::TIMER && event.ptr == ctx;
}

void
AndroidTimer::disable()
{
  assert(!disabled);

  bridge.uninstall();
  event_queue->purge(match_timer, (void *)this);

  if (running)
    disabled = true;
  else
    delete this;
}

void
AndroidTimer::run()
{
  assert(!running);
  assert(!disabled);

  running = true;
  window.on_timer(this);
  running = false;

  if (disabled)
    delete this;
  else
    bridge.install();
}

JNIEXPORT void JNICALL
Java_org_xcsoar_Timer_run(JNIEnv *env, jobject obj, jlong ptr)
{
  AndroidTimer *timer = (AndroidTimer *)(void *)ptr;

  event_queue->push(Event(Event::TIMER, (void *)timer));
}
