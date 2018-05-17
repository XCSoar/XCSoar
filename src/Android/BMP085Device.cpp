/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "NativeBMP085Listener.hpp"
#include "Java/Class.hxx"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"

/* ignoring the temperature, because it is measured inside the IOIO
   box, and NMEAInfo's temperature setting is expected to be "outside
   air temperature" - maybe someday, somebody builds a IOIO adapter
   with a real outside air temperature sensor? */
#if 0
#define USE_TEMPERATURE
#endif

static Java::TrivialClass bmp085_class;
static jmethodID bmp085_ctor, close_method;

void
BMP085Device::Initialise(JNIEnv *env)
{
  bmp085_class.Find(env, "org/xcsoar/GlueBMP085");

  bmp085_ctor = env->GetMethodID(bmp085_class, "<init>",
                                 "(Lorg/xcsoar/IOIOConnectionHolder;IIILorg/xcsoar/BMP085$Listener;)V");
  close_method = env->GetMethodID(bmp085_class, "close", "()V");
}

void
BMP085Device::Deinitialise(JNIEnv *env)
{
  bmp085_class.Clear(env);
}

static jobject
CreateBMP085Device(JNIEnv *env, jobject holder,
                   unsigned twi_num, unsigned eoc_pin,
                   unsigned oversampling,
                   BMP085Listener &listener)
{
  jobject listener2 = NativeBMP085Listener::Create(env, listener);
  jobject device = env->NewObject(bmp085_class, bmp085_ctor, holder,
                                  twi_num, eoc_pin, oversampling,
                                  listener2);
  env->DeleteLocalRef(listener2);
  return device;
}

BMP085Device::BMP085Device(unsigned _index,
                           JNIEnv *env, jobject holder,
                           unsigned twi_num, unsigned eoc_pin,
                           unsigned oversampling)
  :index(_index),
   obj(env, CreateBMP085Device(env, holder,
                               twi_num, eoc_pin, oversampling,
                               *this)),
   kalman_filter(10, 0.0075)
{
}

BMP085Device::~BMP085Device()
{
  JNIEnv *env = Java::GetEnv();
  env->CallVoidMethod(obj.Get(), close_method);
}

gcc_pure
static inline double
ComputeNoncompVario(const double pressure, const double d_pressure)
{
  static constexpr double FACTOR(-2260.389548275485);
  static constexpr double EXPONENT(-0.8097374740609689);
  return FACTOR * pow(pressure, EXPONENT) * d_pressure;
}

void
BMP085Device::onBMP085Values(double temperature,
                             AtmosphericPressure pressure)
{
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

#ifdef USE_TEMPERATURE
  basic.temperature = temperature;
  basic.temperature_available = true;
#endif

  kalman_filter.Update(pressure.GetHectoPascal(), 0.05);

  basic.ProvideNoncompVario(ComputeNoncompVario(kalman_filter.GetXAbs(),
                                                kalman_filter.GetXVel()));
  basic.ProvideStaticPressure(AtmosphericPressure::HectoPascal(kalman_filter.GetXAbs()));

  device_blackboard->ScheduleMerge();
}

void
BMP085Device::onBMP085Error()
{
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);

#ifdef USE_TEMPERATURE
  basic.temperature_available = false;
#endif

  basic.static_pressure_available.Clear();
  basic.noncomp_vario_available.Clear();

  device_blackboard->ScheduleMerge();
}
