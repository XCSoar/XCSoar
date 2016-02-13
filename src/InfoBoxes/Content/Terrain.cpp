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

#include "InfoBoxes/Content/Terrain.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "NMEA/Derived.hpp"

void
UpdateInfoBoxTerrainHeight(InfoBoxData &data)
{
  const TerrainInfo &calculated = CommonInterface::Calculated();
  if (!calculated.terrain_valid){
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(calculated.terrain_altitude);
  data.SetCommentFromAlternateAltitude(calculated.terrain_altitude);
}

void
UpdateInfoBoxTerrainCollision(InfoBoxData &data)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const TerrainInfo &calculated = CommonInterface::Calculated();
  if (!basic.location_available ||
      !calculated.terrain_warning_location.IsValid()) {
    data.SetInvalid();
    return;
  }

  double distance =
    basic.location.DistanceS(calculated.terrain_warning_location);
  data.SetValueFromDistance(distance);
}
