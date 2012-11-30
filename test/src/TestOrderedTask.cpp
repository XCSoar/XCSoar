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

#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/Task/TaskEvents.hpp"
#include "Engine/Task/Ordered/OrderedTaskBehaviour.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/Ordered/Points/ASTPoint.hpp"
#include "Engine/Task/ObservationZones/LineSectorZone.hpp"

#ifdef FIXED_MATH
#define ACCURACY 100
#else
#define ACCURACY 500
#endif

#include "TestUtil.hpp"

static TaskBehaviour task_behaviour;
static OrderedTaskBehaviour ordered_task_behaviour;
static GlidePolar glide_polar(fixed_zero);

static GeoPoint
MakeGeoPoint(double longitude, double latitude)
{
  return GeoPoint(Angle::Degrees(fixed(longitude)),
                  Angle::Degrees(fixed(latitude)));
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
static const Waypoint wp4 = MakeWaypoint(1, 46, 50);
static const Waypoint wp5 = MakeWaypoint(0.3, 46, 50);

static fixed
GetSafetyHeight(const TaskPoint &tp)
{
  switch (tp.GetType()) {
  case TaskPoint::FINISH:
    return task_behaviour.safety_height_arrival;

  default:
    return task_behaviour.route_planner.safety_height_terrain;
  }
}

static void
CheckLeg(const TaskWaypoint &tp, const AircraftState &aircraft,
         const TaskStats &stats)
{
  const GeoPoint destination = tp.GetWaypoint().location;
  const fixed safety_height = GetSafetyHeight(tp);
  const fixed min_arrival_alt = tp.GetWaypoint().elevation + safety_height;
  const GeoVector vector = aircraft.location.DistanceBearing(destination);
  const fixed ld = glide_polar.GetBestLD();
  const fixed height_above_min = aircraft.altitude - min_arrival_alt;
  const fixed height_consumption = vector.distance / ld;
  const ElementStat &leg = stats.current_leg;
  const GlideResult &solution_remaining = leg.solution_remaining;

  ok1(leg.vector_remaining.IsValid());
  ok1(equals(leg.vector_remaining.distance, vector.distance));
  ok1(equals(leg.vector_remaining.bearing, vector.bearing));

  ok1(solution_remaining.IsOk());
  ok1(solution_remaining.vector.IsValid());
  ok1(equals(solution_remaining.vector.distance, vector.distance));
  ok1(equals(solution_remaining.vector.bearing, vector.bearing));
  ok1(equals(solution_remaining.height_glide, height_consumption));
  ok1(equals(solution_remaining.altitude_difference,
             height_above_min - height_consumption));
  ok1(equals(solution_remaining.GetRequiredAltitudeWithDrift(),
             min_arrival_alt + height_consumption));

  if (height_above_min >= height_consumption) {
    /* straight glide */
    ok1(equals(solution_remaining.height_climb, 0));
  } else if (positive(glide_polar.GetMC())) {
    /* climb required */
    ok1(equals(solution_remaining.height_climb,
               height_consumption - height_above_min));
  } else {
    /* climb required, but not possible (MC=0) */
    ok1(equals(solution_remaining.height_climb, 0));
  }
}

static void
CheckTotal(const AircraftState &aircraft, const TaskStats &stats,
           const TaskWaypoint &start, const TaskWaypoint &tp1,
           const TaskWaypoint &finish)
{
  const fixed min_arrival_alt1 = tp1.GetWaypoint().elevation +
    task_behaviour.route_planner.safety_height_terrain;
  const fixed min_arrival_alt2 = finish.GetWaypoint().elevation + task_behaviour.safety_height_arrival;
  const GeoVector vector0 =
    start.GetWaypoint().location.DistanceBearing(tp1.GetWaypoint().location);
  const GeoVector vector1 =
    aircraft.location.DistanceBearing(tp1.GetWaypoint().location);
  const GeoVector vector2 =
    tp1.GetWaypoint().location.DistanceBearing(finish.GetWaypoint().location);
  const fixed ld = glide_polar.GetBestLD();
  const fixed height_consumption1 = vector1.distance / ld;

  const fixed height_consumption2 = vector2.distance / ld;

  const ElementStat &total = stats.total;
  const GlideResult &solution_remaining = total.solution_remaining;
  const fixed distance_nominal = vector0.distance + vector2.distance;
  const fixed distance_ahead = vector1.distance + vector2.distance;

  ok1(equals(stats.distance_nominal, distance_nominal));
  ok1(between(stats.distance_min,
              distance_nominal - fixed(30), distance_nominal));
  ok1(between(stats.distance_max,
              distance_nominal, distance_nominal + fixed(30)));

  ok1(!total.vector_remaining.IsValid());
  ok1(solution_remaining.IsOk());

  ok1(equals(solution_remaining.vector.distance, distance_ahead));
  ok1(equals(solution_remaining.height_glide, distance_ahead / ld));

  fixed alt_required_at_1 = std::max(min_arrival_alt1,
                                     min_arrival_alt2 + height_consumption2);
  fixed alt_required_at_aircraft = alt_required_at_1 + height_consumption1;
  ok1(equals(solution_remaining.GetRequiredAltitudeWithDrift(),
             alt_required_at_aircraft));
  ok1(equals(solution_remaining.altitude_difference,
             aircraft.altitude - alt_required_at_aircraft));
  ok1(equals(solution_remaining.height_climb,
             positive(glide_polar.GetMC())
             ? alt_required_at_aircraft - aircraft.altitude
             : fixed_zero));
}

static void
CheckLegEqualsTotal(const GlideResult &leg, const GlideResult &total)
{
  ok1(total.IsOk());
  ok1(equals(total.height_climb, leg.height_climb));
  ok1(equals(total.height_glide, leg.height_glide));
  ok1(equals(total.altitude_difference, leg.altitude_difference));
  ok1(equals(total.GetRequiredAltitudeWithDrift(), leg.GetRequiredAltitudeWithDrift()));
}

static void
TestFlightToFinish(fixed aircraft_altitude)
{
  OrderedTask task(task_behaviour);
  const StartPoint tp1(new LineSectorZone(wp1.location),
                       wp1, task_behaviour,
                       ordered_task_behaviour.start_constraints);
  task.Append(tp1);
  const FinishPoint tp2(new LineSectorZone(wp2.location),
                        wp2, task_behaviour,
                        ordered_task_behaviour.finish_constraints, false);
  task.Append(tp2);
  task.SetActiveTaskPoint(1);

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = wp1.location;
  aircraft.altitude = aircraft_altitude;
  task.Update(aircraft, aircraft, glide_polar);

  const GeoVector vector = wp1.location.DistanceBearing(wp2.location);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.task_started);
  ok1(!stats.task_finished);
  ok1(stats.flight_mode_final_glide == !negative(stats.total.solution_remaining.altitude_difference));
  ok1(equals(stats.distance_nominal, vector.distance));
  ok1(between(stats.distance_min,
              vector.distance - fixed(20), vector.distance));
  ok1(between(stats.distance_max,
              vector.distance, vector.distance + fixed(20)));

  CheckLeg(tp2, aircraft, stats);

  ok1(!stats.total.vector_remaining.IsValid());
  CheckLegEqualsTotal(stats.current_leg.solution_remaining,
                      stats.total.solution_remaining);
}

static void
TestSimpleTask()
{
  OrderedTask task(task_behaviour);
  const StartPoint tp1(new LineSectorZone(wp1.location),
                       wp1, task_behaviour,
                       ordered_task_behaviour.start_constraints);
  task.Append(tp1);
  const FinishPoint tp2(new LineSectorZone(wp3.location),
                        wp3, task_behaviour,
                        ordered_task_behaviour.finish_constraints, false);
  task.Append(tp2);

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = MakeGeoPoint(0, 44.5);
  aircraft.altitude = fixed(1700);
  task.Update(aircraft, aircraft, glide_polar);

  const GeoVector tp1_to_tp2 = wp1.location.DistanceBearing(wp3.location);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.task_started);
  ok1(!stats.task_finished);
  ok1(!stats.flight_mode_final_glide);
  ok1(equals(stats.distance_nominal, tp1_to_tp2.distance));
  ok1(between(stats.distance_min,
              tp1_to_tp2.distance - fixed(20), tp1_to_tp2.distance));
  ok1(between(stats.distance_max,
              tp1_to_tp2.distance, tp1_to_tp2.distance + fixed(20)));

  CheckLeg(tp1, aircraft, stats);
  CheckTotal(aircraft, stats, tp1, tp1, tp2);
}

static void
TestHighFinish()
{
  OrderedTask task(task_behaviour);
  const StartPoint tp1(new LineSectorZone(wp1.location),
                       wp1, task_behaviour,
                       ordered_task_behaviour.start_constraints);
  task.Append(tp1);
  Waypoint wp2b(wp2);
  wp2b.elevation = fixed(1000);
  const FinishPoint tp2(new LineSectorZone(wp2b.location),
                        wp2b, task_behaviour,
                        ordered_task_behaviour.finish_constraints, false);
  task.Append(tp2);
  task.SetActiveTaskPoint(1);

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = wp1.location;
  aircraft.altitude = fixed(1000);
  task.Update(aircraft, aircraft, glide_polar);

  const GeoVector vector = wp1.location.DistanceBearing(wp2.location);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.task_started);
  ok1(!stats.task_finished);
  ok1(!stats.flight_mode_final_glide);
  ok1(equals(stats.distance_nominal, vector.distance));
  ok1(between(stats.distance_min,
              vector.distance - fixed(20), vector.distance));
  ok1(between(stats.distance_max,
              vector.distance, vector.distance + fixed(20)));

  CheckLeg(tp2, aircraft, stats);

  ok1(!stats.total.vector_remaining.IsValid());
  CheckLegEqualsTotal(stats.current_leg.solution_remaining,
                      stats.total.solution_remaining);
}

static void
TestHighTP()
{
  const fixed width(1);
  OrderedTask task(task_behaviour);
  const StartPoint tp1(new LineSectorZone(wp1.location, width),
                       wp1, task_behaviour,
                       ordered_task_behaviour.start_constraints);
  task.Append(tp1);
  const Waypoint wp3b = MakeWaypoint(wp3, 1500);
  const ASTPoint tp2(new LineSectorZone(wp3b.location, width),
                     wp3b, task_behaviour);
  task.Append(tp2);
  const Waypoint wp4b = MakeWaypoint(wp4, 100);
  const FinishPoint tp3(new LineSectorZone(wp4b.location, width),
                        wp4b, task_behaviour,
                        ordered_task_behaviour.finish_constraints, false);
  task.Append(tp3);
  task.SetActiveTaskPoint(1);

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = wp1.location;
  aircraft.altitude = fixed(2000);
  task.Update(aircraft, aircraft, glide_polar);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.task_started);
  ok1(!stats.task_finished);
  ok1(!stats.flight_mode_final_glide);

  CheckLeg(tp2, aircraft, stats);
  CheckTotal(aircraft, stats, tp1, tp2, tp3);
}

static void
TestHighTPFinal()
{
  const fixed width(1);
  OrderedTask task(task_behaviour);
  const StartPoint tp1(new LineSectorZone(wp1.location, width),
                       wp1, task_behaviour,
                       ordered_task_behaviour.start_constraints);
  task.Append(tp1);
  const Waypoint wp3b = MakeWaypoint(wp3, 1500);
  const ASTPoint tp2(new LineSectorZone(wp3b.location, width),
                     wp3b, task_behaviour);
  task.Append(tp2);
  const Waypoint wp5b = MakeWaypoint(wp5, 200);
  const FinishPoint tp3(new LineSectorZone(wp5b.location, width),
                        wp5b, task_behaviour,
                        ordered_task_behaviour.finish_constraints, false);
  task.Append(tp3);
  task.SetActiveTaskPoint(1);

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = wp1.location;
  aircraft.altitude = fixed(1200);
  task.Update(aircraft, aircraft, glide_polar);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.task_started);
  ok1(!stats.task_finished);
  ok1(!stats.flight_mode_final_glide);

  CheckLeg(tp2, aircraft, stats);
  CheckTotal(aircraft, stats, tp1, tp2, tp3);
}

static void
TestLowTPFinal()
{
  const fixed width(1);
  OrderedTask task(task_behaviour);
  const Waypoint wp1b = MakeWaypoint(wp1, 1500);
  const StartPoint tp1(new LineSectorZone(wp1b.location, width),
                       wp1b, task_behaviour,
                       ordered_task_behaviour.start_constraints);
  task.Append(tp1);
  const ASTPoint tp2(new LineSectorZone(wp2.location, width),
                     wp2, task_behaviour);
  task.Append(tp2);
  const FinishPoint tp3(new LineSectorZone(wp3.location, width),
                        wp3, task_behaviour,
                        ordered_task_behaviour.finish_constraints, false);
  task.Append(tp3);
  task.SetActiveTaskPoint(1);

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = wp1.location;
  aircraft.altitude = fixed(2500);
  task.Update(aircraft, aircraft, glide_polar);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.task_started);
  ok1(!stats.task_finished);
  ok1(!stats.flight_mode_final_glide);

  CheckLeg(tp2, aircraft, stats);
  CheckTotal(aircraft, stats, tp1, tp2, tp3);
}

static void
TestAll()
{
  TestFlightToFinish(fixed(2000));
  TestFlightToFinish(fixed(1000));
  TestSimpleTask();
  TestHighFinish();
  TestHighTP();
  TestHighTPFinal();
  TestLowTPFinal();
}

int main(int argc, char **argv)
{
  plan_tests(728);

  task_behaviour.SetDefaults();

  TestAll();

  glide_polar.SetMC(fixed_one);
  TestAll();

  glide_polar.SetMC(fixed_two);
  TestAll();

  glide_polar.SetMC(fixed_four);
  TestAll();

  return exit_status();
}
