/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
