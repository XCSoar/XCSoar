// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/InputLine.hpp"
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

  if (value < 0 || value > 360)
    return false;

  value_r = Angle::Degrees(value).AsBearing();
  return true;
}
