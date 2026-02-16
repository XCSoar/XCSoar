// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LXEosDevice.hpp"

static Device*
LXEosCreateOnPort([[maybe_unused]] const DeviceConfig& config,
                  [[maybe_unused]] Port& com_port)
{
  return new LXEosDevice(com_port);
}

const struct DeviceRegister lx_eos_driver = {
  "LxEos",
  "Lx Eos / Era",
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  LXEosCreateOnPort,
};
