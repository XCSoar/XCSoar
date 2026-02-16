// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Register.hpp"
#include "Device.hpp"

static Device *
ATR833CreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new ATR833Device(com_port);
}

const DeviceRegister atr833_driver = {
  "ATR833",
  "ATR833",
  DeviceRegister::RAW_GPS_DATA,
  ATR833CreateOnPort,
};
