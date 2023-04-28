// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RouteProfile.hpp"
#include "Map.hpp"
#include "Keys.hpp"
#include "Engine/Route/Config.hpp"

void
Profile::Load(const ProfileMap &map, RoutePlannerConfig &settings)
{
  map.Get(ProfileKeys::SafetyAltitudeTerrain, settings.safety_height_terrain);
  map.GetEnum(ProfileKeys::RoutePlannerMode, settings.mode);
  map.Get(ProfileKeys::RoutePlannerAllowClimb, settings.allow_climb);
  map.Get(ProfileKeys::RoutePlannerUseCeiling, settings.use_ceiling);
  map.GetEnum(ProfileKeys::TurningReach, settings.reach_calc_mode);
  map.GetEnum(ProfileKeys::ReachPolarMode, settings.reach_polar_mode);
}
