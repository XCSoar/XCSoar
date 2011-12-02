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

#include "Profile/UnitsConfig.hpp"
#include "Profile/Profile.hpp"
#include "Units/Settings.hpp"
#include "Units/UnitsGlue.hpp"

static bool
ApplyUnit(Unit &value, Unit new_value)
{
  if (new_value == unUndef)
    return false;

  value = new_value;
  return true;
}

gcc_const
static Unit
ImportSpeedUnit(unsigned tmp)
{
  switch (tmp) {
  case 0:
    return unStatuteMilesPerHour;

  case 1:
    return unKnots;

  case 2:
    return unKiloMeterPerHour;

  default:
    return unUndef;
  }
}

static bool
GetSpeedUnit(const TCHAR *key, Unit &value)
{
  unsigned tmp;
  return Profile::Get(key, tmp) && ApplyUnit(value, ImportSpeedUnit(tmp));
}

gcc_const
static Unit
ImportVerticalSpeedUnit(unsigned tmp)
{
  switch (tmp) {
  case 0:
    return unKnots;

  case 1:
    return unMeterPerSecond;

  case 2:
    return unFeetPerMinute;

  default:
    return unUndef;
  }
}

static bool
GetVerticalSpeedUnit(const TCHAR *key, Unit &value)
{
  unsigned tmp;
  return Profile::Get(key, tmp) && ApplyUnit(value, ImportVerticalSpeedUnit(tmp));
}

gcc_const
static Unit
ImportDistanceUnit(unsigned tmp)
{
  switch (tmp) {
  case 0:
    return unStatuteMiles;

  case 1:
    return unNauticalMiles;

  case 2:
    return unKiloMeter;

  default:
    return unUndef;
  }
}

static bool
GetDistanceUnit(const TCHAR *key, Unit &value)
{
  unsigned tmp;
  return Profile::Get(key, tmp) && ApplyUnit(value, ImportDistanceUnit(tmp));
}

gcc_const
static Unit
ImportAltitudeUnit(unsigned tmp)
{
  switch (tmp) {
  case 0:
    return unFeet;

  case 1:
    return unMeter;

  default:
    return unUndef;
  }
}

static bool
GetAltitudeUnit(const TCHAR *key, Unit &value)
{
  unsigned tmp;
  return Profile::Get(key, tmp) && ApplyUnit(value, ImportAltitudeUnit(tmp));
}

gcc_const
static Unit
ImportTemperatureUnit(unsigned tmp)
{
  switch (tmp) {
  case 0:
    return unGradCelcius;

  case 1:
    return unGradFahrenheit;

  default:
    return unUndef;
  }
}

static bool
GetTemperatureUnit(const TCHAR *key, Unit &value)
{
  unsigned tmp;
  return Profile::Get(key, tmp) &&
    ApplyUnit(value, ImportTemperatureUnit(tmp));
}

gcc_const
static bool
ValidPressureUnit(Unit unit)
{
  return unit == unHectoPascal || unit == unMilliBar ||
    unit == unTorr || unit == unInchMercury;
}

static bool
GetPressureUnit(const TCHAR *key, Unit &value)
{
  Unit tmp;
  if (!Profile::GetEnum(key, tmp) || !ValidPressureUnit(tmp))
    return false;

  value = tmp;
  return true;
}

void
Profile::LoadUnits(UnitSetting &config)
{
  config = Units::LoadFromOSLanguage();

  GetEnum(szProfileLatLonUnits, config.coordinate_format);

  GetSpeedUnit(szProfileSpeedUnitsValue, config.speed_unit);
  GetSpeedUnit(szProfileSpeedUnitsValue, config.wind_speed_unit);
  GetSpeedUnit(szProfileTaskSpeedUnitsValue, config.task_speed_unit);
  GetDistanceUnit(szProfileDistanceUnitsValue, config.distance_unit);
  GetAltitudeUnit(szProfileAltitudeUnitsValue, config.altitude_unit);
  GetTemperatureUnit(szProfileTemperatureUnitsValue, config.temperature_unit);
  GetVerticalSpeedUnit(szProfileLiftUnitsValue, config.vertical_speed_unit);
  GetPressureUnit(szProfilePressureUnitsValue, config.pressure_unit);
}
