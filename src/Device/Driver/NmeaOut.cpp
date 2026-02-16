// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/NmeaOut.hpp"
#include "Device/Driver.hpp"

const struct DeviceRegister nmea_out_driver = {
  "NmeaOut",
  "NMEA output",
  DeviceRegister::NMEA_OUT|DeviceRegister::NO_TIMEOUT,
  nullptr,
};

