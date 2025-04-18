// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Units/System.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"

using std::string_view_literals::operator""sv;

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  Angle bearing;
  double norm;

  bool bearing_valid = line.ReadBearing(bearing);
  bool norm_valid = line.ReadChecked(norm) &&
    norm >= 0 && norm < 2000;

  if (bearing_valid && norm_valid) {
    value_r.bearing = bearing;
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
cai_PCAIB([[maybe_unused]] NMEAInputLine &line, [[maybe_unused]] NMEAInfo &info)
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
cai_PCAID(NMEAInputLine &line, NMEAInfo &data)
{
  line.Skip();

  double value;
  if (line.ReadChecked(value))
    data.ProvidePressureAltitude(value);

  unsigned enl;
  if (line.ReadChecked(enl)) {
    data.engine_noise_level = enl;
    data.engine_noise_level_available.Update(data.clock);
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
cai_w(NMEAInputLine &line, NMEAInfo &info)
{
  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind.Reciprocal());

  line.Skip(2);

  double value;
  if (line.ReadChecked(value))
    info.ProvideBaroAltitudeTrue(value - 1000);

  if (line.ReadChecked(value))
    info.settings.ProvideQNH(AtmosphericPressure::HectoPascal(value),
                             info.clock);

  if (line.ReadChecked(value))
    info.ProvideTrueAirspeed(value / 100);

  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(Units::ToSysUnit((value - 200) / 10.,
                                                  Unit::KNOTS));

  line.Skip(2);

  int i;

  if (line.ReadChecked(i))
    info.settings.ProvideMacCready(Units::ToSysUnit(i / 10., Unit::KNOTS),
                                   info.clock);

  if (line.ReadChecked(i))
    info.settings.ProvideBallastFraction(i / 100., info.clock);

  if (line.ReadChecked(i))
    info.settings.ProvideBugs(i / 100., info.clock);

  return true;
}

bool
CAI302Device::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);

  const auto type = line.ReadView();
  if (type == "$PCAIB"sv)
    return cai_PCAIB(line, info);
  else if (type == "$PCAID"sv)
    return cai_PCAID(line, info);
  else if (type == "!w"sv)
    return cai_w(line, info);
  else
    return false;
}
