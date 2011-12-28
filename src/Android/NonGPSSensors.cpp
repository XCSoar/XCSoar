/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Android/NonGPSSensors.hpp"
#include "Android/Context.hpp"
#include "Engine/Atmosphere/Pressure.hpp"
#include "Java/Class.hpp"
#include "org_xcsoar_NonGPSSensors.h"
#include "DeviceBlackboard.hpp"
#include "Components.hpp"
#include "OS/Clock.hpp"

// These type identifier constants must have the same numerical values as their
// counterparts in the Android API's Sensor class.
const int NonGPSSensors::TYPE_ACCELEROMETER = 0x1;
const int NonGPSSensors::TYPE_GYROSCOPE = 0x4;
const int NonGPSSensors::TYPE_MAGNETIC_FIELD = 0x2;
const int NonGPSSensors::TYPE_PRESSURE = 0x6;

NonGPSSensors::NonGPSSensors(JNIEnv* env, jobject obj)
    : Java::Object(env, obj) {
  Java::Class cls(env, env->GetObjectClass(obj));
  mid_subscribeToSensor_ =
      env->GetMethodID(cls, "subscribeToSensor", "(I)Z");
  mid_cancelSensorSubscription_ =
      env->GetMethodID(cls, "cancelSensorSubscription", "(I)Z");
  mid_subscribedToSensor_ =
      env->GetMethodID(cls, "subscribedToSensor", "(I)Z");
  mid_cancelAllSensorSubscriptions_ =
      env->GetMethodID(cls, "cancelAllSensorSubscriptions", "()V");
  assert(mid_subscribeToSensor_ != NULL);
  assert(mid_cancelSensorSubscription_ != NULL);
  assert(mid_subscribedToSensor_ != NULL);
  assert(mid_cancelAllSensorSubscriptions_ != NULL);
}

NonGPSSensors::~NonGPSSensors() {
  cancelAllSensorSubscriptions();
}

bool NonGPSSensors::subscribeToSensor(int id) {
  JNIEnv* env = Java::GetEnv();
  return env->CallBooleanMethod(get(), mid_subscribeToSensor_, (jint) id);
}

bool NonGPSSensors::cancelSensorSubscription(int id) {
  JNIEnv* env = Java::GetEnv();
  return env->CallBooleanMethod(get(), mid_cancelSensorSubscription_, (jint) id);
}

bool NonGPSSensors::subscribedToSensor(int id) const {
  JNIEnv* env = Java::GetEnv();
  return env->CallBooleanMethod(get(), mid_subscribedToSensor_, (jint) id);
}

void NonGPSSensors::cancelAllSensorSubscriptions() {
  JNIEnv* env = Java::GetEnv();
  env->CallVoidMethod(get(), mid_cancelAllSensorSubscriptions_);
}

NonGPSSensors* NonGPSSensors::create(JNIEnv* env, Context* context,
                                     unsigned int index) {
  Java::Class cls(env, "org/xcsoar/NonGPSSensors");
  jmethodID ctor_id = env->GetMethodID(cls, "<init>",
                                       "(Landroid/content/Context;I)V");
  assert(ctor_id != NULL);

  jobject obj = env->NewObject(cls, ctor_id, context->get(), index);
  assert(obj != NULL);

  NonGPSSensors *non_gps_sensors = new NonGPSSensors(env, obj);
  env->DeleteLocalRef(obj);

  return non_gps_sensors;
}

// Helper for retrieving the set of sensors to which we can subscribe.
void NonGPSSensors::getSubscribableSensors(JNIEnv* env, jobject obj) {
  Java::Class cls(env, env->GetObjectClass(obj));
  jmethodID mid_getSubscribableSensors =
      env->GetMethodID(cls, "getSubscribableSensors", "()[I");
  assert(mid_getSubscribableSensors != NULL);

  jintArray ss_arr = (jintArray) env->CallObjectMethod(
      get(), mid_getSubscribableSensors);
  jsize ss_arr_size = env->GetArrayLength(ss_arr);
  jint* ss_arr_elems = env->GetIntArrayElements(ss_arr, NULL);
  subscribable_sensors_.swap(
      std::vector<int>(ss_arr_elems, ss_arr_elems + ss_arr_size));
  env->ReleaseIntArrayElements(ss_arr, ss_arr_elems, 0);
}

// Helper for the C++ functions called by Java (below).
inline unsigned int getDeviceIndex(JNIEnv *env, jobject obj) {
  jfieldID fid_index = env->GetFieldID(env->GetObjectClass(obj),
                                       "index_", "I");
  return env->GetIntField(obj, fid_index);
}

// Implementations of the various C++ functions called by NonGPSSensors.java.
// This first lot are used to pass sensor values back into XCSoar.

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NonGPSSensors_setAcceleration(
    JNIEnv* env, jobject obj, jfloat ddx, jfloat ddy, jfloat ddz) {
  // TODO
  /*
  const unsigned int index = getDeviceIndex(env, obj);
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  */
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NonGPSSensors_setRotation(
    JNIEnv* env, jobject obj,
    jfloat dtheta_x, jfloat dtheta_y, jfloat dtheta_z) {
  // TODO
  /*
  const unsigned int index = getDeviceIndex(env, obj);
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  */
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NonGPSSensors_setMagneticField(
    JNIEnv* env, jobject obj, jfloat h_x, jfloat h_y, jfloat h_z) {
  // TODO
  /*
  const unsigned int index = getDeviceIndex(env, obj);
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  */
}

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NonGPSSensors_setBarometricPressure(
    JNIEnv* env, jobject obj, jfloat pressure) {
  const unsigned int index = getDeviceIndex(env, obj);
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  basic.ProvideStaticPressure(AtmosphericPressure::HectoPascal(fixed(pressure)));
  device_blackboard->ScheduleMerge();
}
