/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "harness_waypoints.hpp"
#include "test_debug.hpp"

#include "Waypoint/WaypointVisitor.hpp"

#include <stdio.h>
#include <tchar.h>

class WaypointVisitorPrint: public WaypointVisitor {
public:
  WaypointVisitorPrint():count(0) {}

  virtual void Visit(const Waypoint& wp) {
    if (verbose)
      _tprintf(_T("# visiting wp %d, '%s'\n"), wp.id, wp.name.c_str());

    count++;
  }

  unsigned count;

  void reset() {
    count = 0;
  }
};

static unsigned
TestLocation(const Waypoints& waypoints, bool good)
{
  GeoPoint loc(Angle::Zero(), Angle::Zero());
  if (!good)
    loc.longitude = Angle::Degrees(fixed(-23.4));

  const Waypoint *wp = waypoints.LookupLocation(loc);
  if (!wp)
    return !good;

  WaypointVisitorPrint v;
  v.Visit(*wp);
  return good;
}

static unsigned
TestRange(const Waypoints& waypoints, const double range)
{
  const Waypoint *wp = waypoints.LookupId(3);
  if (!wp)
    return 0;

  WaypointVisitorPrint v;
  waypoints.VisitWithinRange(wp->location, fixed(range), v);
  return v.count;
}

static bool
TestNearest(const Waypoints& waypoints)
{
  const Waypoint *wp = waypoints.LookupId(3);
  if (!wp)
    return false;

  wp = waypoints.GetNearest(wp->location, fixed_zero);
  if (!wp)
    return false;

  return wp->id == 3;
}

static bool
TestNearestLandable(const Waypoints& waypoints)
{
  const Waypoint *wp = waypoints.GetNearestLandable(GeoPoint(Angle::Degrees(fixed(0.99)),
                                                             Angle::Degrees(fixed(1.1))),
                                                    fixed(50000));
  if (!wp)
    return false;

  return wp->id == 3;
}

static unsigned
TestCopy(Waypoints& waypoints)
{
  const Waypoint *wp = waypoints.LookupId(5);
  if (!wp)
    return false;

  unsigned size_old = waypoints.size();
  Waypoint wp_copy = *wp;
  wp_copy.id = waypoints.size() + 1;
  waypoints.Append(wp_copy);
  waypoints.Optimise();
  unsigned size_new = waypoints.size();
  return (size_new == size_old + 1);
}

static bool
TestLookup(const Waypoints& waypoints, unsigned id)
{
  const Waypoint* wp;
  wp = waypoints.LookupId(id);
  return wp != NULL;
}

static bool
TestErase(Waypoints& waypoints, unsigned id)
{
  waypoints.Optimise();
  const Waypoint* wp;
  wp = waypoints.LookupId(id);
  if (wp == NULL)
    return false;

  waypoints.Erase(*wp);
  waypoints.Optimise();

  wp = waypoints.LookupId(id);
  return wp == NULL;
}

static bool
TestReplace(Waypoints& waypoints, unsigned id)
{
  const Waypoint* wp;
  wp = waypoints.LookupId(id);
  if (wp == NULL)
    return false;

  tstring oldName = wp->name;

  Waypoint copy = *wp;
  copy.name = _T("Fred");
  waypoints.Replace(*wp, copy);
  waypoints.Optimise();

  wp = waypoints.LookupId(id);
  return wp != NULL && wp->name != oldName && wp->name == _T("Fred");
}

int
main(int argc, char** argv)
{
  if (!parse_args(argc, argv))
    return 0;

  plan_tests(14);

  Waypoints waypoints;

  ok(SetupWaypoints(waypoints), "waypoint setup", 0);

  unsigned size = waypoints.size();

  ok(TestLookup(waypoints, 3), "waypoint lookup", 0);
  ok(!TestLookup(waypoints, 5000), "waypoint bad lookup", 0);
  ok(TestNearest(waypoints), "waypoint nearest", 0);
  ok(TestNearestLandable(waypoints), "waypoint nearest landable", 0);
  ok(TestLocation(waypoints, true), "waypoint location good", 0);
  ok(TestLocation(waypoints, false), "waypoint location bad", 0);
  ok(TestRange(waypoints, 100) == 1, "waypoint visit range 100m", 0);
  ok(TestRange(waypoints, 500000) == waypoints.size(),
     "waypoint range 500000m", 0);

  // test clear
  waypoints.Clear();
  ok(waypoints.size() == 0, "waypoint clear", 0);
  SetupWaypoints(waypoints);
  ok(size == waypoints.size(), "waypoint setup after clear", 0);

  ok(TestCopy(waypoints), "waypoint copy", 0);

  ok(TestErase(waypoints, 3), "waypoint erase", 0);
  ok(TestReplace(waypoints, 4), "waypoint replace", 0);

  return exit_status();
}
