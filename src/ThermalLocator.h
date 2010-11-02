/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#define TLOCATOR_NMIN 5
#define TLOCATOR_NMAX 60

#include "Math/fixed.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Navigation/Flat/FlatPoint.hpp"
#include "Navigation/SpeedVector.hpp"
#include "NMEA/Derived.hpp"

class TaskProjection;

/**
 * Class to estimate the location of the center of a thermal
 * when circling.
 */
class ThermalLocator {
private:

  /** 
   * Class used to hold thermal estimate samples
   */
  struct ThermalLocator_Point 
  {
    /**
     * Default constructor
     * 
     */
    ThermalLocator_Point()
    {
      valid = false;
    }

    /** 
     * Calculate drifted, weighted values of point
     * 
     * @param t Current time
     * @param location_0 Initial location
     * @param wind_drift Wind drift offset
     * @param decay decay factor for weighting
     */
    void Drift(fixed t, 
               const TaskProjection& projection,
               const GeoPoint& wind_drift,
               fixed decay);

    bool valid;                 /**< Whether this point is a valid sample */
    
    GeoPoint location;          /**< Actual location of sample */
    FlatPoint loc_drift;        /**< Projected/drifted sample */
    fixed t_0;                  /**< Time of sample (s) */
    fixed w;                    /**< Scaled updraft value of sample */
    fixed weight;               /**< Weighting used for this point */
  };

public:
  /** 
   * Default constructor.  Initialises object
   * 
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
  void Process(const bool circling,
               const fixed time, 
               const GeoPoint &location, 
               const fixed w,
               const SpeedVector wind,
               THERMAL_LOCATOR_INFO& therm);

private:

  void Reset();

  void invalid_estimate(THERMAL_LOCATOR_INFO &therm) const;

  void glider_average(fixed &xav, fixed& yav);

  fixed est_t;
  GeoPoint est_location;

  void AddPoint(const fixed t, const GeoPoint &location, const fixed w);
  void Update(const fixed t_0,
              const GeoPoint &location_0,
              const SpeedVector wind,
              THERMAL_LOCATOR_INFO &therm);

  void Update_Internal(const fixed t_0,
                       const TaskProjection& projection,
                       const GeoPoint& location_0, 
                       const GeoPoint& traildrift,
                       const fixed decay,
                       THERMAL_LOCATOR_INFO& therm);

  void Drift(const fixed t_0,
             const TaskProjection& projection, 
             const GeoPoint& traildrift,
             const fixed decay);

  ThermalLocator_Point points[TLOCATOR_NMAX]; /**< Circular buffer of points */

  bool initialised;
  unsigned n_index; /**< Index of next point to add */
  unsigned n_points; /**< Number of points in buffer */
};

#endif
