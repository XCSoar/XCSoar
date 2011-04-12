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
AIRSPACE_ALT::get_as_text_units(const bool concise) const
{
  TCHAR buffer[64];

  switch (Base) {
  case abAGL:
    if (!positive(AGL)) {
      _tcscpy(buffer, _T("GND"));
    } else {
      _stprintf(buffer, _T("%d %s AGL"),
                iround(Units::ToUserAltitude(AGL)), Units::GetAltitudeName());
    }
    break;
  case abFL:
    _stprintf(buffer, _T("FL%d"), (int)FL);
    break;
  case abMSL:
    _stprintf(buffer, _T("%d %s"),
              iround(Units::ToUserAltitude(Altitude)), Units::GetAltitudeName());
    break;
  case abUndef:
  default:
    buffer[0] = _T('\0');
    break;
  };
  if (!concise && Base!=abMSL && positive(Altitude)) {
    TCHAR second[64];
    _stprintf(second, _T(" %d %s"),
              iround(Units::ToUserAltitude(Altitude)), Units::GetAltitudeName());
    return tstring(buffer) + second;
  } else
    return tstring(buffer);
}


const tstring 
AbstractAirspace::get_base_text(const bool concise) const
{
  return m_base.get_as_text_units(concise);
}

const tstring 
AbstractAirspace::get_top_text(const bool concise) const
{
  return m_top.get_as_text_units(concise);
}
