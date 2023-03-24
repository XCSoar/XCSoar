// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Angle.hpp"

#include <cassert>

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
  assert(!isnan(value));
  assert(!isinf(value));
  assert(fabs(value) < 100 * FullCircle().Native());

#ifndef FIXED_MATH
  /* Workaround for endless loops below; this is only for release
     builds.  Debug builds will crash due to assertion failure.  Right
     now, I don't know where those abnormal values come from, but I
     hope we'll find out soon - until that happens, this workaround
     reduces some user frustration with XCSoar freezes. */
  if (!isnormal(value))
    return Zero();
#endif

  Angle retval(value);

  while (retval < Zero())
    retval += FullCircle();

  while (retval >= FullCircle())
    retval -= FullCircle();

  return retval;
}

Angle
Angle::AsDelta() const noexcept
{
  assert(!isnan(value));
  assert(!isinf(value));
  assert(fabs(value) < 100 * FullCircle().Native());

#ifndef FIXED_MATH
  /* same workaround as in AsBearing() */
  if (!isnormal(value))
    return Zero();
#endif

  Angle retval(value);

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
