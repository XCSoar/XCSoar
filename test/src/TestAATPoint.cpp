/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Settings.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Geo/Flat/TaskProjection.hpp"
#include "TestUtil.hpp"

static TaskBehaviour task_behaviour;
static OrderedTaskSettings ordered_task_settings;
static GlidePolar glide_polar(fixed(0));

static GeoPoint
MakeGeoPoint(double longitude, double latitude)
{
  return GeoPoint(Angle::Degrees(longitude),
                  Angle::Degrees(latitude));
}

static Waypoint
MakeWaypoint(Waypoint wp, double altitude)
{
  wp.elevation = fixed(altitude);
  return wp;
}

static Waypoint
MakeWaypoint(double longitude, double latitude, double altitude)
{
  return MakeWaypoint(Waypoint(MakeGeoPoint(longitude, latitude)), altitude);
}

static const Waypoint wp1 = MakeWaypoint(0, 45, 50);
static const Waypoint wp2 = MakeWaypoint(0, 45.3, 50);
static const Waypoint wp3 = MakeWaypoint(0, 46, 50);

static void
TestAATPoint()
{
  OrderedTask task(task_behaviour);
  task.Append(StartPoint(new CylinderZone(wp1.location, fixed(500)), wp1,
                         task_behaviour,
                         ordered_task_settings.start_constraints));
  task.Append(AATPoint(new CylinderZone(wp2.location, fixed(10000)), wp2,
                       task_behaviour));
  task.Append(FinishPoint(new CylinderZone(wp3.location, fixed(500)), wp3,
                          task_behaviour,
                          ordered_task_settings.finish_constraints));
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();
  ok1(task.CheckTask());

  AATPoint &ap = (AATPoint &)task.GetPoint(1);

  ok1(!ap.IsTargetLocked());
  ok1(equals(ap.GetTargetLocation(), wp2.location));
  ap.LockTarget(true);
  ok1(ap.IsTargetLocked());
  ok1(equals(ap.GetTargetLocation(), wp2.location));

  GeoPoint target = MakeGeoPoint(0, 45.31);
  ap.SetTarget(target);
  ok1(ap.IsTargetLocked());
  ok1(equals(ap.GetTargetLocation(), wp2.location));

  ap.SetTarget(target, true);
  ok1(ap.IsTargetLocked());
  ok1(equals(ap.GetTargetLocation(), target));

  RangeAndRadial rar = ap.GetTargetRangeRadial();
  ok1(equals(rar.range, fixed(0.1112), 1000));
  ok1(equals(rar.radial.Degrees(), fixed(0), 200));

  target = MakeGeoPoint(0, 45.29);
  ap.SetTarget(target, true);
  rar = ap.GetTargetRangeRadial();
  ok1(equals(rar.range, fixed(-0.1112), 1000));
  ok1(equals(rar.radial.Degrees(), fixed(180), 200) ||
      equals(rar.radial.Degrees(), fixed(-180), 200));

  target = MakeGeoPoint(-0.05, 45.3);
  ap.SetTarget(target, true);
  rar = ap.GetTargetRangeRadial();
  ok1(equals(rar.range, 0.39107));
  ok1(equals(rar.radial.Degrees(), -89.98));

  target = MakeGeoPoint(0.05, 45.3);
  ap.SetTarget(target, true);
  rar = ap.GetTargetRangeRadial();
  ok1(equals(rar.range, 0.39107));
  ok1(equals(rar.radial.Degrees(), 89.98));

  for (int radial = -170; radial <= 170; radial += 10) {
    const Angle radial2 = Angle::Degrees(radial);

    for (int range = 10; range <= 100; range += 10) {
      const fixed range2(fixed(radial >= -90 && radial <= 90
                               ? range : -range) / 100);

      ap.SetTarget(RangeAndRadial{range2, radial2}, task.GetTaskProjection());
      rar = ap.GetTargetRangeRadial();
      ok1(equals(rar.range, range2, 100));
      ok1(equals(rar.radial.Degrees(), radial2.Degrees(), 100));
    }
  }
}

static void
TestAll()
{
  TestAATPoint();
}

int main(int argc, char **argv)
{
  plan_tests(717);

  task_behaviour.SetDefaults();
  ordered_task_settings.SetDefaults();

  TestAll();

  return exit_status();
}
