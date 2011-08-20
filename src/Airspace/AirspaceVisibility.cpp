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

#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Airspace/AirspaceRendererSettings.hpp"

bool
AirspaceVisible::type_visible(const AbstractAirspace& airspace) const
{
  return renderer_settings.display[airspace.GetType()];
}

bool
AirspaceVisible::altitude_visible(const AbstractAirspace& airspace) const
{
  /// @todo airspace visibility did use ToMSL(..., map.Calculated().TerrainAlt); 

  switch (renderer_settings.altitude_mode) {
  case ALLON:
    return true;
  case CLIP:
    return airspace.GetBase().GetAltitude(m_state) <= fixed(renderer_settings.clip_altitude);
  case AUTO:
    return airspace.GetBase().IsBelow(m_state, fixed(computer_settings.warnings.AltWarningMargin))
      && airspace.GetTop().IsAbove(m_state, fixed(computer_settings.warnings.AltWarningMargin));
  case ALLBELOW:
    return airspace.GetBase().IsBelow(m_state, fixed(computer_settings.warnings.AltWarningMargin));
  case INSIDE:
    return (airspace.GetBase().IsBelow(m_state) && airspace.GetTop().IsAbove(m_state));

  case ALLOFF:
    return false;
  }
  return true;
}
