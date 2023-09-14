// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Sensor.hpp"
#include "java/Object.hxx"
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
  // Java objects working with the GPS and the other sensors respectively.
  Java::GlobalCloseable internal_gps;
  Java::GlobalCloseable obj_NonGPSSensors_;
  std::vector<int> subscribable_sensors_;

  InternalSensors(const Java::LocalObject &gps_obj,
                  const Java::LocalObject &sensors_obj) noexcept;
  void getSubscribableSensors(JNIEnv *env, jobject sensors_obj) noexcept;

public:
  static bool Initialise(JNIEnv *env);
  static void Deinitialise(JNIEnv *env) noexcept;

  [[gnu::pure]]
  PortState GetState(JNIEnv *env) const noexcept {
    return AndroidSensor::GetState(env, internal_gps);
  }

  /* Sensor type identifier constants for use with subscription
     methods below.  These must have the same numerical values as
     their counterparts in the Android API's Sensor class. */
  static constexpr int TYPE_ACCELEROMETER = 0x1;
  static constexpr int TYPE_GYROSCOPE = 0x4;
  static constexpr int TYPE_MAGNETIC_FIELD = 0x2;
  static constexpr int TYPE_PRESSURE = 0x6;

  // For information on these methods, see comments around analogous methods
  // in NonGPSSensors.java.
  const auto &getSubscribableSensors() const noexcept {
    return subscribable_sensors_;
  }

  bool SubscribeToSensor(JNIEnv *env, int id) noexcept;
  bool CancelSensorSubscription(JNIEnv *env, int id) noexcept;

  [[gnu::pure]]
  bool IsSubscribedToSensor(JNIEnv *env, int id) const noexcept;

  static InternalSensors *Create(JNIEnv *env, Context *native_view,
                                 jobject permission_manager,
                                 SensorListener &listener);
};
