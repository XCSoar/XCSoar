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

#include "NativeI2CbaroListener.hpp"
#include "I2CbaroListener.hpp"
#include "Atmosphere/Temperature.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Java/Class.hxx"
#include "org_xcsoar_NativeI2CbaroListener.h"

#include <stddef.h>

namespace NativeI2CbaroListener {
  static Java::TrivialClass cls;
  static jmethodID ctor;
  static jfieldID ptr_field;
};

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeI2CbaroListener_onI2CbaroValues(JNIEnv *env, jobject obj,
                                                    jint sensor, jint pressure)
 {
  jlong ptr = env->GetLongField(obj, NativeI2CbaroListener::ptr_field);
  if (ptr == 0)
    return;

  I2CbaroListener &listener = *(I2CbaroListener *)(void *)ptr;
  listener.onI2CbaroValues(sensor, AtmosphericPressure::Pascal(pressure));
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeI2CbaroListener_onI2CbaroError(JNIEnv *env, jobject obj)
{
  jlong ptr = env->GetLongField(obj, NativeI2CbaroListener::ptr_field);
  if (ptr == 0)
    return;

  I2CbaroListener &listener = *(I2CbaroListener *)(void *)ptr;
  listener.onI2CbaroError();
}

void
NativeI2CbaroListener::Initialise(JNIEnv *env)
{
  cls.Find(env, "org/xcsoar/NativeI2CbaroListener");

  ctor = env->GetMethodID(cls, "<init>", "(J)V");
  ptr_field = env->GetFieldID(cls, "ptr", "J");
}

void
NativeI2CbaroListener::Deinitialise(JNIEnv *env)
{
  cls.Clear(env);
}

jobject
NativeI2CbaroListener::Create(JNIEnv *env, I2CbaroListener &listener)
{
  assert(cls != nullptr);

  return env->NewObject(cls, ctor, (jlong)&listener);
}
