// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Ptr.hpp"

class Waypoints;
class RasterTerrain;
class ProgressListener;
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
WaypointPtr
FindHomeId(Waypoints &waypoints,
           PlacesOfInterestSettings &settings) noexcept;

/**
 * Find the configured home by id (in #PlacesOfInterestSettings) in
 * the #Waypoints database.  Sets the home in #Waypoints and updates
 * the home location in #PlacesOfInterestSettings.  Will not update
 * the profile, because that should only be done on user action.
 *
 * @return the home #Waypoint, or nullptr if it not found
 */
WaypointPtr
FindHomeLocation(Waypoints &waypoints,
                 PlacesOfInterestSettings &settings) noexcept;

/**
 * Find the waypoint flagged as "home" in the #Waypoints database,
 * and configures it in #PlacesOfInterestSettings.  Will not update
 * the profile, because that should only be done on user action.
 *
 * @return the home #Waypoint, or nullptr if it not found
 */
WaypointPtr
FindFlaggedHome(Waypoints &waypoints,
                PlacesOfInterestSettings &settings) noexcept;

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
void
SetHome(Waypoints &way_points, const RasterTerrain *terrain,
        PlacesOfInterestSettings &poi_settings,
        TeamCodeSettings &team_code_settings,
        DeviceBlackboard *device_blackboard,
        const bool reset) noexcept;

/**
 * Save the home waypoint and the teamcode reference location to the
 * profile.
 */
void
SaveHome(ProfileMap &profile,
         const PlacesOfInterestSettings &poi_settings,
         const TeamCodeSettings &team_code_settings) noexcept;

/**
 * Reads the waypoints out of the two waypoint files and appends them to the
 * specified waypoint list
 * @param way_points The waypoint list to fill
 * @param terrain RasterTerrain (for automatic waypoint height)
 */
bool
LoadWaypoints(Waypoints &way_points,
              const RasterTerrain *terrain,
              ProgressListener &progress);

/**
 * Create the file user.cup (replacing it if it already exists) and write to
 * this new file every waypoint from way_points that has an origin of USER
 * (i.e., every waypoint in memory designated for inclusion in user.cup).
 *
 * Throws std::runtime_error on error;
 */
void
SaveWaypoints(const Waypoints &way_points);

/**
 * Append one waypoint to the file "user.cup".
 *
 * Throws std::runtime_error on error;
 */
void
SaveWaypoint(const Waypoint &wp);

} // namespace WaypointGlue
