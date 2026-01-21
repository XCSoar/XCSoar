// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/SpeedVector.hpp"
#include "NMEA/Validity.hpp"
#include "time/Stamp.hpp"

#include <boost/circular_buffer.hpp>

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

    Angle track;        // in degrees, from the GPRMC sentence
    Angle angular_rate; // in degrees/sample, from GPRMC or IMU
                        /* positive means right turn
                         * TO DO: as soon as gyros become available use tehir
                         * turn rate info
                         */
    float ground_speed; // in m/s, from the GPRMC sentence
    float tas; // true airspeed in m/s; imact on accuracy if unavailable
  };

  Validity last_track_available, last_ground_speed_available;
  bool usable_tas = true;

  // active is set to true or false by the slot_newFlightMode slot
  bool active;
  // after a successful wind calculation suspend for a number of samples
  int suspend;

  /**
   * The angle turned in the current circle.
   */
  Angle current_circle;

  Angle last_track    = Angle::Zero();
  Angle last_wind_dir = Angle::Zero();

  boost::circular_buffer<Sample> samples{80};

public:
  struct Result
  {
    unsigned quality;
    SpeedVector wind;

    Result() {}
    explicit Result(unsigned _quality) : quality(_quality) {}
    Result(unsigned _quality, SpeedVector _wind)
        : quality(_quality), wind(_wind)
    {
    }

    bool IsValid() const
    {
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
  // remebers the status when circling started
  bool _airspeed_available = false;

  unsigned int EstimateQuality(const double circle_quality,
                               const double fitCosine_quality,
                               const double wind_speed,
                               const char ar_source);
  Result CalcWind(const double quality_metric,
                  const size_t n_samples,
                  const Angle circle,
                  const char ar_source);
  double FitCosine(const size_t n_samples,
                   const double amplitude,
                   const double offset,
                   const Angle phase);
  void ShowResources(const MoreData &info);
};
