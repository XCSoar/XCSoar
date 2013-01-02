/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

KalmanFilter1d::KalmanFilter1d(const fixed var_x_accel)
  :var_x_accel_(var_x_accel)
{
  Reset();
}

KalmanFilter1d::KalmanFilter1d()
  :var_x_accel_(fixed_one)
{
  Reset();
}

void
KalmanFilter1d::Reset()
{
  Reset(fixed_zero, fixed_zero);
}

void
KalmanFilter1d::Reset(const fixed x_abs_value)
{
  Reset(x_abs_value, fixed_zero);
}

void
KalmanFilter1d::Reset(const fixed x_abs_value, const fixed x_vel_value)
{
  x_abs_ = x_abs_value;
  x_vel_ = x_vel_value;
  p_abs_abs_ = fixed(1.e10);
  p_abs_vel_ = fixed_zero;
  p_vel_vel_ = var_x_accel_;
}

void
KalmanFilter1d::Update(const fixed z_abs, const fixed var_z_abs,
                       const fixed dt)
{
  // Some abbreviated constants to make the code line up nicely:
  static const fixed F1 = fixed_one;
  static const fixed F2 = fixed_two;
  static const fixed F4 = fixed_four;

  // Validity checks. TODO: more?
  assert(positive(dt));

  // Note: math is not optimized by hand. Let the compiler sort it out.
  // Predict step.
  // Update state estimate.
  x_abs_ += x_vel_ * dt;
  // Update state covariance. The last term mixes in acceleration noise.
  p_abs_abs_ += F2*dt*p_abs_vel_+ dt*dt*p_vel_vel_+ var_x_accel_*dt*dt*dt*dt/F4;
  p_abs_vel_ +=                      dt*p_vel_vel_+ var_x_accel_*dt*dt*dt/F2;
  p_vel_vel_ +=                                     var_x_accel_*dt*dt;

  // Update step.
  const fixed y = z_abs - x_abs_;  // Innovation.
  const fixed s_inv = F1 / (p_abs_abs_ + var_z_abs);  // Innovation precision.
  const fixed k_abs = p_abs_abs_*s_inv;  // Kalman gain
  const fixed k_vel = p_abs_vel_*s_inv;
  // Update state estimate.
  x_abs_ += k_abs * y;
  x_vel_ += k_vel * y;
  // Update state covariance.
  p_vel_vel_ -= p_abs_vel_*k_vel;
  p_abs_vel_ -= p_abs_vel_*k_abs;
  p_abs_abs_ -= p_abs_abs_*k_abs;
}
