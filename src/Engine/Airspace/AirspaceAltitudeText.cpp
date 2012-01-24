/* Copyright_License {

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
#include "AirspaceAltitude.hpp"
#include "AbstractAirspace.hpp"
#include "Units/Units.hpp"
#include "Util/StaticString.hpp"

#include <stdio.h>

const tstring 
AirspaceAltitude::GetAsTextUnits(const bool concise) const
{
  StaticString<64> buffer;

  switch (type) {
  case AGL:
    if (!positive(altitude_above_terrain))
      buffer = _T("GND");
    else
      buffer.Format(_T("%d %s AGL"),
                    iround(Units::ToUserAltitude(altitude_above_terrain)),
                    Units::GetAltitudeName());
    break;
  case FL:
    buffer.Format(_T("FL%d"), (int)flight_level);
    break;
  case MSL:
    buffer.Format(_T("%d %s"), iround(Units::ToUserAltitude(altitude)),
                  Units::GetAltitudeName());
    break;
  case UNDEFINED:
    buffer.clear();
    break;
  }

  if (!concise && type != MSL && positive(altitude))
    buffer.AppendFormat(_T(" %d %s"), iround(Units::ToUserAltitude(altitude)),
                        Units::GetAltitudeName());

  return tstring(buffer);
}


const tstring 
AbstractAirspace::GetBaseText(const bool concise) const
{
  return altitude_base.GetAsTextUnits(concise);
}

const tstring 
AbstractAirspace::GetTopText(const bool concise) const
{
  return altitude_top.GetAsTextUnits(concise);
}
