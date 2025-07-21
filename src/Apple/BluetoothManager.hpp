// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BluetoothManagerBase.hpp"
#include "IOSBluetoothManager.h"

@class IOSBluetoothManager;

class BluetoothManagerIOS : public BluetoothManager {
public:
  BluetoothManagerIOS();
  virtual ~BluetoothManagerIOS();

  void initialize() override;
  void scanForDevices() override;
  void connectToDevice(const char *identifier) override;
  void sendData(const char *data) override;

private:
  IOSBluetoothManager *manager;
};

extern "C" BluetoothManager *CreateBluetoothManager();
