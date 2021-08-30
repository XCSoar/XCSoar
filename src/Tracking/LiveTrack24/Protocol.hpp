/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in thenv, that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef LIVETRACK24_PROTOCOL_HPP
#define LIVETRACK24_PROTOCOL_HPP

#include <cstdint>

namespace LiveTrack24 {

enum class VehicleType {
  PARAGLIDER = 1,
  FLEX_WING_FAI1 = 2,
  RIGID_WING_FAI5 = 4,
  GLIDER = 8,
  PARAMOTOR = 16,
  TRIKE = 32,
  POWERED_AIRCRAFT = 64,
  HOT_AIR_BALLOON = 128,

  WALK = 16385,
  RUN = 16386,
  BIKE = 16388,

  HIKE = 16400,
  CYCLE = 16401,
  MOUNTAIN_BIKE = 16402,
  MOTORCYCLE = 16403,

  WINDSURF = 16500,
  KITESURF = 16501,
  SAILING = 16502,

  SNOWBOARD = 16600,
  SKI = 16601,
  SNOWKITE = 16602,

  CAR = 17100,
  CAR_4X4 = 17101,
};

} // namespace Livetrack24

#endif
