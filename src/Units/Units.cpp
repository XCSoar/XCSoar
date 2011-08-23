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

CoordinateFormats_t Units::CoordinateFormat;

//SI to Local Units

const UnitDescriptor_t Units::UnitDescriptors[] = {
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

UnitSetting Units::Current = {
  unKiloMeter,
  unMeter,
  unGradCelcius,
  unKiloMeterPerHour,
  unMeterPerSecond,
  unKiloMeterPerHour,
  unKiloMeterPerHour
};

void
Units::LongitudeToDMS(Angle Longitude, int *dd, int *mm, int *ss, bool *east)
{
  // if (Longitude is negative) -> Longitude is West otherwise East
  *east = (Longitude.sign() >= 0);

  unsigned value = (unsigned)(Longitude.magnitude_degrees() * 3600 +
                              fixed_half);

  *ss = value % 60;
  value /= 60;

  *mm = value % 60;
  value /= 60;

  *dd = value;
}

void
Units::LatitudeToDMS(Angle Latitude, int *dd, int *mm, int *ss, bool *north)
{
  // if (Latitude is negative) -> Latitude is South otherwise North
  *north = (Latitude.sign() >= 0);

  unsigned value = (unsigned)(Latitude.magnitude_degrees() * 3600 +
                              fixed_half);

  *ss = value % 60;
  value /= 60;

  *mm = value % 60;
  value /= 60;

  *dd = value;
}

const TCHAR *
Units::GetUnitName(Units_t Unit)
{
  return UnitDescriptors[Unit].Name;
}

CoordinateFormats_t
Units::GetCoordinateFormat()
{
  return CoordinateFormat;
}

void
Units::SetCoordinateFormat(CoordinateFormats_t NewFormat)
{
  CoordinateFormat = NewFormat;
}

Units_t
Units::GetUserDistanceUnit()
{
  return Current.DistanceUnit;
}

void
Units::SetUserDistanceUnit(Units_t NewUnit)
{
  Current.DistanceUnit = NewUnit;
}

Units_t
Units::GetUserAltitudeUnit()
{
  return Current.AltitudeUnit;
}

void
Units::SetUserAltitudeUnit(Units_t NewUnit)
{
  Current.AltitudeUnit = NewUnit;
}

Units_t
Units::GetUserTemperatureUnit()
{
  return Current.TemperatureUnit;
}

void
Units::SetUserTemperatureUnit(Units_t NewUnit)
{
  Current.TemperatureUnit = NewUnit;
}

Units_t
Units::GetUserSpeedUnit()
{
  return Current.SpeedUnit;
}

void
Units::SetUserSpeedUnit(Units_t NewUnit)
{
  Current.SpeedUnit = NewUnit;
}

Units_t
Units::GetUserTaskSpeedUnit()
{
  return Current.TaskSpeedUnit;
}

void
Units::SetUserTaskSpeedUnit(Units_t NewUnit)
{
  Current.TaskSpeedUnit = NewUnit;
}

Units_t
Units::GetUserVerticalSpeedUnit()
{
  return Current.VerticalSpeedUnit;
}

void
Units::SetUserVerticalSpeedUnit(Units_t NewUnit)
{
  Current.VerticalSpeedUnit = NewUnit;
}

Units_t
Units::GetUserWindSpeedUnit()
{
  return Current.WindSpeedUnit;
}

void
Units::SetUserWindSpeedUnit(Units_t NewUnit)
{
  Current.WindSpeedUnit = NewUnit;
}

Units_t
Units::GetUserUnitByGroup(UnitGroup_t UnitGroup)
{
  switch (UnitGroup) {
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
Units::ToUserUnit(fixed Value, Units_t Unit)
{
  const UnitDescriptor_t *pU = &UnitDescriptors[Unit];
  return Value * pU->ToUserFact + pU->ToUserOffset;
}

fixed
Units::ToSysUnit(fixed Value, Units_t Unit)
{
  const UnitDescriptor_t *pU = &UnitDescriptors[Unit];
  return (Value - pU->ToUserOffset) / pU->ToUserFact;
}
