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

#include "Vibrator.hpp"
#include "Main.hpp"
#include "Context.hpp"
#include "Java/Class.hxx"

jmethodID Vibrator::cancel_method, Vibrator::vibrate_method;

void
Vibrator::Initialise(JNIEnv *env)
{
  Java::Class cls(env, "android/os/Vibrator");

  cancel_method = env->GetMethodID(cls, "cancel", "()V");
  vibrate_method = env->GetMethodID(cls, "vibrate", "(J)V");
}

Vibrator *
Vibrator::Create(JNIEnv *env, Context &context)
{
  jobject obj = context.GetVibrator(env);
  if (obj == nullptr)
    return nullptr;

  Vibrator *vibrator = new Vibrator(env, obj);
  env->DeleteLocalRef(obj);
  return vibrator;
}

void
Vibrator::Cancel(JNIEnv *env)
{
  env->CallVoidMethod(object, cancel_method);
}

void
Vibrator::Vibrate(JNIEnv *env, unsigned duration_ms)
{
  env->CallVoidMethod(object, vibrate_method, (jlong)duration_ms);
}

bool Vibrator::IsOSHapticFeedbackEnabled()
{
  return os_haptic_feedback_enabled;
};
