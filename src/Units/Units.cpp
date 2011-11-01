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

// TODO code: units stuff
// - check buffer size in LongitudeToString and LatiditudeToString
// - unit dialog support

#include "Units/Units.hpp"
#include "Math/Angle.hpp"

#include <stdlib.h>
#include <math.h>
#include <tchar.h>

CoordinateFormats Units::coordinate_format;

//SI to Local Units

const UnitDescriptor Units::unit_descriptors[] = {
  { NULL, fixed_one, fixed_zero },
  { _T("km"), fixed_constant(0.001, 0x41893LL), fixed_zero },
  { _T("nm"), fixed_constant(0.000539956803, 0x2362fLL), fixed_zero },
  { _T("sm"), fixed_constant(0.000621371192, 0x28b8eLL), fixed_zero },
  { _T("km/h"), fixed_constant(3.6, 0x39999999LL), fixed_zero },
  { _T("kt"), fixed_constant(1.94384449, 0x1f19fcaeLL), fixed_zero },
  { _T("mph"), fixed_constant(2.23693629, 0x23ca7db5LL), fixed_zero },
  { _T("m/s"), fixed_one, fixed_zero },
  { _T("fpm"), fixed_constant(196.850394, 0xc4d9b36bdLL), fixed_zero },
  { _T("m"), fixed_one, fixed_zero },
  { _T("ft"), fixed_constant(3.2808399, 0x347e51faLL), fixed_zero },
  { _T("FL"), fixed_constant(0.032808399, 0x866219LL), fixed_zero },
  { _T("K"), fixed_one, fixed_zero },
  { _T(DEG)_T("C"), fixed_one, fixed_constant(-273.15, -73323144806LL) },
  { _T(DEG)_T("F"), fixed_constant(1.8, 0x1cccccccLL),
    fixed_constant(-459.67, -123391726059LL) },
  { _T("hPa"), fixed_one, fixed_zero },
  { _T("mmHg"), fixed(0.7500616827041698), fixed_zero },
  { _T("inHg"), fixed(0.0295287441401431), fixed_zero },
};

UnitSetting Units::current = {
  unKiloMeter,
  unMeter,
  unGradCelcius,
  unKiloMeterPerHour,
  unMeterPerSecond,
  unKiloMeterPerHour,
  unKiloMeterPerHour
};

void
Units::LongitudeToDMS(Angle longitude, int *dd, int *mm, int *ss, bool *east)
{
  // if (Longitude is negative) -> Longitude is West otherwise East
  *east = (longitude.Sign() >= 0);

  unsigned value = (unsigned)(longitude.AbsoluteDegrees() * 3600 +
                              fixed_half);

  *ss = value % 60;
  value /= 60;

  *mm = value % 60;
  value /= 60;

  *dd = value;
}

void
Units::LatitudeToDMS(Angle latitude, int *dd, int *mm, int *ss, bool *north)
{
  // if (Latitude is negative) -> Latitude is South otherwise North
  *north = (latitude.Sign() >= 0);

  unsigned value = (unsigned)(latitude.AbsoluteDegrees() * 3600 +
                              fixed_half);

  *ss = value % 60;
  value /= 60;

  *mm = value % 60;
  value /= 60;

  *dd = value;
}

const TCHAR *
Units::GetUnitName(Unit unit)
{
  return unit_descriptors[unit].name;
}

CoordinateFormats
Units::GetCoordinateFormat()
{
  return coordinate_format;
}

void
Units::SetCoordinateFormat(CoordinateFormats format)
{
  coordinate_format = format;
}

Unit
Units::GetUserDistanceUnit()
{
  return current.distance_unit;
}

void
Units::SetUserDistanceUnit(Unit unit)
{
  current.distance_unit = unit;
}

Unit
Units::GetUserAltitudeUnit()
{
  return current.altitude_unit;
}

void
Units::SetUserAltitudeUnit(Unit unit)
{
  current.altitude_unit = unit;
}

Unit
Units::GetUserTemperatureUnit()
{
  return current.temperature_unit;
}

void
Units::SetUserTemperatureUnit(Unit unit)
{
  current.temperature_unit = unit;
}

Unit
Units::GetUserSpeedUnit()
{
  return current.speed_unit;
}

void
Units::SetUserSpeedUnit(Unit unit)
{
  current.speed_unit = unit;
}

Unit
Units::GetUserTaskSpeedUnit()
{
  return current.task_speed_unit;
}

void
Units::SetUserTaskSpeedUnit(Unit unit)
{
  current.task_speed_unit = unit;
}

Unit
Units::GetUserVerticalSpeedUnit()
{
  return current.vertical_speed_unit;
}

void
Units::SetUserVerticalSpeedUnit(Unit unit)
{
  current.vertical_speed_unit = unit;
}

Unit
Units::GetUserWindSpeedUnit()
{
  return current.wind_speed_unit;
}

void
Units::SetUserWindSpeedUnit(Unit unit)
{
  current.wind_speed_unit = unit;
}

Unit
Units::GetUserUnitByGroup(UnitGroup group)
{
  switch (group) {
  case ugNone:
    return unUndef;
  case ugDistance:
    return GetUserDistanceUnit();
  case ugAltitude:
    return GetUserAltitudeUnit();
  case ugTemperature:
    return GetUserTemperatureUnit();
  case ugHorizontalSpeed:
    return GetUserSpeedUnit();
  case ugVerticalSpeed:
    return GetUserVerticalSpeedUnit();
  case ugWindSpeed:
    return GetUserWindSpeedUnit();
  case ugTaskSpeed:
    return GetUserTaskSpeedUnit();
  default:
    return unUndef;
  }
}

const TCHAR *
Units::GetSpeedName()
{
  return GetUnitName(GetUserSpeedUnit());
}

const TCHAR *
Units::GetVerticalSpeedName()
{
  return GetUnitName(GetUserVerticalSpeedUnit());
}

const TCHAR *
Units::GetDistanceName()
{
  return GetUnitName(GetUserDistanceUnit());
}

const TCHAR *
Units::GetAltitudeName()
{
  return GetUnitName(GetUserAltitudeUnit());
}

const TCHAR *
Units::GetTemperatureName()
{
  return GetUnitName(GetUserTemperatureUnit());
}

const TCHAR *
Units::GetTaskSpeedName()
{
  return GetUnitName(GetUserTaskSpeedUnit());
}

fixed
Units::ToUserUnit(fixed value, Unit unit)
{
  const UnitDescriptor *ud = &unit_descriptors[unit];
  return value * ud->factor_to_user + ud->offset_to_user;
}

fixed
Units::ToSysUnit(fixed value, Unit unit)
{
  const UnitDescriptor *ud = &unit_descriptors[unit];
  return (value - ud->offset_to_user) / ud->factor_to_user;
}
