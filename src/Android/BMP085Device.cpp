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
