/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Device/Driver/Flytec.hpp"
#include "Device/Parser.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units.hpp"

#include <stdlib.h>
#include <math.h>

class FlytecDevice : public AbstractDevice {
private:
  Port *port;

public:
  FlytecDevice(Port *_port):port(_port) {}

public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info);
};

/**
 * Parse a "$BRSF" sentence.
 *
 * Example: "$BRSF,063,-013,-0035,1,193,00351,535,485*38"
 */
static bool
FlytecParseBRSF(NMEAInputLine &line, NMEA_INFO &info)
{
  fixed value;

  // 0 = indicated or true airspeed [km/h]
  if (line.read_checked_compare(value, "KH"))
    // XXX is that TAS or IAS?  Documentation isn't clear.
    info.ProvideBothAirspeeds(Units::ToSysUnit(value, unKiloMeterPerHour));

  // 1 = integrated vario [dm/s]
  // 2 = altitude A2 [m] (XXX what's this?)
  // 3 = waypoint
  // 4 = bearing to waypoint [degrees]
  // 5 = distance to waypoint [100m]
  // 6 = MacCready speed to fly [100m/h]
  // 7 = speed to fly, best glide [100m/h]

  return true;
}

/**
 * Parse a "$VMVABD" sentence.
 *
 * Example: "$VMVABD,0000.0,M,0547.0,M,-0.0,,,MS,0.0,KH,22.4,C*65"
 */
static bool
FlytecParseVMVABD(NMEAInputLine &line, NMEA_INFO &info)
{
  fixed value;

  // 0,1 = GPS altitude, unit
  if (line.read_checked_compare(info.GPSAltitude, "M"))
    info.GPSAltitudeAvailable.update(info.Time);

  // 2,3 = baro altitude, unit
  if (line.read_checked_compare(value, "M"))
    info.ProvideBaroAltitudeTrue(value);

  // 4-7 = integrated vario, unit
  line.skip(4);

  // 8,9 = indicated or true airspeed, unit
  if (line.read_checked_compare(value, "KH"))
    // XXX is that TAS or IAS?  Documentation isn't clear.
    info.ProvideBothAirspeeds(Units::ToSysUnit(value, unKiloMeterPerHour));

  // 10,11 = temperature, unit
  info.TemperatureAvailable =
    line.read_checked_compare(value, "C");
  if (info.TemperatureAvailable)
    info.OutsideAirTemperature = Units::ToSysUnit(value, unGradCelcius);

  return true;
}

bool
FlytecDevice::ParseNMEA(const char *_line, NMEA_INFO *info)
{
  NMEAInputLine line(_line);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$BRSF") == 0)
    return FlytecParseBRSF(line, *info);
  else if (strcmp(type, "$VMVABD") == 0)
    return FlytecParseVMVABD(line, *info);
  else
    return false;
}

static Device *
FlytecCreateOnPort(Port *com_port)
{
  return new FlytecDevice(com_port);
}

const struct DeviceRegister flytec_device_driver = {
  _T("Flytec"),
  drfBaroAlt,
  FlytecCreateOnPort,
};
