// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Generic.hpp"
#include "Device/Driver.hpp"

class GenericDevice : public AbstractDevice {
};

static Device *
GenericCreateOnPort([[maybe_unused]] const DeviceConfig &config, [[maybe_unused]] Port &com_port)
{
  return new GenericDevice();
}

const struct DeviceRegister generic_driver = {
  "Generic",
  "Generic",
  0,
  GenericCreateOnPort,
};
