// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Vector.hpp"
#include "Point.hpp"
#include "Geo/GeoBounds.hpp"

void
TracePointVector::ScanBounds(GeoBounds &bounds) const noexcept
{
  for (const auto &i : *this)
    bounds.Extend(i.GetLocation());
}
