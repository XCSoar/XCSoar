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

#ifndef XCSOAR_WAY_POINT_GLUE_HPP
#define XCSOAR_WAY_POINT_GLUE_HPP

#include "Engine/Waypoint/Ptr.hpp"

class Waypoints;
class RasterTerrain;
class OperationEnvironment;
struct PlacesOfInterestSettings;
struct TeamCodeSettings;
class DeviceBlackboard;
class ProfileMap;

/**
 * This class is used to parse different waypoint files
 */
namespace WaypointGlue {
  /**
   * Find the configured home by location (in
   * #PlacesOfInterestSettings) in the #Waypoints database.  Sets the
   * home in #Waypoints and updates the home waypoint id in
   * #PlacesOfInterestSettings.  Will not update the profile, because
   * that should only be done on user action.
   *
   * @return the home #Waypoint, or nullptr if it not found
   */
  WaypointPtr FindHomeId(Waypoints &waypoints,
                         PlacesOfInterestSettings &settings);

  /**
   * Find the configured home by id (in #PlacesOfInterestSettings) in
   * the #Waypoints database.  Sets the home in #Waypoints and updates
   * the home location in #PlacesOfInterestSettings.  Will not update
   * the profile, because that should only be done on user action.
   *
   * @return the home #Waypoint, or nullptr if it not found
   */
  WaypointPtr FindHomeLocation(Waypoints &waypoints,
                               PlacesOfInterestSettings &settings);

  /**
   * Find the waypoint flagged as "home" in the #Waypoints database,
   * and configures it in #PlacesOfInterestSettings.  Will not update
   * the profile, because that should only be done on user action.
   *
   * @return the home #Waypoint, or nullptr if it not found
   */
  WaypointPtr FindFlaggedHome(Waypoints &waypoints,
                              PlacesOfInterestSettings &settings);

  /**
   * This functions checks if the home and teamcode waypoint
   * indices exist and if necessary tries to find new ones in the waypoint list
   * @param way_points Waypoint list
   * @param terrain RasterTerrain (for placing the aircraft
   * in the middle of the terrain if no home was found)
   * @param settings SETTING_COMPUTER (for determining the
   * special waypoint indices)
   * @param reset This should be true if the waypoint file was changed,
   * it resets all special waypoints indices
   */
  void SetHome(Waypoints &way_points, const RasterTerrain *terrain,
               PlacesOfInterestSettings &poi_settings,
               TeamCodeSettings &team_code_settings,
               DeviceBlackboard *device_blackboard,
               const bool reset);

  /**
   * Save the home waypoint and the teamcode reference location to the
   * profile.
   */
  void SaveHome(ProfileMap &profile,
                const PlacesOfInterestSettings &poi_settings,
                const TeamCodeSettings &team_code_settings);

  /**
   * Reads the waypoints out of the two waypoint files and appends them to the
   * specified waypoint list
   * @param way_points The waypoint list to fill
   * @param terrain RasterTerrain (for automatic waypoint height)
   */
  bool LoadWaypoints(Waypoints &way_points,
                     const RasterTerrain *terrain,
                     OperationEnvironment &operation);

  /**
   * Append one waypoint to the file "user.cup".
   *
   * Throws std::runtime_error on error;
   */
  void SaveWaypoints(const Waypoints &way_points);

  /**
   * Append one waypoint to the file "user.cup".
   *
   * Throws std::runtime_error on error;
   */
  void SaveWaypoint(const Waypoint &wp);
};

#endif
