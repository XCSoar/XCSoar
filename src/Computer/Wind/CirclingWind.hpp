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
class CirclingWind {
  /**
   * The windanalyser analyses the list of flightsamples looking for
   * windspeed and direction.
   */
  struct Sample {
    TimeStamp time;

    Angle track;        // in degrees, from the GPRMC sentence
    Angle angular_rate; // in degrees/sample, from GPRMC or gyroscope
                        // positive means right turn
    float ground_speed; // in m/s, from the GPRMC sentence
    float tas; // true airspeed in m/s; impact on accuracy if unavailable
  };

  Validity last_track_available, last_ground_speed_available;
  bool usable_airspeed = true;
  bool usable_gyroscope = false;
  bool fixed_and_aligned = false;

  // active is set to true or false by the slot_newFlightMode slot
  bool active;
  // after a successful wind calculation suspend for a number of samples
  int suspend;

  /**
   * The angle turned in the current circle.
   */
  Angle current_circle;

  Angle last_track = Angle::Zero();
  Angle last_wind_dir = Angle::Zero();

  boost::circular_buffer<Sample> samples{80};

public:
  struct Result {
    unsigned quality;
    SpeedVector wind;

    Result() {}
    explicit Result(unsigned _quality) : quality(_quality) {}
    Result(unsigned _quality, SpeedVector _wind)
        : quality(_quality), wind(_wind) {}

    bool IsValid() const { return quality > 0; }
  };

  /**
   * Clear as if never flown
   */
  void Reset() noexcept;

  /**
   * Called if a new sample is available in the samplelist.
   */
  [[nodiscard]]
  Result NewSample(const MoreData &info, const CirclingInfo &circling) noexcept;

private:
  // remembers the status when circling started
  bool _airspeed_available = false;
  bool _gyroscope_available = false;

  [[nodiscard]]
  unsigned int EstimateQuality(double circle_quality,
                               double fit_cosine_quality,
                               double wind_speed,
                               char angular_rate_source) noexcept;

  [[nodiscard]]
  Result CalcWind(double quality_metric, size_t n_samples,
                  Angle circle, char angular_rate_source) noexcept;

  [[gnu::pure]]
  double FitCosine(size_t n_samples, double amplitude,
                   double offset, Angle phase) noexcept;

  void ShowResources(const MoreData &info) noexcept;
};
