// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/DataField/Enum.hpp"

#include <cstdlib>

static constexpr StaticEnumChoice tracking_intervals[] = {
  { 1, "1 sec" },
  { 2, "2 sec" },
  { 3, "3 sec" },
  { 5, "5 sec" },
  { 10, "10 sec" },
  { 15, "15 sec" },
  { 20, "20 sec" },
  { 30, "30 sec" },
  { 45, "45 sec" },
  { 60, "1 min" },
  { 120, "2 min" },
  { 180, "3 min" },
  { 300, "5 min" },
  { 600, "10 min" },
  { 900, "15 min" },
  { 1200, "20 min" },
  { 1800, "30 min" },
  { 2400, "40 min" },
  { 3000, "50 min" },
  { 3600, "60 min" },
  nullptr,
};

[[gnu::pure]]
static inline unsigned
FindClosestTrackingInterval(unsigned value) noexcept
{
  unsigned closest_value = 0;
  int closest_diff = -1;

  for (const StaticEnumChoice *p = tracking_intervals;
       p->display_string != nullptr; ++p) {
    const int diff = std::abs(int(value) - int(p->id));
    if (closest_diff < 0 || diff < closest_diff) {
      closest_diff = diff;
      closest_value = p->id;
    }
  }

  return closest_value;
}
