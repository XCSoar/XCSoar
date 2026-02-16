// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/IMI.hpp"
#include "Internal.hpp"

static Device *
IMICreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new IMIDevice(com_port);
}

const struct DeviceRegister imi_driver = {
  "IMI ERIXX",
  "IMI ERIXX",
  DeviceRegister::DECLARE | DeviceRegister::LOGGER,
  IMICreateOnPort,
};
