/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "MS5611Device.hpp"
#include "NativeMS5611Listener.hpp"
#include "Java/Class.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Math/LowPassFilter.hpp"
#include "LogFile.hpp"


static Java::TrivialClass ms5611_class;
static jmethodID ms5611_ctor, close_method;

void
MS5611Device::Initialise(JNIEnv *env)
{
  ms5611_class.Find(env, "org/xcsoar/GlueMS5611");

  ms5611_ctor = env->GetMethodID(ms5611_class, "<init>",
                                 "(Lorg/xcsoar/IOIOConnectionHolder;IILorg/xcsoar/MS5611$Listener;)V");
  close_method = env->GetMethodID(ms5611_class, "close", "()V");
}

void
MS5611Device::Deinitialise(JNIEnv *env)
{
  ms5611_class.Clear(env);
}

static jobject
CreateMS5611Device(JNIEnv *env, jobject holder,
                   unsigned twi_num, unsigned sleeptime,
                   MS5611Listener &listener)
{
  jobject listener2 = NativeMS5611Listener::Create(env, listener);
  jobject device = env->NewObject(ms5611_class, ms5611_ctor, holder,
                                  twi_num, sleeptime,
                                  listener2);
  env->DeleteLocalRef(listener2);

  return device;
}

MS5611Device::MS5611Device(unsigned _index,
                           JNIEnv *env, jobject holder,
                           unsigned twi_num, unsigned sleeptime)
  :index(_index),
   obj(env, CreateMS5611Device(env, holder,
                               twi_num, sleeptime,
                               *this)),
   kalman_filter(fixed(10), fixed(0.3))
{
}

MS5611Device::~MS5611Device()
{
  JNIEnv *env = Java::GetEnv();
  env->CallVoidMethod(obj.Get(), close_method);
}

gcc_pure
static inline
fixed ComputeNoncompVario(const fixed pressure, const fixed d_pressure)
{
  static const fixed FACTOR(-2260.389548275485);
  static const fixed EXPONENT(-0.8097374740609689);
  return fixed(FACTOR * pow(pressure, EXPONENT) * d_pressure);
}

void
MS5611Device::onMS5611Values(AtmosphericPressure pressure)
{
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  if (pressure.IsPlausible()) {
    kalman_filter.Update(pressure.GetHectoPascal(), fixed(0.25));

    basic.ProvideNoncompVario(ComputeNoncompVario(kalman_filter.GetXAbs(),
                                                  kalman_filter.GetXVel()));
    basic.ProvideStaticPressure(AtmosphericPressure::HectoPascal(kalman_filter.GetXAbs()));
  }

  device_blackboard->ScheduleMerge();
}

void
MS5611Device::onMS5611Error()
{
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);

  basic.static_pressure_available.Clear();
  basic.noncomp_vario_available.Clear();

  device_blackboard->ScheduleMerge();
}
