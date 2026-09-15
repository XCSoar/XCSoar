// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointGlue.hpp"
#include "Profile/Map.hpp"
#include "Profile/Keys.hpp"
#include "Computer/Settings.hpp"
#include "LogFile.hpp"
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
    LogFmt("FindHomeId: id={} not found in {} waypoints",
           settings.home_waypoint, waypoints.size());
    return nullptr;
  }

  settings.home_location = wp->location;
  settings.home_location_available = true;

  if (wp->has_elevation) {
    settings.home_elevation = wp->elevation;
    settings.home_elevation_available = true;
  } else
    settings.home_elevation_available = false;

  waypoints.SetHome(wp->id);
  return wp;
}

WaypointPtr
FindHomeLocation(Waypoints &waypoints,
                 PlacesOfInterestSettings &settings) noexcept
{
  if (!settings.home_location_available) {
    settings.home_elevation_available = false;
    return nullptr;
  }

  auto wp = waypoints.LookupLocation(settings.home_location, 100);
  if (wp == nullptr) {
    settings.home_location_available = false;
    settings.home_elevation_available = false;
    return nullptr;
  }

  if (wp->has_elevation) {
    settings.home_elevation = wp->elevation;
    settings.home_elevation_available = true;
  } else
    settings.home_elevation_available = false;

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
SetHome(Waypoints &way_points,
        PlacesOfInterestSettings &poi_settings,
        TeamCodeSettings &team_code_settings,
        bool reset) noexcept
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
