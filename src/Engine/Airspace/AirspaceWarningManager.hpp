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
#ifndef AIRSPACE_WARNING_MANAGER_HPP
#define AIRSPACE_WARNING_MANAGER_HPP

#include "AirspaceWarning.hpp"
#include "AirspaceWarningConfig.hpp"
#include "Util/AircraftStateFilter.hpp"
#include "Compiler.h"

#include <list>

class TaskStats;
class GlidePolar;
class Airspaces;
class FlatProjection;
class AirspaceAircraftPerformance;

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
class AirspaceWarningManager {
  AirspaceWarningConfig config;

  const Airspaces &airspaces;

  double prediction_time_glide;
  double prediction_time_filter;

  AircraftStateFilter cruise_filter;
  AircraftStateFilter circling_filter;

  typedef std::list<AirspaceWarning> AirspaceWarningList;

  AirspaceWarningList warnings;

  /**
   * This number is incremented each time this object is modified.
   */
  unsigned serial;

public:
  typedef AirspaceWarningList::const_iterator const_iterator;

  /** 
   * Default constructor
   * 
   * @param airspaces Store of airspaces
   *
   * @return Initialised object
   */
  AirspaceWarningManager(const AirspaceWarningConfig &_config,
                         const Airspaces &_airspaces);

  AirspaceWarningManager(const AirspaceWarningManager &) = delete;

  gcc_pure
  const FlatProjection &GetProjection() const;

  const AirspaceWarningConfig &GetConfig() const {
    return config;
  }

  void SetConfig(const AirspaceWarningConfig &_config);

  /**
   * Returns a serial for the current state.  The serial gets
   * incremented each time the list of warnings is modified.
   */
  unsigned GetSerial() const {
    return serial;
  }

  /**
   * Reset warning list and filter (as in new flight)
   *
   * @param state State to reset filter to
   */
  void Reset(const AircraftState& state);

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
  bool Update(const AircraftState &state, const GlidePolar &glide_polar,
              const TaskStats &task_stats,
              const bool circling, const unsigned dt);

  /**
   * Adjust time of glide predictor
   *
   * @param the_time New time (s)
   */
  void SetPredictionTimeGlide(double time);

  /**
   * Adjust time of state predictor.  Also updates filter time constant
   *
   * @param the_time New time (s)
   */
  void SetPredictionTimeFilter(double time);

  /**
   * Find corresponding airspace warning item in store for an airspace
   *
   * @param airspace Airspace to find warning for
   *
   * @return Reference to airspace warning item
   */
  AirspaceWarning& GetWarning(const AbstractAirspace& airspace);

  /**
   * Find corresponding airspace warning item in store by airspace
   *
   * @param airspace Airspace to find warning for
   *
   * @return Pointer to airspace warning item (or nullptr if not found)
   */
  AirspaceWarning* GetWarningPtr(const AbstractAirspace& airspace);

  /**
   * Return new corresponding airspace warning item in store by airspace
   *
   * @param airspace Airspace for which to create warning for
   *
   * @return Pointer to airspace warning item (or nullptr if not found)
   */
  AirspaceWarning* GetNewWarningPtr(const AbstractAirspace& airspace);

  const AirspaceWarning *GetWarningPtr(const AbstractAirspace &airspace) const {
    return const_cast<AirspaceWarningManager *>(this)->GetWarningPtr(airspace);
  }

  /**
   * Test whether warning list is empty
   *
   * @return True if no warnings in list
   */
  gcc_pure
  bool empty() const {
    return warnings.empty();
  }

  /**
   * Clear all warnings
   */
  void clear() {
    ++serial;
    warnings.clear();
  }

  /**
   * Acknowledge all active warnings
   */
  void AcknowledgeAll();

  /**
   * Return size of warning list
   *
   * @return Number of items in warning list
   */
  AirspaceWarningList::size_type size() const {
    return warnings.size();
  }

  gcc_pure
  const_iterator begin() const {
    return warnings.begin();
  }

  gcc_pure
  const_iterator end() const {
    return warnings.end();
  }

  /**
   * Acknowledge an airspace warning or airspace inside (depending on
   * the state).
   */
  void Acknowledge(const AbstractAirspace &airspace);

  /**
   * Acknowledge an airspace warning
   *
   * @param airspace The airspace subject
   * @param set Whether to set or cancel acknowledgement
   */
  void AcknowledgeWarning(const AbstractAirspace& airspace,
                          const bool set = true);

  /**
   * Acknowledge an airspace inside
   *
   * @param airspace The airspace subject
   * @param set Whether to set or cancel acknowledgement
   */
  void AcknowledgeInside(const AbstractAirspace& airspace,
                         const bool set = true);

  /**
   * Acknowledge all warnings for airspace for whole day
   *
   * @param airspace The airspace subject
   * @param set Whether to set or cancel acknowledgement
   */
  void AcknowledgeDay(const AbstractAirspace& airspace,
                      const bool set = true);

  /**
   * Returns whether the given airspace is acknowledged for the whole day
   *
   * @param airspace The airspace subject
   */
  gcc_pure
  bool GetAckDay(const AbstractAirspace& airspace) const;

  /**
   * Returns true if this airspace would be warned about,
   * i.e. trespassing it would not be possible.
   *
   * This returns false for airspaces that have been "acknowledged for
   * day" (see GetAckDay()) or airspaces that are inactive or
   * airspaces that are not configured for airspace warnings.
   */
  gcc_pure
  bool IsActive(const AbstractAirspace &airspace) const;

private:
  bool UpdateTask(const AircraftState &state, const GlidePolar &glide_polar,
                  const TaskStats &task_stats);
  bool UpdateFilter(const AircraftState& state, const bool circling);
  bool UpdateGlide(const AircraftState& state, const GlidePolar &glide_polar);
  bool UpdateInside(const AircraftState& state, const GlidePolar &glide_polar);

  bool UpdatePredicted(const AircraftState& state, 
                       const GeoPoint &location_predicted,
                       const AirspaceAircraftPerformance &perf,
                       const AirspaceWarning::State warning_state,
                       double max_time);
};

#endif
