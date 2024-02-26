// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"
#include "Geo/SpeedVector.hpp"
#include "Math/Angle.hpp"

#include <string.h>

NMEAInputLine::NMEAInputLine(const char* line) noexcept
  :CSVLine(line)
{
  const char* asterisk = strchr(line, '*');
  if (asterisk != NULL)
    end = asterisk;
}

bool
NMEAInputLine::ReadBearing(Angle &value_r) noexcept
{
  double value;
  if (!ReadChecked(value))
    return false;

  if (value <= -1 || value >= 361)
    return false;

  value_r = Angle::Degrees(value).AsBearing();
  return true;
}

bool
NMEAInputLine::ReadSpeedVectorKPH(SpeedVector &value_r) noexcept
{
  Angle bearing;
  double norm;

  bool bearing_valid = ReadBearing(bearing);
  bool norm_valid = ReadChecked(norm) && norm >= 0 && norm < 2000;

  bool valid = bearing_valid && norm_valid;
  if (valid)
    value_r = {bearing, Units::ToSysUnit(norm, Unit::KILOMETER_PER_HOUR)};

  return valid;
}

bool
NMEAInputLine::ReadSwappedSpeedVectorKPH(SpeedVector &value_r) noexcept
{
  Angle bearing;
  double norm;

  bool norm_valid = ReadChecked(norm) && norm >= 0 && norm < 2000;
  bool bearing_valid = ReadBearing(bearing);

  bool valid = bearing_valid && norm_valid;
  if (valid)
    value_r = {bearing, Units::ToSysUnit(norm, Unit::KILOMETER_PER_HOUR)};

  return valid;
}
