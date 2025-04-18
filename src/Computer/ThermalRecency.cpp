// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalRecency.hpp"
#include "MathTables.h"

double
thermal_recency_fn(unsigned x) noexcept
{
  return x < THERMALRECENCY_SIZE
    ? THERMALRECENCY[x]
    : 0.;
}
