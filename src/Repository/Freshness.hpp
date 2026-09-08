// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <chrono>

namespace Repository {

static constexpr auto REFRESH_INTERVAL = std::chrono::hours(24);

static constexpr bool
IsRefreshDue(std::chrono::system_clock::time_point modified,
             std::chrono::system_clock::time_point now) noexcept
{
  if (modified == std::chrono::system_clock::time_point{})
    return true;

  const auto age = now - modified;
  return age < decltype(age)::zero() || age >= REFRESH_INTERVAL;
}

} // namespace Repository
