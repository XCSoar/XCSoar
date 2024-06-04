// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointGlue.hpp"
#include "Profile/Map.hpp"
#include "Profile/Keys.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "LogFile.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/Waypoints.hpp"
#include "LastUsed.hpp"

namespace WaypointGlue {

WaypointPtr
FindHomeId(Waypoints &waypoints,
           PlacesOfInterestSettings &settings) noexcept
{
  if (settings.home_waypoint < 0)
    return nullptr;

  auto wp = waypoints.LookupId(settings.home_waypoint);
  if (wp == nullptr) {
    settings.home_waypoint = -1;
    return nullptr;
  }

  settings.home_location = wp->location;
  settings.home_location_available = true;
  waypoints.SetHome(wp->id);
  return wp;
}

WaypointPtr
FindHomeLocation(Waypoints &waypoints,
                 PlacesOfInterestSettings &settings) noexcept
{
  if (!settings.home_location_available)
    return nullptr;

  auto wp = waypoints.LookupLocation(settings.home_location, 100);
  if (wp == nullptr) {
    settings.home_location_available = false;
    return nullptr;
  }

  settings.home_waypoint = wp->id;
  waypoints.SetHome(wp->id);
  return wp;
}

WaypointPtr
FindFlaggedHome(Waypoints &waypoints,
                PlacesOfInterestSettings &settings) noexcept
{
  auto wp = waypoints.FindHome();
  if (wp == nullptr)
    return nullptr;

  settings.SetHome(*wp);
  return wp;
}

void
SetHome(Waypoints &way_points, const RasterTerrain *terrain,
        PlacesOfInterestSettings &poi_settings,
        TeamCodeSettings &team_code_settings,
        DeviceBlackboard *device_blackboard,
        const bool reset) noexcept
{
  if (reset)
    poi_settings.home_waypoint = -1;

  // check invalid home waypoint or forced reset due to file change
  auto wp = FindHomeId(way_points, poi_settings);
  if (wp == nullptr) {
    /* fall back to HomeLocation, try to find it in the waypoint
       database */
    wp = FindHomeLocation(way_points, poi_settings);
    if (wp == nullptr)
      // search for home in waypoint list, if we don't have a home
      wp = FindFlaggedHome(way_points, poi_settings);
  }

  if (wp != nullptr)
    LastUsedWaypoints::Add(*wp);

  // check invalid task ref waypoint or forced reset due to file change
  if (reset || way_points.IsEmpty() ||
      !way_points.LookupId(team_code_settings.team_code_reference_waypoint))
    // set team code reference waypoint if we don't have one
    team_code_settings.team_code_reference_waypoint = poi_settings.home_waypoint;

  if (device_blackboard != nullptr) {
    if (wp != nullptr) {
      // OK, passed all checks now
      LogString("Start at home waypoint");
      device_blackboard->SetStartupLocation(wp->location,
                                            wp->GetElevationOrZero());
    } else if (terrain != nullptr) {
      // no home at all, so set it from center of terrain if available
      GeoPoint loc = terrain->GetTerrainCenter();
      LogString("Start at terrain center");
      device_blackboard->SetStartupLocation(loc,
                                            terrain->GetTerrainHeight(loc).GetValueOr0());
    }
  }
}

void
SaveHome(ProfileMap &profile,
         const PlacesOfInterestSettings &poi_settings,
         const TeamCodeSettings &team_code_settings) noexcept
{
  profile.Set(ProfileKeys::HomeWaypoint, poi_settings.home_waypoint);
  if (poi_settings.home_location_available)
    profile.SetGeoPoint(ProfileKeys::HomeLocation, poi_settings.home_location);

  profile.Set(ProfileKeys::TeamcodeRefWaypoint,
              team_code_settings.team_code_reference_waypoint);
}

} // namespace WaypointGlue
