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
#include "Units/Units.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  fixed bearing, norm;

  bool bearing_valid = line.read_checked(bearing);
  bool norm_valid = line.read_checked(norm);

  if (bearing_valid && norm_valid) {
    value_r.bearing = Angle::degrees(bearing);
    value_r.norm = norm / 10;
    return true;
  }

  return false;
}

/*
$PCAIB,<1>,<2>,<CR><LF>
<1> Destination Navpoint elevation in meters, format XXXXX (leading zeros will be transmitted)
<2> Destination Navpoint attribute word, format XXXXX (leading zeros will be transmitted)
*/
static bool
cai_PCAIB(gcc_unused NMEAInputLine &line, gcc_unused NMEA_INFO &info)
{
  return true;
}

/*
$PCAID,<1>,<2>,<3>,<4>*hh<CR><LF>
<1> Logged 'L' Last point Logged 'N' Last Point not logged
<2> Barometer Altitude in meters (Leading zeros will be transmitted)
<3> Engine Noise Level
<4> Log Flags
*hh Checksum, XOR of all bytes of the sentence after the $ and before the *
*/
static bool
cai_PCAID(NMEAInputLine &line, NMEA_INFO &data)
{
  line.skip();

  fixed value;
  if (line.read_checked(value))
    data.ProvidePressureAltitude(value);

  unsigned enl;
  if (line.read_checked(enl)) {
    data.engine_noise_level = enl;
    data.engine_noise_level_available.Update(data.Time);
  }

  return true;
}

/*
!w,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>,<13>*hh<CR><LF>
<1>  Vector wind direction in degrees
<2>  Vector wind speed in 10ths of meters per second
<3>  Vector wind age in seconds
<4>  Component wind in 10ths of Meters per second + 500 (500 = 0, 495 = 0.5 m/s tailwind)
<5>  True altitude in Meters + 1000
<6>  Instrument QNH setting
<7>  True airspeed in 100ths of Meters per second
<8>  Variometer reading in 10ths of knots + 200
<9>  Averager reading in 10ths of knots + 200
<10> Relative variometer reading in 10ths of knots + 200
<11> Instrument MacCready setting in 10ths of knots
<12> Instrument Ballast setting in percent of capacity
<13> Instrument Bug setting
*hh  Checksum, XOR of all bytes
*/
static bool
cai_w(NMEAInputLine &line, NMEA_INFO &info)
{
  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind.Reciprocal());

  line.skip(2);

  fixed value;
  if (line.read_checked(value))
    info.ProvideBaroAltitudeTrue(value - fixed(1000));

  if (line.read_checked(value))
    info.settings.ProvideQNH(value, info.Time);

  if (line.read_checked(value))
    info.ProvideTrueAirspeed(value / 100);

  if (line.read_checked(value))
    info.ProvideTotalEnergyVario(Units::ToSysUnit((value - fixed(200)) / 10,
                                                  unKnots));

  line.skip(2);

  int i;

  if (line.read_checked(i))
    info.settings.ProvideMacCready(Units::ToSysUnit(fixed(i) / 10, unKnots),
                                        info.Time);

  if (line.read_checked(i))
    info.settings.ProvideBallast(fixed(i) / 100, info.Time);

  if (line.read_checked(i))
    info.settings.ProvideBugs(fixed(i) / 100, info.Time);

  return true;
}

bool
CAI302Device::ParseNMEA(const char *String, NMEA_INFO &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$PCAIB") == 0)
    return cai_PCAIB(line, info);

  if (strcmp(type, "$PCAID") == 0)
    return cai_PCAID(line, info);

  if (strcmp(type, "!w") == 0)
    return cai_w(line, info);

  return false;
}
