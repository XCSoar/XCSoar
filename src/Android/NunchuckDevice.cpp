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
