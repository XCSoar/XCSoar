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

#include "AirspaceFormatter.hpp"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "Math/Util.hpp"
#include "Util/StringFormat.hpp"

#include <string.h>

void
AirspaceFormatter::FormatAltitudeShort(TCHAR *buffer,
                                       const AirspaceAltitude &altitude,
                                       bool include_unit)
{
  switch (altitude.reference) {
  case AltitudeReference::AGL:
    if (altitude.altitude_above_terrain <= 0)
      _tcscpy(buffer, _T("GND"));
    else
      if (include_unit)
        StringFormatUnsafe(buffer, _T("%d %s AGL"),
                           iround(Units::ToUserAltitude(altitude.altitude_above_terrain)),
                           Units::GetAltitudeName());
      else
        StringFormatUnsafe(buffer, _T("%d AGL"),
                           iround(Units::ToUserAltitude(altitude.altitude_above_terrain)));
    break;

  case AltitudeReference::STD:
    StringFormatUnsafe(buffer, _T("FL%d"), iround(altitude.flight_level));
    break;

  case AltitudeReference::MSL:
    if (include_unit)
      StringFormatUnsafe(buffer, _T("%d %s"),
                         iround(Units::ToUserAltitude(altitude.altitude)),
                         Units::GetAltitudeName());
    else
      StringFormatUnsafe(buffer, _T("%d"),
                         iround(Units::ToUserAltitude(altitude.altitude)));
    break;

  case AltitudeReference::NONE:
    *buffer = _T('\0');
    break;
  }
}

void
AirspaceFormatter::FormatAltitude(TCHAR *buffer,
                                  const AirspaceAltitude &altitude)
{
  FormatAltitudeShort(buffer, altitude);

  if ((altitude.reference == AltitudeReference::MSL ||
       altitude.reference == AltitudeReference::AGL) &&
      Units::GetUserAltitudeUnit() == Unit::METER)
    /* additionally show airspace altitude in feet, because aviation
       charts usually print altitudes in feet */
    StringFormatUnsafe(buffer + _tcslen(buffer), _T(" (%d %s)"),
                       iround(Units::ToUserUnit(altitude.altitude, Unit::FEET)),
                       Units::GetUnitName(Unit::FEET));

  if (altitude.reference != AltitudeReference::MSL &&
      altitude.altitude > 0)
    StringFormatUnsafe(buffer + _tcslen(buffer), _T(" %d %s"),
                       iround(Units::ToUserAltitude(altitude.altitude)),
                       Units::GetAltitudeName());
}
