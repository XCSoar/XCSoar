/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "UnitsGlue.hpp"
#include "Units.hpp"
#include "Profile/Profile.hpp"

void
Units::LoadFromProfile()
{
  unsigned Speed = 0;
  unsigned Distance = 0;
  unsigned TaskSpeed = 0;
  unsigned Lift = 0;
  unsigned Altitude = 0;
  unsigned Temperature = 0;
  unsigned Temp = 0;

  if (Profile::Get(szProfileLatLonUnits, Temp))
    SetCoordinateFormat((CoordinateFormats_t)Temp);

  Profile::Get(szProfileSpeedUnitsValue, Speed);
  switch (Speed) {
  case 0:
    SetUserSpeedUnit(unStatuteMilesPerHour);
    SetUserWindSpeedUnit(unStatuteMilesPerHour);
    break;
  case 1:
    SetUserSpeedUnit(unKnots);
    SetUserWindSpeedUnit(unKnots);
    break;
  case 2:
  default:
    SetUserSpeedUnit(unKiloMeterPerHour);
    SetUserWindSpeedUnit(unKiloMeterPerHour);
    break;
  }

  Profile::Get(szProfileTaskSpeedUnitsValue, TaskSpeed);
  switch (TaskSpeed) {
  case 0:
    SetUserTaskSpeedUnit(unStatuteMilesPerHour);
    break;
  case 1:
    SetUserTaskSpeedUnit(unKnots);
    break;
  case 2:
  default:
    SetUserTaskSpeedUnit(unKiloMeterPerHour);
    break;
  }

  Profile::Get(szProfileDistanceUnitsValue,Distance);
  switch (Distance) {
  case 0:
    SetUserDistanceUnit(unStatuteMiles);
    break;
  case 1:
    SetUserDistanceUnit(unNauticalMiles);
    break;
  case 2:
  default:
    SetUserDistanceUnit(unKiloMeter);
    break;
  }

  Profile::Get(szProfileAltitudeUnitsValue, Altitude);
  switch (Altitude) {
  case 0:
    SetUserAltitudeUnit(unFeet);
    break;
  case 1:
  default:
    SetUserAltitudeUnit(unMeter);
    break;
  }

  Profile::Get(szProfileTemperatureUnitsValue, Temperature);
  switch (Temperature) {
  default:
  case 0:
    SetUserTemperatureUnit(unGradCelcius);
    break;
  case 1:
    SetUserTemperatureUnit(unGradFahrenheit);
    break;
  }

  Profile::Get(szProfileLiftUnitsValue, Lift);
  switch (Lift) {
  case 0:
    SetUserVerticalSpeedUnit(unKnots);
    break;
  case 1:
  default:
    SetUserVerticalSpeedUnit(unMeterPerSecond);
    break;
  }
}
