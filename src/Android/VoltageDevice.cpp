// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VoltageDevice.hpp"
#include "NativeSensorListener.hpp"
#include "java/Class.hxx"
#include "java/Env.hxx"

static Java::TrivialClass voltage_class;
static jmethodID voltage_ctor;

void
VoltageDevice::Initialise(JNIEnv *env) noexcept
{
  voltage_class.Find(env, "org/xcsoar/GlueVoltage");

  voltage_ctor = env->GetMethodID(voltage_class, "<init>",
                                 "(Lorg/xcsoar/IOIOConnectionHolder;ILorg/xcsoar/SensorListener;)V");
}

void
VoltageDevice::Deinitialise(JNIEnv *env) noexcept
{
  voltage_class.Clear(env);
}

Java::LocalObject
VoltageDevice::Create(JNIEnv *env, jobject holder,
                      unsigned sample_rate,
                      SensorListener &_listener)
{
  const auto listener = NativeSensorListener::Create(env, _listener);
  return Java::NewObjectRethrow(env, voltage_class, voltage_ctor,
                                holder,
                                sample_rate,
                                listener.Get());
}
