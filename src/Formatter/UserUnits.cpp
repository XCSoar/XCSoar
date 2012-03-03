/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Formatter/UserUnits.hpp"
#include "Formatter/Units.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "Math/Angle.hpp"
#include "Atmosphere/Pressure.hpp"
#include "DateTime.hpp"
#include "Util/StringUtil.hpp"

#include <stdio.h>
#include <stdlib.h>

void
Units::FormatUserAltitude(fixed value, TCHAR *buffer,
                          bool include_unit)
{
  FormatAltitude(buffer, value, current.altitude_unit, include_unit);
}

gcc_const
static Unit
GetAlternateAltitudeUnit(Unit unit)
{
  switch (unit) {
  case Unit::METER:
    return Unit::FEET;

  case Unit::FEET:
    return Unit::METER;

  default:
    return unit;
  }
}

void
Units::FormatAlternateUserAltitude(fixed value, TCHAR *buffer,
                                   bool include_unit)
{
  FormatAltitude(buffer, value, GetAlternateAltitudeUnit(current.altitude_unit),
                 include_unit);
}

void
Units::FormatRelativeUserAltitude(fixed value, TCHAR *buffer,
                                  bool include_unit)
{
  FormatRelativeAltitude(buffer, value, current.altitude_unit,
                         include_unit);
}

Unit
Units::FormatSmallUserDistance(TCHAR *buffer, fixed value,
                           bool include_unit, int precision)
{
  return FormatSmallDistance(buffer, value, current.distance_unit,
                             include_unit, precision);
}

Unit
Units::FormatUserDistanceSmart(fixed value, TCHAR *buffer,
                               bool include_unit)
{
  return FormatDistanceSmart(buffer, value, current.distance_unit,
                             include_unit);
}

Unit
Units::FormatUserMapScale(fixed value, TCHAR *buffer,
                          bool include_unit)
{
  return FormatDistanceSmart(buffer, value, current.distance_unit,
                             include_unit, fixed(1000), fixed(9.999));
}

void
Units::FormatUserSpeed(fixed value, TCHAR *buffer,
                       bool include_unit, bool precision)
{
  FormatSpeed(buffer, value, current.speed_unit, include_unit, precision);
}

void
Units::FormatUserWindSpeed(fixed value, TCHAR *buffer,
                           bool include_unit, bool precision)
{
  FormatSpeed(buffer, value, current.wind_speed_unit, include_unit, precision);
}

void
Units::FormatUserTaskSpeed(fixed value, TCHAR *buffer,
                           bool include_unit, bool precision)
{
  FormatSpeed(buffer, value, current.task_speed_unit, include_unit, precision);
}

const TCHAR*
Units::GetUserVerticalSpeedFormat(bool include_unit, bool include_sign)
{
  return GetVerticalSpeedFormat(current.vertical_speed_unit, include_unit,
                                include_sign);
}

fixed
Units::GetUserVerticalSpeedStep()
{
  return GetVerticalSpeedStep(current.vertical_speed_unit);
}

void
Units::FormatUserVerticalSpeed(fixed value, TCHAR *buffer,
                               bool include_unit, bool include_sign)
{
  FormatVerticalSpeed(buffer, value, current.vertical_speed_unit,
                      include_unit, include_sign);
}

void
Units::FormatUserTemperature(fixed value, TCHAR *buffer,
                             bool include_unit)
{
  FormatTemperature(buffer, value, current.temperature_unit, include_unit);
}

void
Units::FormatUserPressure(AtmosphericPressure pressure, TCHAR *buffer,
                          bool include_unit)
{
  FormatPressure(buffer, pressure, current.pressure_unit, include_unit);
}

const TCHAR*
Units::GetUserPressureFormat(bool include_unit)
{
  return GetPressureFormat(current.pressure_unit);
}

fixed
Units::GetUserPressureStep()
{
  return GetPressureStep(current.pressure_unit);
}
