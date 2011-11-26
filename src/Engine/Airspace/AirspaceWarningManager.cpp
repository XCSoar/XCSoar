/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Navigation/Geometry/GeoVector.hpp"
#include "Airspaces.hpp"
#include "AirspaceCircle.hpp"
#include "AirspacePolygon.hpp"
#include "AirspaceIntersectionVisitor.hpp"
#include "AirspaceWarningVisitor.hpp"
#include "Predicate/AirspacePredicateAircraftInside.hpp"
#include "Task/TaskManager.hpp"

#define CRUISE_FILTER_FACT fixed_half

AirspaceWarningManager::AirspaceWarningManager(const Airspaces &_airspaces,
                                               const TaskManager &_task,
                                               fixed prediction_time_glide,
                                               fixed prediction_time_filter):
  airspaces(_airspaces),
  prediction_time_glide(prediction_time_glide),
  prediction_time_filter(prediction_time_filter),
  perf_glide(_task.GetGlidePolar()),
  cruise_filter(prediction_time_filter*CRUISE_FILTER_FACT),
  circling_filter(prediction_time_filter),
  perf_cruise(cruise_filter),
  perf_circling(circling_filter),
  task(_task),
  glide_polar(_task.GetGlidePolar())
{
}

void
AirspaceWarningManager::SetConfig(const AirspaceWarningConfig &_config)
{
  config = _config;

  SetPredictionTimeGlide(fixed(config.WarningTime));
  SetPredictionTimeFilter(fixed(config.WarningTime));
}

void
AirspaceWarningManager::Reset(const AircraftState& state)
{
  warnings.clear();
  cruise_filter.Reset(state);
  circling_filter.Reset(state);
}

void 
AirspaceWarningManager::SetPredictionTimeGlide(const fixed& the_time)
{
  prediction_time_glide = the_time;
}

void 
AirspaceWarningManager::SetPredictionTimeFilter(const fixed& the_time)
{
  prediction_time_filter = the_time;
  cruise_filter.Design(max(fixed(10),prediction_time_filter*CRUISE_FILTER_FACT));
  circling_filter.Design(max(fixed(10),prediction_time_filter));
}

AirspaceWarning& 
AirspaceWarningManager::GetWarning(const AbstractAirspace& airspace)
{
  AirspaceWarning* warning = GetWarningPtr(airspace);
  if (warning)
    return *warning;

  // not found, create new entry
  warnings.push_back(AirspaceWarning(airspace));
  return warnings.back();
}


AirspaceWarning* 
AirspaceWarningManager::GetWarningPtr(const AbstractAirspace& airspace) 
{
  for (AirspaceWarningList::iterator it = warnings.begin();
       it != warnings.end(); ++it)
    if (&(it->get_airspace()) == &airspace)
      return &(*it);

  return NULL;
}

bool 
AirspaceWarningManager::Update(const AircraftState& state,
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
  for (AirspaceWarningList::iterator it = warnings.begin();
       it != warnings.end(); ++it)
    it->save_state();

  // check from strongest to weakest alerts
  UpdateInside(state);
  UpdateGlide(state);
  UpdateFilter(state, circling);
  UpdateTask(state);

  // action changes
  for (AirspaceWarningList::iterator it = warnings.begin();
       it != warnings.end(); ) {
    if (it->warning_live(config.AcknowledgementTime, dt)) {
      if (it->changed_state())
        changed = true;

      it++;
    } else {
      if (!it->trivial()) {
        //changed = true; // was downgraded to eliminate
      }
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
class AirspaceIntersectionWarningVisitor: 
  public AirspaceIntersectionVisitor {
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
  AirspaceIntersectionWarningVisitor(const AircraftState &state,
                                     const AirspaceAircraftPerformance &perf,
                                     AirspaceWarningManager &warning_manager,
                                     const AirspaceWarning::State warning_state,
                                     const fixed max_time,
                                     const fixed max_alt = -fixed_one):
    m_state(state),
    m_perf(perf),
    m_warning_manager(warning_manager),
    m_warning_state(warning_state),
    m_max_time(max_time),
    m_found(false),
    m_max_alt(max_alt),
    mode_inside(false)
    {      
    };

/** 
 * Check whether this intersection should be added to, or updated in, the warning manager
 * 
 * @param airspace Airspace corresponding to current intersection
 */
  void intersection(const AbstractAirspace& airspace) {
    if (!airspace.IsActive())
      return; // ignore inactive airspaces completely

    if (!m_warning_manager.GetConfig().class_enabled(airspace.GetType()) ||
        exclude_alt(airspace))
      return;

    AirspaceWarning& warning = m_warning_manager.GetWarning(airspace);
    if (warning.state_accepted(m_warning_state)) {

      AirspaceInterceptSolution solution;

      if (mode_inside) {
        airspace.Intercept(m_state, m_perf, solution, m_state.location, m_state.location);
      } else {
        solution = intercept(airspace, m_state, m_perf);
      }
      if (!solution.IsValid())
        return;
      if (solution.elapsed_time > m_max_time)
        return;

      warning.update_solution(m_warning_state, solution);
      m_found = true;
    }
  }
  void Visit(const AirspaceCircle& as) {
    intersection(as);
  }
  void Visit(const AirspacePolygon& as) {
    intersection(as);
  }
  void Visit(const Airspace& a) {
    AirspaceVisitor::Visit(a);
  }

/** 
 * Determine whether intersections for this type were found (new or modified)
 * 
 * @return True if intersections were found
 */
  bool found() const {
    return m_found;
  }

  void set_mode(bool m) {
    mode_inside = m;
  }
private:
  const AircraftState m_state;
  const AirspaceAircraftPerformance &m_perf;
  AirspaceWarningManager &m_warning_manager;
  const AirspaceWarning::State m_warning_state;
  const fixed m_max_time;
  bool m_found;
  const fixed m_max_alt;
  bool mode_inside;

  bool exclude_alt(const AbstractAirspace& airspace) {
    if (!positive(m_max_alt)) {
      return false;
    }
    return (airspace.GetBaseAltitude(m_state)> m_max_alt);
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

  const fixed max_time_limit = min(fixed(config.WarningTime), max_time);

  // the ceiling is the max height for predicted intrusions, given
  // that you may be climbing.  the ceiling is nominally set at 1000m
  // above the current altitude, but the 1000m margin should be at
  // least as big as config.AltWarningMargin since if the airspace is
  // visible according to that display mode, it should have warnings
  // collected for it.  It is very unlikely users will have more than 1000m
  // in AltWarningMargin anyway.

  const fixed ceiling = state.altitude + fixed(max((unsigned)1000, config.AltWarningMargin));

  AirspaceIntersectionWarningVisitor visitor(state, perf, 
                                             *this, 
                                             warning_state, max_time_limit,
                                             ceiling);

  GeoVector vector_predicted(state.location, location_predicted);
  airspaces.visit_intersecting(state.location, vector_predicted, visitor);

  visitor.set_mode(true);
  airspaces.visit_inside(state.location, visitor);

  return visitor.found();
}


bool 
AirspaceWarningManager::UpdateTask(const AircraftState& state)
{
  if (!task.GetActiveTaskPoint()) {
    // empty task, nothing to do
    return false;
  }

  const GlideResult &solution = task.GetStats().current_leg.solution_remaining;
  if (!solution.IsOk() || !solution.IsAchievable())
    /* glide solver failed, cannot continue */
    return false;

  AirspaceAircraftPerformanceTask perf_task(state, glide_polar, task);
  const GeoPoint location_tp = task.GetActiveTaskPoint()->GetLocationRemaining();
  const fixed time_remaining = task.GetStats().current_leg.solution_remaining.time_elapsed; 

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
AirspaceWarningManager::UpdateGlide(const AircraftState& state)
{
  const GeoPoint location_predicted = 
    state.GetPredictedState(prediction_time_glide).location;

  return UpdatePredicted(state, location_predicted,
                          perf_glide,
                          AirspaceWarning::WARNING_GLIDE, prediction_time_glide);
}


bool 
AirspaceWarningManager::UpdateInside(const AircraftState& state)
{
  bool found = false;

  AirspacePredicateAircraftInside condition(state);

  Airspaces::AirspaceVector results = airspaces.find_inside(state, condition);
  for (Airspaces::AirspaceVector::iterator it = results.begin();
       it != results.end(); ++it) {

    const AbstractAirspace& airspace = *it->get_airspace();

    if (!airspace.IsActive())
      continue; // ignore inactive airspaces

    if (!config.class_enabled(airspace.GetType()))
      continue;

    AirspaceWarning& warning = GetWarning(airspace);

    if (warning.state_accepted(AirspaceWarning::WARNING_INSIDE)) {
      GeoPoint c = airspace.ClosestPoint(state.location);
      GeoVector vector_exit(state.location, c);
      AirspaceInterceptSolution solution;
      airspace.Intercept(state, vector_exit, perf_glide, solution); 

      warning.update_solution(AirspaceWarning::WARNING_INSIDE, solution);
      found = true;
    }
  }

  return found;
}


void
AirspaceWarningManager::VisitWarnings(AirspaceWarningVisitor& visitor) const
{
  for (AirspaceWarningList::const_iterator it = warnings.begin();
       it != warnings.end(); ++it) {
    visitor.Visit(*it);
  }
}


void 
AirspaceWarningManager::AcknowledgeWarning(const AbstractAirspace& airspace,
                                            const bool set)
{
  GetWarning(airspace).acknowledge_warning(set);
}

void 
AirspaceWarningManager::AcknowledgeInside(const AbstractAirspace& airspace,
                                           const bool set)
{
  GetWarning(airspace).acknowledge_inside(set);
}

void 
AirspaceWarningManager::AcknowledgeDay(const AbstractAirspace& airspace,
                                        const bool set)
{
  GetWarning(airspace).acknowledge_day(set);
}

bool
AirspaceWarningManager::GetAckDay(const AbstractAirspace& airspace)
{
  AirspaceWarning* warning = GetWarningPtr(airspace);
  return (warning != NULL ? warning->get_ack_day() : false);
}


void 
AirspaceWarningManager::AcknowledgeAll()
{
  for (AirspaceWarningList::iterator it = warnings.begin();
       it != warnings.end(); ++it) {
    (*it).acknowledge_warning(true);
    (*it).acknowledge_inside(true);
  }
}
