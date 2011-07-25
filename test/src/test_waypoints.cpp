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

#include "harness_waypoints.hpp"
#include "test_debug.hpp"

#include "Waypoint/WaypointVisitor.hpp"

#include <stdio.h>
#include <tchar.h>

class WaypointVisitorPrint: public WaypointVisitor {
public:
  WaypointVisitorPrint():count(0) {};

  virtual void Visit(const Waypoint& wp) {
    if (verbose) {
      _tprintf(_T("# visiting wp %d, '%s'\n"), wp.id, wp.Name.c_str());
    }
    count++;
  }
  unsigned count;
  void reset() {
    count = 0;
  }
};

static unsigned
test_location(const Waypoints& waypoints, bool good)
{
  GeoPoint loc(Angle::zero(), Angle::zero());
  if (!good) {
    loc.Longitude = Angle::degrees(fixed(-23.4));
  }
  const Waypoint *r = waypoints.lookup_location(loc);
  if (r) {
    WaypointVisitorPrint v;
    v.Visit(*r);
    return good;
  } else {
    return !good;
  }
}


static unsigned
test_range(const Waypoints& waypoints, const double range)
{
  const Waypoint *r = waypoints.lookup_id(3);
  if (r) {
    WaypointVisitorPrint v;
    waypoints.visit_within_range(r->Location, fixed(range), v);
    return v.count;
  } else {
    return 0;
  }
}

static bool
test_nearest(const Waypoints& waypoints)
{
  const Waypoint *r = waypoints.lookup_id(3);
  if (!r)
    return false;

  r = waypoints.get_nearest(r->Location, fixed_zero);
  if (!r)
    return false;

  return r->id == 3;
}

static bool
test_nearest_landable(const Waypoints& waypoints)
{
  const Waypoint *r = waypoints.get_nearest_landable(GeoPoint(Angle::degrees(fixed(0.99)),
                                                              Angle::degrees(fixed(1.1))),
                                                     fixed(50000));
  if (!r)
    return false;

  return r->id == 3;
}

static unsigned
test_copy(Waypoints& waypoints)
{
  const Waypoint *r = waypoints.lookup_id(5);
  if (!r) {
    return false;
  }
  unsigned size_old = waypoints.size();
  Waypoint wp = *r;
  wp.id = waypoints.size()+1;
  waypoints.append(wp);
  waypoints.optimise();
  unsigned size_new = waypoints.size();
  return (size_new == size_old+1);
}

static bool
test_lookup(const Waypoints& waypoints, unsigned id)
{
  const Waypoint* wp;
  wp = waypoints.lookup_id(id);
  if (wp== NULL) {
    return false;
  }
  return true;
}

static bool
test_erase(Waypoints& waypoints, unsigned id)
{
  waypoints.optimise();
  const Waypoint* wp;
  wp = waypoints.lookup_id(id);
  if (wp== NULL) {
    return false;
  }
  waypoints.erase(*wp);
  waypoints.optimise();

  wp = waypoints.lookup_id(id);
  if (wp!= NULL) {
    return false;
  }
  return true;
}

static bool
test_replace(Waypoints& waypoints, unsigned id)
{
  const Waypoint* wp;
  wp = waypoints.lookup_id(id);
  if (wp== NULL) {
    return false;
  }
  tstring oldName = wp->Name;

  Waypoint copy = *wp;
  copy.Name = _T("Fred");
  waypoints.replace(*wp,copy);
  waypoints.optimise();

  wp = waypoints.lookup_id(id);
  if (wp== NULL) {
    return false;
  }
  return (wp->Name != oldName) && (wp->Name == _T("Fred"));
}

int main(int argc, char** argv)
{
  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(14);

  Waypoints waypoints;

  ok(setup_waypoints(waypoints),"waypoint setup",0);

  unsigned size = waypoints.size();

  ok(test_lookup(waypoints,3),"waypoint lookup",0);
  ok(!test_lookup(waypoints,5000),"waypoint bad lookup",0);
  ok(test_nearest(waypoints),"waypoint nearest",0);
  ok(test_nearest_landable(waypoints),"waypoint nearest landable",0);
  ok(test_location(waypoints,true),"waypoint location good",0);
  ok(test_location(waypoints,false),"waypoint location bad",0);
  ok(test_range(waypoints,100)==1,"waypoint visit range 100m",0);
  ok(test_range(waypoints,500000)== waypoints.size(),"waypoint range 500000m",0);

  // test clear
  waypoints.clear();
  ok(waypoints.size()==0,"waypoint clear",0);
  setup_waypoints(waypoints);
  ok(size == waypoints.size(),"waypoint setup after clear",0);

  ok(test_copy(waypoints),"waypoint copy",0);

  ok(test_erase(waypoints,3),"waypoint erase",0);
  ok(test_replace(waypoints,4),"waypoint replace",0);

  return exit_status();
}
