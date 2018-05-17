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

#ifndef XCSOAR_FLIGHT_LOGGER_HPP
#define XCSOAR_FLIGHT_LOGGER_HPP

#include "Time/BrokenDateTime.hpp"
#include "OS/Path.hpp"

struct MoreData;
struct DerivedInfo;

/**
 * This class logs start and landing into a file, to be used as a
 * flying log book.
 *
 * Before first using it, this object must be initialised explicitly
 * by calling Reset().
 *
 * Depends on #FlyingComputer.
 */
class FlightLogger {
  AllocatedPath path = nullptr;

  double last_time;
  bool seen_on_ground, seen_flying;

  /**
   * Set to the most recently observed start time.  It gets cleared
   * after a landing has been logged.
   */
  BrokenDateTime start_time;

  BrokenDateTime landing_time;

public:
  FlightLogger() {
    Reset();
  }

  /**
   * Call this before Tick().
   */
  void SetPath(Path _path) {
    path = _path;
  }

  void Reset();

  /**
   * Call this periodically.
   */
  void Tick(const MoreData &basic, const DerivedInfo &calculated);

private:
  void LogEvent(const BrokenDateTime &date_time, const char *type);

  void TickInternal(const MoreData &basic, const DerivedInfo &calculated);
};

#endif
