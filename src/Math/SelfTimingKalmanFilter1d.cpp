// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SelfTimingKalmanFilter1d.hpp"
#include "time/Cast.hxx"

#include <algorithm>

void
SelfTimingKalmanFilter1d::Update(const double z_abs,
                                 const double var_z_abs) noexcept
{
  const auto now = Clock::now();

  /* if we're called too quickly (less than 1us), round dt up to 1us
     to avoid problems in KalmanFilter1d::Update() */
  const auto dt = std::max<Duration>(now - last_update_time,
                                     std::chrono::microseconds{1});
  last_update_time = now;

  if (dt > max_dt)
    filter_.Reset();
  filter_.Update(z_abs, var_z_abs, ToFloatSeconds(dt));
}
