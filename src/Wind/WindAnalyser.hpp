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

#ifndef WINDANALYSER_H
#define WINDANALYSER_H

#include "Vector.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Util/StaticArray.hpp"

struct MoreData;
struct DerivedInfo;

/**
 * Class to provide wind estimates from circling
 */
class WindAnalyser
{
  /**
   * The windanalyser analyses the list of flightsamples looking for
   * windspeed and direction.
   */
  struct Sample {
    Vector v;
    fixed t;
    fixed mag;
  };

  //we are counting the number of circles, the first onces are probably not very round
  int circleCount;
  //true=left, false=right
  bool circleLeft;
  //active is set to true or false by the slot_newFlightMode slot
  bool active;
  int circleDeg;
  Angle last_track;
  bool pastHalfway;
  Vector minVector;
  Vector maxVector;
  bool curModeOK;
  bool first;
  int startcircle;

  GeoPoint climbstartpos;
  GeoPoint climbendpos;
  fixed climbstarttime;
  fixed climbendtime;

  StaticArray<Sample, 50> windsamples;

public:
  struct Result {
    unsigned quality;
    Vector wind;

    Result() {}
    Result(int _quality):quality(_quality) {}
    Result(int _quality, Vector _wind)
      :quality(_quality), wind(_wind) {}

    bool IsValid() const {
      return quality > 0;
    }
  };

  /**
   * Clear as if never flown
   */
  void reset();

  // Signals
  /**
   * Send if a new windmeasurement has been made. The result is included in wind,
   * the quality of the measurement (1-5; 1 is bad, 5 is excellent) in quality.
   */
  void newMeasurement(Vector wind, int quality);

  // Public slots
  /**
   * Called if the flightmode changes
   */
  void slot_newFlightMode(const DerivedInfo &derived,
                          bool left, int marker);

  /**
   * Called if a new sample is available in the samplelist.
   */
  Result NewSample(const MoreData &info, DerivedInfo &derived);

private:
  Result _calcWind(const MoreData &info, DerivedInfo &derived);
};

#endif
