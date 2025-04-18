// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InternalSensors.hpp"
#include "NativeSensorListener.hpp"
#include "Context.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "java/Array.hxx"
#include "java/Class.hxx"
#include "java/Env.hxx"

static Java::TrivialClass gps_cls, sensors_cls;
static jmethodID gps_ctor_id;
static jmethodID sensors_ctor_id;
static jmethodID mid_sensors_getSubscribableSensors;
static jmethodID mid_sensors_subscribeToSensor_;
static jmethodID mid_sensors_cancelSensorSubscription_;
static jmethodID mid_sensors_subscribedToSensor_;

bool
InternalSensors::Initialise(JNIEnv *env)
{
  assert(!gps_cls.IsDefined());
  assert(!sensors_cls.IsDefined());
  assert(env != nullptr);

  gps_cls.Find(env, "org/xcsoar/InternalGPS");

  gps_ctor_id = env->GetMethodID(gps_cls, "<init>",
                                 "(Landroid/content/Context;"
                                 "Lorg/xcsoar/PermissionManager;"
                                 "Lorg/xcsoar/SensorListener;)V");

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
  assert(mid_sensors_subscribeToSensor_ != nullptr);
  assert(mid_sensors_cancelSensorSubscription_ != nullptr);
  assert(mid_sensors_subscribedToSensor_ != nullptr);

  return true;
}

void
InternalSensors::Deinitialise(JNIEnv *env) noexcept
{
  gps_cls.Clear(env);
  sensors_cls.Clear(env);
}

InternalSensors::InternalSensors(const Java::LocalObject &gps_obj,
                                 const Java::LocalObject &sensors_obj) noexcept
  :internal_gps(std::move(gps_obj)),
   obj_NonGPSSensors_(std::move(sensors_obj))
{
  // Import the list of subscribable sensors from the NonGPSSensors object.
  getSubscribableSensors(gps_obj.GetEnv(), gps_obj);
}

bool
InternalSensors::SubscribeToSensor(JNIEnv *env, int id) noexcept
{
  return env->CallBooleanMethod(obj_NonGPSSensors_.Get(),
                                mid_sensors_subscribeToSensor_, (jint) id);
}

bool
InternalSensors::CancelSensorSubscription(JNIEnv *env, int id) noexcept
{
  return env->CallBooleanMethod(obj_NonGPSSensors_.Get(),
                                mid_sensors_cancelSensorSubscription_,
                                (jint)id);
}

bool
InternalSensors::IsSubscribedToSensor(JNIEnv *env, int id) const noexcept
{
  return env->CallBooleanMethod(obj_NonGPSSensors_.Get(),
                                mid_sensors_subscribedToSensor_, (jint)id);
}

InternalSensors *
InternalSensors::Create(JNIEnv *env, Context *context,
                        jobject permission_manager,
                        SensorListener &_listener)
{
  assert(sensors_cls != nullptr);
  assert(gps_cls != nullptr);

  const auto listener = NativeSensorListener::Create(env, _listener);

  // Construct InternalGPS object.
  auto gps_obj = Java::NewObjectRethrow(env, gps_cls, gps_ctor_id,
                                        context->Get(),
                                        permission_manager,
                                        listener.Get());
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
InternalSensors::getSubscribableSensors(JNIEnv *env,
                                        [[maybe_unused]] jobject sensors_obj) noexcept
{
  jintArray ss_arr = (jintArray)
    env->CallObjectMethod(obj_NonGPSSensors_.Get(),
                          mid_sensors_getSubscribableSensors);
  jsize ss_arr_size = env->GetArrayLength(ss_arr);
  const Java::IntArrayElements ss_arr_elems{env, ss_arr};
  subscribable_sensors_.assign(ss_arr_elems.get(),
                               ss_arr_elems.get() + ss_arr_size);
}
