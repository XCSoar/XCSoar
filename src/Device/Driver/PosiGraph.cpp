/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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

class PGDevice : public LXDevice {
public:
  PGDevice(Port &_port, unsigned baud_rate, unsigned bulk_baud_rate)
    :LXDevice(_port, baud_rate, bulk_baud_rate) {}

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
  char type[16];
  line.Read(type, 16);

  // $GPWIN ... Winpilot proprietary sentance includinh baro altitude
  // $GPWIN ,01900 , 0 , 5159 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 * 6 B , 0 7 * 6 0 E
  if (StringIsEqual(type, "$GPWIN"))
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
  _T("PosiGraph Logger"),
  _T("PosiGraph Logger"),
  DeviceRegister::DECLARE | DeviceRegister::BULK_BAUD_RATE |
  DeviceRegister::LOGGER,
  PGCreateOnPort,
};
