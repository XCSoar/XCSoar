// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Attitude.hpp"

void
AttitudeState::Complement(const AttitudeState &add) noexcept
{
  if (bank_angle_available.Complement(add.bank_angle_available))
    bank_angle = add.bank_angle;

  if (pitch_angle_available.Complement(add.pitch_angle_available))
    pitch_angle = add.pitch_angle;

  if (heading_available.Complement(add.heading_available))
    heading = add.heading;
}

void
AttitudeState::Expire(TimeStamp now) noexcept
{
  bank_angle_available.Expire(now, std::chrono::seconds(5));
  pitch_angle_available.Expire(now, std::chrono::seconds(5));
  heading_available.Expire(now, std::chrono::seconds(5));
}
