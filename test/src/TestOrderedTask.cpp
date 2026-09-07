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
#include "Engine/Task/ObservationZones/Boundary.hpp"
#include "Geo/Math.hpp"
#include "Math/Constants.hpp"

#include <cmath>

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

/**
 * While the start point is still the active task point, the aircraft
 * has not started yet: it must still leave through the boundary, so
 * the samples it collects inside the sector constrain nothing.
 *
 * Two things follow, and this checks both.  The origin of the first
 * leg stays on the boundary, moving by at most one of the nodes the
 * boundary is sampled at.  The minimum remaining task distance stays
 * put as the aircraft moves about inside the sector.
 */
static void
TestStartLegOrigin()
{
  ordered_task_settings.SetDefaults();

  constexpr double START_RADIUS = 10000;

  /* CylinderZone::GetBoundary() samples the circle at 20 points, so
     the leg origin can step from one of them to the next as the
     aircraft drifts; that step grows with the radius */
  constexpr double BOUNDARY_STEP = M_2PI * START_RADIUS / 20;

  OrderedTask task(task_behaviour);
  task.Append(StartPoint(std::make_unique<CylinderZone>(wp1->location,
                                                        START_RADIUS),
                         WaypointPtr(wp1), task_behaviour,
                         ordered_task_settings.start_constraints));
  task.Append(ASTPoint(std::make_unique<CylinderZone>(wp3->location, 500),
                       WaypointPtr(wp3), task_behaviour));
  task.Append(FinishPoint(std::make_unique<CylinderZone>(wp4->location, 500),
                          WaypointPtr(wp4), task_behaviour,
                          ordered_task_settings.finish_constraints));
  task.UpdateGeometry();

  ok1(!IsError(task.CheckTask()));

  /* fly a straight line inside the start cylinder, well clear of its
     boundary, so the aircraft never starts */

  auto state_last = MakeTimedAircraft(0, 44.96, 2000, FloatDuration{3600});
  GeoPoint previous = GeoPoint::Invalid();
  double previous_distance_min = -1;

  for (unsigned i = 0; i < 12; ++i) {
    const auto state = MakeTimedAircraft(0.001 * i, 44.96, 2000,
                                         FloatDuration{3600 + 5 * i});
    task.Update(state, state_last, glide_polar);
    state_last = state;

    const GeoPoint origin = task.GetPoint(0).GetLocationRemaining();

    /* the start has not been crossed yet */
    ok1(task.GetActiveTaskPointIndex() == 0);

    /* thus the leg origin must be a point where the start can still
       be crossed, i.e. on the boundary of the start cylinder */
    ok1(equals(wp1->location.Distance(origin), START_RADIUS));

    /* the origin stays on the boundary and moves by at most one
       node.  The two writers minimise different objectives over the
       same nodes, so they can settle one node apart; this bounds
       that step. */
    ok1(!previous.IsValid() ||
        previous.Distance(origin) < 1.5 * BOUNDARY_STEP);

    /* the samples collected inside the sector must not shorten the
       task: the minimum remaining distance does not depend on where
       inside the start the aircraft happens to be */
    const double distance_min = task.GetStats().distance_min;
    ok1(previous_distance_min < 0 ||
        equals(distance_min, previous_distance_min));

    previous = origin;
    previous_distance_min = distance_min;
  }
}

static void
AppendStartNavigationTask(OrderedTask &task)
{
  task.Append(StartPoint(std::make_unique<CylinderZone>(wp1->location, 3000),
                         WaypointPtr(wp1), task_behaviour,
                         ordered_task_settings.start_constraints));
  task.Append(AATPoint(std::make_unique<CylinderZone>(wp5->location, 20000),
                       WaypointPtr(wp5), task_behaviour));
  task.Append(FinishPoint(std::make_unique<CylinderZone>(wp4->location, 500),
                          WaypointPtr(wp4), task_behaviour,
                          ordered_task_settings.finish_constraints));
  task.UpdateGeometry();
}

/**
 * The start boundary node which minimises the distance from the
 * aircraft via that node to the next task point.
 */
static GeoPoint
FindBestStartNode(const OrderedTask &task, const GeoPoint &location)
{
  const GeoPoint &next = task.GetPoint(1).GetLocationRemaining();

  GeoPoint best = GeoPoint::Invalid();
  double best_distance = 0;
  for (const GeoPoint &node : task.GetPoint(0).GetBoundary()) {
    const double distance = ::DoubleDistance(location, node, next);
    if (!best.IsValid() || distance < best_distance) {
      best = node;
      best_distance = distance;
    }
  }

  return best;
}

/**
 * While the start is the active task point, the task navigates to the
 * start boundary node giving the shortest flight to the next task
 * point, and the bearing line points at the same node.  The minimum
 * distance search, which also writes a start node, must not move it.
 *
 * A large AAT area next makes the two searches disagree often.  A
 * reset task forgets the node chosen during the previous flight.
 */
static void
TestStartNavigationLocation()
{
  ordered_task_settings.SetDefaults();

  OrderedTask task(task_behaviour);
  AppendStartNavigationTask(task);

  ok1(!IsError(task.CheckTask()));

  /* circle inside the start cylinder, well clear of its boundary, so
     the aircraft never starts */

  constexpr double CIRCLE_RADIUS = 0.016; // degrees of latitude
  const double lon_scale = 1 / std::cos(wp1->location.latitude.Radians());

  auto state_last = MakeTimedAircraft(0, 45 + CIRCLE_RADIUS, 2000,
                                      FloatDuration{3600});

  for (unsigned i = 1; i <= 40; ++i) {
    const double a = 0.25 * i;
    const auto state =
      MakeTimedAircraft(CIRCLE_RADIUS * lon_scale * std::sin(a),
                        45 + CIRCLE_RADIUS * std::cos(a), 2000,
                        FloatDuration{3600 + 2 * i});
    task.Update(state, state_last, glide_polar);
    state_last = state;

    const GeoPoint origin = task.GetPoint(0).GetLocationRemaining();

    ok1(task.GetActiveTaskPointIndex() == 0);
    ok1(FindBestStartNode(task, state.location).Distance(origin) < 1);
    ok1(task.GetStats().current_leg.location_remaining
        .Distance(origin) < 1);
  }

  /* after a reset, the start on the ground is that of a task which
     was never flown */
  task.Reset();

  OrderedTask fresh(task_behaviour);
  AppendStartNavigationTask(fresh);

  auto ground = MakeTimedAircraft(-0.05, 44.96, 2000, FloatDuration{7200});
  ground.flying = false;
  task.Update(ground, ground, glide_polar);
  fresh.Update(ground, ground, glide_polar);

  ok1(task.GetPoint(0).GetLocationRemaining()
      .Distance(fresh.GetPoint(0).GetLocationRemaining()) < 1);
  ok1(equals(task.GetStats().total.planned.GetDistance(),
             fresh.GetStats().total.planned.GetDistance()));
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
  plan_tests(746 + 8 + 49 + 123);

  task_behaviour.SetDefaults();

  TestTravelledDistance();
  TestStartLegOrigin();
  TestStartNavigationLocation();
  TestAll();

  glide_polar.SetMC(1);
  TestAll();

  glide_polar.SetMC(2);
  TestAll();

  glide_polar.SetMC(4);
  TestAll();

  return exit_status();
}
