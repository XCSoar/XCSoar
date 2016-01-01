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

#ifndef XCSOAR_CIRCLING_WIND_HPP
#define XCSOAR_CIRCLING_WIND_HPP

#include "Geo/SpeedVector.hpp"
#include "Util/StaticArray.hxx"
#include "NMEA/Validity.hpp"

struct MoreData;
struct CirclingInfo;

/**
 * Class to provide wind estimates from circling
 */
class CirclingWind
{
  /**
   * The windanalyser analyses the list of flightsamples looking for
   * windspeed and direction.
   */
  struct Sample
  {
    double time;
    SpeedVector vector;
  };

  Validity last_track_available, last_ground_speed_available;

  // we are counting the number of circles, the first onces are probably not very round
  int circle_count;
  // active is set to true or false by the slot_newFlightMode slot
  bool active;

  /**
   * The angle turned in the current circle.
   */
  Angle current_circle;

  Angle last_track;

  StaticArray<Sample, 50> samples;

public:
  struct Result
  {
    unsigned quality;
    SpeedVector wind;

    Result() {}
    Result(unsigned _quality):quality(_quality) {}
    Result(unsigned _quality, SpeedVector _wind)
      :quality(_quality), wind(_wind) {}

    bool IsValid() const {
      return quality > 0;
    }
  };

  /**
   * Clear as if never flown
   */
  void Reset();

  /**
   * Called if a new sample is available in the samplelist.
   */
  Result NewSample(const MoreData &info, const CirclingInfo &circling);

private:
  Result CalcWind();
};

#endif
