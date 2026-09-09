// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

@class IOSBluetoothManager;

class DetectDeviceListener;
class PortBridge;

/**
 * The iOS counterpart of Android's #BluetoothHelper: provides
 * Bluetooth LE device discovery and GATT serial port connections via
 * CoreBluetooth.
 *
 * All methods may be called from any thread except the CoreBluetooth
 * dispatch queue (see #IOSBluetoothManager).
 */
class BluetoothHelper final {
  IOSBluetoothManager *manager;

public:
  BluetoothHelper() noexcept;
  ~BluetoothHelper() noexcept;

  BluetoothHelper(const BluetoothHelper &) = delete;
  BluetoothHelper &operator=(const BluetoothHelper &) = delete;

  /**
   * Is Bluetooth switched on?
   */
  [[gnu::pure]]
  bool IsEnabled() const noexcept;

  /**
   * Look up the name of a peripheral which was discovered or
   * connected earlier in this session.  May be called from any
   * thread, including the CoreBluetooth dispatch queue.
   *
   * @param address the identifier UUID of the peripheral
   * @return the peripheral's name or nullptr if it is unknown; the
   * returned string is never freed
   */
  [[gnu::pure]]
  const char *GetNameFromAddress(const char *address) const noexcept;

  /**
   * Start scanning for Bluetooth devices.  Call
   * RemoveDetectDeviceListener() with the same listener when you're
   * done.
   */
  void AddDetectDeviceListener(DetectDeviceListener &l) noexcept;

  /**
   * Stop scanning for Bluetooth devices.
   *
   * @param l the listener passed to AddDetectDeviceListener()
   */
  void RemoveDetectDeviceListener(DetectDeviceListener &l) noexcept;

  /**
   * Open a GATT serial port connection to the given peripheral.  The
   * connection is established asynchronously; this method never
   * returns nullptr, and problems are reported through the bridge's
   * #PortState.
   *
   * The returned object is owned by the caller.
   */
  PortBridge *connect(const char *address) noexcept;
};
