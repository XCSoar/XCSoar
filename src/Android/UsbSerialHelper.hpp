// SPDX-License-Identifier: GPL-2.0-only
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"

class Context;
class PortBridge;
class DetectDeviceListener;

class UsbSerialHelper final : protected Java::GlobalObject {
public:
  /**
   * Global initialisation.  Looks up the methods of the
   * UsbSerialHelper Java class.
   */
  static bool Initialise(JNIEnv *env) noexcept;
  static void Deinitialise(JNIEnv *env) noexcept;

  UsbSerialHelper(JNIEnv *env, Context &context);
  ~UsbSerialHelper() noexcept;

  /**
   * Start scanning for USB serial devices.  Call
   * RemoveDetectDeviceListener() with the returned value when you're
   * done.
   */
  Java::LocalObject AddDetectDeviceListener(JNIEnv *env,
                                            DetectDeviceListener &l) noexcept;

  /**
   * Stop scanning for USB serial devices.
   *
   * @param l the return value of AddDetectDeviceListener()
   */
  void RemoveDetectDeviceListener(JNIEnv *env, jobject l) noexcept;

  PortBridge *Connect(JNIEnv *env, const char *name, unsigned baud);
};
