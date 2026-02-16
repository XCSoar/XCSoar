// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

// created 20070428 sgi, basic NMEA sentance supported
// WARNING PosiGraph dont send any GGA sentance
// GGA sentance trigger nav process in XCSoar
// Posigraph driver trigger Nav process with RMC sentance (if driver is devA())

// ToDo

// adding baro alt sentance paser to support baro source priority  if (d == pDevPrimaryBaroSource){...}

#include "Device/Driver/PosiGraph.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "Device/Driver.hpp"
#include "Device/Config.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

using std::string_view_literals::operator""sv;

class PGDevice : public LXDevice {
public:
  PGDevice(Port &_port, unsigned baud_rate, unsigned bulk_baud_rate)
    :LXDevice(_port, baud_rate, bulk_baud_rate, true) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

static bool
GPWIN(NMEAInputLine &line, NMEAInfo &info)
{
  line.Skip(2);

  double value;
  if (line.ReadChecked(value))
    info.ProvidePressureAltitude(value / 10);

  return false;
}

bool
PGDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  NMEAInputLine line(String);

  const auto type = line.ReadView();

  // $GPWIN ... Winpilot proprietary sentance includinh baro altitude
  // $GPWIN ,01900 , 0 , 5159 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 * 6 B , 0 7 * 6 0 E
  if (type == "$GPWIN"sv)
    return GPWIN(line, info);
  else
    return LXDevice::ParseNMEA(String, info);
}

static Device *
PGCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new PGDevice(com_port, config.baud_rate, config.bulk_baud_rate);
}

const struct DeviceRegister posigraph_driver = {
  "PosiGraph Logger",
  "PosiGraph Logger",
  DeviceRegister::DECLARE | DeviceRegister::BULK_BAUD_RATE |
  DeviceRegister::LOGGER,
  PGCreateOnPort,
};
