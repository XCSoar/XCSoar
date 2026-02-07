// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/CAI302.hpp"
#include "Internal.hpp"

static Device *
CAI302CreateOnPort(const DeviceConfig &config, Port &port)
{
  return new CAI302Device(config, port);
}

const struct DeviceRegister cai302_driver = {
  "CAI 302",
  "Cambridge CAI302",
  DeviceRegister::BULK_BAUD_RATE |
  DeviceRegister::DECLARE | DeviceRegister::LOGGER | DeviceRegister::MANAGE |
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  CAI302CreateOnPort,
};
