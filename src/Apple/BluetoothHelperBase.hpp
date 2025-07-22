// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PortBridge.hpp"
#include "DetectDeviceListener.hpp"

class BluetoothHelper {
public:
  virtual ~BluetoothHelper() = default;

  [[gnu::pure]] virtual bool HasBluetoothSupport() const noexcept = 0;
  [[gnu::pure]] virtual bool IsEnabled() const noexcept = 0;
  [[gnu::pure]] virtual const char *GetNameFromAddress(const char *address) const noexcept = 0;
  [[gnu::const]] bool HasLe() const noexcept {
		return true;
	}
   virtual void AddDetectDeviceListener(DetectDeviceListener &l) noexcept = 0;
   virtual void RemoveDetectDeviceListener(DetectDeviceListener &l) noexcept = 0;
	// virtual Java::LocalObject connectSensor(const char *address, SensorListener &listener) = 0;
	virtual PortBridge *connect(const char *address) = 0;
	// virtual PortBridge *connectHM10(const char *address) = 0;
	virtual PortBridge *createServer() = 0;
};
