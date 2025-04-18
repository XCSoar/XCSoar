// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/SpeedVector.hpp"
#include "util/StaticArray.hxx"
#include "NMEA/Validity.hpp"
#include "time/Stamp.hpp"

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
    TimeStamp time;
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
