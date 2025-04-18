// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NativePortListener.hpp"
#include "Device/Port/Listener.hpp"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "org_xcsoar_NativePortListener.h"

#include <cstddef>

namespace NativePortListener {
static Java::TrivialClass cls;
static jmethodID ctor;
static jfieldID ptr_field;
} // namespace NativePortListener

JNIEXPORT void JNICALL
Java_org_xcsoar_NativePortListener_portStateChanged(JNIEnv *env, jobject obj)
{
  jlong ptr = env->GetLongField(obj, NativePortListener::ptr_field);
  if (ptr == 0)
    /* not yet set */
    return;

  PortListener &listener = *(PortListener *)(void *)ptr;
  listener.PortStateChanged();
}

JNIEXPORT void JNICALL
Java_org_xcsoar_NativePortListener_portError(JNIEnv *env, jobject obj,
                                             jstring msg)
{
  jlong ptr = env->GetLongField(obj, NativePortListener::ptr_field);
  if (ptr == 0)
    /* not yet set */
    return;

  PortListener &listener = *(PortListener *)(void *)ptr;
  listener.PortError(Java::String::GetUTFChars(env, msg).c_str());
}

void
NativePortListener::Initialise(JNIEnv *env)
{
  cls.Find(env, "org/xcsoar/NativePortListener");

  ctor = env->GetMethodID(cls, "<init>", "(J)V");
  ptr_field = env->GetFieldID(cls, "ptr", "J");
}

void
NativePortListener::Deinitialise(JNIEnv *env)
{
  cls.Clear(env);
}

jobject
NativePortListener::Create(JNIEnv *env, PortListener &listener)
{
  assert(cls != nullptr);

  return env->NewObject(cls, ctor, (jlong)&listener);
}
