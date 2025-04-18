// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenDateTime.hpp"
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

bool Run(DebugReplay &replay, FlightTimeResult &result);

void FlightTimes(DebugReplay &replay, std::vector<FlightTimeResult> &results);
