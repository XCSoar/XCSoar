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

#include "NunchuckDevice.hpp"
#include "NativeNunchuckListener.hpp"
#include "Java/Class.hxx"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "LogFile.hpp"

#include <stdlib.h>

static Java::TrivialClass nunchuck_class;
static jmethodID nunchuck_ctor, close_method;

void
NunchuckDevice::Initialise(JNIEnv *env)
{
  nunchuck_class.Find(env, "org/xcsoar/GlueNunchuck");

  nunchuck_ctor = env->GetMethodID(nunchuck_class, "<init>",
                                 "(Lorg/xcsoar/IOIOConnectionHolder;IILorg/xcsoar/Nunchuck$Listener;)V");
  close_method = env->GetMethodID(nunchuck_class, "close", "()V");
}

void
NunchuckDevice::Deinitialise(JNIEnv *env)
{
  nunchuck_class.Clear(env);
}

static jobject
CreateNunchuckDevice(JNIEnv *env, jobject holder,
                   unsigned twi_num, unsigned sample_rate,
                   NunchuckListener &listener)
{
  jobject listener2 = NativeNunchuckListener::Create(env, listener);
  jobject device = env->NewObject(nunchuck_class, nunchuck_ctor, holder,
                                  twi_num, sample_rate,
                                  listener2);
  env->DeleteLocalRef(listener2);

  return device;
}

NunchuckDevice::NunchuckDevice(unsigned _index,
                           JNIEnv *env, jobject holder,
                           unsigned twi_num, unsigned sample_rate)
  :index(_index),
   obj(env, CreateNunchuckDevice(env, holder,
                               twi_num, sample_rate,
                               *this)),
   kalman_filter(10, 0.3)
{
}

NunchuckDevice::~NunchuckDevice()
{
  JNIEnv *env = Java::GetEnv();
  env->CallVoidMethod(obj.Get(), close_method);
}

void
NunchuckDevice::onNunchuckValues(int joy_x, int joy_y, int acc_x, int acc_y, int acc_z, int switches)
{
  static int joy_state_x, joy_state_y;

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  basic.UpdateClock();
  basic.alive.Update(basic.clock);

  // Nunchuck really connected  ?
  if (joy_x < 1000) {

    basic.acceleration.ProvideGLoad(acc_z / 1000., true);

    device_blackboard->ScheduleMerge();

    int new_joy_state_x = 0, new_joy_state_y = 0; 
    if (joy_x < -50) new_joy_state_x = -1; else if (joy_x > 50) new_joy_state_x = 1;
    if (joy_y < -50) new_joy_state_y = -1; else if (joy_y > 50) new_joy_state_y = 1;

    if (new_joy_state_x && new_joy_state_x != joy_state_x) {
      if (new_joy_state_x < 0) {
        // generate event
      } else {
        // generate event
      }
    }
    joy_state_x = new_joy_state_x;

    if (new_joy_state_y && new_joy_state_y != joy_state_y) {
      if (new_joy_state_y < 0) {
        // generate event
      } else {
        // generate event
      }
    }
    joy_state_y = new_joy_state_y;
  }

  // Kludge: some IOIO digital inputs can be used without a Nunchuck.
  for (int i=0; i<8; i++) {
    if (switches & (1<<i)) {
      // generate event
    }
  }

  device_blackboard->ScheduleMerge();
}

void
NunchuckDevice::onNunchuckError()
{
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);

  basic.acceleration.Reset();

  device_blackboard->ScheduleMerge();
}
