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

#include "VoltageDevice.hpp"
#include "NativeVoltageListener.hpp"
#include "Java/Class.hxx"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Atmosphere/Temperature.hpp"

#include <stdlib.h>

static Java::TrivialClass voltage_class;
static jmethodID voltage_ctor, close_method;

void
VoltageDevice::Initialise(JNIEnv *env)
{
  voltage_class.Find(env, "org/xcsoar/GlueVoltage");

  voltage_ctor = env->GetMethodID(voltage_class, "<init>",
                                 "(Lorg/xcsoar/IOIOConnectionHolder;ILorg/xcsoar/Voltage$Listener;)V");
  close_method = env->GetMethodID(voltage_class, "close", "()V");
}

void
VoltageDevice::Deinitialise(JNIEnv *env)
{
  voltage_class.Clear(env);
}

static jobject
CreateVoltageDevice(JNIEnv *env, jobject holder,
                   unsigned sample_rate,
                   VoltageListener &listener)
{
  jobject listener2 = NativeVoltageListener::Create(env, listener);
  jobject device = env->NewObject(voltage_class, voltage_ctor, holder,
                                  sample_rate,
                                  listener2);
  env->DeleteLocalRef(listener2);

  return device;
}

VoltageDevice::VoltageDevice(unsigned _index,
                             JNIEnv *env, jobject holder,
                             double _offset, double _factor,
                             unsigned sample_rate)
  :index(_index),
   obj(env, CreateVoltageDevice(env, holder,
                               sample_rate,
                               *this)),
   offset(_offset),
   factor(_factor)
{
}

VoltageDevice::~VoltageDevice()
{
  JNIEnv *env = Java::GetEnv();
  env->CallVoidMethod(obj.Get(), close_method);
}

void
VoltageDevice::onVoltageValues(int temp_adc, int voltage_index, int volt_adc)
{
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  // When no calibration data present, use defaults
  if (factor == 0) {
    // Set default for temp sensor only when sensor present.
    if (temp_adc >= 0 && offset == 0)
      offset = -130;
    factor = 0.01599561738;
    basic.ProvideSensorCalibration(factor, offset);
  }

  if (temp_adc >= 0) {
    auto v = Temperature::FromCelsius(offset + temp_adc);
    if (temperature_filter.Update(v.ToNative()))
      v = Temperature::FromNative(temperature_filter.Average());
    basic.temperature = v;
    basic.temperature_available = true;
  } else {
    basic.temperature_available = false;
  }

  if ((unsigned)voltage_index < NUMBER_OF_VOLTAGES) {
    auto v = factor * volt_adc;
    if (voltage_filter[voltage_index].Update(v))
      v = voltage_filter[voltage_index].Average();
    basic.voltage = v;
    basic.voltage_available.Update(basic.clock);
  }

  device_blackboard->ScheduleMerge();
}

void
VoltageDevice::onVoltageError()
{
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);

  basic.temperature_available = false;
  temperature_filter.Reset();
  basic.voltage_available.Clear();
  for (unsigned i=0; i<NUMBER_OF_VOLTAGES; i++)
    voltage_filter[i].Reset();

  device_blackboard->ScheduleMerge();
}
