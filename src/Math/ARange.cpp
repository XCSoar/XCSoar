// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ARange.hpp"

bool
AngleRange::Extend(Angle value) noexcept
{
  const Angle length = GetLength();

  /* check if value is nearer to start or to end, and extend that
     side */

  Angle start_delta = (start - value).AsBearing();
  if (start_delta + length >= Angle::FullCircle())
    /* it's inside, no change required */
    return false;

  Angle end_delta = (value - end).AsBearing();
  if (start_delta < end_delta)
    start = value;
  else
    end = value;

  return true;
}

bool
AngleRange:: IntersectWith(const AngleRange &other) noexcept
{
  bool result = false;

  if (IsInside(other.start)) {
    start = other.start;
    result = true;
  } else if (other.IsInside(start))
    result = true;

  if (IsInside(other.end)) {
    end = other.end;
    result = true;
  } else if (other.IsInside(end))
    result = true;

  return result;
}
