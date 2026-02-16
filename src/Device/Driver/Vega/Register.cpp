// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Vega.hpp"
#include "Internal.hpp"

static Device *
VegaCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new VegaDevice(com_port);
}

const struct DeviceRegister vega_driver = {
  "Vega",
  "Vega",
  DeviceRegister::MANAGE |
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  VegaCreateOnPort,
};
