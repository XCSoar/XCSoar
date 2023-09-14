// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NunchuckDevice.hpp"
#include "NativeSensorListener.hpp"
#include "java/Class.hxx"
#include "java/Env.hxx"

static Java::TrivialClass nunchuck_class;
static jmethodID nunchuck_ctor;

void
NunchuckDevice::Initialise(JNIEnv *env) noexcept
{
  nunchuck_class.Find(env, "org/xcsoar/GlueNunchuck");

  nunchuck_ctor = env->GetMethodID(nunchuck_class, "<init>",
                                 "(Lorg/xcsoar/IOIOConnectionHolder;IILorg/xcsoar/SensorListener;)V");
}

void
NunchuckDevice::Deinitialise(JNIEnv *env) noexcept
{
  nunchuck_class.Clear(env);
}

Java::LocalObject
NunchuckDevice::Create(JNIEnv *env, jobject holder,
                       unsigned twi_num, unsigned sample_rate,
                       SensorListener &_listener)
{
  const auto listener = NativeSensorListener::Create(env, _listener);
  return Java::NewObjectRethrow(env, nunchuck_class, nunchuck_ctor,
                                holder,
                                twi_num, sample_rate,
                                listener.Get());
}
