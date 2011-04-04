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

#include "WayPointGlue.hpp"
#include "SettingsComputer.hpp"
#include "DeviceBlackboard.hpp"
#include "Profile/Profile.hpp"
#include "StringUtil.hpp"
#include "LogFile.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/Waypoints.hpp"
#include "WayPointFile.hpp"
#include "Language.hpp"
#include "ProgressGlue.hpp"
#include "Components.hpp"
#include "NMEA/Aircraft.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"

#include <tchar.h>
#include <windef.h> /* for MAX_PATH */

namespace WayPointGlue {
  static WayPointFile *wp_file0, *wp_file1, *wp_file2, *wp_file3;
}

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
WayPointGlue::SetHome(Waypoints &way_points, const RasterTerrain *terrain,
                      SETTINGS_COMPUTER &settings,
                      const bool reset)
{
  LogStartUp(_T("SetHome"));

  // check invalid home waypoint or forced reset due to file change
  const Waypoint *wp = reset ? NULL : way_points.lookup_id(settings.HomeWaypoint);
  if (wp == NULL && settings.HomeLocationAvailable) {
    /* fall back to HomeLocation, try to find it in the waypoint
       database */
    wp = way_points.lookup_location(settings.HomeLocation, fixed(100));
    if (wp != NULL && wp->is_airport())
      settings.SetHome(*wp);
  }

  if (wp != NULL) {
    // home waypoint found
    way_points.set_home(settings.HomeWaypoint);
  } else {
    // search for home in waypoint list, if we don't have a home
    wp = way_points.find_home();
    if (wp != NULL)
      settings.SetHome(*wp);
    else
      settings.ClearHome();
  }

  // check invalid task ref waypoint or forced reset due to file change
  if (reset || way_points.empty() ||
      !way_points.lookup_id(settings.TeamCodeRefWaypoint))
    // set team code reference waypoint if we don't have one
    settings.TeamCodeRefWaypoint = settings.HomeWaypoint;

  if (const Waypoint *wp = way_points.lookup_id(settings.HomeWaypoint)) {
    // OK, passed all checks now
    LogStartUp(_T("Start at home waypoint"));
    device_blackboard.SetStartupLocation(wp->Location, wp->Altitude);
  } else if (terrain != NULL) {
    // no home at all, so set it from center of terrain if available
    GeoPoint loc = terrain->GetTerrainCenter();
    LogStartUp(_T("Start at terrain center"));
    device_blackboard.SetStartupLocation(loc, fixed_zero);
  }

  // Save the home waypoint number in the resgistry
  Profile::Set(szProfileHomeWaypoint,settings.HomeWaypoint);
  if (settings.HomeLocationAvailable)
    Profile::SetGeoPoint(szProfileHomeLocation, settings.HomeLocation);

  Profile::Set(szProfileTeamcodeRefWaypoint,settings.TeamCodeRefWaypoint);
}


bool
WayPointGlue::ReadWaypoints(Waypoints &way_points,
                            const RasterTerrain *terrain)
{
  LogStartUp(_T("ReadWaypoints"));
  ProgressGlue::Create(_("Loading Waypoints..."));

  bool found = false;
  TCHAR szFile[MAX_PATH];

  // Delete old waypoints
  way_points.clear();

  // tear down old parsers
  Close();

  // ### FIRST FILE ###

  // Get first waypoint filename
  if (Profile::GetPath(szProfileWayPointFile, szFile))
    wp_file0 = WayPointFile::create(szFile, 0);

  // If waypoint file exists
  if (wp_file0 != NULL) {
    // parse the file
    wp_file0->SetTerrain(terrain);
    if (wp_file0->Parse(way_points)) {
      found = true;
      // Set waypoints writable flag
      way_points.set_file0_writable(wp_file0->IsWritable());
    } else {
      LogStartUp(_T("Parse error in waypoint file 1"));
    }
  } else {
    LogStartUp(_T("No waypoint file 1"));
  }

  // ### SECOND FILE ###

  // Get second waypoint filename
  if (Profile::GetPath(szProfileAdditionalWayPointFile, szFile))
    wp_file1 = WayPointFile::create(szFile, 1);

  // If waypoint file exists
  if (wp_file1 != NULL) {
    // parse the file
    wp_file1->SetTerrain(terrain);
    if (wp_file1->Parse(way_points)) {
      found = true;
    } else {
      LogStartUp(_T("Parse error in waypoint file 2"));
    }
  } else {
    LogStartUp(_T("No waypoint file 2"));
  }

  // ### WATCHED WAYPOINT/THIRD FILE ###

  // Get third waypoint filename
  if (Profile::GetPath(szProfileWatchedWayPointFile, szFile))
    wp_file2 = WayPointFile::create(szFile, 2);

  // If waypoint file exists
  if (wp_file2 != NULL) {
    // parse the file
    wp_file2->SetTerrain(terrain);
    if (wp_file2->Parse(way_points)) {
      found = true;
    } else {
      LogStartUp(_T("Parse error in waypoint file 3"));
    }
  } else {
    LogStartUp(_T("No waypoint file 3"));
  }

  // ### MAP/FOURTH FILE ###

  // If no waypoint file found yet
  if (!found) {
    // Get the map filename
    Profile::GetPath(szProfileMapFile, szFile);
    _tcscat(szFile, _T("/"));
    _tcscat(szFile, _T("waypoints.xcw"));

    wp_file3 = WayPointFile::create(szFile, 3);

    // Test if waypoints.xcw can be loaded, otherwise try waypoints.cup
    if (wp_file3 == NULL) {
      // Get the map filename
      Profile::GetPath(szProfileMapFile, szFile);
      _tcscat(szFile, _T("/"));
      _tcscat(szFile, _T("waypoints.cup"));

      wp_file3 = WayPointFile::create(szFile, 3);
    }

    // If waypoint file inside map file exists
    if (wp_file3 != NULL) {
      // parse the file
      wp_file3->SetTerrain(terrain);
      if (wp_file3->Parse(way_points, true)) {
        found = true;
      } else {
        LogStartUp(_T("Parse error in map waypoint file"));
      }
    } else {
      LogStartUp(_T("No waypoint file in the map file"));
    }
  }

  // Optimise the waypoint list after attaching new waypoints
  way_points.optimise();

  // Return whether waypoints have been loaded into the waypoint list
  return found;
}


bool
WayPointGlue::SaveWaypoints(const Waypoints &way_points)
{
  bool result = false;
  LogStartUp(_T("SaveWaypoints"));

  // ### FIRST FILE ###
  if (wp_file0 != NULL) {
    if (!wp_file0->Save(way_points)) {
      LogStartUp(_T("Waypoint file 1 can not be written"));
    } else {
      result = true;
      LogStartUp(_T("Waypoint file 1 saved"));
    }
  }

  // ### SECOND FILE ###
  if (wp_file1 != NULL) {
    if (!wp_file1->Save(way_points)) {
      LogStartUp(_T("Waypoint file 2 can not be written"));
    } else {
      result = true;
      LogStartUp(_T("Waypoint file 2 saved"));
    }
  }

  // ### THIRD FILE ###
  if (wp_file2 != NULL) {
    if (!wp_file1->Save(way_points)) {
      LogStartUp(_T("Waypoint file 3 can not be written"));
    } else {
      result = true;
      LogStartUp(_T("Waypoint file 3 saved"));
    }
  }

  return result;
}

void
WayPointGlue::Close()
{
  delete wp_file0;
  wp_file0 = NULL;

  delete wp_file1;
  wp_file1 = NULL;

  delete wp_file2;
  wp_file2 = NULL;

  delete wp_file3;
  wp_file3 = NULL;
}
