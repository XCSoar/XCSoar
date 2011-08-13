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

/* This library was originally imported from Cumulus
   http://kflog.org/cumulus/ */

#include "Wind/WindMeasurementList.hpp"
#include "Math/FastMath.h"

#include <stdlib.h>
#include <algorithm>

using std::min;

/**
 * Returns the weighted mean windvector over the stored values, or 0
 * if no valid vector could be calculated (for instance: too little or
 * too low quality data).
 */
const Vector
WindMeasurementList::getWind(fixed Time, fixed alt, bool &found) const
{
  //relative weight for each factor
  #define REL_FACTOR_QUALITY 100
  #define REL_FACTOR_ALTITUDE 100
  #define REL_FACTOR_TIME 200
  #define TIME_RANGE 36 // one hour

  int altRange = 1000; //conf->getWindAltitudeRange();
  int timeRange = TIME_RANGE * 100; //conf->getWindTimeRange();

  fixed k(0.0025);

  unsigned int total_quality = 0;

  Vector result(fixed_zero, fixed_zero);
  int now = (int)(Time);

  found = false;

  fixed override_time(1.1);
  bool overridden = false;

  for (unsigned i = 0; i < measurements.size(); i++) {
    const WindMeasurement &m = measurements[i];
    fixed altdiff = (alt - m.altitude) / altRange;
    fixed timediff = fabs(fixed(now - m.time) / timeRange);

    if ((fabs(altdiff) < fixed_one) && (timediff < fixed_one)) {
      // measurement quality
      unsigned int q_quality = min(5,m.quality) * REL_FACTOR_QUALITY / 5;

      // factor in altitude difference between current altitude and
      // measurement.  Maximum alt difference is 1000 m.
      unsigned int a_quality =
          iround(((fixed_two / (altdiff * altdiff + fixed_one)) - fixed_one)
          * REL_FACTOR_ALTITUDE);

      // factor in timedifference. Maximum difference is 1 hours.
      unsigned int t_quality =
          iround(k * (fixed_one - timediff) / (timediff * timediff + k)
          * REL_FACTOR_TIME);

      if (m.quality == 6) {
        if (timediff < override_time) {
          // over-ride happened, so re-set accumulator
          override_time = timediff;
          total_quality = 0;
          result.x = fixed_zero;
          result.y = fixed_zero;
          overridden = true;
        } else {
          // this isn't the latest over-ride or obtained fix, so ignore
          continue;
        }
      } else {
        if (timediff < override_time) {
          // a more recent fix was obtained than the over-ride, so start using
          // that one
          override_time = timediff;
          if (overridden) {
            // re-set accumulators
            overridden = false;
            total_quality = 0;
            result.x = fixed_zero;
            result.y = fixed_zero;
          }
        }
      }

      unsigned int quality = q_quality * (a_quality * t_quality);
      result.x += m.vector.x * quality;
      result.y += m.vector.y * quality;
      total_quality += quality;
    }
  }

  if (total_quality > 0) {
    found = true;
    result = Vector(result.x / (int)total_quality,
                    result.y / (int)total_quality);
  }

  return result;
}

/**
 * Adds the windvector vector with quality quality to the list.
 */
void
WindMeasurementList::addMeasurement(fixed Time, Vector vector, fixed alt,
    int quality)
{
  WindMeasurement &wind =
      measurements.full() ? measurements[getLeastImportantItem(Time)] :
                            measurements.append();

  wind.vector = vector;
  wind.quality = quality;
  wind.altitude = alt;
  wind.time = (long)Time;
}

/**
 * getLeastImportantItem is called to identify the item that should be
 * removed if the list is too full. Reimplemented from LimitedList.
 */
unsigned
WindMeasurementList::getLeastImportantItem(fixed Time)
{
  int maxscore = 0;
  unsigned int founditem = measurements.size() - 1;

  for (int i = founditem; i >= 0; i--) {
    int score = measurements[i].Score((long)Time);
    if (score > maxscore) {
      maxscore = score;
      founditem = i;
    }
  }

  return founditem;
}

void
WindMeasurementList::Reset()
{
  measurements.clear();
}
