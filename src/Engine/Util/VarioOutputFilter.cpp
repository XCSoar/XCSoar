// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioOutputFilter.hpp"

#include "Math/LowPassFilter.hpp"

#include <cmath>

using namespace std::chrono_literals;

void
VarioOutputFilter::Reset(double value) noexcept
{
  time_defined = false;
  output = value;
}

void
VarioOutputFilter::Design(double new_period) noexcept
{
  period_seconds = new_period;
  time_defined = false;
}

bool
VarioOutputFilter::Update(double vario) noexcept
{
  if (period_seconds <= 0) {
    output = vario;
    return true;
  }

  const auto now = Clock::now();

  if (!time_defined) {
    last_filter_time = last_trail_time = now;
    time_defined = true;
    output = vario;
    return true;
  }

  const double dt =
    std::chrono::duration<double>(now - last_filter_time).count();
  last_filter_time = now;

  if (dt > 0) {
    const double alpha = 1. - std::exp(-dt / period_seconds);
    output = LowPassFilter(output, vario, alpha);
  }

  const double trail_dt =
    std::chrono::duration<double>(now - last_trail_time).count();
  if (trail_dt >= 1.) {
    last_trail_time = now;
    return true;
  }

  return false;
}
