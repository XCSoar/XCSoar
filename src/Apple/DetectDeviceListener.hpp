// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * C++ wrapper for the Java class BluetoothAdapter.LeScanCallback.
 */
class DetectDeviceListener {
public:
  /* keep this in sync with android/src/DetectDeviceListener.java */
  enum class Type {
    IOIO = 1,
    BLUETOOTH_CLASSIC = 2,
    BLUETOOTH_LE = 3,
    USB_SERIAL = 4,
  };

  static constexpr uint64_t FEATURE_HM10 = 0x1;
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
