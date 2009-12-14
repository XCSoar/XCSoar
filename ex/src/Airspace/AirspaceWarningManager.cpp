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
  for (AirspaceWarningList::iterator it = m_warnings.begin();
       it != m_warnings.end(); ++it) {
    if (&(it->get_airspace()) == &airspace) {
      return (*it);
    }
  }

  // not found, create new entry
  m_warnings.push_back(AirspaceWarning(airspace));
  return m_warnings.back();
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

    if (it->action_updates()) {
      if (it->changed_state()) {
        changed = true;
      }
      it++;
    } else {
      if (!it->trivial()) {
        changed = true; // was downgraded to eliminate
      }
      it = m_warnings.erase(it);
    }
  }

  return changed;
}


class AirspaceIntersectionWarningVisitor: 
  public AirspaceIntersectionVisitor {
public:
  AirspaceIntersectionWarningVisitor(const AIRCRAFT_STATE &state,
                                     const AirspaceAircraftPerformance &perf,
                                     AirspaceWarningManager &warning_manager,
                                     const AirspaceWarning::AirspaceWarningState warning_state,
                                     const fixed max_time):
    m_state(state),
    m_perf(perf),
    m_warning_manager(warning_manager),
    m_warning_state(warning_state),
    m_max_time(max_time),
    m_found(false)
    {      
    };

  void intersection(const AbstractAirspace& airspace) {
    AirspaceWarning& warning = m_warning_manager.get_warning(airspace);
    if (warning.state_accepted(m_warning_state)) {
      AirspaceInterceptSolution solution = intercept(airspace, m_state, m_perf);
      if (solution.valid() && (solution.elapsed_time <= m_max_time)) {
        warning.update_solution(m_warning_state, solution);
        m_found = true;
      }
    }
  }
  virtual void Visit(const AirspaceCircle& as) {
    intersection(as);
  }
  virtual void Visit(const AirspacePolygon& as) {
    intersection(as);
  }
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
};


bool 
AirspaceWarningManager::update_predicted(const AIRCRAFT_STATE& state, 
                                         const GEOPOINT &location_predicted,
                                         const AirspaceAircraftPerformance &perf,
                                         const AirspaceWarning::AirspaceWarningState& warning_state,
                                         const fixed max_time) 
{
  AirspaceIntersectionWarningVisitor visitor(state, perf, *this, warning_state, max_time);

  GeoVector vector_predicted(state.Location, location_predicted);
  m_airspaces.visit_intersecting(state.Location, vector_predicted, visitor, true);

  return visitor.found();
}


bool 
AirspaceWarningManager::update_task(const AIRCRAFT_STATE& state)
{

  return false;
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
