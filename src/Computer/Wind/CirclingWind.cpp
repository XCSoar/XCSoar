// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* This library was originally imported from Cumulus
   http://kflog.org/cumulus/ */

#include "CirclingWind.hpp"

#include "LogFile.hpp"
#include "Math/Util.hpp"
#include "NMEA/CirclingInfo.hpp"
#include "NMEA/MoreData.hpp"

#include <algorithm>
#include <cmath>

static constexpr double INITIAL_MIN_FIT_METRIC = 10000;

/*
About Windanalysation

In this implementation, the wind is analyzed by assuming the recorded ground
speeds, minus the true airspeed values, as a cosine curve. The wind speed is
calculated from the amplitude of this curve.

In order to determine the wind direction, the curve is iteratively adapted to
a cosine curve.

The change in the rotation rate is used as a quality parameter for the flown
circles. If the rotation rate changes only slightly over the course of a
circle, the circle in question is used for the wind calculation. As a further
quality parameter, the deviation (sum of the distance squares) is used from the
result of the curve fitting when determining the direction.

The rotation rate is derived from changes in the GPS track, or from a (simple)
IMU, if available. IMU data is more reliable especially at higher wind speeds.

The algorithm uses True Airspeed to compensate for changing airspeed during the
circle. The algorithm also runs without the availability of True Airspeed, but
then without this compensation.

Some of the errors made here will be averaged-out by the WindStore, which keeps
a number of wind measurements and calculates a weighted average based on
quality.

Feb 2026
RN
*/

void CirclingWind::Reset() noexcept {
  active = false;
  usable_airspeed = true;
  usable_gyroscope = false;
  fixed_and_aligned = false;
}

void CirclingWind::ShowResources(const MoreData &info) noexcept {
  const auto airspeed_available =
      info.airspeed_real && info.airspeed_available.IsValid();
  const auto gyroscope_available =
      info.gyroscope.available && info.gyroscope.real;

  if (airspeed_available != _airspeed_available) {
    if (!airspeed_available)
      LogString("CirclingWind: Airspeed not usable!");
    else
      LogString("CirclingWind: Using Airspeed");
  }

  if (gyroscope_available != _gyroscope_available) {
    if (gyroscope_available)
      LogString("CirclingWind: Using Gyro");
    else
      LogString("CirclingWind: Gyro not usable!");
  }

  _airspeed_available = airspeed_available;
  _gyroscope_available = gyroscope_available;
}

CirclingWind::Result CirclingWind::NewSample(const MoreData &info,
                                             const CirclingInfo &circling) noexcept {
  if (!circling.circling) {
    Reset();
    return Result(0);
  }

  if (!active) {
    active = true;

    // latch flags for the duration of current thermal
    usable_airspeed = info.airspeed_real && info.airspeed_available.IsValid();
    usable_gyroscope = info.gyroscope.available && info.gyroscope.real;
    fixed_and_aligned = info.gyroscope.fixed_and_aligned;

    CirclingWind::ShowResources(info);
    current_circle = Angle::Zero();
    last_track = info.track;
    last_track_available.Clear();
    last_ground_speed_available.Clear();
    samples.clear();
    suspend = 0;
  }

  if (last_track_available.FixTimeWarp(info.track_available,
                                       std::chrono::seconds(30)) ||
      last_ground_speed_available.FixTimeWarp(info.ground_speed_available,
                                              std::chrono::seconds(30))) {
    /* time warp: start from scratch */
    Reset();
    return Result(0);
  }

  if (!info.track_available.Modified(last_track_available) ||
      !info.ground_speed_available.Modified(last_ground_speed_available))
    // no new GPS fix
    return Result(0);

  last_track_available = info.track_available;
  last_ground_speed_available = info.ground_speed_available;

  char angular_rate_source = ' '; // remembers where angular rate came from
  Sample sample;
  sample.time = info.time;
  sample.track = info.track;
  sample.ground_speed = info.ground_speed;
  sample.tas = usable_airspeed ? info.true_airspeed : 0;

  /* Calculate the angular rate at which the aircraft is circling.
   * Select the best available source: gyroscope or GPS track.
   * sample.angular_rate in degrees/sec, positive on right turns.
   */
  if (usable_gyroscope) {
    angular_rate_source = 'g';
    /* angular rate from gyroscope is better at higher winds
     * angular rate is a combination of X and Z axis
     * that senses roll and yaw, predominantly.
     */
    if (fixed_and_aligned) {
      sample.angular_rate =
          Angle::Degrees(copysign(hypot(info.gyroscope.angular_rate_X.Native(),
                                        info.gyroscope.angular_rate_Z.Native()),
                                  info.gyroscope.angular_rate_Z.Native()));
    } else {
      // orientation of the instrument is unknown
      sample.angular_rate = Angle::Degrees(
          copysign(SpaceDiagonal(info.gyroscope.angular_rate_X.Native(),
                                 info.gyroscope.angular_rate_Y.Native(),
                                 info.gyroscope.angular_rate_Z.Native()),
                   (info.track - last_track).AsDelta().Native()));
      last_track = info.track;
    }
  } else {
    angular_rate_source = 't';
    /* we use GPS track in the absence of a gyroscope
       wont work so well at high wind speeds (> 30 km/h)
    */
    sample.angular_rate = (info.track - last_track).AsDelta();
    last_track = info.track;
  }

  // insert the newest sample at position 0
  samples.push_front(sample);

  Result result(0);

  // avoid WindCalc for every single sample
  if (suspend > 0) {
    suspend -= 1;
  } else {
    // how many samples for the full circle?
    bool full_circle = false;
    size_t idx;
    current_circle = Angle::Zero();
    Angle angular_rate_sum = Angle::Zero();
    for (idx = 1; (idx < samples.size()) && !full_circle; idx++) {
      current_circle += (samples[idx - 1].track - samples[idx].track).AsDelta();
      angular_rate_sum += samples[idx - 1].angular_rate;
      full_circle = current_circle.Absolute() > Angle::FullCircle();
    }
    const auto n_samples = idx; // in the full circle

    // how big is the change in turn rate?
    if (full_circle && (n_samples > 8)) {
      const Angle angular_rate_avg = angular_rate_sum / (n_samples - 1);

      double max_angular_rate_deviation = 0;
      for (size_t i = 1; i < n_samples; i++) {
        const Angle v = samples[i].angular_rate - angular_rate_avg;
        max_angular_rate_deviation =
            std::max(std::abs(v.Degrees()), max_angular_rate_deviation);
      }

      // this should be small (< 0.3) for a good enough circle
      const double circle_quality_metric =
          std::abs(max_angular_rate_deviation / angular_rate_avg.Degrees());

      if (circle_quality_metric < 1.0) {
        result = CalcWind(circle_quality_metric, n_samples, current_circle,
                          angular_rate_source);

        /* we have a good circle, wait for 1/4 circle before calculating the
           next */
        if (result.quality)
          suspend = n_samples / 4;
        else
          suspend = 0;
      }
    }
  }
  return result;
}

CirclingWind::Result CirclingWind::CalcWind(double quality_metric,
                                            size_t n_samples,
                                            Angle circle,
                                            char angular_rate_source) noexcept {
  assert(n_samples > 1);
  assert(!samples.empty());

  // reject if average time step greater than 2.0 seconds
  const auto measure_time = (samples[0].time - samples[n_samples - 1].time);
  const auto avg_step_width = measure_time / (n_samples - 1);
  if (avg_step_width <= std::chrono::seconds{0})
    return Result(0);
  if (avg_step_width > std::chrono::seconds{2})
    return Result(0);

  // reject if step width isn't sufficiently uniform
  for (size_t i = 1; i < n_samples; i++)
    if (std::chrono::abs(samples[i - 1].time - samples[i].time -
                         avg_step_width) > (avg_step_width * 0.05))
      return Result(0);

  /* the fraction of the last step to be removed from the sum over the full
     circle */
  const auto last_track_change =
      (samples[n_samples - 2].track - samples[n_samples - 1].track).AsDelta();
  if (last_track_change.Absolute() < Angle::Degrees(0.1))
    return Result(0); // will never happen!
  const auto excess_circle = (circle - Angle::FullCircle()).AsDelta();
  const double last_step_fraction_excess =
      1.0 - (last_track_change - excess_circle) / last_track_change;
  double last_val = 0;

  /* find average to determine wind speed
     this tolerates TAS == 0 if airspeed isn't available */
  double speed_offset = 0;
  for (size_t i = 0; i < n_samples; i++) {
    last_val = (samples[i].ground_speed - samples[i].tas);
    speed_offset += last_val;
  }
  speed_offset -= (last_val * last_step_fraction_excess);
  speed_offset /= ((double)n_samples - last_step_fraction_excess); // average

  // estimate wind speed by amplitude of wind speed samples
  double wind_speed = 0;
  for (size_t i = 0; i < n_samples; i++) {
    last_val = std::abs(samples[i].ground_speed - samples[i].tas - speed_offset);
    wind_speed += last_val;
  }
  wind_speed -= (last_val * last_step_fraction_excess);
  wind_speed /= ((double)n_samples - last_step_fraction_excess); // average
  wind_speed *=
      M_PI_2; // the ratio of the average to the amplitude of a sine curve

  if (wind_speed >= 30)
    // limit to reasonable values (30 m/s), reject otherwise
    return Result(0);

  // determine bearing of the wind & prepare for the iterative search
  Angle wind_bearing = Angle::Zero();
  Angle search_midpoint = Angle::HalfCircle();
  int search_steps = 6;
  // cover more then a full circle
  Angle search_step_width = circle.Absolute() / search_steps;
  search_steps += 1;
  Angle probe_point;
  double fit_metric;
  // initialize to a large number
  double min_fit_metric = INITIAL_MIN_FIT_METRIC;

  /* this is a "safety net", absolutely not necessary,
   * since the number of iterations is fix at compile time
   */
  int max_iterations = 10;
  // find best fit to cosine curve
  do {
    // make sure we don't loop forever
    assert(max_iterations > 0);
    max_iterations -= 1;

    probe_point =
        search_midpoint - (search_step_width * ((double)search_steps / 2.0));

    for (int s = 0; s <= search_steps; s++) {
      fit_metric = FitCosine(n_samples, wind_speed, speed_offset, probe_point);
      if (fit_metric < min_fit_metric) {
        min_fit_metric = fit_metric;
        search_midpoint = probe_point;
      }
      probe_point += search_step_width;
    }

    // hone in on the interesting part
    search_steps = 3;
    search_step_width = search_step_width / 2.0;
  } while (search_step_width > Angle::Degrees(2));
  wind_bearing = search_midpoint.AsBearing();

  /* If we knew the latency of the GPS receiver we would correct wind_bearing
   * here! ToDo: determine the latency of the GPS receiver in use 0.25 sec is a
   * guesstimate for a PowerFLARM
   */
  const double latency_GPS =
      0.25; // seconds from antenna signal to result in info
  // Compensate calculated wind bearing for this latency
  wind_bearing -= (circle * latency_GPS / measure_time.count());

  if (min_fit_metric > (INITIAL_MIN_FIT_METRIC - 1)) {
    return Result(0); // it doesn't converge
  }

  SpeedVector wind(wind_bearing.AsBearing(), wind_speed);
  const auto quality = EstimateQuality(quality_metric, min_fit_metric,
                                       wind_speed, angular_rate_source);

  return Result(quality, wind);
}

unsigned int CirclingWind::EstimateQuality(double circle_quality,
                                           double fit_cosine_quality,
                                           double wind_speed,
                                           char angular_rate_source) noexcept {
  /* The return value matches the quality number of WindStore::SlotMeasurement
   * This estimation is heuristic, not scientific.
   */
  /* Perfect circles are skewed, if GPS-track is the criteria, with higher
   * winds, hence allow some margin for this. wind_speed in m/s
   */
  const double roundness_skew =
      wind_speed * ((angular_rate_source == 'g') ? 0.01 : 0.02);

  int quality = 5; // top quality, perfect circle
  if (circle_quality > (0.2 + roundness_skew))
    quality = 4;
  if (circle_quality > (0.3 + roundness_skew))
    quality = 3;
  if (circle_quality > (0.4 + roundness_skew))
    quality = 2;
  if (circle_quality > (0.5 + roundness_skew))
    quality = 1;
  if (circle_quality > (0.7 + roundness_skew))
    return 0;

  // fine tune the quality number to reflect quality of fit to the cosine
  if (fit_cosine_quality > 10)
    quality -= 1; // penalty for bad fit
  if (fit_cosine_quality < 5)
    quality += 1; // bonus for good fit
  if (fit_cosine_quality < 1)
    quality += 1; // extra bonus for very good fit
  quality = std::max(std::min(quality, 5), 0);

  return quality;
}

double CirclingWind::FitCosine(size_t n_samples, double amplitude,
                               double offset, Angle phase) noexcept {
  /* fit the measured speed data to an inverse cosine
   * uses the "sum of squares" algorithm
   */
  double sum_of_squares_of_deltas = 0;

  for (size_t i = 0; i < n_samples; i++) {
    const double a = (samples[i].track - phase).AsBearing().Radians();
    double net_speed_diff = samples[i].ground_speed - samples[i].tas - offset;
    /* to make the fit metric (somewhat) independent of the amplitude
     * wind speeds less than 1 m/s are not interesting anyways
     */
    if (amplitude > 1.0)
      net_speed_diff /= amplitude;
    sum_of_squares_of_deltas += Square(-fastcosine(a) - net_speed_diff);
  }
  /* tighter circles getting a smaller result, that is intended.
   * Because wider circles tend to be search circles, hence not so round
   */
  return sum_of_squares_of_deltas;
}
