/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
