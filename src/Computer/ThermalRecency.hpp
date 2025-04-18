// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cmath>

constexpr unsigned THERMALRECENCY_SIZE = 60;

[[gnu::const]]
static inline double
thermal_fn(int x) noexcept
{
  return std::exp((-0.2 / THERMALRECENCY_SIZE)
                  * std::pow((double)x, 1.5));
}

[[gnu::const]]
double
thermal_recency_fn(unsigned x) noexcept;
