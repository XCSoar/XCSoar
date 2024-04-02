// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* This library was originally imported from Cumulus
   http://kflog.org/cumulus/ */

#define INITIAL_min_fit_metric        10000

#include "CirclingWind2.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/CirclingInfo.hpp"
#include "Math/Util.hpp"
#include "LogFile.hpp"

#include <algorithm>

/*
About Windanalysation

In this implementation, the wind is analyzed by assuming the recorded ground
speeds, minus the true airspeed values, as a cosine curve. The wind speed is
calculated from the amplitude of this curve.

In order to determine the wind direction, the curve is iteratively adapted to
a cosine curve.

The change in the rotation rate is used as a quality parameter for the flown
circles. If the rotation rate changes only slightly over the course of a circle,
the circle in question is used for the wind calculation. As a further quality
parameter, the deviation (sum of the distance squares) is used from the result
of the curve fitting when determining the direction.

The algorithm uses True Airspeed to compensate for changing airspeed during the
circle. The algorithm also runs without the availability of True Airspeed, but
then without this compensation.

In a future implementation, the change in rotation rate can be derived from
(simple) IMUs. However, this presupposes that IMUs are given an infrastructure
in XCSoar.

Some of the errors made here will be averaged-out by the WindStore, which keeps
a number of windmeasurements and calculates a weighted average based on quality.
*/

void
CirclingWind2::Reset()
{
  active = false;
  useable_tas = true;
}

CirclingWind2::Result
CirclingWind2::NewSample(const MoreData &info, const CirclingInfo &circling)
{
  if (!circling.circling) {
    Reset();
    return Result(0);
  }

  if (!active) {
    active = true;

    const auto real_tas = info.airspeed_real && info.airspeed_available.IsValid();
    if (useable_tas && !real_tas) LogString("CirclingWind2: TAS not usable!\n");
    useable_tas = real_tas;

    current_circle = Angle::Zero();
    last_track_available.Clear();
    last_ground_speed_available.Clear();
    samples.clear();
    suspend = 0;
  }

  if (last_track_available.FixTimeWarp(info.track_available,
                                       std::chrono::seconds(30)) ||
      last_ground_speed_available.FixTimeWarp(info.ground_speed_available,
                                              std::chrono::seconds(30)))
    /* time warp: start from scratch */
    Reset();

  if (!info.track_available.Modified(last_track_available) ||
      !info.ground_speed_available.Modified(last_ground_speed_available))
    /* no updated GPS fix */
    return Result(0);

  last_track_available = info.track_available;
  last_ground_speed_available = info.ground_speed_available;

  Sample sample;
  sample.time = info.clock;
  sample.track = info.track;
  sample.ground_speed = info.ground_speed;
  sample.tas = useable_tas ? info.true_airspeed : 0;
  // To DO: use turn rate info if and when IMUs are supportet in XCSoar
  // sample.turn_rate = circling.turn_rate_heading; // from gyro
  // insert the newest sample at position 0
  samples.push_front(sample);


  Result result(0);

  if (suspend-- <= 0) { // avoid WindCalc for every single sample.

    // how many samples for the full circly?
    bool fullCircle = false;
    size_t idx;
    current_circle = Angle::Zero();
    for (idx=1; (idx < samples.size()) && !fullCircle; idx++) {
      current_circle += (samples[idx-1].track - samples[idx].track).AsDelta();
      fullCircle = current_circle.Absolute() > Angle::FullCircle();
    }
    const auto n_samples = idx; // in the full circle

    // how big is the change in turn rate?
    Angle turnrate_avg = Angle::Zero();
    if (fullCircle && (n_samples > 8)) {
      turnrate_avg = current_circle / n_samples;

      double max_turnrate_deviation = 0;
      for (size_t i=1; i < n_samples;i++) {
        const Angle v = (samples[i-1].track - samples[i].track).AsDelta() - turnrate_avg;
        // v = samples[i].turn_rate - turnrate_avg; // TO DO: use this if IMU is available
        max_turnrate_deviation = std::max(v.AbsoluteDegrees(),max_turnrate_deviation);
      }

      // this should be small (< 0.3) for a good enough circle
      const double circle_quality_metric = max_turnrate_deviation / turnrate_avg.Degrees();

      if (circle_quality_metric < 0.6) {
        result = CalcWind(circle_quality_metric,n_samples,current_circle);

        // now that we've seen a good circle, wait for 1/4 circle before calculating the next
        suspend = n_samples / 4;
      }
    }
  }
  return result;
}

CirclingWind2::Result
CirclingWind2::CalcWind(double quality_metric, const size_t n_samples, [[ maybe_unused ]] const Angle circle)
{
  assert(n_samples > 1);
  assert(!samples.empty());

  int quality = 5; // top quality, perfect circle
  if (quality_metric > 0.2) quality = 4;
  if (quality_metric > 0.3) quality = 3;
  if (quality_metric > 0.4) quality = 2;
  if (quality_metric > 0.5) quality = 1;
  if (quality_metric > 0.7) return Result(0);

  // reject if average time step greater than 2.0 seconds
  const auto measure_time = (samples[0].time - samples[n_samples-1].time);
  if ((measure_time / n_samples) > std::chrono::seconds{2})
    return Result(0);

  // find average to determine wind speed
  // this tolerates TAS == 0 if airspeed in't available
  double speed_offset = 0;
  for (size_t i = 0; i < n_samples; i++) {
    speed_offset += (samples[i].ground_speed - samples[i].tas);
  }
  speed_offset /= n_samples;

  // estimate wind speed by amplitude of wind speed data
  double wind_speed = 0;
  for (size_t i = 0; i < n_samples; i++) {
    wind_speed += abs(samples[i].ground_speed - samples[i].tas - speed_offset);
  }
  // TO DO: subtract the ammount exceeding the full circle
  wind_speed /= n_samples; // average
  wind_speed *= 1.5708; // the ratio of the average to the amplitude of a sine curve

  if (wind_speed >= 30)
    // limit to reasonable values (30 m/s), reject otherwise
    return Result(0);

  // determine bearing of the wind
  // prepare for the iterative search
  Angle wind_dir = Angle::Degrees(0);
  Angle search_midpoint = Angle::Degrees(180);
  int search_steps = 6;
  // Angle search_step_width = Angle::FullCircle() / search_steps;
  // cover more that a full circle
  Angle search_step_width = circle.Absolute() / search_steps;
  search_steps += 1;
  Angle probe_point;
  double fit_metric;
  // initialize to a large number
  double min_fit_metric = INITIAL_min_fit_metric;

  // this is a "safety net", absolutely not necessary, since the number of iterations is fix at compile time
  int max_iterations = 10;
  // attempt to fit to cosine curve
  do {
    // make sure we don't loop forever
    assert(max_iterations > 0); 
    max_iterations -= 1;

    probe_point = search_midpoint - (search_step_width * ((double)search_steps/2.0));

    for (int s=0; s <= search_steps; s++) {
      fit_metric = FitCosine(n_samples,wind_speed,speed_offset,probe_point);
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
  wind_dir = search_midpoint.AsBearing();

  if (min_fit_metric > (INITIAL_min_fit_metric - 1)) {
    LogString("CirclingWind2 doesn't converge");
    return Result(0); // it doesn't converge
  }

  // TO DO: iterate over wind_speed?

  // fine tune the quality number to reflect the quality of the fit
  if (min_fit_metric > 10) quality -= 1; // penalty for bad fit
  if (min_fit_metric < 5)  quality += 1; // bonus for good fit
  if (min_fit_metric < 1)  quality += 1; // extra bonus for very good fit
  quality = std::max(std::min(quality,5),0);

  SpeedVector wind(wind_dir.AsBearing(), wind_speed);
  char line_buf[200];
  snprintf(line_buf,sizeof(line_buf),"CalcWind2 circling: %1.1f°, %1.1f km/h, Q=%d, QM=%1.2f, fit=%1.1f",
    wind_dir.AsBearing().Degrees(),wind_speed*3.6,quality,quality_metric,min_fit_metric);
  LogString(line_buf);

  return Result(quality, wind);
}

double
CirclingWind2::FitCosine(const size_t n_samples, const double amplitude, const double offset, const Angle phase)
// fit the measured speed data to a cosine
// uses the "sum of squares" algorithm
{
  double sum_of_squares_of_deltas = 0;

  for (size_t i = 0; i < n_samples; i++) {
    const double cosine_curve = fastcosine((samples[i].track + phase).Radians());
    double net_speed_diff = samples[i].ground_speed - samples[i].tas - offset;
    // to make the fit metric (somewhat) independent of the amplitude
    if (amplitude > 1.0) net_speed_diff /= amplitude;
    sum_of_squares_of_deltas += Square(cosine_curve-net_speed_diff);
  }
  return sum_of_squares_of_deltas;
}
