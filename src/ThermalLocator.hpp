/*
Copyright_License {

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

#ifndef THERMALLOCATOR_H
#define THERMALLOCATOR_H

#include "Math/fixed.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Navigation/Flat/FlatPoint.hpp"
#include "Navigation/SpeedVector.hpp"

class TaskProjection;

#define TLOCATOR_NMIN 5
#define TLOCATOR_NMAX 60

struct ThermalLocatorInfo;

/**
 * Class to estimate the location of the center of a thermal
 * when circling.
 */
class ThermalLocator {
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
    void Drift(fixed t, const TaskProjection& projection,
               const GeoPoint& wind_drift);

    /** Actual location of sample */
    GeoPoint location;
    /** Projected/drifted sample */
    FlatPoint loc_drift;
    /** Time of sample (s) */
    fixed t_0;
    /** Scaled updraft value of sample */
    fixed w;
    /** Lift weighting used for this point */
    fixed lift_weight;
    /** Recency weighting used for this point */
    fixed recency_weight;
  };

public:
  /** 
   * Default constructor.  Initialises object
   */
  ThermalLocator();

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
  void Process(const bool circling, const fixed time, const GeoPoint &location,
               const fixed w, const SpeedVector wind,
               ThermalLocatorInfo& therm);

  /**
   * Reset as if never flown
   */
  void Reset();

private:
  FlatPoint glider_average();

  void AddPoint(const fixed t, const GeoPoint &location, const fixed w);
  void Update(const fixed t_0, const GeoPoint &location_0,
              const SpeedVector wind, ThermalLocatorInfo &therm);

  void Drift(const fixed t_0, const TaskProjection& projection,
             const GeoPoint& traildrift);

  /** Circular buffer of points */
  Point points[TLOCATOR_NMAX];

  /** Index of next point to add */
  unsigned n_index;
  /** Number of points in buffer */
  unsigned n_points;
};

#endif
