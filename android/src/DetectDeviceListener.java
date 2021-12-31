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

package org.xcsoar;

/**
 * Continuously receives callbacks about detected or updated devices.
 */
interface DetectDeviceListener {
  /* keep this in sync with src/Android/DetectDeviceListener.hpp */
  static final int TYPE_IOIO = 1;
  static final int TYPE_BLUETOOTH_CLASSIC = 2;
  static final int TYPE_BLUETOOTH_LE = 3;
  static final int TYPE_USB_SERIAL = 4;

  static final long FEATURE_HM10 = 0x1;
  static final long FEATURE_HEART_RATE = 0x2;
  static final long FEATURE_FLYTEC_SENSBOX = 0x4;

  /**
   * A new device was detected or new information about a device
   * became available.
   *
   * @param type the type of device
   * @param address a type-specific address, probably not
   * human-readable (e.g. Bluetooth MAC)
   * @param name a human-readable name; may be null if this is not
   * (yet) known
   * @param features a (type-specific) bit mask of detected features
   */
  void onDeviceDetected(int type, String address,
                        String name, long features);
}
