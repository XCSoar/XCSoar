// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Types.hpp"
#include "Device/Declaration.hpp"

#include <tchar.h>

class Angle;
struct BrokenDateTime;
struct Waypoint;

namespace IMI {

struct AngleConverter {
  union
  {
    struct
    {
      IMIDWORD milliminutes :16;
      IMIDWORD degrees :8;
      IMIDWORD sign :1;
    };
    IMIDWORD value;
  };

  AngleConverter() {}
  AngleConverter(Angle angle);
};

void
ConvertToChar(const TCHAR* unicode, char* ascii, int outSize);

BrokenDateTime
ConvertToDateTime(IMIDATETIMESEC in);

void
ConvertOZ(const Declaration::TurnPoint &tp, bool is_start,
          bool is_finish, TWaypoint &imiWp);

void
ConvertWaypoint(const Waypoint &wp, TWaypoint &imiWp);


} // namespace IMI
