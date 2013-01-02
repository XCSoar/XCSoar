/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Util/StringUtil.hpp"

#include <stdio.h>
#include <stdlib.h>

void
FormatUserAltitude(fixed value, TCHAR *buffer, bool include_unit)
{
  FormatAltitude(buffer, value, Units::GetUserAltitudeUnit(), include_unit);
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
FormatAlternateUserAltitude(fixed value, TCHAR *buffer, bool include_unit)
{
  FormatAltitude(buffer, value,
                 GetAlternateAltitudeUnit(Units::GetUserAltitudeUnit()),
                 include_unit);
}

void
FormatRelativeUserAltitude(fixed value, TCHAR *buffer, bool include_unit)
{
  FormatRelativeAltitude(buffer, value, Units::GetUserAltitudeUnit(),
                         include_unit);
}

void
FormatUserDistance(fixed value, TCHAR *buffer, bool include_unit, int precision)
{
  FormatDistance(buffer, value, Units::GetUserDistanceUnit(),
                 include_unit, precision);
}

Unit
FormatSmallUserDistance(TCHAR *buffer, fixed value, bool include_unit,
                        int precision)
{
  return FormatSmallDistance(buffer, value, Units::GetUserDistanceUnit(),
                             include_unit, precision);
}

Unit
FormatUserDistanceSmart(fixed value, TCHAR *buffer, bool include_unit,
                        fixed small_unit_threshold, fixed precision_threshold)
{
  return FormatDistanceSmart(buffer, value, Units::GetUserDistanceUnit(),
                             include_unit, small_unit_threshold,
                             precision_threshold);
}

Unit
FormatUserMapScale(fixed value, TCHAR *buffer, bool include_unit)
{
  return FormatDistanceSmart(buffer, value, Units::GetUserDistanceUnit(),
                             include_unit, fixed(1000), fixed(9.999));
}

void
FormatUserSpeed(fixed value, TCHAR *buffer, bool include_unit, bool precision)
{
  FormatSpeed(buffer, value, Units::GetUserSpeedUnit(), include_unit,
              precision);
}

void
FormatUserWindSpeed(fixed value, TCHAR *buffer, bool include_unit,
                    bool precision)
{
  FormatSpeed(buffer, value, Units::GetUserWindSpeedUnit(), include_unit,
              precision);
}

void
FormatUserTaskSpeed(fixed value, TCHAR *buffer, bool include_unit,
                    bool precision)
{
  FormatSpeed(buffer, value, Units::GetUserTaskSpeedUnit(), include_unit,
              precision);
}

const TCHAR*
GetUserVerticalSpeedFormat(bool include_unit, bool include_sign)
{
  return GetVerticalSpeedFormat(Units::GetUserVerticalSpeedUnit(), include_unit,
                                include_sign);
}

fixed
GetUserVerticalSpeedStep()
{
  return GetVerticalSpeedStep(Units::GetUserVerticalSpeedUnit());
}

void
FormatUserVerticalSpeed(fixed value, TCHAR *buffer, bool include_unit,
                        bool include_sign)
{
  FormatVerticalSpeed(buffer, value, Units::GetUserVerticalSpeedUnit(),
                      include_unit, include_sign);
}

void
FormatUserTemperature(fixed value, TCHAR *buffer, bool include_unit)
{
  FormatTemperature(buffer, value, Units::GetUserTemperatureUnit(),
                    include_unit);
}

void
FormatUserPressure(AtmosphericPressure pressure, TCHAR *buffer,
                   bool include_unit)
{
  FormatPressure(buffer, pressure, Units::GetUserPressureUnit(), include_unit);
}

const TCHAR*
GetUserPressureFormat(bool include_unit)
{
  return GetPressureFormat(Units::GetUserPressureUnit());
}

fixed
GetUserPressureStep()
{
  return GetPressureStep(Units::current.pressure_unit);
}
