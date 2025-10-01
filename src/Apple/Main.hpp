// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class BluetoothHelper;

extern BluetoothHelper *bluetooth_helper;

// Initialize Apple-specific services (bluetooth, etc.)
void InitializeAppleServices();

// Cleanup Apple-specific services
void DeinitializeAppleServices();
