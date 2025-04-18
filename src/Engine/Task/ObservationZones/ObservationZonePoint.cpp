// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ObservationZonePoint.hpp"

bool
ObservationZonePoint::Equals(const ObservationZonePoint &other) const noexcept
{
  return GetShape() == other.GetShape() &&
    GetReference() == other.GetReference();
}
