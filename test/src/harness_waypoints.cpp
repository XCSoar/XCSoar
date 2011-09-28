/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
bool setup_waypoints(Waypoints &waypoints, const unsigned n) 
{

  Waypoint wp = waypoints.create(GeoPoint(Angle::Degrees(fixed_zero),
                                          Angle::Degrees(fixed_zero)));
  wp.type = Waypoint::TYPE_AIRFIELD;
  wp.altitude = fixed(0.25);
  waypoints.append(wp);

  wp = waypoints.create(GeoPoint(Angle::Degrees(fixed_zero), 
                                 Angle::Degrees(fixed_one)));
  wp.type = Waypoint::TYPE_AIRFIELD;
  wp.altitude = fixed(0.25);
  waypoints.append(wp);

  wp = waypoints.create(GeoPoint(Angle::Degrees(fixed_one), 
                                 Angle::Degrees(fixed_one)));
  wp.name = _T("Hello");
  wp.type = Waypoint::TYPE_AIRFIELD;
  wp.altitude = fixed_half;
  waypoints.append(wp);

  wp = waypoints.create(GeoPoint(Angle::Degrees(fixed(0.8)), 
                                 Angle::Degrees(fixed(0.5))));
  wp.name = _T("Unk");
  wp.type = Waypoint::TYPE_AIRFIELD;
  wp.altitude = fixed(0.25);
  waypoints.append(wp);

  wp = waypoints.create(GeoPoint(Angle::Degrees(fixed_one), 
                                 Angle::Degrees(fixed_zero)));
  wp.type = Waypoint::TYPE_AIRFIELD;
  wp.altitude = fixed(0.25);
  waypoints.append(wp);

  wp = waypoints.create(GeoPoint(Angle::Degrees(fixed_zero), 
                                 Angle::Degrees(fixed(0.23))));
  wp.type = Waypoint::TYPE_AIRFIELD;
  wp.altitude = fixed(0.25);
  waypoints.append(wp);

  for (unsigned i=0; i<(unsigned)std::max((int)n-6,0); i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    double z = rand()% std::max(terrain_height,1);
    wp = waypoints.create(GeoPoint(Angle::Degrees(fixed(x/1000.0)), 
                                   Angle::Degrees(fixed(y/1000.0))));
    wp.type = Waypoint::TYPE_NORMAL;
    wp.altitude = fixed(z);
    waypoints.append(wp);
  }
  waypoints.optimise();

  if (verbose) {
    std::ofstream fin("results/res-wp-in.txt");
    for (unsigned i=1; i<=waypoints.size(); i++) {
      const Waypoint *wp = waypoints.lookup_id(i);
      if (wp != NULL)
        fin << *wp;
    }
  }
  return true;
}

