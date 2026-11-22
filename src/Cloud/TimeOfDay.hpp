// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <chrono>
#include <cstdint>

/**
 * The difference @p a - @p b between two times of day in
 * milliseconds (UTC), as fix packets carry them.  A client's time
 * may exceed one day after midnight, and the server's wraps at
 * midnight, so the difference is taken modulo one day and lies in
 * the range (-12 h, 12 h].
 */
constexpr std::chrono::milliseconds
TimeOfDayDelta(uint32_t a, uint32_t b) noexcept
{
  constexpr int64_t day = 24LL * 60 * 60 * 1000;

  int64_t delta = (int64_t(a) - int64_t(b)) % day;
  if (delta <= -day / 2)
    delta += day;
  else if (delta > day / 2)
    delta -= day;

  return std::chrono::milliseconds{delta};
}

/**
 * A fix up to this much older than the client's last one is a late
 * packet; a bigger step back is a time warp (e.g. a new GPS time).
 */
static constexpr std::chrono::milliseconds MAX_FIX_REORDER =
  std::chrono::minutes(1);

/**
 * Whether a fix at @p time arrived late, after the newer fix at
 * @p last, e.g. because UDP reordered the packets.
 */
constexpr bool
IsLateFix(uint32_t time, uint32_t last) noexcept
{
  const auto delta = TimeOfDayDelta(time, last);
  return delta < std::chrono::milliseconds{0} && delta >= -MAX_FIX_REORDER;
}
