/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "SettingsComputer.hpp"

bool
AirspaceVisible::type_visible(const AbstractAirspace& airspace) const
{
  return m_settings.DisplayAirspaces[airspace.get_type()];
}

bool
AirspaceVisible::altitude_visible(const AbstractAirspace& airspace) const
{
  /// @todo airspace visibility did use ToMSL(..., map.Calculated().TerrainAlt); 

  switch (m_settings.AltitudeMode) {

  case ALLON:
    return true;

  case CLIP:
    if (airspace.get_base_altitude() <= fixed(m_settings.ClipAltitude))
      return true;
    else
      return false;

  case AUTO:

    if ((m_altitude >= (airspace.get_base_altitude() - fixed(m_settings.airspace_warnings.AltWarningMargin)))
        && (m_altitude <= (airspace.get_top_altitude() + fixed(m_settings.airspace_warnings.AltWarningMargin))))
      return true;
    else
      return false;

  case ALLBELOW:
    if (m_altitude >= (airspace.get_base_altitude() - fixed(m_settings.airspace_warnings.AltWarningMargin)))
      return true;
    else
      return false;

  case INSIDE:
    if ((m_altitude >= airspace.get_base_altitude()) 
        && (m_altitude <= airspace.get_top_altitude()))
      return true;
    else
      return false;

  case ALLOFF:
    return false;
  }
  return true;
}
