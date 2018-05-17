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

#include "NativeLeScanCallback.hpp"
#include "LeScanCallback.hpp"
#include "Main.hpp"
#include "Java/Class.hxx"
#include "Java/String.hxx"
#include "org_xcsoar_NativeLeScanCallback.h"

namespace NativeLeScanCallback {
  static Java::TrivialClass cls;
  static jmethodID ctor;
  static jfieldID ptr_field;
};

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeLeScanCallback_onLeScan(JNIEnv *env, jobject obj,
                                              jstring _address, jstring _name)
{
  jlong ptr = env->GetLongField(obj, NativeLeScanCallback::ptr_field);
  if (ptr == 0)
    return;

  char address[64], name[256];
  Java::String::CopyTo(env, _address, address, sizeof(address));
  Java::String::CopyTo(env, _name, name, sizeof(name));

  LeScanCallback &cb = *(LeScanCallback *)(void *)ptr;
  cb.OnLeScan(address, name);
}

void
NativeLeScanCallback::Initialise(JNIEnv *env)
{
  if (android_api_level < 18 ||
      !cls.FindOptional(env, "org/xcsoar/NativeLeScanCallback"))
    /* Bluetooth LE not supported on this Android version */
    return;

  ctor = env->GetMethodID(cls, "<init>", "(J)V");
  ptr_field = env->GetFieldID(cls, "ptr", "J");
}

void
NativeLeScanCallback::Deinitialise(JNIEnv *env)
{
  cls.ClearOptional(env);
}

jobject
NativeLeScanCallback::Create(JNIEnv *env, LeScanCallback &cb)
{
  if (!cls.IsDefined())
    /* Bluetooth LE not supported on this Android version */
    return nullptr;

  return env->NewObject(cls, ctor, (jlong)&cb);
}
