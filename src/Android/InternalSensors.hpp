// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"
#include "java/Class.hxx"
#include "java/Closeable.hxx"

#include <jni.h>
#include <vector>

class Context;
class SensorListener;

/**
 * Consolidated class for handling Java objects that work with Android GPS
 * and sensor facilities. Public methods handle activation and deactivation of
 * specific sensors.
 */
class InternalSensors {
  static Java::TrivialClass gps_cls, sensors_cls;

  // IDs for methods in InternalGPS.java.
  static jmethodID gps_ctor_id, close_method;

  // IDs for methods in NonGPSSensors.java.
  static jmethodID sensors_ctor_id;
  static jmethodID mid_sensors_getSubscribableSensors;
  static jmethodID mid_sensors_subscribeToSensor_;
  static jmethodID mid_sensors_cancelSensorSubscription_;
  static jmethodID mid_sensors_subscribedToSensor_;

public:
  static bool Initialise(JNIEnv *env);
  static void Deinitialise(JNIEnv *env);

 private:
  // Java objects working with the GPS and the other sensors respectively.
  Java::GlobalCloseable internal_gps;
  Java::GlobalCloseable obj_NonGPSSensors_;
  std::vector<int> subscribable_sensors_;

  InternalSensors(const Java::LocalObject &gps_obj,
                  const Java::LocalObject &sensors_obj) noexcept;
  void getSubscribableSensors(JNIEnv* env, jobject sensors_obj);
 public:
  /* Sensor type identifier constants for use with subscription
     methods below.  These must have the same numerical values as
     their counterparts in the Android API's Sensor class. */
  static constexpr int TYPE_ACCELEROMETER = 0x1;
  static constexpr int TYPE_GYROSCOPE = 0x4;
  static constexpr int TYPE_MAGNETIC_FIELD = 0x2;
  static constexpr int TYPE_PRESSURE = 0x6;

  // For information on these methods, see comments around analogous methods
  // in NonGPSSensors.java.
  const auto &getSubscribableSensors() const {
    return subscribable_sensors_;
  }

  bool subscribeToSensor(int id);
  bool cancelSensorSubscription(int id);
  bool subscribedToSensor(int id) const;

  static InternalSensors *create(JNIEnv* env, Context* native_view,
                                 SensorListener &listener);
};
