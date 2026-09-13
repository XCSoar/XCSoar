// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Engine/Task/TaskEvents.hpp"
#include "Engine/Task/Ordered/Settings.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/Ordered/Points/ASTPoint.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Engine/Task/ObservationZones/LineSectorZone.hpp"
#include "Engine/Task/ObservationZones/SectorZone.hpp"

#define ACCURACY 500

#include "TestUtil.hpp"

static TaskBehaviour task_behaviour;
static OrderedTaskSettings ordered_task_settings;
static GlidePolar glide_polar(0);

static constexpr GeoPoint
MakeGeoPoint(double longitude, double latitude) noexcept
{
  return {Angle::Degrees(longitude), Angle::Degrees(latitude)};
}

static Waypoint
MakeWaypoint(Waypoint wp, double altitude) noexcept
{
  wp.elevation = altitude;
  wp.has_elevation = true;
  return wp;
}

static Waypoint
MakeWaypoint(double longitude, double latitude, double altitude) noexcept
{
  return MakeWaypoint(Waypoint(MakeGeoPoint(longitude, latitude)), altitude);
}

template<typename... Args>
static WaypointPtr
MakeWaypointPtr(Args&&... args) noexcept
{
  return WaypointPtr(new Waypoint(MakeWaypoint(std::forward<Args>(args)...)));
}

static constexpr AircraftState
MakeAircraft(GeoPoint location, double altitude) noexcept
{
  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = location;
  aircraft.altitude = altitude;
  return aircraft;
}

static constexpr AircraftState
MakeAircraft(double longitude, double latitude, double altitude) noexcept
{
  return MakeAircraft(MakeGeoPoint(longitude, latitude), altitude);
}

static const auto wp1 = MakeWaypointPtr(0, 45, 50);
static const auto wp2 = MakeWaypointPtr(0, 45.3, 50);
static const auto wp3 = MakeWaypointPtr(0, 46, 50);
static const auto wp4 = MakeWaypointPtr(1, 46, 50);
static const auto wp5 = MakeWaypointPtr(0.3, 46, 50);

static double
GetSafetyHeight([[maybe_unused]] const TaskPoint &tp) noexcept
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

static constexpr AircraftState
MakeTimedAircraft(double longitude, double latitude, double altitude,
                  FloatDuration time) noexcept
{
  AircraftState aircraft = MakeAircraft(longitude, latitude, altitude);
  aircraft.time = TimeStamp{time};
  aircraft.flying = true;
  return aircraft;
}

static void
ExitStartCylinder(OrderedTask &task, const GeoPoint &center,
                  double altitude)
{
  const auto state_last = MakeTimedAircraft(center.longitude.Degrees(),
                                            center.latitude.Degrees(),
                                            altitude, FloatDuration{3600});
  const auto state_now = MakeTimedAircraft(center.longitude.Degrees(),
                                           center.latitude.Degrees() + 0.006,
                                           altitude, FloatDuration{3660});
  task.Update(state_now, state_last, glide_polar);
  ok1(task.GetStats().start.HasStarted());
}

static void
CheckTravelledDistance(const TaskStats &stats)
{
  ok1(stats.total.planned.IsDefined());
  ok1(stats.total.remaining.IsDefined());
  ok1(stats.total.travelled.IsDefined());
  ok1(equals(stats.total.travelled.GetDistance(),
             stats.total.planned.GetDistance() -
             stats.total.remaining.GetDistance()));
}

static void
CheckCurrentLegTravelled(const TaskStats &stats)
{
  ok1(stats.current_leg.travelled.IsDefined());
  ok1(stats.current_leg.travelled.GetDistance() > 1000);
  ok1(stats.current_leg.travelled.GetSpeed() > 0);
  ok1(equals(stats.current_leg.travelled.GetDistance(),
             stats.total.travelled.GetDistance()));
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
  const StartPoint tp1(std::make_unique<LineSectorZone>(wp1->location),
                       WaypointPtr(wp1), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  const FinishPoint tp2(std::make_unique<LineSectorZone>(wp2->location),
                        WaypointPtr(wp2), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp2);
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();

  ok1(!IsError(task.CheckTask()));

  const auto aircraft = MakeAircraft(wp1->location, aircraft_altitude);
  task.Update(aircraft, aircraft, glide_polar);

  const GeoVector vector = wp1->location.DistanceBearing(wp2->location);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.HasStarted());
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
  const StartPoint tp1(std::make_unique<LineSectorZone>(wp1->location),
                       WaypointPtr(wp1), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  const FinishPoint tp2(std::make_unique<LineSectorZone>(wp3->location),
                        WaypointPtr(wp3), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp2);
  task.UpdateGeometry();

  ok1(!IsError(task.CheckTask()));

  const auto aircraft = MakeAircraft(0, 44.5, 1700);
  task.Update(aircraft, aircraft, glide_polar);

  const GeoVector tp1_to_tp2 = wp1->location.DistanceBearing(wp3->location);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.HasStarted());
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
  const StartPoint tp1(std::make_unique<LineSectorZone>(wp1->location),
                       WaypointPtr(wp1), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  Waypoint wp2b(*wp2);
  wp2b.elevation = 1000;
  wp2b.has_elevation = true;
  const FinishPoint tp2(std::make_unique<LineSectorZone>(wp2b.location),
                        WaypointPtr(new Waypoint(wp2b)), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp2);
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();

  ok1(!IsError(task.CheckTask()));

  const auto aircraft = MakeAircraft(wp1->location, 1000);
  task.Update(aircraft, aircraft, glide_polar);

  const GeoVector vector = wp1->location.DistanceBearing(wp2->location);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.HasStarted());
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
  const StartPoint tp1(std::make_unique<LineSectorZone>(wp1->location, width),
                       WaypointPtr(wp1), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  const ASTPoint tp2(std::make_unique<LineSectorZone>(wp3->location, width),
                     MakeWaypointPtr(*wp3, 1500), task_behaviour);
  task.Append(tp2);
  const FinishPoint tp3(std::make_unique<LineSectorZone>(wp4->location, width),
                        MakeWaypointPtr(*wp4, 100), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp3);
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();

  ok1(!IsError(task.CheckTask()));

  const auto aircraft = MakeAircraft(wp1->location, 2000);
  task.Update(aircraft, aircraft, glide_polar);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.HasStarted());
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
  const StartPoint tp1(std::make_unique<LineSectorZone>(wp1->location, width),
                       WaypointPtr(wp1), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  const ASTPoint tp2(std::make_unique<LineSectorZone>(wp3->location, width),
                     MakeWaypointPtr(*wp3, 1500), task_behaviour);
  task.Append(tp2);
  const FinishPoint tp3(std::make_unique<LineSectorZone>(wp5->location, width),
                        MakeWaypointPtr(*wp5, 200), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp3);
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();

  ok1(!IsError(task.CheckTask()));

  const auto aircraft = MakeAircraft(wp1->location, 1200);
  task.Update(aircraft, aircraft, glide_polar);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.HasStarted());
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
  const StartPoint tp1(std::make_unique<LineSectorZone>(wp1->location, width),
                       MakeWaypointPtr(*wp1, 1500), task_behaviour,
                       ordered_task_settings.start_constraints);
  task.Append(tp1);
  const ASTPoint tp2(std::make_unique<LineSectorZone>(wp2->location, width),
                     WaypointPtr(wp2), task_behaviour);
  task.Append(tp2);
  const FinishPoint tp3(std::make_unique<LineSectorZone>(wp3->location, width),
                        WaypointPtr(wp3), task_behaviour,
                        ordered_task_settings.finish_constraints, false);
  task.Append(tp3);
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();

  ok1(!IsError(task.CheckTask()));

  const auto aircraft = MakeAircraft(wp1->location, 2500);
  task.Update(aircraft, aircraft, glide_polar);

  const TaskStats &stats = task.GetStats();
  ok1(stats.task_valid);
  ok1(!stats.start.HasStarted());
  ok1(!stats.task_finished);
  ok1(!stats.flight_mode_final_glide);

  CheckLeg(tp2, aircraft, stats);
  CheckTotal(aircraft, stats, tp1, tp2, tp3);
}

static void
TestTravelledDistance()
{
  ordered_task_settings.SetDefaults();

  {
    OrderedTask task(task_behaviour);
    const StartPoint tp1(std::make_unique<LineSectorZone>(wp1->location),
                         WaypointPtr(wp1), task_behaviour,
                         ordered_task_settings.start_constraints);
    task.Append(tp1);
    const FinishPoint tp2(std::make_unique<LineSectorZone>(wp2->location),
                          WaypointPtr(wp2), task_behaviour,
                          ordered_task_settings.finish_constraints, false);
    task.Append(tp2);
    task.SetActiveTaskPoint(1);
    task.UpdateGeometry();
    ok1(!IsError(task.CheckTask()));

    const auto aircraft = MakeAircraft(wp1->location, 2000);
    task.Update(aircraft, aircraft, glide_polar);

    const TaskStats &stats = task.GetStats();
    CheckTravelledDistance(stats);
    ok1(equals(stats.total.travelled.GetDistance(), 0));
  }

  {
    const double width(1);
    OrderedTask task(task_behaviour);
    task.Append(StartPoint(std::make_unique<CylinderZone>(wp1->location, 500),
                           WaypointPtr(wp1), task_behaviour,
                           ordered_task_settings.start_constraints));
    const ASTPoint tp2(std::make_unique<LineSectorZone>(wp3->location, width),
                       MakeWaypointPtr(*wp3, 1500), task_behaviour);
    task.Append(tp2);
    const FinishPoint tp3(std::make_unique<LineSectorZone>(wp4->location, width),
                          MakeWaypointPtr(*wp4, 100), task_behaviour,
                          ordered_task_settings.finish_constraints, false);
    task.Append(tp3);
    task.SetActiveTaskPoint(1);
    task.UpdateGeometry();
    ok1(!IsError(task.CheckTask()));

    ExitStartCylinder(task, wp1->location, 2000);

    const auto state_last = MakeTimedAircraft(0, 45.05, 2000,
                                              FloatDuration{3720});
    const auto state_now = MakeTimedAircraft(0, 45.15, 2000,
                                             FloatDuration{3780});
    task.Update(state_now, state_last, glide_polar);
    CheckTravelledDistance(task.GetStats());
    CheckCurrentLegTravelled(task.GetStats());
  }

  {
    OrderedTask task(task_behaviour);
    task.Append(StartPoint(std::make_unique<CylinderZone>(wp1->location, 500),
                           WaypointPtr(wp1), task_behaviour,
                           ordered_task_settings.start_constraints));
    task.Append(AATPoint(std::make_unique<CylinderZone>(wp2->location, 10000),
                         WaypointPtr(wp2), task_behaviour));
    task.Append(FinishPoint(std::make_unique<CylinderZone>(wp3->location, 500),
                            WaypointPtr(wp3), task_behaviour,
                            ordered_task_settings.finish_constraints));
    task.SetActiveTaskPoint(1);
    task.UpdateGeometry();
    ok1(!IsError(task.CheckTask()));

    AATPoint &aat = (AATPoint &)task.GetPoint(1);
    aat.SetTarget(MakeGeoPoint(0, 45.31), true);
    task.UpdateGeometry();

    ExitStartCylinder(task, wp1->location, 2000);

    const auto state_last = MakeTimedAircraft(0, 45.05, 2000,
                                              FloatDuration{3720});
    const auto state_now = MakeTimedAircraft(0, 45.15, 2000,
                                             FloatDuration{3780});
    task.Update(state_now, state_last, glide_polar);
    CheckTravelledDistance(task.GetStats());
    CheckCurrentLegTravelled(task.GetStats());
  }
}

struct StartLegStats {
  double remaining, planned, distance_min;
};

/**
 * Fly towards a start observation zone, with a finish point due north,
 * and collect the values the option is supposed to affect.
 */
static StartLegStats
FlyToStart(std::unique_ptr<ObservationZonePoint> start_zone,
           bool navigate_nearest, const AircraftState &aircraft)
{
  OrderedTaskSettings settings = task_behaviour.ordered_defaults;
  settings.navigate_nearest = navigate_nearest;

  OrderedTask task(task_behaviour);
  task.SetOrderedTaskSettings(settings);

  const StartPoint tp1(std::move(start_zone), WaypointPtr(wp1),
                       task_behaviour, settings.start_constraints);
  task.Append(tp1);
  const FinishPoint tp2(std::make_unique<LineSectorZone>(wp3->location),
                        WaypointPtr(wp3), task_behaviour,
                        settings.finish_constraints, false);
  task.Append(tp2);
  task.UpdateGeometry();

  ok1(!IsError(task.CheckTask()));

  task.Update(aircraft, aircraft, glide_polar);

  const TaskStats &stats = task.GetStats();
  const StartLegStats result{stats.current_leg.vector_remaining.distance,
                             stats.total.planned.GetDistance(),
                             stats.distance_min};

  /* once the start is no longer the active task point, navigation
     must let go of the observation zone again */
  task.SetActiveTaskPoint(1);
  task.Update(aircraft, aircraft, glide_polar);
  ok1(task.GetPoint(0).GetLocationNavigation() ==
      task.GetPoint(0).GetLocationRemaining());

  return result;
}

/**
 * Fly towards a finish observation zone, with the start already behind,
 * and return the remaining distance of the current leg.
 */
static double
FlyToFinish(std::unique_ptr<ObservationZonePoint> finish_zone,
            bool navigate_nearest, const AircraftState &aircraft)
{
  OrderedTaskSettings settings = task_behaviour.ordered_defaults;
  settings.navigate_nearest = navigate_nearest;

  OrderedTask task(task_behaviour);
  task.SetOrderedTaskSettings(settings);

  const StartPoint tp1(std::make_unique<LineSectorZone>(wp1->location, 1000),
                       WaypointPtr(wp1), task_behaviour,
                       settings.start_constraints);
  task.Append(tp1);
  const FinishPoint tp2(std::move(finish_zone), WaypointPtr(wp3),
                        task_behaviour, settings.finish_constraints, false);
  task.Append(tp2);
  task.UpdateGeometry();

  ok1(!IsError(task.CheckTask()));

  task.SetActiveTaskPoint(1);
  task.Update(aircraft, aircraft, glide_polar);

  return task.GetStats().current_leg.vector_remaining.distance;
}

/**
 * With "navigate to nearest point" enabled, a start line and a start
 * cylinder are navigated to at their nearest point, while the task keeps
 * referring to the start waypoint.  A sector, and a cylinder the
 * aircraft is inside of, are left alone.
 */
static void
TestStartNearestPoint()
{
  /* the aircraft is behind the start line, well east of its center */
  const auto aircraft = MakeTimedAircraft(0.05, 44.95, 1500,
                                          FloatDuration{3600});

  const auto line_off =
    FlyToStart(std::make_unique<LineSectorZone>(wp1->location, 20000),
               false, aircraft);
  const auto line_on =
    FlyToStart(std::make_unique<LineSectorZone>(wp1->location, 20000),
               true, aircraft);

  ok1(equals(line_off.remaining, aircraft.location.Distance(wp1->location)));

  /* The line runs east/west through wp1, thus its nearest point is due
     north of the aircraft.  Its ends follow a great circle and bulge
     about 7.8 m north of the latitude of wp1, so the expected value is
     only good to a few tens of metres; ACCURACY 500 would spend most
     of its relative budget on that. */
  ok1(equals(line_on.remaining,
             aircraft.location.Distance(MakeGeoPoint(0.05, 45)), 100));

  /* the option changes navigation only, not the task */
  ok1(equals(line_off.planned, line_on.planned));
  ok1(equals(line_off.distance_min, line_on.distance_min));

  /* a start cylinder is navigated to at its near edge, one radius
     short of the center on the bearing to the aircraft; the radius is
     small enough to leave the aircraft outside the cylinder */
  const auto cylinder_off =
    FlyToStart(std::make_unique<CylinderZone>(wp1->location, 5000),
               false, aircraft);
  const auto cylinder_on =
    FlyToStart(std::make_unique<CylinderZone>(wp1->location, 5000),
               true, aircraft);

  ok1(equals(cylinder_on.remaining,
             aircraft.location.Distance(wp1->location) - 5000, 100));
  ok1(cylinder_on.remaining < cylinder_off.remaining);
  ok1(equals(cylinder_off.planned, cylinder_on.planned));

  /* a sector covers only part of the circle, so it keeps the node
     find_best_start() picks for it */
  const auto sector_off =
    FlyToStart(std::make_unique<SectorZone>(wp1->location, 10000),
               false, aircraft);
  const auto sector_on =
    FlyToStart(std::make_unique<SectorZone>(wp1->location, 10000),
               true, aircraft);

  ok1(equals(sector_off.remaining, sector_on.remaining));

  /* inside a start cylinder the nearest point of the rim is behind the
     aircraft, away from the next task point, so the node
     find_best_start() picks is kept */
  const auto inside_off =
    FlyToStart(std::make_unique<CylinderZone>(wp1->location, 20000),
               false, aircraft);
  const auto inside_on =
    FlyToStart(std::make_unique<CylinderZone>(wp1->location, 20000),
               true, aircraft);

  ok1(equals(inside_off.remaining, inside_on.remaining));
}

/**
 * With "navigate to nearest point" enabled, a finish line is reached at
 * its nearest point.  A finish cylinder already refers to its rim.
 */
static void
TestFinishNearestPoint()
{
  /* the aircraft is short of the finish, well east of its center */
  const auto aircraft = MakeTimedAircraft(0.05, 45.95, 1500,
                                          FloatDuration{3600});

  const auto line_off =
    FlyToFinish(std::make_unique<LineSectorZone>(wp3->location, 20000),
                false, aircraft);
  const auto line_on =
    FlyToFinish(std::make_unique<LineSectorZone>(wp3->location, 20000),
                true, aircraft);

  ok1(equals(line_off, aircraft.location.Distance(wp3->location), 100));
  ok1(equals(line_on,
             aircraft.location.Distance(MakeGeoPoint(0.05, 46)), 100));

  const auto cylinder_on =
    FlyToFinish(std::make_unique<CylinderZone>(wp3->location, 3000),
                true, aircraft);

  /* the minimum distance path already picks a point on the rim of a
     finish cylinder rather than its center, so the option only makes
     that point the nearest one */
  ok1(equals(cylinder_on,
             aircraft.location.Distance(wp3->location) - 3000, 100));
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

int main()
{
  plan_tests(746 + 8 + 31);

  task_behaviour.SetDefaults();

  TestTravelledDistance();
  TestAll();

  glide_polar.SetMC(1);
  TestAll();

  glide_polar.SetMC(2);
  TestAll();

  glide_polar.SetMC(4);
  TestAll();

  TestStartNearestPoint();
  TestFinishNearestPoint();

  return exit_status();
}
