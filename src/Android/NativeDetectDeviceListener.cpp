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

#include "NativeDetectDeviceListener.hpp"
#include "DetectDeviceListener.hpp"
#include "Main.hpp"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "org_xcsoar_NativeDetectDeviceListener.h"

namespace NativeDetectDeviceListener {
static Java::TrivialClass cls;
static jmethodID ctor;
static jfieldID ptr_field;
} // namespace NativeDetectDeviceListener

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeDetectDeviceListener_onDeviceDetected(JNIEnv *env, jobject obj,
                                                            jint type,
                                                            jstring _address,
                                                            jstring _name,
                                                            jlong features)
{
  jlong ptr = env->GetLongField(obj, NativeDetectDeviceListener::ptr_field);
  if (ptr == 0)
    return;

  const auto address = Java::String::GetUTFChars(env, _address);
  const auto name = _name != nullptr
    ? Java::String::GetUTFChars(env, _name)
    : Java::StringUTFChars{};

  DetectDeviceListener &cb = *(DetectDeviceListener *)(void *)ptr;
  cb.OnDeviceDetected(DetectDeviceListener::Type{type},
                      address.c_str(), name.c_str(),
                      features);
}

void
NativeDetectDeviceListener::Initialise(JNIEnv *env) noexcept
{
  cls.Find(env, "org/xcsoar/NativeDetectDeviceListener");
  ctor = env->GetMethodID(cls, "<init>", "(J)V");
  ptr_field = env->GetFieldID(cls, "ptr", "J");
}

void
NativeDetectDeviceListener::Deinitialise(JNIEnv *env) noexcept
{
  cls.ClearOptional(env);
}

Java::LocalObject
NativeDetectDeviceListener::Create(JNIEnv *env,
                                   DetectDeviceListener &cb) noexcept
{
  return {env, env->NewObject(cls, ctor, (jlong)&cb)};
}
