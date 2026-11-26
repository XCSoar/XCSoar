// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ClimbRateMonitor.hpp"
#include "Interface.hpp"
#include "Audio/Sound.hpp"
#include "Audio/SpokenVerticalSpeed.hpp"
#include "FLARM/Status.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Units/Units.hpp"

#include <chrono>

using namespace std::chrono;

/**
 * Speak this often; it is also the period of the average that is
 * spoken (VarioInfo::average).
 */
static constexpr auto INTERVAL = seconds(30);

/**
 * Is a FLARM collision alarm (with an aircraft or an obstacle) or an
 * airspace warning active?  The pilot needs the attention, and a new
 * announcement would cut off a spoken alert.
 */
static bool
IsWarningActive(const NMEAInfo &basic) noexcept
{
  /* as in TrafficMonitor: an "info alert" is not a collision alarm */
  const auto &flarm = basic.flarm.status;
  if (flarm.available &&
      flarm.alarm_level != FlarmTraffic::AlarmType::NONE &&
      flarm.alarm_level != FlarmTraffic::AlarmType::INFO_ALERT)
    return true;

  if (backend_components == nullptr)
    return false;

  const auto *airspace_warnings = backend_components->GetAirspaceWarnings();
  if (airspace_warnings == nullptr)
    return false;

  const auto top = airspace_warnings->GetTopWarning();
  return top && top->IsActive();
}

void
ClimbRateMonitor::Check() noexcept
{
  const auto &basic = CommonInterface::Basic();
  const auto &calculated = CommonInterface::Calculated();

  if (!basic.time_available)
    return;

  /* start counting when circling begins; the clock also restarts
     when the time jumps back (replay, midnight).  The circling flag
     may stay set after landing, and without a vario the average is
     made of zeros. */
  if (!CommonInterface::GetUISettings().sound.spoken_climb_rate ||
      !calculated.circling || !calculated.flight.flying ||
      !basic.brutto_vario_available) {
    spoken_clock.Update(basic.time);
    return;
  }

  if (!spoken_clock.CheckAdvance(basic.time, INTERVAL))
    return;

  /* skip this one rather than talk over a warning */
  if (IsWarningActive(basic))
    return;

  const auto clips =
    GetSpokenVerticalSpeedClips(Units::ToUserVSpeed(calculated.average),
                                Units::GetUserVerticalSpeedUnit());
  PlayResources(clips);
}
