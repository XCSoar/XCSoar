// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Angle.hpp"

#include <cassert>
#include <cmath>

namespace {

/**
 * Reduce \a v radians to the half-open interval [0, 2π) in O(1) time.
 * NaN and infinity map to 0 so callers never spin on pathological values.
 */
double
NormalizeFullCircleRadians(const double v) noexcept
{
  if (std::isnan(v) || std::isinf(v))
    return 0;

  const double two_pi = Angle::FullCircle().Native();
  double r = std::fmod(v, two_pi);
  if (r < 0)
    r += two_pi;
  return r;
}

/** Beyond this |radian| magnitude, iterated wrap is too slow on the UI thread. */
constexpr double kAsBearingFmodThresholdRadians =
    1024 * Angle::FullCircle().Native();

} // namespace

Angle::DMS
Angle::ToDMS() const noexcept
{
  DMS dms;
  dms.negative = value < 0;

  unsigned value = lround(AbsoluteDegrees() * 3600);

  dms.seconds = value % 60;
  value /= 60;

  dms.minutes = value % 60;
  value /= 60;

  dms.degrees = value;

  return dms;
}

Angle::DMM
Angle::ToDMM() const noexcept
{
  DMM dmm;
  dmm.positive = value >= 0;

  unsigned value = lround(AbsoluteDegrees() * 60000);
  dmm.degrees = value / 60000;
  value %= 60000;
  dmm.minutes = value / 1000;
  dmm.decimal_minutes = value % 1000;

  return dmm;
}

double
Angle::AbsoluteDegrees() const noexcept
{
  return Absolute().Degrees();
}

double
Angle::AbsoluteRadians() const noexcept
{
  return Absolute().Radians();
}

Angle
Angle::AsBearing() const noexcept
{
  assert(!std::isnan(value));
  assert(!std::isinf(value));

  const double v = value;
  if (fabs(v) >= kAsBearingFmodThresholdRadians)
    return Native(NormalizeFullCircleRadians(v));

  /* Historic path: repeated ±2π matches prior floating-point rounding in
     typical ranges (see unit tests). */
  Angle retval(v);
  while (retval < Zero())
    retval += FullCircle();

  while (retval >= FullCircle())
    retval -= FullCircle();

  return retval;
}

Angle
Angle::AsDelta() const noexcept
{
  assert(!std::isnan(value));
  assert(!std::isinf(value));

  const double v = value;
  if (fabs(v) >= kAsBearingFmodThresholdRadians) {
    double r = NormalizeFullCircleRadians(v);
    const double half = HalfCircle().Native();
    if (r > half)
      r -= FullCircle().Native();
    return Native(r);
  }

  Angle retval(v);
  while (retval <= -HalfCircle())
    retval += FullCircle();

  while (retval > HalfCircle())
    retval -= FullCircle();

  return retval;
}

Angle
Angle::Reciprocal() const noexcept
{
  return (*this + HalfCircle()).AsBearing();
}

Angle
Angle::HalfAngle(const Angle end) const noexcept
{
  if (value == end.value) {
    return Reciprocal();
  } else if (value > end.value) {
    if ((*this - end) < HalfCircle())
      return (*this + end).Half().Reciprocal();
    else
      return (*this + end).Half();
  } else {
    if ((end - *this) < HalfCircle())
      return (*this + end).Half().Reciprocal();
    else
      return (*this + end).Half();
  }
}

Angle
Angle::Fraction(const Angle end, const double fraction) const noexcept
{
  if (value == end.value)
    return Angle(value);

  const Angle diff = Angle(end.value - value).AsDelta();
  return Angle(value + diff.value * fraction);
}

bool
Angle::Between(const Angle start, const Angle end) const noexcept
{
  Angle width = (end - start).AsBearing();
  Angle delta = (*this - start).AsBearing();

  return delta <= width;
}

bool
Angle::CompareRoughly(Angle other, Angle threshold) const noexcept
{
  const Angle delta = (*this - other).AsDelta();
  return delta >= -threshold && delta <= threshold;
}
