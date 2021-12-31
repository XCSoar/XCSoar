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

#ifndef XCSOAR_ANDROID_BLUETOOTH_HELPER_HPP
#define XCSOAR_ANDROID_BLUETOOTH_HELPER_HPP

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

  BluetoothHelper(JNIEnv *env, Context &context);

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

#endif
