// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../Volkslogger.hpp"
#include "Internal.hpp"
#include "Device/Config.hpp"

static Device *
VolksloggerCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  unsigned bulkrate;
  if (config.bulk_baud_rate == 0)
    bulkrate = config.baud_rate; //Default: use standard baud rate
  else
    bulkrate = config.bulk_baud_rate;

  //lowest value the Volkslogger api can accept as bulkrate is 9600
  if(bulkrate < 9600)
    bulkrate = 9600;


  return new VolksloggerDevice(com_port, bulkrate);
}

const struct DeviceRegister volkslogger_driver = {
  "Volkslogger",
  "Volkslogger",
  DeviceRegister::DECLARE | DeviceRegister::LOGGER |
  DeviceRegister::BULK_BAUD_RATE,
  VolksloggerCreateOnPort,
};
