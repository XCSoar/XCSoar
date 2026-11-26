// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AircraftWarningMonitor.hpp"
#include "AircraftWarning.hpp"
#include "Interface.hpp"
#include "Message.hpp"
#include "Audio/Sound.hpp"
#include "Language/Language.hpp"

#include <chrono>

static constexpr auto REPEAT_INTERVAL = std::chrono::seconds{5};

static void
Consider(bool active, PeriodClock &clock,
         const char *text, const char *wav, bool &busy) noexcept
{
  if (!active) {
    clock.Reset();
    return;
  }

  /* a higher-priority warning already occupies this tick */
  if (busy)
    return;
  busy = true;

  if (!clock.CheckUpdate(REPEAT_INTERVAL))
    return;

  PlayResource(wav);
  Message::AddMessage(text);
}

void
AircraftWarningMonitor::Check() noexcept
{
  const auto &basic = CommonInterface::Basic();
  const bool flying = CommonInterface::Calculated().flight.flying;
  const auto &polar =
    CommonInterface::GetComputerSettings().polar.glide_polar_task;

  bool busy = false;
  Consider(flying && IsStallWarning(basic.switch_state,
                                    basic.stall_ratio_available.IsValid(),
                                    basic.stall_ratio),
           stall_clock, _("Stall"), "IDR_WAV_STALL", busy);
  Consider(flying && IsOverspeedWarning(basic.switch_state, polar,
                                        basic.airspeed_available.IsValid(),
                                        basic.airspeed_real,
                                        basic.indicated_airspeed),
           overspeed_clock, _("Overspeed"), "IDR_WAV_OVERSPEED", busy);
  Consider(IsAirbrakeWarning(basic.switch_state, flying),
           airbrake_clock, _("Airbrakes unlocked"),
           "IDR_WAV_AIRBRAKES", busy);
  Consider(IsGearWarning(basic.switch_state, flying),
           gear_clock, _("Check gear"), "IDR_WAV_CHECK_GEAR", busy);
}
