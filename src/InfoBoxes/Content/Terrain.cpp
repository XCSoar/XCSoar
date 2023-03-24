// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Terrain.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "NMEA/Derived.hpp"

void
UpdateInfoBoxTerrainHeight(InfoBoxData &data) noexcept
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
UpdateInfoBoxTerrainCollision(InfoBoxData &data) noexcept
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
