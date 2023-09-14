// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NativeInputListener.hpp"
#include "io/DataHandler.hpp"
#include "java/Array.hxx"
#include "java/Class.hxx"
#include "org_xcsoar_NativeInputListener.h"

#include <cstddef>

namespace NativeInputListener {
static Java::TrivialClass cls;
static jmethodID ctor;
static jfieldID ptr_field;
} // namespace NativeInputListener

JNIEXPORT void JNICALL
Java_org_xcsoar_NativeInputListener_dataReceived(JNIEnv *env, jobject obj,
                                                 jbyteArray data, jint length)
{
  jlong ptr = env->GetLongField(obj, NativeInputListener::ptr_field);
  if (ptr == 0)
    /* not yet set */
    return;

  DataHandler &handler = *(DataHandler *)(void *)ptr;

  const Java::ByteArrayElements elems{env, data};
  handler.DataReceived({(const std::byte *)elems.get(), std::size_t(length)});
}

void
NativeInputListener::Initialise(JNIEnv *env)
{
  cls.Find(env, "org/xcsoar/NativeInputListener");

  ctor = env->GetMethodID(cls, "<init>", "(J)V");
  ptr_field = env->GetFieldID(cls, "ptr", "J");
}

void
NativeInputListener::Deinitialise(JNIEnv *env)
{
  cls.Clear(env);
}

jobject
NativeInputListener::Create(JNIEnv *env, DataHandler &handler)
{
  assert(cls != nullptr);

  return env->NewObject(cls, ctor, (jlong)&handler);
}
