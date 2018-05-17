/* Copyright_License {

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

#include "Printing.hpp"
#include "harness_waypoints.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "OS/FileUtil.hpp"
#include "test_debug.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>

/** 
 * Initialises waypoints with random and non-random waypoints
 * for testing
 *
 * @param waypoints waypoints class to add waypoints to
 */
bool SetupWaypoints(Waypoints &waypoints, const unsigned n)
{
  Waypoint wp = waypoints.Create(GeoPoint(Angle::Zero(),
                                          Angle::Zero()));
  wp.type = Waypoint::Type::AIRFIELD;
  wp.elevation = 0.25;
  waypoints.Append(std::move(wp));

  wp = waypoints.Create(GeoPoint(Angle::Zero(),
                                 Angle::Degrees(1)));
  wp.type = Waypoint::Type::AIRFIELD;
  wp.elevation = 0.25;
  waypoints.Append(std::move(wp));

  wp = waypoints.Create(GeoPoint(Angle::Degrees(1),
                                 Angle::Degrees(1)));
  wp.name = _T("Hello");
  wp.type = Waypoint::Type::AIRFIELD;
  wp.elevation = 0.5;
  waypoints.Append(std::move(wp));

  wp = waypoints.Create(GeoPoint(Angle::Degrees(0.8),
                                 Angle::Degrees(0.5)));
  wp.name = _T("Unk");
  wp.type = Waypoint::Type::AIRFIELD;
  wp.elevation = 0.25;
  waypoints.Append(std::move(wp));

  wp = waypoints.Create(GeoPoint(Angle::Degrees(1),
                                 Angle::Zero()));
  wp.type = Waypoint::Type::AIRFIELD;
  wp.elevation = 0.25;
  waypoints.Append(std::move(wp));

  wp = waypoints.Create(GeoPoint(Angle::Zero(),
                                 Angle::Degrees(0.23)));
  wp.type = Waypoint::Type::AIRFIELD;
  wp.elevation = 0.25;
  waypoints.Append(std::move(wp));

  for (unsigned i=0; i<(unsigned)std::max((int)n-6,0); i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    double z = rand()% std::max(terrain_height,1);
    wp = waypoints.Create(GeoPoint(Angle::Degrees(x / 1000.0),
                                   Angle::Degrees(y / 1000.0)));
    wp.type = Waypoint::Type::NORMAL;
    wp.elevation = z;
    waypoints.Append(std::move(wp));
  }
  waypoints.Optimise();

  if (verbose) {
    Directory::Create(Path(_T("output/results")));
    std::ofstream fin("output/results/res-wp-in.txt");
    for (unsigned i=1; i<=waypoints.size(); i++) {
      const auto wpt = waypoints.LookupId(i);
      if (wpt)
        fin << *wpt;
    }
  }
  return true;
}

