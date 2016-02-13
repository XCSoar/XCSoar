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

#include "KalmanFilter1d.hpp"
#include "Util.hpp"

#include <assert.h>

KalmanFilter1d::KalmanFilter1d(const double var_x_accel)
  :var_x_accel_(var_x_accel)
{
  Reset();
}

KalmanFilter1d::KalmanFilter1d()
  :var_x_accel_(1)
{
  Reset();
}

void
KalmanFilter1d::Reset()
{
  Reset(0, 0);
}

void
KalmanFilter1d::Reset(const double x_abs_value)
{
  Reset(x_abs_value, 0);
}

void
KalmanFilter1d::Reset(const double x_abs_value, const double x_vel_value)
{
  x_abs_ = x_abs_value;
  x_vel_ = x_vel_value;
  p_abs_abs_ = 1.e6;
  p_abs_vel_ = 0;
  p_vel_vel_ = var_x_accel_;
}

void
KalmanFilter1d::Update(const double z_abs, const double var_z_abs,
                       const double dt)
{
  // Some abbreviated constants to make the code line up nicely:
  static constexpr double F1 = 1;

  // Validity checks. TODO: more?
  assert(dt > 0);

  // Note: math is not optimized by hand. Let the compiler sort it out.
  // Predict step.
  // Update state estimate.
  x_abs_ += x_vel_ * dt;
  // Update state covariance. The last term mixes in acceleration noise.
  const auto dt2 = Square(dt);
  const auto dt3 = dt * dt2;
  const auto dt4 = Square(dt2);
  p_abs_abs_ += 2 * dt * p_abs_vel_ + dt2 * p_vel_vel_ + var_x_accel_ * dt4 / 4;
  p_abs_vel_ += dt * p_vel_vel_ + var_x_accel_ * dt3 / 2;
  p_vel_vel_ += var_x_accel_ * dt2;

  // Update step.
  const auto y = z_abs - x_abs_;  // Innovation.
  const auto s_inv = F1 / (p_abs_abs_ + var_z_abs);  // Innovation precision.
  const auto k_abs = p_abs_abs_*s_inv;  // Kalman gain
  const auto k_vel = p_abs_vel_*s_inv;
  // Update state estimate.
  x_abs_ += k_abs * y;
  x_vel_ += k_vel * y;
  // Update state covariance.
  p_vel_vel_ -= p_abs_vel_*k_vel;
  p_abs_vel_ -= p_abs_vel_*k_abs;
  p_abs_abs_ -= p_abs_abs_*k_abs;
}
