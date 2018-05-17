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

#ifndef XCSOAR_DEVICE_DRIVER_VOLKSLOGGER_DATABASE_HPP
#define XCSOAR_DEVICE_DRIVER_VOLKSLOGGER_DATABASE_HPP

#include "Compiler.h"

#include <stdint.h>

struct GeoPoint;

namespace Volkslogger {
#pragma pack(push, 1)

  struct TableHeader {
    uint16_t start_offset;
    uint16_t end_offset;
    uint8_t dslaenge;
    uint8_t keylaenge;
  } gcc_packed;

  struct Waypoint {
    char name[6];
    uint8_t type_and_longitude_sign;
    uint8_t latitude[3];
    uint8_t longitude[3];

    gcc_pure
    GeoPoint GetLocation() const;
    void SetLocation(GeoPoint gp);
  } gcc_packed;

  struct DeclarationWaypoint : public Waypoint{
    uint8_t direction;
    uint8_t oz_parameter;
    uint8_t oz_shape;
  } gcc_packed;

  struct Route {
    char name[14];
    Waypoint waypoints[10];
  } gcc_packed;

  struct Pilot {
    char name[16];
  } gcc_packed;

#pragma pack(pop)
}

#endif
