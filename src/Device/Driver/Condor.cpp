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
#include "Protection.hpp"
#include "Units.hpp"
#include "Device/Parser.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

#include <stdlib.h>

class CondorDevice : public AbstractDevice {
public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info,
                         bool enable_baro);
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
cLXWP0(NMEAInputLine &line, NMEA_INFO *GPS_INFO, bool enable_baro)
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
    GPS_INFO->ProvideTrueAirspeedWithAltitude(Units::ToSysUnit(airspeed,
                                                               unKiloMeterPerHour),
                                              alt);

  if (enable_baro)
    // ToDo check if QNH correction is needed!
    GPS_INFO->ProvideBaroAltitudeTrue(NMEA_INFO::BARO_ALTITUDE_LX, alt);

  if (line.read_checked(value)) {
    GPS_INFO->ProvideTotalEnergyVario(value);
    TriggerVarioUpdate();
  }

  line.skip(6);

  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    GPS_INFO->ProvideExternalWind(wind);

  return true;
}

static bool
cLXWP1(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  (void)GPS_INFO;
  return true;
}

static bool
cLXWP2(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  (void)GPS_INFO;
  return true;
}

bool
CondorDevice::ParseNMEA(const char *String, NMEA_INFO *GPS_INFO,
                        bool enable_baro)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$LXWP0") == 0)
    return cLXWP0(line, GPS_INFO, enable_baro);

  if (strcmp(type, "$LXWP1") == 0)
    return cLXWP1(line, GPS_INFO);

  if (strcmp(type, "$LXWP2") == 0)
    return cLXWP2(line, GPS_INFO);

  return false;
}

static Device *
CondorCreateOnPort(Port *com_port)
{
  return new CondorDevice();
}

const struct DeviceRegister condorDevice = {
  _T("Condor"),
  drfBaroAlt,
  CondorCreateOnPort,
};
