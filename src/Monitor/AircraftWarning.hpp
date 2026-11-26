// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "NMEA/SwitchState.hpp"

/** Vega stall_ratio at which the stall warning fires. */
static constexpr double STALL_RATIO_LIMIT = 0.9;

[[gnu::pure]]
constexpr bool
IsStallWarning(const SwitchState &sw, bool stall_ratio_available,
               double stall_ratio) noexcept
{
  if (sw.vega.GetStall())
    return true;
  return stall_ratio_available && stall_ratio >= STALL_RATIO_LIMIT;
}

[[gnu::pure]]
inline bool
IsOverspeedWarning(const SwitchState &sw, const GlidePolar &polar,
                   bool airspeed_available, bool airspeed_real,
                   double indicated_airspeed) noexcept
{
  if (sw.vega.GetNeverExceed())
    return true;
  if (!polar.IsValid() || !airspeed_available || !airspeed_real)
    return false;
  return indicated_airspeed > polar.GetVMax();
}

[[gnu::pure]]
constexpr bool
IsAirbrakeWarning(const SwitchState &sw, bool flying) noexcept
{
  return flying &&
    sw.airbrake_state == SwitchState::AirbrakeState::NOT_LOCKED;
}

[[gnu::pure]]
constexpr bool
IsGearWarning(const SwitchState &sw, bool flying) noexcept
{
  if (!flying || !sw.vega.IsDefined())
    return false;
  if (sw.vega.GetGearLanding())
    return true;
  return sw.vega.GetFlapLanding() && !sw.vega.GetGearExtended();
}
