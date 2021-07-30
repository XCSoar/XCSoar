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
