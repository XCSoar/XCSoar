// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "List.hpp"

const FlarmTraffic *
TrafficList::FindMaximumAlert() const noexcept
{
  const FlarmTraffic *alert = NULL;

  for (const auto &traffic : list)
    if (traffic.HasAlarm() &&
        (alert == NULL ||
         ((unsigned)traffic.alarm_level > (unsigned)alert->alarm_level ||
          (traffic.alarm_level == alert->alarm_level &&
           /* if the levels match -> let the distance decide (smaller
              distance wins) */
           traffic.distance < alert->distance))))
      alert = &traffic;

  return alert;
}

bool
TrafficList::InCloseRange() const noexcept
{
  return std::any_of(list.begin(), list.end(), [](const auto &traffic)
    { return traffic.distance < (RoughDistance)4000; });
}
