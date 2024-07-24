// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Fenix/FenixDevice.hpp"
#include "Device/Driver/Fenix.hpp"
#include "Device/Config.hpp"



static Device *FenixCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new FenixDevice(com_port);
}

const struct DeviceRegister fenix_driver = {
  _T("Fenix"),
  _T("Fenix"),
  DeviceRegister::DECLARE | DeviceRegister::LOGGER |
  DeviceRegister::MANAGE |
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  FenixCreateOnPort,
};
