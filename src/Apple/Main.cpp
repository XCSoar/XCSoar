// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Main.hpp"
#include "BluetoothHelper.hpp"

BluetoothHelper *bluetooth_helper;

// Initialize bluetooth helper - this will be called from the main XCSoar
// startup
void
InitializeAppleServices()
{
  bluetooth_helper = CreateBluetoothHelper();

  if (!bluetooth_helper->HasBluetoothSupport()) {
    bluetooth_helper = nullptr;
  }
}

// Cleanup bluetooth helper - this will be called from XCSoar shutdown
void
DeinitializeAppleServices()
{
  if (bluetooth_helper) {
    delete bluetooth_helper;
    bluetooth_helper = nullptr;
  }
}
