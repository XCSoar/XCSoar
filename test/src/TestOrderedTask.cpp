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

#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/Task/TaskEvents.hpp"
#include "Engine/Task/Ordered/Settings.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/Ordered/Points/ASTPoint.hpp"
#include "Engine/Task/ObservationZones/LineSectorZone.hpp"

#define ACCURACY 500

#include "TestUtil.hpp"

static TaskBehaviour task_behaviour;
static OrderedTaskSettings ordered_task_settings;
static GlidePolar glide_polar(0);

static GeoPoint
MakeGeoPoint(double longitude, double latitude)
{
  return GeoPoint(Angle::Degrees(longitude),
                  Angle::Degrees(latitude));
}

static Waypoint
MakeWaypoint(Waypoint wp, double altitude)
{
  wp.elevation = altitude;
  return wp;
}

static Waypoint
MakeWaypoint(double longitude, double latitude, double altitude)
{
  return MakeWaypoint(Waypoint(MakeGeoPoint(longitude, latitude)), altitude);
}

template<typename... Args>
static WaypointPtr
MakeWaypointPtr(Args&&... args)
{
  return WaypointPtr(new Waypoint(MakeWaypoint(std::forward<Args>(args)...)));
}

static const auto wp1 = MakeWaypointPtr(0, 45, 50);
static const auto wp2 = MakeWaypointPtr(0, 45.3, 50);
static const auto wp3 = MakeWaypointPtr(0, 46, 50);
static const auto wp4 = MakeWaypointPtr(1, 46, 50);
static const auto wp5 = MakeWaypointPtr(0.3, 46, 50);

static double
GetSafetyHeight(const TaskPoint &tp)
{
  return task_behaviour.safety_height_arrival;
}

static void
CheckLeg(const TaskWaypoint &tp, const AircraftState &aircraft,
         const TaskStats &stats)
{
  const auto destination = tp.GetWaypoint().location;
  const auto safety_height = GetSafetyHeight(tp);
  const auto min_arrival_alt = tp.GetWaypoint().elevation + safety_height;
  const auto vector = aircraft.location.DistanceBearing(destination);
  const auto ld = glide_polar.GetBestLD();
  const auto height_above_min = aircraft.altitude - min_arrival_alt;
  const auto height_consumption = vector.distance / ld;
  const auto &leg = stats.current_leg;
  const auto &solution_remaining = leg.solution_remaining;

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
  } else if (glide_polar.GetMC() > 0) {
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
  const auto min_arrival_alt1 = tp1.GetWaypoint().elevation +
    task_behaviour.safety_height_arrival;
  const auto min_arrival_alt2 = finish.GetWaypoint().elevation +
    task_behaviour.safety_height_arrival;
  const auto vector0 =
    start.GetWaypoint().location.DistanceBearing(tp1.GetWaypoint().location);
  const auto vector1 =
    aircraft.location.DistanceBearing(tp1.GetWaypoint().location);
  const auto vector2 =
    tp1.GetWaypoint().location.DistanceBearing(finish.GetWaypoint().location);
  const auto ld = glide_polar.GetBestLD();
  const auto height_consumption1 = vector1.distance / ld;

  const auto height_consumption2 = vector2.distance / ld;

  const auto &total = stats.total;
  const auto &solution_remaining = total.solution_remaining;
  const auto distance_nominal = vector0.distance + vector2.distance;
  const auto distance_ahead = vector1.distance + vector2.distance;

  ok1(equals(stats.distance_nominal, distance_nominal));
  ok1(equals(stats.distance_min, distance_nominal));
  ok1(equals(stats.distance_max, distance_nominal));

  ok1(!total.vector_remaining.IsValid());
  ok1(solution_remaining.IsOk());

  ok1(equals(solution_remaining.vector.distance, distance_ahead));
  ok1(equals(solution_remaining.height_glide, distance_ahead / ld));

  auto alt_required_at_1 = std::max(min_arrival_alt1,
                                    min_arrival_alt2 + height_consumption2);
  auto alt_required_at_aircraft = alt_required_at_1 + height_consumption1;
  ok1(equals(solution_remaining.GetRequiredAltitudeWithDrift(),
             alt_required_at_aircraft));
  ok1(equals(solution_remaining.altitude_difference,
             aircraft.altitude - alt_required_at_aircraft));
  ok1(equals(solution_remaining.height_climb,
             glide_polar.GetMC() > 0
             ? alt_required_at_aircraft - aircraft.altitude
             : 0));
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
TestFlightToFinish(double aircraft_altitude)
{
  OrderedTask task(task_behaviour);
  const StartPoint tp1(new LineSectorZone(wp1->location),
                       WaypointPtr(wp1), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  const FinishPoint tp2(new LineSectorZone(wp2->location),
                        WaypointPtr(wp2), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp2);
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = wp1->location;
  aircraft.altitude = aircraft_altitude;
  task.Update(aircraft, aircraft, glide_polar);

  const GeoVector vector = wp1->location.DistanceBearing(wp2->location);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.task_started);
  ok1(!stats.task_finished);
  ok1(stats.flight_mode_final_glide == (stats.total.solution_remaining.altitude_difference >= 0));
  ok1(equals(stats.distance_nominal, vector.distance));
  ok1(equals(stats.distance_min, vector.distance));
  ok1(equals(stats.distance_max, vector.distance));

  CheckLeg(tp2, aircraft, stats);

  ok1(!stats.total.vector_remaining.IsValid());
  CheckLegEqualsTotal(stats.current_leg.solution_remaining,
                      stats.total.solution_remaining);
}

static void
TestSimpleTask()
{
  OrderedTask task(task_behaviour);
  const StartPoint tp1(new LineSectorZone(wp1->location),
                       WaypointPtr(wp1), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  const FinishPoint tp2(new LineSectorZone(wp3->location),
                        WaypointPtr(wp3), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp2);
  task.UpdateGeometry();

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = MakeGeoPoint(0, 44.5);
  aircraft.altitude = 1700;
  task.Update(aircraft, aircraft, glide_polar);

  const GeoVector tp1_to_tp2 = wp1->location.DistanceBearing(wp3->location);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.task_started);
  ok1(!stats.task_finished);
  ok1(!stats.flight_mode_final_glide);
  ok1(equals(stats.distance_nominal, tp1_to_tp2.distance));
  ok1(equals(stats.distance_min, tp1_to_tp2.distance));
  ok1(equals(stats.distance_max, tp1_to_tp2.distance));

  CheckLeg(tp1, aircraft, stats);
  CheckTotal(aircraft, stats, tp1, tp1, tp2);
}

static void
TestHighFinish()
{
  OrderedTask task(task_behaviour);
  const StartPoint tp1(new LineSectorZone(wp1->location),
                       WaypointPtr(wp1), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  Waypoint wp2b(*wp2);
  wp2b.elevation = 1000;
  const FinishPoint tp2(new LineSectorZone(wp2b.location),
                        WaypointPtr(new Waypoint(wp2b)), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp2);
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = wp1->location;
  aircraft.altitude = 1000;
  task.Update(aircraft, aircraft, glide_polar);

  const GeoVector vector = wp1->location.DistanceBearing(wp2->location);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.task_started);
  ok1(!stats.task_finished);
  ok1(!stats.flight_mode_final_glide);
  ok1(equals(stats.distance_nominal, vector.distance));
  ok1(equals(stats.distance_min, vector.distance));
  ok1(equals(stats.distance_max, vector.distance));

  CheckLeg(tp2, aircraft, stats);

  ok1(!stats.total.vector_remaining.IsValid());
  CheckLegEqualsTotal(stats.current_leg.solution_remaining,
                      stats.total.solution_remaining);
}

static void
TestHighTP()
{
  const double width(1);
  OrderedTask task(task_behaviour);
  const StartPoint tp1(new LineSectorZone(wp1->location, width),
                       WaypointPtr(wp1), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  const ASTPoint tp2(new LineSectorZone(wp3->location, width),
                     MakeWaypointPtr(*wp3, 1500), task_behaviour);
  task.Append(tp2);
  const FinishPoint tp3(new LineSectorZone(wp4->location, width),
                        MakeWaypointPtr(*wp4, 100), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp3);
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = wp1->location;
  aircraft.altitude = 2000;
  task.Update(aircraft, aircraft, glide_polar);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.task_started);
  ok1(!stats.task_finished);
  ok1(!stats.flight_mode_final_glide);

  CheckLeg(tp2, aircraft, stats);
  CheckTotal(aircraft, stats, tp1, tp2, tp3);
}

static void
TestHighTPFinal()
{
  const double width(1);
  OrderedTask task(task_behaviour);
  const StartPoint tp1(new LineSectorZone(wp1->location, width),
                       WaypointPtr(wp1), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  const ASTPoint tp2(new LineSectorZone(wp3->location, width),
                     MakeWaypointPtr(*wp3, 1500), task_behaviour);
  task.Append(tp2);
  const FinishPoint tp3(new LineSectorZone(wp5->location, width),
                        MakeWaypointPtr(*wp5, 200), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp3);
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = wp1->location;
  aircraft.altitude = 1200;
  task.Update(aircraft, aircraft, glide_polar);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.task_started);
  ok1(!stats.task_finished);
  ok1(!stats.flight_mode_final_glide);

  CheckLeg(tp2, aircraft, stats);
  CheckTotal(aircraft, stats, tp1, tp2, tp3);
}

static void
TestLowTPFinal()
{
  const double width(1);
  OrderedTask task(task_behaviour);
  const StartPoint tp1(new LineSectorZone(wp1->location, width),
                       MakeWaypointPtr(*wp1, 1500), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  const ASTPoint tp2(new LineSectorZone(wp2->location, width),
                     WaypointPtr(wp2), task_behaviour);
  task.Append(tp2);
  const FinishPoint tp3(new LineSectorZone(wp3->location, width),
                        WaypointPtr(wp3), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp3);
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();

  ok1(task.CheckTask());

  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = wp1->location;
  aircraft.altitude = 2500;
  task.Update(aircraft, aircraft, glide_polar);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.task_started);
  ok1(!stats.task_finished);
  ok1(!stats.flight_mode_final_glide);

  CheckLeg(tp2, aircraft, stats);
  CheckTotal(aircraft, stats, tp1, tp2, tp3);
}

static void
TestAll()
{
  TestFlightToFinish(2000);
  TestFlightToFinish(1000);
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

  glide_polar.SetMC(1);
  TestAll();

  glide_polar.SetMC(2);
  TestAll();

  glide_polar.SetMC(4);
  TestAll();

  return exit_status();
}
