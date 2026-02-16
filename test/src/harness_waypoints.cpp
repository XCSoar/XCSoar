// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Printing.hpp"
#include "harness_waypoints.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "system/FileUtil.hpp"
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
  wp.has_elevation = true;
  waypoints.Append(std::move(wp));

  wp = waypoints.Create(GeoPoint(Angle::Zero(),
                                 Angle::Degrees(1)));
  wp.type = Waypoint::Type::AIRFIELD;
  wp.elevation = 0.25;
  wp.has_elevation = true;
  waypoints.Append(std::move(wp));

  wp = waypoints.Create(GeoPoint(Angle::Degrees(1),
                                 Angle::Degrees(1)));
  wp.name = "Hello";
  wp.type = Waypoint::Type::AIRFIELD;
  wp.elevation = 0.5;
  wp.has_elevation = true;
  waypoints.Append(std::move(wp));

  wp = waypoints.Create(GeoPoint(Angle::Degrees(0.8),
                                 Angle::Degrees(0.5)));
  wp.name = "Unk";
  wp.type = Waypoint::Type::AIRFIELD;
  wp.elevation = 0.25;
  wp.has_elevation = true;
  waypoints.Append(std::move(wp));

  wp = waypoints.Create(GeoPoint(Angle::Degrees(1),
                                 Angle::Zero()));
  wp.type = Waypoint::Type::AIRFIELD;
  wp.elevation = 0.25;
  wp.has_elevation = true;
  waypoints.Append(std::move(wp));

  wp = waypoints.Create(GeoPoint(Angle::Zero(),
                                 Angle::Degrees(0.23)));
  wp.type = Waypoint::Type::AIRFIELD;
  wp.elevation = 0.25;
  wp.has_elevation = true;
  waypoints.Append(std::move(wp));

  for (unsigned i=0; i<(unsigned)std::max((int)n-6,0); i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    double z = rand()% std::max(terrain_height,1);
    wp = waypoints.Create(GeoPoint(Angle::Degrees(x / 1000.0),
                                   Angle::Degrees(y / 1000.0)));
    wp.type = Waypoint::Type::NORMAL;
    wp.elevation = z;
    wp.has_elevation = true;
    waypoints.Append(std::move(wp));
  }
  waypoints.Optimise();

  if (verbose) {
    Directory::Create(Path("output/results"));
    std::ofstream fin("output/results/res-wp-in.txt");
    for (unsigned i=1; i<=waypoints.size(); i++) {
      const auto wpt = waypoints.LookupId(i);
      if (wpt)
        fin << *wpt;
    }
  }
  return true;
}

