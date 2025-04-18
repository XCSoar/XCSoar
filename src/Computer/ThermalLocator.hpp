// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ThermalRecency.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/Flat/FlatPoint.hpp"
#include "time/Stamp.hpp"

#include <array>

struct SpeedVector;
class FlatProjection;
struct ThermalLocatorInfo;

/**
 * Class to estimate the location of the center of a thermal
 * when circling.
 */
class ThermalLocator {
public:
  static constexpr unsigned TLOCATOR_NMIN = 5;
  static constexpr unsigned TLOCATOR_NMAX = THERMALRECENCY_SIZE;

private:
  /** Class used to hold thermal estimate samples */
  struct Point 
  {
    /** 
     * Calculate drifted, weighted values of point
     * 
     * @param t Current time
     * @param location_0 Initial location
     * @param wind_drift Wind drift offset
     * @param decay decay factor for weighting
     */
    void Drift(TimeStamp t, const FlatProjection &projection,
               const GeoPoint& wind_drift);

    /** Actual location of sample */
    GeoPoint location;
    /** Projected/drifted sample */
    FlatPoint loc_drift;
    /** Time of sample (s) */
    TimeStamp t_0;
    /** Scaled updraft value of sample */
    double w;
    /** Lift weighting used for this point */
    double lift_weight;
    /** Recency weighting used for this point */
    double recency_weight;
  };

  /** Circular buffer of points */
  std::array<Point, TLOCATOR_NMAX> points;

  /** Index of next point to add */
  unsigned n_index;
  /** Number of points in buffer */
  unsigned n_points;

public:
  /** 
   * Update locator estimate.  If not in circling mode, resets the 
   * object.
   * 
   * @param circling Whether aircraft is in circling mode
   * @param time Time of fix (s)
   * @param location Location of aircraft
   * @param w Net updraft speed (m/s)
   * @param wind Wind vector
   * @param therm Output thermal estimate data
   */
  void Process(bool circling, TimeStamp time, const GeoPoint &location,
               double w, SpeedVector wind,
               ThermalLocatorInfo& therm);

  /**
   * Reset as if never flown
   */
  void Reset();

private:
  FlatPoint glider_average();

  void AddPoint(TimeStamp t, const GeoPoint &location, double w) noexcept;
  void Update(TimeStamp t_0, const GeoPoint &location_0,
              SpeedVector wind, ThermalLocatorInfo &therm);

  void Drift(TimeStamp t_0, const FlatProjection &projection,
             const GeoPoint &traildrift);
};
