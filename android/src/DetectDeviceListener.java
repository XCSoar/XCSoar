// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
