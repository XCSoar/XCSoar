// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskPoint.hpp"
#include "Geo/GeoVector.hpp"

GeoVector
TaskPoint::GetNextLegVector() const noexcept
{
  return GeoVector::Invalid();
}
