// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/BlueFlyVario.hpp"
#include "Internal.hpp"

static Device *
BlueFlyCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new BlueFlyDevice(com_port);
}

const struct DeviceRegister bluefly_driver = {
  "BlueFly",
  "BlueFly Vario",
  DeviceRegister::MANAGE,
  BlueFlyCreateOnPort,
};
