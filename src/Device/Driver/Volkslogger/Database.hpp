// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <cstdint>

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

    [[gnu::pure]]
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
