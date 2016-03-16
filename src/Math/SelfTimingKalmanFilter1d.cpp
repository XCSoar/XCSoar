/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

#include "SelfTimingKalmanFilter1d.hpp"
#include "OS/Clock.hpp"

#include <algorithm>

SelfTimingKalmanFilter1d::SelfTimingKalmanFilter1d(const double max_dt,
                                                   const double var_x_accel)
    : filter_(var_x_accel) {
  SetMaxDt(max_dt);
}

SelfTimingKalmanFilter1d::SelfTimingKalmanFilter1d(const double max_dt) {
  SetMaxDt(max_dt);
}

void
SelfTimingKalmanFilter1d::SetMaxDt(const double max_dt)
{
  // It's OK, albeit silly, to have a zero max_dt value. We just always reset.
  max_dt_us_ = max_dt < 0 ? 0u : unsigned(max_dt * 1e6);
}

double
SelfTimingKalmanFilter1d::GetMaxDt() const
{
  return max_dt_us_ / 1e6;
}

void
SelfTimingKalmanFilter1d::Update(const double z_abs, const double var_z_abs)
{
  const unsigned int t_us = MonotonicClockUS();

  /* if we're called too quickly (less than 1us), round dt up to 1us
     to avoid problems in KalmanFilter1d::Update() */
  const unsigned int dt_us = std::max(t_us - t_last_update_us_, uint64_t(1));

  t_last_update_us_ = t_us;

  if (dt_us > max_dt_us_)
    filter_.Reset();
  filter_.Update(z_abs, var_z_abs, dt_us / 1e6);
}
