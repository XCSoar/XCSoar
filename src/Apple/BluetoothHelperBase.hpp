// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DetectDeviceListener.hpp"
#include "NativeDetectDeviceListener.h"
#include "PortBridge.hpp"

class SensorListener;

class BluetoothHelper {
public:
  virtual ~BluetoothHelper() = default;

  [[gnu::pure]] virtual bool HasBluetoothSupport() const noexcept = 0;
  [[gnu::pure]] virtual bool IsEnabled() const noexcept = 0;
  [[gnu::pure]] virtual const char *
  GetNameFromAddress(const char *address) const noexcept = 0;
  [[gnu::const]] bool HasLe() const noexcept { return true; }
  virtual NativeDetectDeviceListener *
  AddDetectDeviceListener(DetectDeviceListener &l) noexcept = 0;
  virtual void
  RemoveDetectDeviceListener(NativeDetectDeviceListener *l) noexcept = 0;
  virtual void connectSensor(const char *address, SensorListener &listener) = 0;
  virtual PortBridge *connect(const char *address) = 0;
  virtual PortBridge *connectHM10(const char *address) = 0;
  virtual PortBridge *createServer() = 0;
};
