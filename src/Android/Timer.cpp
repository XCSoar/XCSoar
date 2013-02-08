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

#include "Android/Timer.hpp"
#include "Android/Main.hpp"
#include "Java/Global.hpp"
#include "Java/Class.hpp"
#include "Event/Timer.hpp"
#include "Event/Android/Event.hpp"
#include "Event/Android/Queue.hpp"
#include "org_xcsoarte_Timer.h"
#include "Compiler.h"

#include <assert.h>

static Java::TrivialClass cls;
static jmethodID ctor;
jmethodID AndroidTimer::Bridge::install_method;
jmethodID AndroidTimer::Bridge::uninstall_method;

void
AndroidTimer::Bridge::Initialise(JNIEnv *env)
{
  assert(cls == NULL);
  assert(env != NULL);

  cls.Find(env, "org/xcsoarte/Timer");

  ctor = env->GetMethodID(cls, "<init>", "(JI)V");
  install_method = env->GetMethodID(cls, "install", "()V");
  uninstall_method = env->GetMethodID(cls, "uninstall", "()V");
}

void
AndroidTimer::Bridge::Deinitialise(JNIEnv *env)
{
  assert(env != NULL);

  cls.Clear(env);
}

AndroidTimer::Bridge::Bridge(JNIEnv *env, jlong ptr, jint period)
{
  jobject obj = env->NewObject(cls, ctor, ptr, period);

  Set(env, obj);
  env->DeleteLocalRef(obj);
}

AndroidTimer::AndroidTimer(Timer &_timer, unsigned ms)
  :timer(_timer), bridge(Java::GetEnv(), (jlong)this, ms),
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

  bridge.uninstall(Java::GetEnv());
  event_queue->Purge(match_timer, (void *)this);

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
  assert(timer.IsActive());

  running = true;
  timer.Invoke();
  running = false;

  if (disabled)
    delete this;
  else
    bridge.install(Java::GetEnv());
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoarte_Timer_run(JNIEnv *env, jobject obj, jlong ptr)
{
  AndroidTimer *timer = (AndroidTimer *)(void *)ptr;

  event_queue->Push(Event(Event::TIMER, (void *)timer));
}
