// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Main.hpp"
#include "BluetoothHelper.hpp"
#include "DetectDeviceListener.hpp"
#include "PortBridge.hpp"

BluetoothHelper *bluetooth_helper;

int
main(int argc, char **argv)
{
  bluetooth_helper = CreateBluetoothHelper();

  if (!bluetooth_helper->HasBluetoothSupport()) {
    bluetooth_helper = nullptr;
  }
}
