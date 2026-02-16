// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/FLARM.hpp"
#include "Device.hpp"

static Device *
FlarmCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new FlarmDevice(com_port);
}

const struct DeviceRegister flarm_driver = {
  "FLARM", "FLARM",
  DeviceRegister::DECLARE | DeviceRegister::LOGGER | DeviceRegister::MANAGE,
  FlarmCreateOnPort,
};
