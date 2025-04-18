// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
