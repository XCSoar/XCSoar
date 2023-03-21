// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskComputer.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "NMEA/Aircraft.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Settings.hpp"

#include <algorithm>

using std::max;
using namespace std::chrono;

// JMW TODO: abstract up to higher layer so a base copy of this won't
// call any event

TaskComputer::TaskComputer(ProtectedTaskManager &_task,
                           const Airspaces &airspace_database,
                           const ProtectedAirspaceWarningManager *warnings)
  :task(_task),
   route(airspace_database, warnings),
   contest(trace.GetFull(), trace.GetContest(), trace.GetSprint())
{
  task.SetRoutePlanner(&route.GetProtectedRoutePlanner());
}

void
TaskComputer::ResetFlight([[maybe_unused]] const bool full)
{
  task.Reset();
  route.ResetFlight();
  trace.Reset();
  contest.Reset();

  valid_last_state = false;
  last_flying = false;

  last_location_available.Clear();
}

void
TaskComputer::ProcessBasicTask(const MoreData &basic,
                               DerivedInfo &calculated,
                               const ComputerSettings &settings_computer,
                               bool force)
{
  trace.Update(settings_computer, basic, calculated);

  ProtectedTaskManager::ExclusiveLease _task(task);

  _task->SetTaskBehaviour(settings_computer.task);

  if (force || (last_location_available &&
                basic.location_available.Modified(last_location_available))) {
    const AircraftState current_as = ToAircraftState(basic, calculated);
    const AircraftState &last_as = valid_last_state ? last_state : current_as;

    _task->Update(current_as, last_as);

    last_state = current_as;
    valid_last_state = true;

    const auto fallback_mc = calculated.last_thermal.IsDefined() &&
      calculated.last_thermal_average_smooth > 0
      ? calculated.last_thermal_average_smooth
      : 0;
    if (_task->UpdateAutoMC(current_as, fallback_mc))
      calculated.ProvideAutoMacCready(basic.clock,
                                      _task->GetGlidePolar().GetMC());
  }

  last_location_available = basic.location_available;

  calculated.task_stats = _task->GetStats();
  calculated.ordered_task_stats = _task->GetOrderedTask().GetStats();
  calculated.common_stats = _task->GetCommonStats();
  calculated.glide_polar_safety = _task->GetSafetyPolar();
}

void
TaskComputer::ProcessMoreTask(const MoreData &basic,
                              DerivedInfo &calculated,
                              const ComputerSettings &settings_computer)
{
  const GlidePolar &glide_polar = settings_computer.polar.glide_polar_task;
  const GlidePolar &safety_polar = calculated.glide_polar_safety;

  route.ProcessRoute(basic, calculated,
                     settings_computer.task.glide,
                     settings_computer.task.route_planner,
                     glide_polar, safety_polar);

  if (settings_computer.features.block_stf_enabled)
    calculated.V_stf = calculated.common_stats.V_block;
  else
    calculated.V_stf = calculated.common_stats.V_dolphin;

  if (calculated.task_stats.current_leg.vector_remaining.IsValid()) {
    const GeoVector &v = calculated.task_stats.current_leg.vector_remaining;
    calculated.auto_zoom_distance = v.distance;
  }
}

[[gnu::pure]]
static TracePoint
Predicted(const ContestSettings &settings,
          const MoreData &basic,
          const ElementStat &current_leg)
{
  if (!basic.time_available || !basic.gps_altitude_available ||
      !settings.predict ||
      !current_leg.location_remaining.IsValid() ||
      !current_leg.solution_remaining.IsDefined())
    return TracePoint::Invalid();

  /* predict that the next task point will be reached, using the
     calculated remaining time and the minimum arrival altitude */
  return TracePoint(current_leg.location_remaining,
                    (basic.time + current_leg.time_remaining_now).Cast<duration<unsigned>>(),
                    current_leg.solution_remaining.min_arrival_altitude,
                    0, 0);
}

void
TaskComputer::ProcessIdle(const MoreData &basic, DerivedInfo &calculated,
                          const ComputerSettings &settings_computer,
                          bool exhaustive)
{
  contest.SetPredicted(Predicted(settings_computer.contest, basic,
                                 calculated.task_stats.current_leg));

  if (exhaustive)
    contest.SolveExhaustive(settings_computer.contest,
                            calculated.contest_stats);
  else
    contest.Solve(settings_computer.contest, calculated.contest_stats);

  const AircraftState as = ToAircraftState(basic, calculated);

  ProtectedTaskManager::ExclusiveLease _task(task);
  _task->UpdateIdle(as);
}

void 
TaskComputer::ProcessAutoTask([[maybe_unused]] const NMEAInfo &basic,
                              const DerivedInfo &calculated)
{
  if (!calculated.flight.flying) {
    /* not flying (yet) */
    last_flying = false;
    return;
  }

  if (last_flying)
    /* still flying, not a takeoff */
    return;

  last_flying = true;

  if (calculated.altitude_agl_valid && calculated.altitude_agl > 500)
    return;

  ProtectedTaskManager::ExclusiveLease _task(task);
  _task->TakeoffAutotask(calculated.flight.takeoff_location,
                         calculated.terrain_altitude);
}

void 
TaskComputer::SetTerrain(const RasterTerrain* _terrain)
{
  route.set_terrain(_terrain);
}
