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
#include "Units/Descriptor.hpp"
#include "Math/Angle.hpp"

#include <stdlib.h>
#include <math.h>
#include <tchar.h>

UnitSetting Units::current = {
  unKiloMeter,
  unMeter,
  unGradCelcius,
  unKiloMeterPerHour,
  unMeterPerSecond,
  unKiloMeterPerHour,
  unKiloMeterPerHour,
  unHectoPascal,
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

CoordinateFormats
Units::GetCoordinateFormat()
{
  return current.coordinate_format;
}

Unit
Units::GetUserDistanceUnit()
{
  return current.distance_unit;
}

Unit
Units::GetUserAltitudeUnit()
{
  return current.altitude_unit;
}

Unit
Units::GetUserTemperatureUnit()
{
  return current.temperature_unit;
}

Unit
Units::GetUserSpeedUnit()
{
  return current.speed_unit;
}

Unit
Units::GetUserTaskSpeedUnit()
{
  return current.task_speed_unit;
}

Unit
Units::GetUserVerticalSpeedUnit()
{
  return current.vertical_speed_unit;
}

Unit
Units::GetUserWindSpeedUnit()
{
  return current.wind_speed_unit;
}

Unit
Units::GetUserPressureUnit()
{
  return current.pressure_unit;
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
  case ugPressure:
    return GetUserPressureUnit();
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

const TCHAR *
Units::GetPressureName()
{
  return GetUnitName(GetUserPressureUnit());
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
