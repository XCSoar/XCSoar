// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Flytec.hpp"
#include "Device.hpp"

static Device *
FlytecCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new FlytecDevice(com_port);
}

const struct DeviceRegister flytec_driver = {
  "Flytec", "Flytec 5030 / Brauniger",
  0 /* DeviceRegister::LOGGER deactivated until current firmware supports this */,
  FlytecCreateOnPort,
};
