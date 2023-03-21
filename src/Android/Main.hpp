// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Context;
class NativeView;
class Vibrator;
class BluetoothHelper;
class UsbSerialHelper;
class IOIOHelper;

/**
 * A copy of android.os.Build.VERSION.SDK_INT.
 */
extern unsigned android_api_level;

extern Context *context;

extern NativeView *native_view;

extern Vibrator *vibrator;
extern bool os_haptic_feedback_enabled;

extern BluetoothHelper *bluetooth_helper;
extern UsbSerialHelper *usb_serial_helper;
extern IOIOHelper *ioio_helper;
