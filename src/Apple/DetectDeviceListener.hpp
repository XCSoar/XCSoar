// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * A listener interface which receives callbacks about detected
 * (Bluetooth) devices.  This is a copy of the Android declaration
 * (src/Android/DetectDeviceListener.hpp) so shared code like the port
 * picker can use the same interface on both platforms.
 *
 * Keep this in sync with src/Android/DetectDeviceListener.hpp and
 * android/src/DetectDeviceListener.java.
 */
class DetectDeviceListener {
public:
  /* keep this in sync with src/Android/DetectDeviceListener.hpp and
     android/src/DetectDeviceListener.java */
  enum class Type {
    IOIO = 1,
    BLUETOOTH_CLASSIC = 2,
    BLUETOOTH_LE = 3,
    USB_SERIAL = 4,
  };

  static constexpr uint64_t FEATURE_BLE_SERIAL = 0x1;
  static constexpr uint64_t FEATURE_HEART_RATE = 0x2;
  static constexpr uint64_t FEATURE_FLYTEC_SENSBOX = 0x4;

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
  virtual void OnDeviceDetected(Type type, const char *address,
                                const char *name,
                                uint64_t features) noexcept = 0;
};
