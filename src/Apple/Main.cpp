// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Main.hpp"
#include "PortBridge.hpp"
#include "BluetoothHelper.hpp"
#include "DetectDeviceListener.hpp"

using namespace UI;

BluetoothHelper *bluetooth_helper = new BluetoothHelper();

if (!bluetooth_helper->HasBluetoothSupport()) {
	bluetooth_helper = nullptr;
}
