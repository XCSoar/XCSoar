// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BucketWind.hpp"

#include <cmath>

namespace {

constexpr unsigned MIN_BINS = 12;
constexpr double MIN_TAS = 1;
constexpr double MIN_WIND = 0.1;
constexpr double MAX_WIND = 30;

[[gnu::const]]
double
Det3(double a00, double a01, double a02,
     double a10, double a11, double a12,
     double a20, double a21, double a22) noexcept
{
  return a00 * (a11 * a22 - a12 * a21)
       - a01 * (a10 * a22 - a12 * a20)
       + a02 * (a10 * a21 - a11 * a20);
}

/**
 * Least-squares fit of y = mean + b·cosθ + c·sinθ.
 *
 * The 3×3 system is the normal equations for those three unknowns.
 * Returns false if the tracks do not span enough angles (singular).
 */
bool
FitMeanCosSin(double n,
              double sum_cos, double sum_sin,
              double sum_cos2, double sum_sin2, double sum_cos_sin,
              double sum_y, double sum_y_cos, double sum_y_sin,
              double &mean, double &b, double &c) noexcept
{
  const double det = Det3(n,       sum_cos,     sum_sin,
                          sum_cos, sum_cos2,    sum_cos_sin,
                          sum_sin, sum_cos_sin, sum_sin2);
  if (std::fabs(det) < 1e-9)
    return false;

  /* Cramer's rule: replace one column at a time with the right-hand
     side (sum_y, sum_y_cos, sum_y_sin). */
  mean = Det3(sum_y,     sum_cos,     sum_sin,
              sum_y_cos, sum_cos2,    sum_cos_sin,
              sum_y_sin, sum_cos_sin, sum_sin2) / det;

  b = Det3(n,       sum_y,     sum_sin,
           sum_cos, sum_y_cos, sum_cos_sin,
           sum_sin, sum_y_sin, sum_sin2) / det;

  c = Det3(n,       sum_cos,     sum_y,
           sum_cos, sum_cos2,    sum_y_cos,
           sum_sin, sum_cos_sin, sum_y_sin) / det;
  return true;
}

} // namespace

unsigned
BucketWind::BinIndex(Angle track) noexcept
{
  const double deg = track.AsBearing().Degrees();
  const unsigned i = unsigned(std::floor(deg / BIN_WIDTH_DEG));
  return i % N_BINS;
}

Angle
BucketWind::BinCentre(unsigned i) noexcept
{
  return Angle::Degrees((i + 0.5) * BIN_WIDTH_DEG);
}

void
BucketWind::Reset() noexcept
{
  for (auto &bin : bins)
    bin = {};
}

void
BucketWind::Update(TimeStamp time, Angle track,
                   double ground_speed) noexcept
{
  auto &bin = bins[BinIndex(track)];
  bin.time = time;
  bin.ground_speed = ground_speed;
}

BucketWind::Result
BucketWind::Fit(TimeStamp now, FloatDuration max_age) const noexcept
{
  Result result;

  double n = 0;
  double sum_cos = 0, sum_sin = 0;
  double sum_cos2 = 0, sum_sin2 = 0, sum_cos_sin = 0;
  double sum_gs = 0, sum_gs_cos = 0, sum_gs_sin = 0;

  for (unsigned i = 0; i < N_BINS; ++i) {
    const Bin &bin = bins[i];
    if (!bin.time.IsDefined() || now < bin.time)
      continue;
    if (now - bin.time > max_age)
      continue;

    const double theta = BinCentre(i).Radians();
    const double cos_t = std::cos(theta);
    const double sin_t = std::sin(theta);
    const double gs = bin.ground_speed;

    n += 1;
    sum_cos += cos_t;
    sum_sin += sin_t;
    sum_cos2 += cos_t * cos_t;
    sum_sin2 += sin_t * sin_t;
    sum_cos_sin += cos_t * sin_t;
    sum_gs += gs;
    sum_gs_cos += gs * cos_t;
    sum_gs_sin += gs * sin_t;
  }

  result.n_bins = unsigned(n);
  if (result.n_bins < MIN_BINS)
    return result;

  double mean_gs, cos_coeff, sin_coeff;
  if (!FitMeanCosSin(n, sum_cos, sum_sin, sum_cos2, sum_sin2, sum_cos_sin,
                     sum_gs, sum_gs_cos, sum_gs_sin,
                     mean_gs, cos_coeff, sin_coeff))
    return result;

  /* Amplitude of the cosine is the wind speed (light-wind model).
     Peak ground speed is downwind, so wind-from is the opposite. */
  const double wind_speed = std::hypot(cos_coeff, sin_coeff);
  if (wind_speed < MIN_WIND || wind_speed >= MAX_WIND || mean_gs < MIN_TAS)
    return result;

  const Angle downwind = Angle::Radians(std::atan2(sin_coeff, cos_coeff));
  result.tas = mean_gs;
  result.wind = SpeedVector(downwind.Reciprocal().AsBearing(), wind_speed);
  result.valid = true;
  return result;
}
