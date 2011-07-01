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

#include "Device/Driver/Condor.hpp"
#include "Device/Driver.hpp"
#include "Units/Units.hpp"
#include "Device/Parser.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Compiler.h"

#include <stdlib.h>

class CondorDevice : public AbstractDevice {
public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO &info);
};

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  fixed bearing, norm;

  bool bearing_valid = line.read_checked(bearing);
  bool norm_valid = line.read_checked(norm);

  if (bearing_valid && norm_valid) {
    // Condor 1.1.4 outputs the direction that the wind is going to,
    // _not_ the direction it is coming from !!
    //
    // This seems to differ from the output that the LX devices are giving !!
    value_r.bearing = Angle::degrees(bearing).Reciprocal();
    value_r.norm = Units::ToSysUnit(norm, unKiloMeterPerHour);
    return true;
  } else
    return false;
}

static bool
cLXWP0(NMEAInputLine &line, NMEA_INFO &info)
{
  /*
  $LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1

   0 logger_stored (Y/N)
   1 IAS (kph) ----> Condor uses TAS!
   2 baroaltitude (m)
   3 vario (m/s)
   4-8 unknown
   9 heading of plane
  10 windcourse (deg)
  11 windspeed (kph)
  */

  fixed value;

  line.skip();

  fixed airspeed;
  bool tas_available = line.read_checked(airspeed);

  fixed alt = line.read(fixed_zero);

  if (tas_available)
    info.ProvideTrueAirspeedWithAltitude(Units::ToSysUnit(airspeed,
                                                               unKiloMeterPerHour),
                                              alt);

  // ToDo check if QNH correction is needed!
  info.ProvideBaroAltitudeTrue(alt);

  if (line.read_checked(value))
    info.ProvideTotalEnergyVario(value);

  line.skip(6);

  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind);

  return true;
}

static bool
cLXWP1(gcc_unused NMEAInputLine &line, gcc_unused NMEA_INFO &info)
{
  return true;
}

static bool
cLXWP2(gcc_unused NMEAInputLine &line, gcc_unused NMEA_INFO &info)
{
  return true;
}

bool
CondorDevice::ParseNMEA(const char *String, NMEA_INFO &info)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$LXWP0") == 0)
    return cLXWP0(line, info);

  if (strcmp(type, "$LXWP1") == 0)
    return cLXWP1(line, info);

  if (strcmp(type, "$LXWP2") == 0)
    return cLXWP2(line, info);

  return false;
}

static Device *
CondorCreateOnPort(gcc_unused Port *com_port)
{
  return new CondorDevice();
}

const struct DeviceRegister condorDevice = {
  _T("Condor"),
  _T("Condor Soaring Simulator"),
  0,
  CondorCreateOnPort,
};
