/* Copyright_License {

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
#include "AirspaceAltitude.hpp"
#include "AbstractAirspace.hpp"
#include "Units/Units.hpp"

#include <stdio.h>

const tstring 
AirspaceAltitude::GetAsTextUnits(const bool concise) const
{
  TCHAR buffer[64];

  switch (type) {
  case abAGL:
    if (!positive(altitude_above_terrain)) {
      _tcscpy(buffer, _T("GND"));
    } else {
      _stprintf(buffer, _T("%d %s AGL"),
                iround(Units::ToUserAltitude(altitude_above_terrain)), Units::GetAltitudeName());
    }
    break;
  case abFL:
    _stprintf(buffer, _T("FL%d"), (int)flight_level);
    break;
  case abMSL:
    _stprintf(buffer, _T("%d %s"),
              iround(Units::ToUserAltitude(altitude)), Units::GetAltitudeName());
    break;
  case abUndef:
  default:
    buffer[0] = _T('\0');
    break;
  };
  if (!concise && type!=abMSL && positive(altitude)) {
    TCHAR second[64];
    _stprintf(second, _T(" %d %s"),
              iround(Units::ToUserAltitude(altitude)), Units::GetAltitudeName());
    return tstring(buffer) + second;
  } else
    return tstring(buffer);
}


const tstring 
AbstractAirspace::get_base_text(const bool concise) const
{
  return m_base.GetAsTextUnits(concise);
}

const tstring 
AbstractAirspace::get_top_text(const bool concise) const
{
  return m_top.GetAsTextUnits(concise);
}
