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

#include "NativeBMP085Listener.hpp"
#include "BMP085Listener.hpp"
#include "Atmosphere/Temperature.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Java/Class.hxx"
#include "org_xcsoar_NativeBMP085Listener.h"

#include <stddef.h>

namespace NativeBMP085Listener {
  static Java::TrivialClass cls;
  static jmethodID ctor;
  static jfieldID ptr_field;
};

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeBMP085Listener_onBMP085Values(JNIEnv *env, jobject obj,
                                                    jdouble temperature,
                                                    jint pressure)
{
  jlong ptr = env->GetLongField(obj, NativeBMP085Listener::ptr_field);
  if (ptr == 0)
    return;

  BMP085Listener &listener = *(BMP085Listener *)(void *)ptr;
  listener.onBMP085Values(CelsiusToKelvin(temperature),
                          AtmosphericPressure::Pascal(pressure));
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeBMP085Listener_onBMP085Error(JNIEnv *env, jobject obj)
{
  jlong ptr = env->GetLongField(obj, NativeBMP085Listener::ptr_field);
  if (ptr == 0)
    return;

  BMP085Listener &listener = *(BMP085Listener *)(void *)ptr;
  listener.onBMP085Error();
}

void
NativeBMP085Listener::Initialise(JNIEnv *env)
{
  cls.Find(env, "org/xcsoar/NativeBMP085Listener");

  ctor = env->GetMethodID(cls, "<init>", "(J)V");
  ptr_field = env->GetFieldID(cls, "ptr", "J");
}

void
NativeBMP085Listener::Deinitialise(JNIEnv *env)
{
  cls.Clear(env);
}

jobject
NativeBMP085Listener::Create(JNIEnv *env, BMP085Listener &listener)
{
  assert(cls != nullptr);

  return env->NewObject(cls, ctor, (jlong)&listener);
}
