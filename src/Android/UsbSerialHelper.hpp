/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; version 2
  of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef ANDROID_USBSERIALHELPER_H
#define ANDROID_USBSERIALHELPER_H

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

#endif //ANDROID_USBSERIALHELPER_H
