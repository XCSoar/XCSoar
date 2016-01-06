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

#ifndef AIRCRAFT_STATE_FILTER_HPP
#define AIRCRAFT_STATE_FILTER_HPP

#include "Math/Filter.hpp"
#include "Math/DiffFilter.hpp"
#include "Navigation/Aircraft.hpp"

/**
 * Class for filtering aircraft state (location and altitude) 
 * in order to derive average speed, bearing and climb rate
 */
class AircraftStateFilter {
  DiffFilter x_diff_filter, y_diff_filter, alt_diff_filter;
  Filter x_low_pass, y_low_pass, alt_low_pass;
  AircraftState last_state;
  double x, y;
  double v_x, v_y, v_alt;

public:
  /**
   * Non-initialising default constructor.  To initialise this
   * instance, call Design() and Reset().
   */
  AircraftStateFilter() = default;

  /** 
   * Constructor
   * 
   * @param cutoff_wavelength -3db cutoff wavelength (s) of filters
   */
  AircraftStateFilter(const double cutoff_wavelength);

  /**
   * Reset filters to initial state
   *
   * @param state State to reset to
   */
  void Reset(const AircraftState &state);

  /**
   * Update the filters.  Expects time to have advanced;
   * if it has retreated, will reset the filter to the new state.
   *
   * @param state New state
   */
  void Update(const AircraftState &state);

  /**
   * Re-design filter.  Used to adjust the time constant of
   * the low pass filter.  If this fails, the low pass filter will
   * pass all frequencies through.
   *
   * @param cutoff_wavelength -3db filter wavelength (s)
   *
   * @return True if design was successfull
   */
  bool Design(const double cutoff_wavelength);

  /**
   * Return filtered speed
   *
   * @return Speed (m/s)
   */
  double GetSpeed() const;

  /**
   * Return filtered track bearing
   *
   * @return Track bearing (deg true north)
   */
  Angle GetBearing() const;

  /**
   * Return filtered climb rate
   *
   * @return Climb rate (m/s)
   */
  inline double GetClimbRate() const {
    return v_alt;
  }

  /**
   * Calculate predicted state in future
   *
   * @param in_time Time step for extrapolation (s)
   * @return Predicted aircraft state in in_time seconds
   */
  AircraftState GetPredictedState(double in_time) const;
};

#endif
