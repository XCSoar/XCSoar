/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "DeviceBlackboard.hpp"
#include "Profile/Profile.hpp"
#include "StringUtil.hpp"
#include "LogFile.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/Waypoints.hpp"
#include "WayPointFile.hpp"
#include "Language.hpp"
#include "ProgressGlue.hpp"

#include <tchar.h>

#include <memory>

static std::auto_ptr<WayPointFile> wp_file0, wp_file1, wp_file2;

/**
 * This functions checks if the home, alternate 1/2 and teamcode waypoint
 * indices exist and if necessary tries to find new ones in the waypoint list
 * @param way_points Waypoint list
 * @param terrain RasterTerrain (for placing the aircraft
 * in the middle of the terrain if no home was found)
 * @param settings SETTING_COMPUTER (for determining the
 * special waypoint indices)
 * @param reset This should be true if the waypoint file was changed,
 * it resets all special waypoints indices
 * @param set_location If true, the SetStartupLocation function will be called
 */
void
WayPointGlue::SetHome(Waypoints &way_points, const RasterTerrain *terrain,
                      SETTINGS_COMPUTER &settings,
                      const bool reset, const bool set_location)
{
  LogStartUp(_T("SetHome"));

  // check invalid home waypoint or forced reset due to file change
  const Waypoint *wp = reset ? NULL : way_points.lookup_id(settings.HomeWaypoint);
  if (wp != NULL) {
    // home waypoint found
    way_points.set_home(settings.HomeWaypoint);
  } else {
    // search for home in waypoint list, if we don't have a home
    settings.HomeWaypoint = -1;

    wp = way_points.find_home();
    if (wp)
      settings.HomeWaypoint = wp->id;
  }

  // reset Alternates
  if (reset || way_points.empty() ||
      !way_points.lookup_id(settings.Alternate1) ||
      !way_points.lookup_id(settings.Alternate2)) {
    settings.Alternate1 = -1;
    settings.Alternate2 = -1;
  }

  // check invalid task ref waypoint or forced reset due to file change
  if (reset || way_points.empty() ||
      !way_points.lookup_id(settings.TeamCodeRefWaypoint))
    // set team code reference waypoint if we don't have one
    settings.TeamCodeRefWaypoint = settings.HomeWaypoint;

  if (set_location) {
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
  }

  //
  // Save the home waypoint number in the resgistry
  //
  // VENTA3> this is probably useless, since HomeWayPoint &c were currently
  //         just loaded from registry.
  Profile::Set(szProfileHomeWaypoint,settings.HomeWaypoint);
  Profile::Set(szProfileAlternate1,settings.Alternate1);
  Profile::Set(szProfileAlternate2,settings.Alternate2);
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
  wp_file0.reset();
  wp_file1.reset();
  wp_file2.reset();

  // ### FIRST FILE ###

  // Get first waypoint filename
  if (Profile::GetPath(szProfileWayPointFile, szFile))
    wp_file0.reset(WayPointFile::create(szFile, 0));

  // If waypoint file exists
  if (wp_file0.get() != NULL) {
    // parse the file
    if (wp_file0->Parse(way_points, terrain)) {
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
    wp_file1.reset(WayPointFile::create(szFile, 0));

  // If waypoint file exists
  if (wp_file1.get() != NULL) {
    // parse the file
    if (wp_file1->Parse(way_points, terrain)) {
      found = true;
    } else {
      LogStartUp(_T("Parse error in waypoint file 2"));
    }
  } else {
    LogStartUp(_T("No waypoint file 2"));
  }

  // ### MAP/THIRD FILE ###

  // If no waypoint file found yet
  if (!found) {
    // Get the map filename
    Profile::GetPath(szProfileMapFile, szFile);
    _tcscat(szFile, _T("/"));
    _tcscat(szFile, _T("waypoints.xcw"));

    wp_file2.reset(WayPointFile::create(szFile, 2));

    // If waypoint file inside map file exists
    if (wp_file2.get() != NULL) {
      // parse the file
      if (wp_file2->Parse(way_points, terrain)) {
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


void
WayPointGlue::SaveWaypoints(const Waypoints &way_points)
{
  LogStartUp(_T("SaveWaypoints"));

  // ### FIRST FILE ###
  if (wp_file0.get() != NULL) {
    if (!wp_file0->Save(way_points)) {
      LogStartUp(_T("Save error in waypoint file 1"));
    } else {
      LogStartUp(_T("Waypoint file 1 can not be written"));
    }
  }

  // ### SECOND FILE ###
  if (wp_file1.get() != NULL) {
    if (!wp_file1->Save(way_points)) {
      LogStartUp(_T("Save error in waypoint file 2"));
    } else {
      LogStartUp(_T("Waypoint file 2 can not be written"));
    }
  }
}

