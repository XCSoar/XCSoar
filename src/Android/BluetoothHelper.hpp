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

#include "util/Compiler.h"

#include <jni.h>

class LeScanCallback;
class PortBridge;

namespace BluetoothHelper {

/**
 * Global initialisation.  Looks up the methods of the
 * BluetoothHelper Java class.
 */
bool Initialise(JNIEnv *env);
void Deinitialise(JNIEnv *env);

/**
 * Is the default Bluetooth adapter enabled in the Android Bluetooth
 * settings?
 */
gcc_pure
bool isEnabled(JNIEnv *env);

gcc_pure
const char *GetNameFromAddress(JNIEnv *env, const char *address);

/**
 * Returns a list of all bonded devices.
 */
gcc_malloc
jobjectArray list(JNIEnv *env);

/**
 * Does the device support Bluetooth LE?
 */
gcc_const
bool HasLe(JNIEnv *env);

/**
 * Start scanning for Bluetooth LE devices.  Call StopLeScan() with
 * the returned value when you're done.  Returns nullptr on error.
 */
jobject StartLeScan(JNIEnv *env, LeScanCallback &cb);

/**
 * Stop scanning for Bluetooth LE devices.
 *
 * @param cb the return value of StartLeScan(); the local reference
 * will be deleted by this function
 */
void StopLeScan(JNIEnv *env, jobject cb);

gcc_malloc
PortBridge *connect(JNIEnv *env, const char *address);

gcc_malloc
PortBridge *createServer(JNIEnv *env);

} // namespace BluetoothHelper

#endif
