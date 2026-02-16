// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/LX.hpp"
#include "Internal.hpp"
#include "Device/Config.hpp"

static Device *
LXCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  const bool uses_speed = config.UsesSpeed();
  const unsigned baud_rate = uses_speed ? config.baud_rate : 0;
  const unsigned bulk_baud_rate = uses_speed ? config.bulk_baud_rate : 0;

  const bool is_nano = config.BluetoothNameStartsWith("LXNAV-NANO");

  return new LXDevice(com_port, baud_rate, bulk_baud_rate, config.use_second_device, is_nano);
}

const struct DeviceRegister lx_driver = {
  "LX",
  "LXNAV",
  DeviceRegister::DECLARE | DeviceRegister::LOGGER |
  DeviceRegister::PASS_THROUGH |
  DeviceRegister::BULK_BAUD_RATE |
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  LXCreateOnPort,
};
