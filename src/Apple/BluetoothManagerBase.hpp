// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class BluetoothManager {
public:
  virtual ~BluetoothManager() = default;

  virtual void initialize() = 0;
  virtual void scanForDevices() = 0;
  virtual void connectToDevice(const char *identifier) = 0;
  virtual void sendData(const char *data) = 0;
};
