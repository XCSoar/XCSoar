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
#ifndef AIRSPACE_WARNING_MANAGER_HPP
#define AIRSPACE_WARNING_MANAGER_HPP

#include "Util/NonCopyable.hpp"
#include "AirspaceWarning.hpp"
#include "AirspaceWarningConfig.hpp"
#include "AirspaceAircraftPerformance.hpp"
#include "Compiler.h"

#include <list>

class TaskManager;
class Airspaces;
class AirspaceWarningVisitor;

/**
 * Class to detect and track airspace warnings
 *
 * Several types of airspace checks are performed:
 * - Interior (whether aircraft is inside the airspace)
 * - Glide polar (short range predicted warning based on MacCready performance)
 * - Cruise Filter (medium range predicted warning based on low pass filtered state)
 * - Climb Filter (longer range predicted warning based on low pass filtered state)
 * - Task (longer range predicted warning based on current leg of task)
 *
 */
class AirspaceWarningManager: 
  public NonCopyable
{
  AirspaceWarningConfig config;

  const Airspaces& m_airspaces;

  fixed m_prediction_time_glide;
  fixed m_prediction_time_filter;

  AirspaceAircraftPerformanceGlide m_perf_glide;
  AircraftStateFilter m_cruise_filter;
  AircraftStateFilter m_circling_filter;
  AirspaceAircraftPerformanceStateFilter m_perf_cruise;  
  AirspaceAircraftPerformanceStateFilter m_perf_circling;  

  const TaskManager& m_task;

  const GlidePolar& m_glide_polar;

  typedef std::list<AirspaceWarning> AirspaceWarningList;

  AirspaceWarningList m_warnings;

public:
  /** 
   * Default constructor
   * 
   * @param airspaces Store of airspaces
   * @param task_manager Task manager holding task
   * @param prediction_time_glide Time (s) of glide predictor (default 15 s)
   * @param prediction_time_filter Time (s) of state filter predictor (default 60 s)
   *
   * @return Initialised object
   */
  AirspaceWarningManager(const Airspaces& airspaces,
                         const TaskManager &task_manager,
                         const fixed& prediction_time_glide=fixed(15.0),
                         const fixed& prediction_time_filter=fixed(60.0));

  const AirspaceWarningConfig &get_config() const {
    return config;
  }

  void set_config(const AirspaceWarningConfig &_config);

/** 
 * Reset warning list and filter (as in new flight)
 * 
 * @param state State to reset filter to
 */
  void reset(const AircraftState& state);

/** 
 * Perform predictions and interior search to update warning list If
 * not in circling mode, the prediction based on the state filter
 * predictor not used, since a long term prediction is not valid
 * if in cruise mode.
 * 
 * @param state Current aircraft state
 * @param circling Whether aircraft is circling
 * @param dt Time step since last update
 * 
 * @return True if warnings changed
 */
  bool update(const AircraftState &state, const bool circling, const unsigned dt);

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
 * Find corresponding airspace warning item in store by index 
 * 
 * @param index Sequence index to find warning for
 * 
 * @return Pointer to airspace warning item (or NULL if not found)
 */
  gcc_pure
  const AirspaceWarning* get_warning(const unsigned index) const;

/** 
 * Find corresponding airspace warning item in store by airspace 
 * 
 * @param airspace Airspace to find warning for
 * 
 * @return Pointer to airspace warning item (or NULL if not found)
 */
  AirspaceWarning* get_warning_ptr(const AbstractAirspace& airspace);

/** 
 * Find index of airspace warning item in store for an airspace
 * 
 * @param airspace Airspace to find warning for
 * 
 * @return Index of warning item in list or -1 if not found
 */
  gcc_pure
  int get_warning_index(const AbstractAirspace& airspace) const;

/** 
 * Test whether warning list is empty
 * 
 * @return True if no warnings in list
 */
  gcc_pure
  bool empty() const {
    return m_warnings.empty();
  }

/** 
 * Clear all warnings
 */
  void clear() {
    m_warnings.clear();
  }

/**
 * Acknowledge all active warnings
 */
  void acknowledge_all();

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

/** 
 * Acknowledge an airspace warning
 * 
 * @param airspace The airspace subject
 * @param set Whether to set or cancel acknowledgement
 */
  void acknowledge_warning(const AbstractAirspace& airspace,
                           const bool set=true);

/** 
 * Acknowledge an airspace inside
 * 
 * @param airspace The airspace subject
 * @param set Whether to set or cancel acknowledgement
 */
  void acknowledge_inside(const AbstractAirspace& airspace,
                          const bool set=true);

/** 
 * Acknowledge all warnings for airspace for whole day
 * 
 * @param airspace The airspace subject
 * @param set Whether to set or cancel acknowledgement
 */
  void acknowledge_day(const AbstractAirspace& airspace,
                       const bool set=true);

  /**
   * Returns whether the given airspace is acknowledged for the whole day
   *
   * @param airspace The airspace subject
   */
  bool get_ack_day(const AbstractAirspace& airspace);

private:
  bool update_task(const AircraftState& state);
  bool update_filter(const AircraftState& state, const bool circling);
  bool update_glide(const AircraftState& state);
  bool update_inside(const AircraftState& state);

  bool update_predicted(const AircraftState& state, 
                        const GeoPoint &location_predicted,
                        const AirspaceAircraftPerformance &perf,
                        const AirspaceWarning::AirspaceWarningState& warning_state,
                        const fixed max_time);
};

#endif
