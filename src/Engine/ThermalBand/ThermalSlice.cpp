// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#include "ThermalSlice.hpp"

#include <cassert>
#include <cmath>

void
ThermalSlice::Merge(const ThermalSlice& o) noexcept
{
  const double n_new = n + o.n;
  if (n_new>0) {
    w_n = (w_n*n + o.w_n*o.n)/n_new;
  } else {
    w_n = 0;
  }
  n = n_new;

  const auto dt_new = dt + o.dt;
  if (dt_new.count() > 0) {
    w_t = (w_t * dt + o.w_t * o.dt) / dt_new;
  } else {
    w_t = 0;
  }
  dt = dt_new;
}

void
ThermalSlice::Update(const ThermalSlice &o, const double dh) noexcept
{
  dt = (o.time - time) * n;
  w_t = w_n = (dt.count() != 0) ? dh * n / dt.count() : o.w_n;
  dt = std::chrono::abs(dt);
}
