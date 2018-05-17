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

#include "FlyingState.hpp"

void
FlyingState::Reset()
{
  flying = false;
  on_ground = false;
  powered = false;
  flight_time = 0;
  takeoff_time = -1;
  takeoff_location.SetInvalid();
  release_time = -1;
  release_location.SetInvalid();
  power_on_time = -1;
  power_on_location.SetInvalid();
  power_off_time = -1;
  power_off_location.SetInvalid();
  far_location.SetInvalid();
  far_distance = -1;
  landing_time = -1;
  landing_location.SetInvalid();
}
