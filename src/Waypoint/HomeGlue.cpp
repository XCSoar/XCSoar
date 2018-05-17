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

#include "WaypointGlue.hpp"
#include "Profile/Map.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "LogFile.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/Waypoints.hpp"
#include "LastUsed.hpp"

WaypointPtr
WaypointGlue::FindHomeId(Waypoints &waypoints,
                         PlacesOfInterestSettings &settings)
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
WaypointGlue::FindHomeLocation(Waypoints &waypoints,
                               PlacesOfInterestSettings &settings)
{
  if (!settings.home_location_available)
    return nullptr;

  auto wp = waypoints.LookupLocation(settings.home_location, 100);
  if (wp == nullptr || !wp->IsAirport()) {
    settings.home_location_available = false;
    return nullptr;
  }

  settings.home_waypoint = wp->id;
  waypoints.SetHome(wp->id);
  return wp;
}

WaypointPtr
WaypointGlue::FindFlaggedHome(Waypoints &waypoints,
                              PlacesOfInterestSettings &settings)
{
  auto wp = waypoints.FindHome();
  if (wp == nullptr)
    return nullptr;

  settings.SetHome(*wp);
  return wp;
}

void
WaypointGlue::SetHome(Waypoints &way_points, const RasterTerrain *terrain,
                      PlacesOfInterestSettings &poi_settings,
                      TeamCodeSettings &team_code_settings,
                      DeviceBlackboard *device_blackboard,
                      const bool reset)
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
      LogFormat("Start at home waypoint");
      device_blackboard->SetStartupLocation(wp->location, wp->elevation);
    } else if (terrain != nullptr) {
      // no home at all, so set it from center of terrain if available
      GeoPoint loc = terrain->GetTerrainCenter();
      LogFormat("Start at terrain center");
      device_blackboard->SetStartupLocation(loc,
                                            terrain->GetTerrainHeight(loc).GetValueOr0());
    }
  }
}

void
WaypointGlue::SaveHome(ProfileMap &profile,
                       const PlacesOfInterestSettings &poi_settings,
                       const TeamCodeSettings &team_code_settings)
{
  profile.Set(ProfileKeys::HomeWaypoint, poi_settings.home_waypoint);
  if (poi_settings.home_location_available)
    profile.SetGeoPoint(ProfileKeys::HomeLocation, poi_settings.home_location);

  profile.Set(ProfileKeys::TeamcodeRefWaypoint,
              team_code_settings.team_code_reference_waypoint);
}
