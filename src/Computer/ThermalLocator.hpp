/*
Copyright_License {

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

#ifndef THERMALLOCATOR_H
#define THERMALLOCATOR_H

#include "Geo/GeoPoint.hpp"
#include "Geo/Flat/FlatPoint.hpp"

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
  static constexpr unsigned TLOCATOR_NMAX = 60;

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
    void Drift(double t, const FlatProjection &projection,
               const GeoPoint& wind_drift);

    /** Actual location of sample */
    GeoPoint location;
    /** Projected/drifted sample */
    FlatPoint loc_drift;
    /** Time of sample (s) */
    double t_0;
    /** Scaled updraft value of sample */
    double w;
    /** Lift weighting used for this point */
    double lift_weight;
    /** Recency weighting used for this point */
    double recency_weight;
  };

  /** Circular buffer of points */
  Point points[TLOCATOR_NMAX];

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
  void Process(bool circling, double time, const GeoPoint &location,
               double w, SpeedVector wind,
               ThermalLocatorInfo& therm);

  /**
   * Reset as if never flown
   */
  void Reset();

private:
  FlatPoint glider_average();

  void AddPoint(double t, const GeoPoint &location, double w);
  void Update(double t_0, const GeoPoint &location_0,
              SpeedVector wind, ThermalLocatorInfo &therm);

  void Drift(double t_0, const FlatProjection &projection,
             const GeoPoint &traildrift);
};

#endif
