// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Traffic.hpp"
#include "util/StaticArray.hxx"

#include <cstdint>
#include <optional>

struct FlarmStatus;

/**
 * What a spoken traffic alert says about the most threatening
 * intruder of a FLARM collision alarm: "Traffic urgent! Three
 * o'clock. Above."
 */
struct SpokenTrafficAlert {
  enum class Vertical : uint8_t {
    UNKNOWN,
    LEVEL,
    ABOVE,
    BELOW,
  };

  /** LOW, IMPORTANT or URGENT */
  FlarmTraffic::AlarmType level;

  /** The clock position, 1 to 12, or 0 for a non-directional intruder */
  uint8_t clock;

  Vertical vertical;

  constexpr bool
  operator==(const SpokenTrafficAlert &) const noexcept = default;

  /**
   * The alert for the collision alarm in a PFLAU sentence, or
   * std::nullopt if there is no aircraft alarm.  Obstacle and alert
   * zone alarms are not spoken.
   */
  [[gnu::pure]]
  static std::optional<SpokenTrafficAlert>
  FromStatus(const FlarmStatus &status) noexcept;

  /**
   * The sound resources to play one after another.
   */
  [[gnu::pure]]
  StaticArray<const char *, 3> GetClips() const noexcept;
};
