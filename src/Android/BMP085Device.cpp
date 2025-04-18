// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BMP085Device.hpp"
#include "NativeSensorListener.hpp"
#include "java/Class.hxx"
#include "java/Env.hxx"

static Java::TrivialClass bmp085_class;
static jmethodID bmp085_ctor;

void
BMP085Device::Initialise(JNIEnv *env) noexcept
{
  bmp085_class.Find(env, "org/xcsoar/GlueBMP085");

  bmp085_ctor = env->GetMethodID(bmp085_class, "<init>",
                                 "(Lorg/xcsoar/IOIOConnectionHolder;IIILorg/xcsoar/SensorListener;)V");
}

void
BMP085Device::Deinitialise(JNIEnv *env) noexcept
{
  bmp085_class.Clear(env);
}

Java::LocalObject
BMP085Device::Create(JNIEnv *env, jobject holder,
                     unsigned twi_num, unsigned eoc_pin,
                     unsigned oversampling,
                     SensorListener &_listener)
{
  const auto listener = NativeSensorListener::Create(env, _listener);
  return Java::NewObjectRethrow(env, bmp085_class, bmp085_ctor,
                                holder,
                                twi_num, eoc_pin, oversampling,
                                listener.Get());
}
