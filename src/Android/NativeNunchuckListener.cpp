/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "NativeNunchuckListener.hpp"
#include "NunchuckListener.hpp"
#include "java/Class.hxx"
#include "org_xcsoar_NativeNunchuckListener.h"

#include <cstddef>

namespace NativeNunchuckListener {
  static Java::TrivialClass cls;
  static jmethodID ctor;
  static jfieldID ptr_field;
};

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeNunchuckListener_onNunchuckValues(JNIEnv *env, jobject obj,
                       jint joy_x, jint joy_y, jint acc_x, jint acc_y, jint acc_z, jint switches)
 {
  jlong ptr = env->GetLongField(obj, NativeNunchuckListener::ptr_field);
  if (ptr == 0)
    return;

  NunchuckListener &listener = *(NunchuckListener *)(void *)ptr;
  listener.onNunchuckValues(joy_x, joy_y, acc_x, acc_y, acc_z, switches);
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeNunchuckListener_onNunchuckError(JNIEnv *env, jobject obj)
{
  jlong ptr = env->GetLongField(obj, NativeNunchuckListener::ptr_field);
  if (ptr == 0)
    return;

  NunchuckListener &listener = *(NunchuckListener *)(void *)ptr;
  listener.onNunchuckError();
}

void
NativeNunchuckListener::Initialise(JNIEnv *env)
{
  cls.Find(env, "org/xcsoar/NativeNunchuckListener");

  ctor = env->GetMethodID(cls, "<init>", "(J)V");
  ptr_field = env->GetFieldID(cls, "ptr", "J");
}

void
NativeNunchuckListener::Deinitialise(JNIEnv *env)
{
  cls.Clear(env);
}

jobject
NativeNunchuckListener::Create(JNIEnv *env, NunchuckListener &listener)
{
  assert(cls != nullptr);

  return env->NewObject(cls, ctor, (jlong)&listener);
}
