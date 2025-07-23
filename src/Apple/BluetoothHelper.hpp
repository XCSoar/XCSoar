// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BluetoothHelperBase.hpp"
#include "DetectDeviceListener.hpp"
#include "IOSBluetoothManager.h"
#include "NativeDetectDeviceListener.h"
#include "PortBridge.hpp"

@class IOSBluetoothManager;

class BluetoothHelperIOS : public BluetoothHelper {
public:
  BluetoothHelperIOS();
  virtual ~BluetoothHelperIOS();

  [[gnu::pure]] bool HasBluetoothSupport() const noexcept override;
  [[gnu::pure]] bool IsEnabled() const noexcept override;
  [[gnu::pure]] const char *
  GetNameFromAddress(const char *address) const noexcept override;

  NativeDetectDeviceListener *
  AddDetectDeviceListener(DetectDeviceListener &l) noexcept override;
  void
  RemoveDetectDeviceListener(NativeDetectDeviceListener *l) noexcept override;

  // Java::LocalObject connectSensor(const char *address, SensorListener
  // &listener) override;
  PortBridge *connect(const char *address) override;
  // PortBridge *connectHM10(const char *address) override;
  PortBridge *createServer() override;

private:
  IOSBluetoothManager *manager;
};

extern "C" BluetoothHelper *CreateBluetoothHelper();
