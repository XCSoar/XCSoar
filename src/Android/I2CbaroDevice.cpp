// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "I2CbaroDevice.hpp"
#include "NativeSensorListener.hpp"
#include "java/Class.hxx"
#include "java/Env.hxx"

static Java::TrivialClass i2cbaro_class;
static jmethodID i2cbaro_ctor;

void
I2CbaroDevice::Initialise(JNIEnv *env) noexcept
{
  i2cbaro_class.Find(env, "org/xcsoar/GlueI2Cbaro");

  i2cbaro_ctor = env->GetMethodID(i2cbaro_class, "<init>",
                                 "(Lorg/xcsoar/IOIOConnectionHolder;IIIIILorg/xcsoar/SensorListener;)V");
}

void
I2CbaroDevice::Deinitialise(JNIEnv *env) noexcept
{
  i2cbaro_class.Clear(env);
}

Java::LocalObject
I2CbaroDevice::Create(JNIEnv *env, jobject holder, unsigned index,
                      unsigned twi_num, unsigned i2c_addr,
                      unsigned sample_rate, unsigned flags,
                      SensorListener &_listener)
{
  const auto listener = NativeSensorListener::Create(env, _listener);
  return Java::NewObjectRethrow(env, i2cbaro_class, i2cbaro_ctor,
                                holder,
                                index,
                                twi_num, i2c_addr, sample_rate, flags,
                                listener.Get());
}
