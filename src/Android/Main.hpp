// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Context;
class NativeView;
class Vibrator;
class BluetoothHelper;
class UsbSerialHelper;
class IOIOHelper;

extern Context *context;

inline class _jobject *permission_manager;

extern NativeView *native_view;

extern Vibrator *vibrator;

extern BluetoothHelper *bluetooth_helper;
extern UsbSerialHelper *usb_serial_helper;
extern IOIOHelper *ioio_helper;
