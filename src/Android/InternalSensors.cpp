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

#include "InternalSensors.hpp"
#include "NativeSensorListener.hpp"
#include "Context.hpp"
#include "Atmosphere/Pressure.hpp"
#include "org_xcsoar_InternalGPS.h"
#include "org_xcsoar_NonGPSSensors.h"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "system/Clock.hpp"
#include "java/Env.hxx"
#include "util/Compiler.h"

Java::TrivialClass InternalSensors::gps_cls, InternalSensors::sensors_cls;
jmethodID InternalSensors::gps_ctor_id, InternalSensors::close_method;
jmethodID InternalSensors::sensors_ctor_id;
jmethodID InternalSensors::mid_sensors_getSubscribableSensors;
jmethodID InternalSensors::mid_sensors_subscribeToSensor_;
jmethodID InternalSensors::mid_sensors_cancelSensorSubscription_;
jmethodID InternalSensors::mid_sensors_subscribedToSensor_;
jmethodID InternalSensors::mid_sensors_cancelAllSensorSubscriptions_;

bool
InternalSensors::Initialise(JNIEnv *env)
{
  assert(!gps_cls.IsDefined());
  assert(!sensors_cls.IsDefined());
  assert(env != nullptr);

  gps_cls.Find(env, "org/xcsoar/InternalGPS");

  gps_ctor_id = env->GetMethodID(gps_cls, "<init>",
                                 "(Landroid/content/Context;Lorg/xcsoar/SensorListener;)V");
  close_method = env->GetMethodID(gps_cls, "close", "()V");

  sensors_cls.Find(env, "org/xcsoar/NonGPSSensors");

  sensors_ctor_id = env->GetMethodID(sensors_cls, "<init>",
                                     "(Landroid/content/Context;Lorg/xcsoar/SensorListener;)V");

  mid_sensors_getSubscribableSensors =
    env->GetMethodID(sensors_cls, "getSubscribableSensors", "()[I");
  assert(mid_sensors_getSubscribableSensors != nullptr);

  mid_sensors_subscribeToSensor_ =
      env->GetMethodID(sensors_cls, "subscribeToSensor", "(I)Z");
  mid_sensors_cancelSensorSubscription_ =
      env->GetMethodID(sensors_cls, "cancelSensorSubscription", "(I)Z");
  mid_sensors_subscribedToSensor_ =
      env->GetMethodID(sensors_cls, "subscribedToSensor", "(I)Z");
  mid_sensors_cancelAllSensorSubscriptions_ =
      env->GetMethodID(sensors_cls, "cancelAllSensorSubscriptions", "()V");
  assert(mid_sensors_subscribeToSensor_ != nullptr);
  assert(mid_sensors_cancelSensorSubscription_ != nullptr);
  assert(mid_sensors_subscribedToSensor_ != nullptr);
  assert(mid_sensors_cancelAllSensorSubscriptions_ != nullptr);

  return true;
}

void
InternalSensors::Deinitialise(JNIEnv *env)
{
  gps_cls.Clear(env);
  sensors_cls.Clear(env);
}

InternalSensors::InternalSensors(const Java::LocalObject &gps_obj,
                                 const Java::LocalObject &sensors_obj) noexcept
  :obj_InternalGPS_(std::move(gps_obj)),
   obj_NonGPSSensors_(std::move(sensors_obj))
{
  // Import the list of subscribable sensors from the NonGPSSensors object.
  getSubscribableSensors(gps_obj.GetEnv(), gps_obj);
}

InternalSensors::~InternalSensors()
{
  // Unsubscribe from sensors and the GPS.
  cancelAllSensorSubscriptions();
  JNIEnv *env = Java::GetEnv();
  env->CallVoidMethod(obj_InternalGPS_.Get(), close_method);
}

bool
InternalSensors::subscribeToSensor(int id)
{
  JNIEnv *env = Java::GetEnv();
  return env->CallBooleanMethod(obj_NonGPSSensors_.Get(),
                                mid_sensors_subscribeToSensor_, (jint) id);
}

bool
InternalSensors::cancelSensorSubscription(int id)
{
  JNIEnv *env = Java::GetEnv();
  return env->CallBooleanMethod(obj_NonGPSSensors_.Get(),
                                mid_sensors_cancelSensorSubscription_,
                                (jint)id);
}

bool
InternalSensors::subscribedToSensor(int id) const
{
  JNIEnv *env = Java::GetEnv();
  return env->CallBooleanMethod(obj_NonGPSSensors_.Get(),
                                mid_sensors_subscribedToSensor_, (jint)id);
}

void
InternalSensors::cancelAllSensorSubscriptions()
{
  JNIEnv *env = Java::GetEnv();
  env->CallVoidMethod(obj_NonGPSSensors_.Get(),
                      mid_sensors_cancelAllSensorSubscriptions_);
}

InternalSensors *
InternalSensors::create(JNIEnv *env, Context *context,
                        SensorListener &_listener)
{
  assert(sensors_cls != nullptr);
  assert(gps_cls != nullptr);

  const auto listener = NativeSensorListener::Create(env, _listener);

  // Construct InternalGPS object.
  auto gps_obj = Java::NewObjectRethrow(env, gps_cls, gps_ctor_id,
                                        context->Get(), listener.Get());
  assert(gps_obj != nullptr);

  // Construct NonGPSSensors object.
  auto sensors_obj =
    Java::NewObjectRethrow(env, sensors_cls, sensors_ctor_id,
                           context->Get(), listener.Get());
  assert(sensors_obj != nullptr);

  return new InternalSensors(std::move(gps_obj), std::move(sensors_obj));
}

// Helper for retrieving the set of sensors to which we can subscribe.
void
InternalSensors::getSubscribableSensors(JNIEnv *env, jobject sensors_obj)
{
  jintArray ss_arr = (jintArray)
    env->CallObjectMethod(obj_NonGPSSensors_.Get(),
                          mid_sensors_getSubscribableSensors);
  jsize ss_arr_size = env->GetArrayLength(ss_arr);
  jint *ss_arr_elems = env->GetIntArrayElements(ss_arr, nullptr);
  subscribable_sensors_.assign(ss_arr_elems, ss_arr_elems + ss_arr_size);
  env->ReleaseIntArrayElements(ss_arr, ss_arr_elems, 0);
}
