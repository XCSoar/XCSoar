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

#include "FlightStatusPanel.hpp"
#include "Interface.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Language/Language.hpp"

enum Controls {
  Location,
  Altitude,
  MaxHeightGain,
  Near,
  Bearing,
  Distance,
};

void
FlightStatusPanel::Refresh()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (basic.location_available)
    SetText(Location, FormatGeoPoint(basic.location));
  else
    ClearText(Location);

  if (basic.gps_altitude_available)
    SetText(Altitude, FormatUserAltitude(basic.gps_altitude));
  else
    ClearText(Altitude);

  SetText(MaxHeightGain, FormatUserAltitude(calculated.max_height_gain));

  if (nearest_waypoint) {
    GeoVector vec(basic.location,
                  nearest_waypoint->location);

    SetText(Near, nearest_waypoint->name.c_str());

    SetText(Bearing, FormatBearing(vec.bearing).c_str());

    SetText(Distance, FormatUserDistanceSmart(vec.distance));
  } else {
    SetText(Near, _T("-"));
    SetText(Bearing, _T("-"));
    SetText(Distance, _T("-"));
  }
}

void
FlightStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddReadOnly(_("Location"));
  AddReadOnly(_("Altitude"));
  AddReadOnly(_("Max. height gain"));
  AddReadOnly(_("Near"));
  AddReadOnly(_("Bearing"));
  AddReadOnly(_("Distance"));
}
