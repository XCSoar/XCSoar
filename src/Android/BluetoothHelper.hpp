// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"
#include <jni.h>

class Context;
class SensorListener;
class DetectDeviceListener;
class PortBridge;

class BluetoothHelper final : protected Java::GlobalObject {
public:
  /**
   * Global initialisation.  Looks up the methods of the
   * BluetoothHelper Java class.
   */
  static bool Initialise(JNIEnv *env) noexcept;
  static void Deinitialise(JNIEnv *env) noexcept;

  BluetoothHelper(JNIEnv *env, Context &context,
                  jobject permission_manager);

  /**
   * Is the default Bluetooth adapter enabled in the Android Bluetooth
   * settings?
   */
  [[gnu::pure]]
  bool IsEnabled(JNIEnv *env) const noexcept;

  [[gnu::pure]]
  const char *GetNameFromAddress(JNIEnv *env,
                                 const char *address) const noexcept;

  /**
   * Does the device support Bluetooth LE?
   */
  [[gnu::const]]
  bool HasLe(JNIEnv *env) const noexcept;

  /**
   * Start scanning for Bluetooth devices.  Call
   * RemoveDetectDeviceListener() with the returned value when you're
   * done.
   */
  Java::LocalObject AddDetectDeviceListener(JNIEnv *env,
                                            DetectDeviceListener &l) noexcept;

  /**
   * Stop scanning for Bluetooth devices.
   *
   * @param l the return value of AddDetectDeviceListener()
   */
  void RemoveDetectDeviceListener(JNIEnv *env, jobject l) noexcept;

  Java::LocalObject connectSensor(JNIEnv *env, const char *address,
                                  SensorListener &listener);

  PortBridge *connect(JNIEnv *env, const char *address);

  PortBridge *connectHM10(JNIEnv *env, const char *address);

  PortBridge *createServer(JNIEnv *env);
};
