// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

static constexpr unsigned NUMDEV = 8;
static constexpr unsigned INTERNAL_DEVICE_SLOT = NUMDEV - 1;

#if defined(ANDROID) || defined(__APPLE__)
#define HAVE_INTERNAL_GPS
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
/* Bluetooth LE serial ports via CoreBluetooth, see
   src/Apple/BluetoothHelper.hpp */
#define HAVE_APPLE_BLUETOOTH
#endif
