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

#include "Internal.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "Engine/Navigation/SpeedVector.hpp"
#include "Units/Units.hpp"

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  fixed bearing, norm;

  bool bearing_valid = line.read_checked(bearing);
  bool norm_valid = line.read_checked(norm);

  if (bearing_valid && norm_valid) {
    value_r.bearing = Angle::degrees(bearing);
    value_r.norm = Units::ToSysUnit(norm, unKiloMeterPerHour);
    return true;
  } else
    return false;
}

static bool
LXWP0(NMEAInputLine &line, NMEA_INFO &info)
{
  /*
  $LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1

   0 loger_stored (Y/N)
   1 IAS (kph) ----> Condor uses TAS!
   2 baroaltitude (m)
   3-8 vario (m/s) (last 6 measurements in last second)
   9 heading of plane
  10 windcourse (deg)
  11 windspeed (kph)
  */

  fixed value;

  line.skip();

  fixed airspeed;
  bool tas_available = line.read_checked(airspeed);

  fixed alt = fixed_zero;
  if (line.read_checked(alt))
    /* a dump on a LX7007 has confirmed that the LX sends uncorrected
       altitude above 1013.25hPa here */
    info.ProvidePressureAltitude(alt);

  if (tas_available)
    info.ProvideTrueAirspeedWithAltitude(Units::ToSysUnit(airspeed,
                                                          unKiloMeterPerHour),
                                         alt);

  if (line.read_checked(value))
    info.ProvideTotalEnergyVario(value);

  line.skip(6);

  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind);

  return true;
}

static bool
LXWP1(gcc_unused NMEAInputLine &line, gcc_unused NMEA_INFO &info)
{
  /*
   * $LXWP1,
   * serial number,
   * instrument ID,
   * software version,
   * hardware version,
   * license string
   */
  return true;
}

static bool
LXWP2(NMEAInputLine &line, NMEA_INFO &info)
{
  /*
   * $LXWP2,
   * maccready value, (m/s)
   * ballast, (1.0 - 1.5)
   * bugs, (0 - 100%)
   * polar_a,
   * polar_b,
   * polar_c,
   * audio volume
   */

  fixed value;
  // MacCready value
  if (line.read_checked(value))
    info.settings.ProvideMacCready(value, info.clock);

  // Ballast
  if (line.read_checked(value))
    info.settings.ProvideBallastOverload(value, info.clock);

  // Bugs
  if (line.read_checked(value))
    info.settings.ProvideBugs((fixed(100) - value) / 100, info.clock);

  return true;
}

bool
LXDevice::ParseNMEA(const char *String, NMEA_INFO &info)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$LXWP0") == 0)
    return LXWP0(line, info);

  if (strcmp(type, "$LXWP1") == 0)
    return LXWP1(line, info);

  if (strcmp(type, "$LXWP2") == 0)
    return LXWP2(line, info);

  return false;
}
