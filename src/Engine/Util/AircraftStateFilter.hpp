// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  AircraftStateFilter() noexcept = default;

  /** 
   * Constructor
   * 
   * @param cutoff_wavelength -3db cutoff wavelength (s) of filters
   */
  AircraftStateFilter(const double cutoff_wavelength) noexcept;

  /**
   * Reset filters to initial state
   *
   * @param state State to reset to
   */
  void Reset(const AircraftState &state) noexcept;

  /**
   * Update the filters.  Expects time to have advanced;
   * if it has retreated, will reset the filter to the new state.
   *
   * @param state New state
   */
  void Update(const AircraftState &state) noexcept;

  /**
   * Re-design filter.  Used to adjust the time constant of
   * the low pass filter.  If this fails, the low pass filter will
   * pass all frequencies through.
   *
   * @param cutoff_wavelength -3db filter wavelength (s)
   *
   * @return True if design was successfull
   */
  bool Design(FloatDuration cutoff_wavelength) noexcept;

  /**
   * Return filtered speed
   *
   * @return Speed (m/s)
   */
  [[gnu::pure]]
  double GetSpeed() const noexcept;

  /**
   * Return filtered track bearing
   *
   * @return Track bearing (deg true north)
   */
  [[gnu::pure]]
  Angle GetBearing() const noexcept;

  /**
   * Return filtered climb rate
   *
   * @return Climb rate (m/s)
   */
  [[gnu::pure]]
  double GetClimbRate() const noexcept {
    return v_alt;
  }

  /**
   * Calculate predicted state in future
   *
   * @param in_time Time step for extrapolation (s)
   * @return Predicted aircraft state in in_time seconds
   */
  [[gnu::pure]]
  AircraftState GetPredictedState(FloatDuration in_time) const noexcept;
};
