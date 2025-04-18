// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#pragma once

#include "Navigation/Aircraft.hpp"

class AircraftSim {
  AircraftState state, state_last;

public:
  const AircraftState& GetState() const {
    return state;
  }
  const AircraftState& GetLastState() const {
    return state_last;
  }
  AircraftState& GetState() {
    return state;
  }

  void SetWind(double speed, Angle direction);

  void Start(const GeoPoint& location_start,
             const GeoPoint& location_last,
             double altitude);

  bool Update(const Angle heading,
              const FloatDuration timestep=std::chrono::seconds{1}) noexcept;

  auto GetTime() const noexcept {
    return state.time;
  }

private:
  void Integrate(Angle heading,
                 FloatDuration timestep) noexcept;

  GeoPoint GetEndPoint(Angle heading,
                       FloatDuration timestep) const noexcept;
};
