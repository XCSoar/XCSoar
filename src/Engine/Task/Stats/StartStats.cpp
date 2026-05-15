// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StartStats.hpp"
#include "Navigation/Aircraft.hpp"
#include "time/RoughTime.hpp"
#include "time/FloatDuration.hxx"

#include <cassert>
#include <cmath>

void
StartStats::SetStarted(const AircraftState &aircraft,
                       const TimeSpan *pev_span) noexcept
{
  time = aircraft.time;
  altitude = aircraft.altitude;
  ground_speed = aircraft.ground_speed;
  pev_offset_available = false;
  pev_offset_seconds = 0;

  if (pev_span != nullptr && pev_span->IsDefined() &&
      pev_span->GetStart().IsValid() && aircraft.time.IsDefined()) {
    const FloatDuration d = aircraft.time - TimeStamp{pev_span->GetStart()};
    pev_offset_seconds = static_cast<int>(std::lround(d.count()));
    pev_offset_available = true;
  }

  assert(HasStarted());
}
