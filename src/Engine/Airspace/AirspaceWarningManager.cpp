/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "AirspaceCircle.hpp"
#include "AirspacePolygon.hpp"
#include "AirspaceIntersectionVisitor.hpp"
#include "Task/TaskManager.hpp"


AirspaceWarningManager::AirspaceWarningManager(const Airspaces& airspaces,
                                               const AIRCRAFT_STATE &state,
                                               const TaskManager &task_manager,
                                               const fixed& prediction_time_glide,
                                               const fixed& prediction_time_filter):
  m_airspaces(airspaces),
  m_prediction_time_glide(prediction_time_glide),
  m_prediction_time_filter(prediction_time_filter),
  m_perf_glide(task_manager.get_glide_polar()),
  m_state_filter(state, prediction_time_filter),
  m_perf_filter(m_state_filter),
  m_task(task_manager),
  m_glide_polar(task_manager.get_glide_polar())
{
}

void
AirspaceWarningManager::reset(const AIRCRAFT_STATE& state)
{
  m_warnings.clear();

  m_state_filter.reset(state);
}

void 
AirspaceWarningManager::set_prediction_time_glide(const fixed& the_time)
{
  m_prediction_time_glide = the_time;
}

void 
AirspaceWarningManager::set_prediction_time_filter(const fixed& the_time)
{
  m_prediction_time_filter = the_time;
  m_state_filter.design(m_prediction_time_filter); // or multiple of?
}


AirspaceWarning& 
AirspaceWarningManager::get_warning(const AbstractAirspace& airspace)
{
  AirspaceWarning* warning = get_warning_ptr(airspace);

  if (warning) {
    return *warning;
  } else {
    // not found, create new entry
    m_warnings.push_back(AirspaceWarning(airspace));
    return m_warnings.back();
  }
}


AirspaceWarning* 
AirspaceWarningManager::get_warning_ptr(const AbstractAirspace& airspace) 
{
  for (AirspaceWarningList::iterator it = m_warnings.begin();
       it != m_warnings.end(); ++it) {
    if (&(it->get_airspace()) == &airspace) {
      return &(*it);
    }
  }
  return NULL;
}

AirspaceWarning* 
AirspaceWarningManager::get_warning(const unsigned index)
{
  unsigned i=0;
  for (AirspaceWarningList::iterator it = m_warnings.begin();
       it != m_warnings.end(); ++it, ++i) {
    if (i==index) {
      return &(*it);
    }
  }
  return NULL;
}


int
AirspaceWarningManager::get_warning_index(const AbstractAirspace& airspace) const
{
  int i=0;
  for (AirspaceWarningList::const_iterator it = m_warnings.begin();
       it != m_warnings.end(); ++it, ++i) {
    if (&(it->get_airspace()) == &airspace) {
      return i;
    }
  }
  return -1;
}


bool 
AirspaceWarningManager::update(const AIRCRAFT_STATE& state)
{
  bool changed = false;

  // save old state
  for (AirspaceWarningList::iterator it = m_warnings.begin();
       it != m_warnings.end(); ++it) {
    it->save_state();
  }

  // check from strongest to weakest alerts 

  update_inside(state);
  update_glide(state);
  update_filter(state);
  update_task(state);

  // action changes
  for (AirspaceWarningList::iterator it = m_warnings.begin();
       it != m_warnings.end(); ) {

    if (it->warning_live()) {
      if (it->changed_state()) {
        changed = true;
      }
      it++;
    } else {
      if (!it->trivial()) {
//        changed = true; // was downgraded to eliminate
      }
      it = m_warnings.erase(it);
    }
  }

  // sort by importance, most severe top
  m_warnings.sort();

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
  AirspaceIntersectionWarningVisitor(const AIRCRAFT_STATE &state,
                                     const AirspaceAircraftPerformance &perf,
                                     AirspaceWarningManager &warning_manager,
                                     const AirspaceWarning::AirspaceWarningState warning_state,
                                     const fixed max_time,
                                     const fixed max_alt = -fixed_one):
    m_state(state),
    m_perf(perf),
    m_warning_manager(warning_manager),
    m_warning_state(warning_state),
    m_max_time(max_time),
    m_found(false),
    m_max_alt(max_alt)
    {      
    };

/** 
 * Check whether this intersection should be added to, or updated in, the warning manager
 * 
 * @param airspace Airspace corresponding to current intersection
 */
  void intersection(const AbstractAirspace& airspace) {
    if (exclude_alt(airspace)) {
      return;
    }
    AirspaceWarning& warning = m_warning_manager.get_warning(airspace);
    if (warning.state_accepted(m_warning_state)) {
      AirspaceInterceptSolution solution = intercept(airspace, m_state, m_perf);
      if (solution.valid() && (solution.elapsed_time <= m_max_time)) {
        warning.update_solution(m_warning_state, solution);
        m_found = true;
      }
    }
  }
  void Visit(const AirspaceCircle& as) {
    intersection(as);
  }
  void Visit(const AirspacePolygon& as) {
    intersection(as);
  }

/** 
 * Determine whether intersections for this type were found (new or modified)
 * 
 * @return True if intersections were found
 */
  bool found() const {
    return m_found;
  }
private:
  const AIRCRAFT_STATE m_state;
  const AirspaceAircraftPerformance &m_perf;
  AirspaceWarningManager &m_warning_manager;
  const AirspaceWarning::AirspaceWarningState m_warning_state;
  const fixed m_max_time;
  bool m_found;
  const fixed m_max_alt;

  bool exclude_alt(const AbstractAirspace& airspace) {
    if (!positive(m_max_alt)) {
      return false;
    }
    return (airspace.get_base_altitude()> m_max_alt);
  }


};


bool 
AirspaceWarningManager::update_predicted(const AIRCRAFT_STATE& state, 
                                         const GEOPOINT &location_predicted,
                                         const AirspaceAircraftPerformance &perf,
                                         const AirspaceWarning::AirspaceWarningState& warning_state,
                                         const fixed max_time) 
{
  AirspaceIntersectionWarningVisitor visitor(state, perf, *this, warning_state, max_time,
    state.NavAltitude + 1000);

  GeoVector vector_predicted(state.Location, location_predicted);
  m_airspaces.visit_intersecting(state.Location, vector_predicted, visitor, true);

  return visitor.found();
}


bool 
AirspaceWarningManager::update_task(const AIRCRAFT_STATE& state)
{
  if (!m_task.getActiveTaskPoint()) {
    // empty task, nothing to do
    return false;
  }

  AirspaceAircraftPerformanceTask perf_task(state, m_glide_polar, m_task);
  const GEOPOINT location_tp = m_task.getActiveTaskPoint()->get_location_remaining();
  const fixed time_remaining = m_task.get_stats().current_leg.solution_remaining.TimeElapsed; 

  return update_predicted(state, location_tp, perf_task,
                          AirspaceWarning::WARNING_TASK, time_remaining);
}


bool 
AirspaceWarningManager::update_filter(const AIRCRAFT_STATE& state)
{
  m_state_filter.update(state);

  const GEOPOINT location_predicted = 
    m_state_filter.get_predicted_state(m_prediction_time_filter).Location;

  return update_predicted(state, location_predicted,
                          m_perf_filter,
                          AirspaceWarning::WARNING_FILTER, m_prediction_time_filter);
}


bool 
AirspaceWarningManager::update_glide(const AIRCRAFT_STATE& state)
{
  const GEOPOINT location_predicted = 
    state.get_predicted_state(m_prediction_time_glide).Location;

  return update_predicted(state, location_predicted,
                          m_perf_glide,
                          AirspaceWarning::WARNING_GLIDE, m_prediction_time_glide);
}


bool 
AirspaceWarningManager::update_inside(const AIRCRAFT_STATE& state)
{
  bool found = false;

  AirspacePredicateAircraftInside condition(state);

  Airspaces::AirspaceVector results = m_airspaces.find_inside(state, condition);
  for (Airspaces::AirspaceVector::iterator it = results.begin();
       it != results.end(); ++it) {

    const AbstractAirspace& airspace = *it->get_airspace();
    AirspaceWarning& warning = get_warning(airspace);

    if (warning.state_accepted(AirspaceWarning::WARNING_INSIDE)) {
      GEOPOINT c = airspace.closest_point(state.Location);
      GeoVector vector_exit(state.Location, c);
      AirspaceInterceptSolution solution;
      airspace.intercept(state, vector_exit, m_perf_glide, solution); 

      warning.update_solution(AirspaceWarning::WARNING_INSIDE, solution);
      found = true;
    }
  }

  return found;
}


void
AirspaceWarningManager::visit_warnings(AirspaceWarningVisitor& visitor) const
{
  for (AirspaceWarningList::const_iterator it = m_warnings.begin();
       it != m_warnings.end(); ++it) {
    it->Accept(visitor);
  }
}


void 
AirspaceWarningManager::acknowledge_warning(const AbstractAirspace& airspace,
                                            const bool set)
{
  get_warning(airspace).acknowledge_warning(set);
}

void 
AirspaceWarningManager::acknowledge_inside(const AbstractAirspace& airspace,
                                           const bool set)
{
  get_warning(airspace).acknowledge_inside(set);
}

void 
AirspaceWarningManager::acknowledge_day(const AbstractAirspace& airspace,
                                        const bool set)
{
  get_warning(airspace).acknowledge_day(set);
}

