// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/GDL90.hpp"
#include "Driver.hpp"

static Device *
GDL90CreateOnPort([[maybe_unused]] const DeviceConfig &config,
                  [[maybe_unused]] Port &com_port)
{
  LoadFromProfile(gdl90_settings);
  return new GDL90Device();
}

const struct DeviceRegister gdl90_driver = {
  "GDL90",
  "GDL90",
  DeviceRegister::MANAGE | DeviceRegister::RAW_GPS_DATA,
  GDL90CreateOnPort,
};

