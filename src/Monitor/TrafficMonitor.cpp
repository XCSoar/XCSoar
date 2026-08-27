// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficMonitor.hpp"
#include "Interface.hpp"
#include "Hardware/Vibrator.hpp"

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

  if (!escalated)
    return;

#ifdef HAVE_VIBRATOR
  Vibrate(HapticFeedbackType::ALARM);
#endif
}
