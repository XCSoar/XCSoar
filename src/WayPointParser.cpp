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

#include "WayPointParser.h"
#include "DeviceBlackboard.hpp"
#include "Registry.hpp"
#include "Profile.hpp"
#include "LocalPath.hpp"
#include "UtilsText.hpp"
#include "StringUtil.hpp"
#include "LogFile.hpp"
#include "RasterTerrain.h"
#include "Waypoint/Waypoints.hpp"
#include "UtilsFile.hpp"
#include "Units.hpp"
#include "WayPointFile.hpp"

#include <algorithm>
using std::min;
using std::max;

#include <tchar.h>
#include <stdio.h>

#ifdef HAVE_POSIX
#include <errno.h>
#endif

#include "wcecompat/ts_string.h"

WayPointFile* WayPointParser::wp_file0 = NULL;
WayPointFile* WayPointParser::wp_file1 = NULL;
WayPointFile* WayPointParser::wp_file2 = NULL;


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
SetHome(const Waypoints &way_points, const RasterTerrain *terrain,
        SETTINGS_COMPUTER &settings,
        const bool reset, const bool set_location)
{
  LogStartUp(TEXT("SetHome\n"));

  // check invalid home waypoint or forced reset due to file change
  if (reset || way_points.empty() ||
      !way_points.lookup_id(settings.HomeWaypoint)) {
    // search for home in waypoint list, if we don't have a home
    settings.HomeWaypoint = -1;

    const Waypoint* wp = way_points.find_home();
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
      LogStartUp(TEXT("Start at home waypoint\n"));
      device_blackboard.SetStartupLocation(wp->Location, wp->Altitude);
    } else if (terrain != NULL) {
      // no home at all, so set it from center of terrain if available
      GEOPOINT loc;
      if (terrain->GetTerrainCenter(&loc)) {
        LogStartUp(TEXT("Start at terrain center\n"));
        device_blackboard.SetStartupLocation(loc, 0);
      }
    }
  }

  //
  // Save the home waypoint number in the resgistry
  //
  // VENTA3> this is probably useless, since HomeWayPoint &c were currently
  //         just loaded from registry.
  SetToRegistry(szRegistryHomeWaypoint,settings.HomeWaypoint);
  SetToRegistry(szRegistryAlternate1,settings.Alternate1);
  SetToRegistry(szRegistryAlternate2,settings.Alternate2);
  SetToRegistry(szRegistryTeamcodeRefWaypoint,settings.TeamCodeRefWaypoint);
}


bool
WayPointParser::ReadWaypoints(Waypoints &way_points,
                              const RasterTerrain *terrain)
{
  LogStartUp(TEXT("ReadWaypoints\n"));

  bool found = false;
  TCHAR szFile[MAX_PATH];

  // Delete old waypoints
  CloseWaypoints(way_points);

  // tear down old parsers
  if (wp_file0) {
    delete wp_file0;
  }
  if (wp_file1) {
    delete wp_file1;
  }
  if (wp_file2) {
    delete wp_file2;
  }

  // ### FIRST FILE ###

  // Get first waypoint filename
  GetRegistryString(szRegistryWayPointFile, szFile, MAX_PATH);
  // and clear registry setting (if loading goes totally wrong)
  SetRegistryString(szRegistryWayPointFile, TEXT("\0"));

  wp_file0 = WayPointFile::create(szFile, 0);

  // If waypoint file exists
  if (wp_file0) {
    // parse the file
    if (wp_file0->Parse(way_points, terrain)) {
      // reset the registry to the actual file name
      SetRegistryString(szRegistryWayPointFile, szFile);

      found = true;
      // Set waypoints writable flag
      way_points.set_file0_writable(wp_file0->IsWritable());
    } else {
      LogStartUp(TEXT("Parse error in waypoint file 1\n"));
    }
  } else {
    LogStartUp(TEXT("No waypoint file 1\n"));
  }

  // ### SECOND FILE ###

  // Get second waypoint filename
  GetRegistryString(szRegistryAdditionalWayPointFile, szFile, MAX_PATH);
  // and clear registry setting (if loading goes totally wrong)
  SetRegistryString(szRegistryAdditionalWayPointFile, TEXT("\0"));

  wp_file1 = WayPointFile::create(szFile, 1);
  // If waypoint file exists
  if (wp_file1) {
    // parse the file
    if (wp_file1->Parse(way_points, terrain)) {
      // reset the registry to the actual file name
      SetRegistryString(szRegistryAdditionalWayPointFile, szFile);
      found = true;
    } else {
      LogStartUp(TEXT("Parse error in waypoint file 2\n"));
    }
  } else {
    LogStartUp(TEXT("No waypoint file 2\n"));
  }

  // ### MAP/THIRD FILE ###

  // If no waypoint file found yet
  if (!found) {
    // Get the map filename
    GetRegistryString(szRegistryMapFile, szFile, MAX_PATH);
    _tcscat(szFile, TEXT("/"));
    _tcscat(szFile, TEXT("waypoints.xcw"));

    wp_file2 = WayPointFile::create(szFile, 2);

    // If waypoint file inside map file exists
    if (wp_file2) {
      // parse the file
      if (wp_file2->Parse(way_points, terrain)) {
        found = true;
      } else {
        LogStartUp(TEXT("Parse error in map waypoint file\n"));
      }
    } else {
      LogStartUp(TEXT("No waypoint file in the map file\n"));
    }
  }

  // Optimise the waypoint list after attaching new waypoints
  way_points.optimise();

  // Return whether waypoints have been loaded into the waypoint list
  return found;
}


void
WayPointParser::SaveWaypoints(Waypoints &way_points)
{
  LogStartUp(TEXT("SaveWaypoints\n"));

  // ### FIRST FILE ###
  if (wp_file0) {
    if (!wp_file0->Save(way_points)) {
      LogStartUp(TEXT("Save error in waypoint file 1\n"));
    } else {
      LogStartUp(TEXT("Waypoint file 1 can not be written\n"));
    }
  }

  // ### SECOND FILE ###
  if (wp_file1) {
    if (!wp_file1->Save(way_points)) {
      LogStartUp(TEXT("Save error in waypoint file 2\n"));
    } else {
      LogStartUp(TEXT("Waypoint file 2 can not be written\n"));
    }
  }
}

void
WayPointParser::CloseWaypoints(Waypoints &way_points)
{
  // Clear the waypoint list
  way_points.clear();
}


