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
#ifndef AIRSPACE_WARNING_MANAGER_HPP
#define AIRSPACE_WARNING_MANAGER_HPP

#include "Util/NonCopyable.hpp"
#include "Airspaces.hpp"
#include "AirspaceWarning.hpp"
#include "AirspaceWarningVisitor.hpp"
#include <list>

/**
 * Class to detect and track airspace warnings
 */
class AirspaceWarningManager: 
  public NonCopyable
{
public:
  /** 
   * Default constructor
   * 
   * @param airspaces Store of airspaces
   * @param state Initial state of aircraft
   * @param glide_polar Polar used for glide predictions
   * @param prediction_time_glide Time (s) of glide predictor (default 15 s)
   * @param prediction_time_filter Time (s) of state filter predictor (default 60 s)
   *
   * @return Initialised object
   */
  AirspaceWarningManager(const Airspaces& airspaces,
                         const AIRCRAFT_STATE &state,
                         const GlidePolar &glide_polar,
                         const fixed& prediction_time_glide=15.0,
                         const fixed& prediction_time_filter=60.0):
    m_airspaces(airspaces),
    m_prediction_time_glide(prediction_time_glide),
    m_prediction_time_filter(prediction_time_filter),
    m_perf_glide(glide_polar),
    m_state_filter(state, prediction_time_filter),
    m_perf_filter(m_state_filter)
    {}

  typedef std::list<AirspaceWarning> AirspaceWarningList; /**< Type of warning storage */

/** 
 * Reset warning list and filter (as in new flight)
 * 
 * @param state State to reset filter to
 */
  void reset(const AIRCRAFT_STATE& state);

/** 
 * Perform predictions and interior search to update warning list
 * 
 * @param state Current aircraft state
 * 
 * @return True if warnings changed
 */
  bool update(const AIRCRAFT_STATE &state);

/** 
 * Adjust time of glide predictor
 * 
 * @param the_time New time (s)
 */
  void set_prediction_time_glide(const fixed& the_time);

/** 
 * Adjust time of state predictor.  Also updates filter time constant
 * 
 * @param the_time New time (s)
 */
  void set_prediction_time_filter(const fixed& the_time);

/** 
 * Find corresponding airspace warning item in store for an airspace
 * 
 * @param airspace Airspace to find warning for
 * 
 * @return Reference to airspace warning item
 */
  AirspaceWarning& get_warning(const AbstractAirspace& airspace);

/** 
 * Test whether warning list is empty
 * 
 * @return True if no warnings in list
 */
  bool empty() const {
    return m_warnings.empty();
  }

/** 
 * Return size of warning list
 * 
 * @return Number of items in warning list
 */
  size_t size() const {
    return m_warnings.size();
  }

/** 
 * Visit each warnings in the store
 * 
 * @param visitor Visitor for warnings
 */
  void visit_warnings(AirspaceWarningVisitor& visitor) const;

private:
  const Airspaces& m_airspaces;

  fixed m_prediction_time_glide;
  fixed m_prediction_time_filter;

  AirspaceAircraftPerformanceGlide m_perf_glide;
  AircraftStateFilter m_state_filter;
  AirspaceAircraftPerformanceStateFilter m_perf_filter;  

  AirspaceWarningList m_warnings;

  bool update_task(const AIRCRAFT_STATE& state);
  bool update_filter(const AIRCRAFT_STATE& state);
  bool update_glide(const AIRCRAFT_STATE& state);
  bool update_inside(const AIRCRAFT_STATE& state);

  bool update_predicted(const AIRCRAFT_STATE& state, 
                        const GEOPOINT &location_predicted,
                        const AirspaceAircraftPerformance &perf,
                        const AirspaceWarning::AirspaceWarningState& warning_state,
                        const fixed max_time);

};

#endif
