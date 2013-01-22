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
#include "AirspaceWarningManager.hpp"
#include "Geo/GeoVector.hpp"
#include "Airspaces.hpp"
#include "AirspaceCircle.hpp"
#include "AirspacePolygon.hpp"
#include "AirspaceIntersectionVisitor.hpp"
#include "Task/Stats/TaskStats.hpp"
#include "Predicate/AirspacePredicateAircraftInside.hpp"

#define CRUISE_FILTER_FACT fixed(0.5)

AirspaceWarningManager::AirspaceWarningManager(const Airspaces &_airspaces,
                                               fixed prediction_time_glide,
                                               fixed prediction_time_filter)
  :airspaces(_airspaces),
   prediction_time_glide(prediction_time_glide),
   prediction_time_filter(prediction_time_filter),
   cruise_filter(prediction_time_filter * CRUISE_FILTER_FACT),
   circling_filter(prediction_time_filter),
   perf_cruise(cruise_filter),
   perf_circling(circling_filter)
{
}

const TaskProjection &
AirspaceWarningManager::GetProjection() const
{
  return airspaces.GetProjection();
}

void
AirspaceWarningManager::SetConfig(const AirspaceWarningConfig &_config)
{
  config = _config;

  SetPredictionTimeGlide(fixed(config.warning_time));
  SetPredictionTimeFilter(fixed(config.warning_time));
}

void
AirspaceWarningManager::Reset(const AircraftState &state)
{
  warnings.clear();
  cruise_filter.Reset(state);
  circling_filter.Reset(state);
}

void 
AirspaceWarningManager::SetPredictionTimeGlide(fixed time)
{
  prediction_time_glide = time;
}

void 
AirspaceWarningManager::SetPredictionTimeFilter(fixed time)
{
  prediction_time_filter = time;
  cruise_filter.Design(std::max(fixed(10),
                                prediction_time_filter * CRUISE_FILTER_FACT));
  circling_filter.Design(std::max(fixed(10), prediction_time_filter));
}

AirspaceWarning& 
AirspaceWarningManager::GetWarning(const AbstractAirspace &airspace)
{
  AirspaceWarning* warning = GetWarningPtr(airspace);
  if (warning)
    return *warning;

  // not found, create new entry
  warnings.push_back(AirspaceWarning(airspace));
  return warnings.back();
}


AirspaceWarning* 
AirspaceWarningManager::GetWarningPtr(const AbstractAirspace &airspace)
{
  for (auto &w : warnings)
    if (&(w.GetAirspace()) == &airspace)
      return &w;

  return NULL;
}

bool 
AirspaceWarningManager::Update(const AircraftState& state,
                               const GlidePolar &glide_polar,
                               const TaskStats &task_stats,
                               const bool circling,
                               const unsigned dt)
{
  bool changed = false;

  // update warning states
  if (airspaces.empty()) {
    // no airspaces, no warnings possible
    assert(warnings.empty());
    return false;
  }

  // save old state
  for (auto &w : warnings)
    w.SaveState();

  // check from strongest to weakest alerts
  UpdateInside(state, glide_polar);
  UpdateGlide(state, glide_polar);
  UpdateFilter(state, circling);
  UpdateTask(state, glide_polar, task_stats);

  // action changes
  for (auto it = warnings.begin(), end = warnings.end(); it != end;) {
    if (it->WarningLive(config.acknowledgement_time, dt)) {
      if (it->ChangedState())
        changed = true;

      it++;
    } else {
      it = warnings.erase(it);
    }
  }

  // sort by importance, most severe top
  warnings.sort();

  return changed;
}

/**
 * Class used temporarily to check intersections with warning system
 */
class AirspaceIntersectionWarningVisitor gcc_final
  : public AirspaceIntersectionVisitor
{
  const AircraftState state;
  const AirspaceAircraftPerformance &perf;
  AirspaceWarningManager &warning_manager;
  const AirspaceWarning::State warning_state;
  const fixed max_time;
  bool found;
  const fixed max_alt;
  bool mode_inside;

public:
  /**
   * Constructor
   *
   * @param state State of aircraft
   * @param perf Aircraft performance model
   * @param warning_manager Warning manager to add items to
   * @param warning_state Type of warning
   * @param max_time Time limit of intercept
   * @param max_alt Maximum height of base to allow (optional)
   *
   * @return Initialised object
   */
  AirspaceIntersectionWarningVisitor(const AircraftState &_state,
                                     const AirspaceAircraftPerformance &_perf,
                                     AirspaceWarningManager &_warning_manager,
                                     const AirspaceWarning::State _warning_state,
                                     const fixed _max_time,
                                     const fixed _max_alt = fixed(-1)):
    state(_state),
    perf(_perf),
    warning_manager(_warning_manager),
    warning_state(_warning_state),
    max_time(_max_time),
    found(false),
    max_alt(_max_alt),
    mode_inside(false)
    {      
    };

  /**
   * Check whether this intersection should be added to, or updated in, the warning manager
   *
   * @param airspace Airspace corresponding to current intersection
   */
  void Intersection(const AbstractAirspace& airspace) {
    if (!airspace.IsActive())
      return; // ignore inactive airspaces completely

    if (!warning_manager.GetConfig().IsClassEnabled(airspace.GetType()) ||
        ExcludeAltitude(airspace))
      return;

    AirspaceWarning& warning = warning_manager.GetWarning(airspace);
    if (warning.IsStateAccepted(warning_state)) {

      AirspaceInterceptSolution solution;

      if (mode_inside) {
        airspace.Intercept(state, perf, solution, state.location, state.location);
      } else {
        solution = Intercept(airspace, state, perf);
      }
      if (!solution.IsValid())
        return;
      if (solution.elapsed_time > max_time)
        return;

      warning.UpdateSolution(warning_state, solution);
      found = true;
    }
  }

  virtual void Visit(const AbstractAirspace &as) gcc_override {
    Intersection(as);
  }

  void Visit(const Airspace& a) {
    AirspaceVisitor::Visit(a);
  }

  /**
   * Determine whether intersections for this type were found (new or modified)
   *
   * @return True if intersections were found
   */
  bool Found() const {
    return found;
  }

  void SetMode(bool m) {
    mode_inside = m;
  }

private:
  bool ExcludeAltitude(const AbstractAirspace& airspace) {
    if (!positive(max_alt))
      return false;

    return (airspace.GetBaseAltitude(state) > max_alt);
  }
};


bool 
AirspaceWarningManager::UpdatePredicted(const AircraftState& state, 
                                         const GeoPoint &location_predicted,
                                         const AirspaceAircraftPerformance &perf,
                                         const AirspaceWarning::State warning_state,
                                         const fixed max_time) 
{
  // this is the time limit of intrusions, beyond which we are not interested.
  // it can be the minimum of the user set warning time, or the time of the 
  // task segment

  const fixed max_time_limit = std::min(fixed(config.warning_time), max_time);

  // the ceiling is the max height for predicted intrusions, given
  // that you may be climbing.  the ceiling is nominally set at 1000m
  // above the current altitude, but the 1000m margin should be at
  // least as big as config.AltWarningMargin since if the airspace is
  // visible according to that display mode, it should have warnings
  // collected for it.  It is very unlikely users will have more than 1000m
  // in AltWarningMargin anyway.

  const fixed ceiling = state.altitude
    + fixed(std::max((unsigned)1000, config.altitude_warning_margin));

  AirspaceIntersectionWarningVisitor visitor(state, perf, 
                                             *this, 
                                             warning_state, max_time_limit,
                                             ceiling);

  airspaces.VisitIntersecting(state.location, location_predicted, visitor);

  visitor.SetMode(true);
  airspaces.VisitInside(state.location, visitor);

  return visitor.Found();
}


bool 
AirspaceWarningManager::UpdateTask(const AircraftState &state,
                                   const GlidePolar &glide_polar,
                                   const TaskStats &task_stats)
{
  if (!glide_polar.IsValid())
    return false;

  const ElementStat &current_leg = task_stats.current_leg;

  if (!task_stats.task_valid || !current_leg.location_remaining.IsValid())
    return false;

  const GlideResult &solution = current_leg.solution_remaining;
  if (!solution.IsOk() || !solution.IsAchievable())
    /* glide solver failed, cannot continue */
    return false;

  AirspaceAircraftPerformanceTask perf_task(glide_polar,
                                            current_leg.solution_remaining);
  GeoPoint location_tp = current_leg.location_remaining;
  const fixed time_remaining = solution.time_elapsed;

  const GeoVector vector(state.location, location_tp);
  fixed max_distance = config.warning_time * glide_polar.GetVMax();
  if (vector.distance > max_distance)
    /* limit the distance to what our glider can actually fly within
       the configured warning time */
    location_tp = state.location.IntermediatePoint(location_tp, max_distance);

  return UpdatePredicted(state, location_tp, perf_task,
                          AirspaceWarning::WARNING_TASK, time_remaining);
}


bool 
AirspaceWarningManager::UpdateFilter(const AircraftState& state, const bool circling)
{
  // update both filters even though we are using only one
  cruise_filter.Update(state);
  circling_filter.Update(state);

  const GeoPoint location_predicted = circling?
    circling_filter.GetPredictedState(prediction_time_filter).location:
    cruise_filter.GetPredictedState(prediction_time_filter).location;

  if (circling) 
    return UpdatePredicted(state, location_predicted,
                            perf_circling,
                            AirspaceWarning::WARNING_FILTER, prediction_time_filter);
  else
    return UpdatePredicted(state, location_predicted,
                            perf_cruise,
                            AirspaceWarning::WARNING_FILTER, prediction_time_filter);
}


bool 
AirspaceWarningManager::UpdateGlide(const AircraftState &state,
                                    const GlidePolar &glide_polar)
{
  const GeoPoint location_predicted = 
    state.GetPredictedState(prediction_time_glide).location;

  const AirspaceAircraftPerformanceGlide perf_glide(glide_polar);
  return UpdatePredicted(state, location_predicted,
                          perf_glide,
                          AirspaceWarning::WARNING_GLIDE, prediction_time_glide);
}


bool 
AirspaceWarningManager::UpdateInside(const AircraftState& state,
                                     const GlidePolar &glide_polar)
{
  bool found = false;

  AirspacePredicateAircraftInside condition(state);

  Airspaces::AirspaceVector results = airspaces.FindInside(state, condition);
  for (const auto &i : results) {
    const AbstractAirspace& airspace = *i.GetAirspace();

    if (!airspace.IsActive())
      continue; // ignore inactive airspaces

    if (!config.IsClassEnabled(airspace.GetType()))
      continue;

    AirspaceWarning& warning = GetWarning(airspace);

    if (warning.IsStateAccepted(AirspaceWarning::WARNING_INSIDE)) {
      GeoPoint c = airspace.ClosestPoint(state.location, GetProjection());
      const AirspaceAircraftPerformanceGlide perf_glide(glide_polar);
      AirspaceInterceptSolution solution;
      airspace.Intercept(state, c, GetProjection(), perf_glide, solution);

      warning.UpdateSolution(AirspaceWarning::WARNING_INSIDE, solution);
      found = true;
    }
  }

  return found;
}

void 
AirspaceWarningManager::AcknowledgeWarning(const AbstractAirspace& airspace,
                                            const bool set)
{
  GetWarning(airspace).AcknowledgeWarning(set);
}

void 
AirspaceWarningManager::AcknowledgeInside(const AbstractAirspace& airspace,
                                           const bool set)
{
  GetWarning(airspace).AcknowledgeInside(set);
}

void 
AirspaceWarningManager::AcknowledgeDay(const AbstractAirspace& airspace,
                                        const bool set)
{
  GetWarning(airspace).AcknowledgeDay(set);
}

bool
AirspaceWarningManager::GetAckDay(const AbstractAirspace &airspace) const
{
  const AirspaceWarning *warning = GetWarningPtr(airspace);
  return (warning != NULL ? warning->GetAckDay() : false);
}


void 
AirspaceWarningManager::AcknowledgeAll()
{
  for (auto &w : warnings) {
    w.AcknowledgeWarning(true);
    w.AcknowledgeInside(true);
  }
}
