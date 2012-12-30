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

#include "test_debug.hpp"

#include "Waypoint/WaypointVisitor.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Geo/GeoVector.hpp"

#include <stdio.h>
#include <tchar.h>

class WaypointPredicateCounter: public WaypointVisitor
{
public:
  typedef std::function<bool(const Waypoint &wp)> Predicate;

private:
  Predicate predicate;
  unsigned count;

public:
  WaypointPredicateCounter(const Predicate &_predicate)
    :predicate(_predicate), count(0) {}

  virtual void Visit(const Waypoint &wp) {
    if (predicate(wp))
      count++;
  }

  unsigned GetCounter() const {
    return count;
  }
};

static void
AddSpiralWaypoints(Waypoints &waypoints,
                   const GeoPoint &center = GeoPoint(Angle::Degrees(51.4),
                                                     Angle::Degrees(7.85)),
                   Angle angle_start = Angle::Degrees(0),
                   Angle angle_step = Angle::Degrees(15),
                   fixed distance_start = fixed(0),
                   fixed distance_step = fixed(1000),
                   fixed distance_max = fixed(150000))
{
  assert(positive(distance_step));

  for (unsigned i = 0;; ++i) {
    GeoVector vector;
    vector.distance = distance_start + distance_step * i;
    if (vector.distance > distance_max)
      break;

    vector.bearing = angle_start + angle_step * i;

    Waypoint waypoint;
    waypoint.location = vector.EndPoint(center);
    waypoint.original_id = i;
    waypoint.elevation = fixed(i * 10 - 500);

    StaticString<256> buffer;

    if (i % 7 == 0) {
      buffer = _T("Airfield");
      waypoint.type = Waypoint::Type::AIRFIELD;
    } else if (i % 3 == 0) {
      buffer = _T("Field");
      waypoint.type = Waypoint::Type::OUTLANDING;
    } else
      buffer = _T("Waypoint");

    buffer.AppendFormat(_T(" #%d"), i + 1);
    waypoint.name = buffer;

    waypoints.Append(std::move(waypoint));
  }

  waypoints.Optimise();
}

static void
TestLookups(const Waypoints &waypoints, const GeoPoint &center)
{
  const Waypoint *waypoint;

  ok1((waypoint = waypoints.LookupId(0)) == NULL);
  ok1((waypoint = waypoints.LookupId(1)) != NULL);
  ok1(waypoint->original_id == 0);
  ok1((waypoint = waypoints.LookupId(151)) != NULL);
  ok1(waypoint->original_id == 150);
  ok1((waypoint = waypoints.LookupId(152)) == NULL);
  ok1((waypoint = waypoints.LookupId(160)) == NULL);

  ok1((waypoint = waypoints.LookupLocation(center, fixed(0))) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.LookupName(_T("Waypoint #5"))) != NULL);
  ok1(waypoint->original_id == 4);

  ok1((waypoint = waypoints.LookupLocation(waypoint->location, fixed(10000))) != NULL);
  ok1(waypoint->original_id == 4);
}

class BeginsWith
{
  const TCHAR *prefix;

public:
  BeginsWith(const TCHAR *_prefix):prefix(_prefix) {}

  bool operator()(const Waypoint &waypoint) {
    return StringStartsWith(waypoint.name.c_str(), prefix);
  }
};

static void
TestNamePrefixVisitor(const Waypoints &waypoints, const TCHAR *prefix,
                      unsigned expected_results)
{
  WaypointPredicateCounter::Predicate predicate = BeginsWith(prefix);
  WaypointPredicateCounter prefix_counter(predicate);
  waypoints.VisitNamePrefix(prefix, prefix_counter);
  ok1(prefix_counter.GetCounter() == expected_results);
}

static void
TestNamePrefixVisitor(const Waypoints &waypoints)
{
  TestNamePrefixVisitor(waypoints, _T(""), 151);
  TestNamePrefixVisitor(waypoints, _T("Foo"), 0);
  TestNamePrefixVisitor(waypoints, _T("a"), 0);
  TestNamePrefixVisitor(waypoints, _T("A"), 22);
  TestNamePrefixVisitor(waypoints, _T("Air"), 22);
  TestNamePrefixVisitor(waypoints, _T("Field"), 51 - 8);
}

class CloserThan
{
  fixed distance;
  GeoPoint location;

public:
  CloserThan(fixed _distance, const GeoPoint &_location)
    :distance(_distance), location(_location) {}

  bool operator()(const Waypoint &waypoint) {
    return location.Distance(waypoint.location) < distance;
  }
};

static void
TestRangeVisitor(const Waypoints &waypoints, const GeoPoint &location,
                      fixed distance, unsigned expected_results)
{
  WaypointPredicateCounter::Predicate predicate = CloserThan(distance, location);
  WaypointPredicateCounter distance_counter(predicate);
  waypoints.VisitWithinRange(location, distance, distance_counter);
  ok1(distance_counter.GetCounter() == expected_results);
}

static void
TestRangeVisitor(const Waypoints &waypoints, const GeoPoint &center)
{
  TestRangeVisitor(waypoints, center, fixed(1), 1);
  TestRangeVisitor(waypoints, center, fixed(999), 1);
  TestRangeVisitor(waypoints, center, fixed(1300), 2);
  TestRangeVisitor(waypoints, center, fixed(10500), 11);
  TestRangeVisitor(waypoints, center, fixed(1000000), 151);
}

static bool
OriginalIDAbove5(const Waypoint &waypoint) {
  return waypoint.original_id > 5;
}

static void
TestGetNearest(const Waypoints &waypoints, const GeoPoint &center)
{
  const Waypoint *waypoint;
  GeoPoint near = GeoVector(fixed(250), Angle::Degrees(15)).EndPoint(center);
  GeoPoint far = GeoVector(fixed(750), Angle::Degrees(15)).EndPoint(center);
  GeoPoint further = GeoVector(fixed(4200), Angle::Degrees(48)).EndPoint(center);

  ok1((waypoint = waypoints.GetNearest(center, fixed(1))) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.GetNearest(center, fixed(10000))) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.GetNearest(near, fixed(1))) == NULL);

  ok1((waypoint = waypoints.GetNearest(near, fixed(10000))) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.GetNearest(far, fixed(1))) == NULL);

  ok1((waypoint = waypoints.GetNearest(far, fixed(10000))) != NULL);
  ok1(waypoint->original_id == 1);

  ok1((waypoint = waypoints.GetNearestLandable(center, fixed(1))) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.GetNearestLandable(center, fixed(10000))) != NULL);
  ok1(waypoint->original_id == 0);

  ok1((waypoint = waypoints.GetNearestLandable(further, fixed(1))) == NULL);

  ok1((waypoint = waypoints.GetNearestLandable(further, fixed(10000))) != NULL);
  ok1(waypoint->original_id == 3);

  ok1((waypoint = waypoints.GetNearestIf(center, fixed(1), OriginalIDAbove5)) == NULL);

  ok1((waypoint = waypoints.GetNearestIf(center, fixed(10000), OriginalIDAbove5)) != NULL);
  ok1(waypoint->original_id == 6);
}

static void
TestIterator(const Waypoints &waypoints)
{
  unsigned count = 0;
  for (auto it = waypoints.begin(), end = waypoints.end(); it != end; ++it)
    count++;

  ok1(count == 151);
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
  waypoints.Append(std::move(wp_copy));
  waypoints.Optimise();
  unsigned size_new = waypoints.size();
  return (size_new == size_old + 1);
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
  if (!ParseArgs(argc, argv))
    return 0;

  plan_tests(52);

  Waypoints waypoints;
  GeoPoint center(Angle::Degrees(51.4), Angle::Degrees(7.85));

  // AddSpiralWaypoints creates 151 waypoints from
  // 0km to 150km distance in 1km steps
  AddSpiralWaypoints(waypoints, center);

  ok1(!waypoints.IsEmpty());
  ok1(waypoints.size() == 151);

  TestLookups(waypoints, center);
  TestNamePrefixVisitor(waypoints);
  TestRangeVisitor(waypoints, center);
  TestGetNearest(waypoints, center);
  TestIterator(waypoints);

  ok(TestCopy(waypoints), "waypoint copy", 0);
  ok(TestErase(waypoints, 3), "waypoint erase", 0);
  ok(TestReplace(waypoints, 4), "waypoint replace", 0);

  // test clear
  waypoints.Clear();
  ok1(waypoints.IsEmpty());
  ok1(waypoints.size() == 0);

  return exit_status();
}
