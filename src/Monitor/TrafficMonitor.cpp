// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficMonitor.hpp"
#include "Interface.hpp"
#include "Form/Button.hpp"
#include "Audio/Sound.hpp"

#include <chrono>

using namespace std::chrono;

/**
 * While the alarm persists, speak a changed clock position or
 * vertical position not more often than this.
 */
static constexpr auto REPEAT_INTERVAL = seconds(5);

void
TrafficMonitor::Speak(bool escalated) noexcept
{
  const auto &basic = CommonInterface::Basic();
  const auto alert = SpokenTrafficAlert::FromStatus(basic.flarm.status);
  if (!alert) {
    spoken.reset();
    return;
  }

  /* speak a new or higher alarm at once, and a change while it
     persists only after a while, so the clock position does not
     chatter at a boundary */
  if (!escalated &&
      (*alert == spoken ||
       (spoken_time.IsDefined() &&
        basic.clock - spoken_time < REPEAT_INTERVAL)))
    return;

  spoken = alert;
  spoken_time = basic.clock;

  const auto clips = alert->GetClips();
  PlayResources(clips);
}

void
TrafficMonitor::Check() noexcept
{
  const auto &status = CommonInterface::Basic().flarm.status;

  const auto alarm_level = status.available
    ? status.alarm_level
    : FlarmTraffic::AlarmType::NONE;

  /* an "info alert" is not a collision warning, and it must not
     escalate the alarm level either */
  const bool is_warning = alarm_level != FlarmTraffic::AlarmType::NONE &&
    alarm_level != FlarmTraffic::AlarmType::INFO_ALERT;
  const bool was_warning = last != FlarmTraffic::AlarmType::NONE &&
    last != FlarmTraffic::AlarmType::INFO_ALERT;

  /* warn on a new alarm and on each escalation, but not while the
     same alarm persists */
  const bool escalated = is_warning &&
    (!was_warning || alarm_level > last);

  last = alarm_level;

  if (CommonInterface::GetUISettings().traffic.spoken_alerts) {
    Speak(escalated);
  } else {
    /* once turned on again, speak the current alarm as a new one */
    spoken.reset();
    spoken_time = TimeStamp::Undefined();
  }

  if (!escalated)
    return;

#ifdef HAVE_VIBRATOR
  PlayHapticFeedback(HapticFeedbackType::ALARM);
#endif
}
