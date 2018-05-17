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

#ifndef PYTHON_FLIGHTTIMES_HPP
#define PYTHON_FLIGHTTIMES_HPP

#include "Time/BrokenDateTime.hpp"
#include "Geo/GeoPoint.hpp"
#include <vector>

struct PowerState {
  BrokenDateTime time;
  GeoPoint location;
  enum { ON, OFF } state;

  PowerState() {
    time.Clear();
    location.SetInvalid();
    state = OFF;
  }
};

struct FlightTimeResult {
  BrokenDateTime takeoff_time, release_time, landing_time;
  GeoPoint takeoff_location, release_location, landing_location;

  std::vector<PowerState> power_states;

  FlightTimeResult() {
    takeoff_time.Clear();
    landing_time.Clear();
    release_time.Clear();

    takeoff_location.SetInvalid();
    landing_location.SetInvalid();
    release_location.SetInvalid();
  }
};

struct MoreData;
struct FlyingState;
struct DerivedInfo;
class DebugReplay;
struct FlightFix;

void Update(const MoreData &basic, const FlyingState &state, FlightTimeResult &result);

void Update(const MoreData &basic, const DerivedInfo &calculated, FlightTimeResult &result);

void Finish(const MoreData &basic, const DerivedInfo &calculated, FlightTimeResult &result);

bool Run(DebugReplay &replay, FlightTimeResult &result);

void FlightTimes(DebugReplay &replay, std::vector<FlightTimeResult> &results);

#endif /* PYTHON_FLIGHTTIMES_HPP */
