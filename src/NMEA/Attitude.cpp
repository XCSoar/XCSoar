/*
Copyright_License {

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

#include "NMEA/Attitude.hpp"

void
AttitudeState::Complement(const AttitudeState &add)
{
  if (bank_angle_available.Complement(add.bank_angle_available))
    bank_angle = add.bank_angle;

  if (pitch_angle_available.Complement(add.pitch_angle_available))
    pitch_angle = add.pitch_angle;

  if (heading_available.Complement(add.heading_available))
    heading = add.heading;
}

void
AttitudeState::Expire(double now)
{
  bank_angle_available.Expire(now, 5);
  pitch_angle_available.Expire(now, 5);
  heading_available.Expire(now, 5);
}
