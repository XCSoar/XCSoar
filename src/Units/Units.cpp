/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "Atmosphere/Pressure.hpp"

UnitSetting Units::current = {
  Unit::KILOMETER,
  Unit::METER,
  Unit::DEGREES_CELCIUS,
  Unit::KILOMETER_PER_HOUR,
  Unit::METER_PER_SECOND,
  Unit::KILOMETER_PER_HOUR,
  Unit::KILOMETER_PER_HOUR,
  Unit::HECTOPASCAL,
  Unit::KG_PER_M2,
  Unit::KG,
};

void
Units::SetConfig(const UnitSetting &new_config)
{
  current = new_config;
}

Unit
Units::GetUserWingLoadingUnit()
{
  return current.wing_loading_unit;
}

Unit
Units::GetUserMassUnit()
{
  return current.mass_unit;
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
  return current.GetByGroup(group);
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
Units::GetWindSpeedName()
{
  return GetUnitName(GetUserWindSpeedUnit());
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

double
Units::ToUserPressure(AtmosphericPressure value)
{
  return ToUserPressure(value.GetHectoPascal());
}

AtmosphericPressure
Units::FromUserPressure(double value)
{
  return AtmosphericPressure::HectoPascal(ToSysPressure(value));
}
