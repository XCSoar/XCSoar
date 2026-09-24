// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>

/**
 * A time zone described by a POSIX TZ string such as
 * "CET-1CEST,M3.5.0,M10.5.0/3".
 *
 * Such a string carries the daylight saving time rules itself, which
 * allows us to calculate the UTC offset for any point in time without
 * a copy of the zoneinfo database; that database does not exist on all
 * of our targets (e.g. Kobo), and where it does exist, it cannot be
 * queried portably.
 */
class PosixTimeZone {
public:
  /**
   * The date and time of a daylight saving time transition, in one of
   * the three formats POSIX allows.
   */
  struct Transition {
    enum class Format : uint8_t {
      /** "Mm.w.d": the #week th #day_of_week of #month */
      MONTH_WEEK_DAY,

      /** "Jn": the #day th day of the year, never counting February 29 */
      JULIAN,

      /** "n": the #day th day of the year, counting February 29 */
      ZERO_BASED_JULIAN,
    };

    Format format;

    /** 1..12 (#Format::MONTH_WEEK_DAY only) */
    uint8_t month;

    /** 1..5, where 5 means "the last one" (#Format::MONTH_WEEK_DAY only) */
    uint8_t week;

    /** 0..6, where 0 is Sunday (#Format::MONTH_WEEK_DAY only) */
    uint8_t day_of_week;

    /** 1..365 resp. 0..365 (the two Julian formats only) */
    uint16_t day;

    /** the local time of day at which the transition takes place */
    std::chrono::seconds time;

    [[gnu::pure]]
    std::chrono::sys_days GetDate(std::chrono::year year) const noexcept;
  };

private:
  /** seconds east of UTC while standard time is in effect */
  std::chrono::seconds standard_offset{};

  /** seconds east of UTC while daylight saving time is in effect */
  std::chrono::seconds daylight_offset{};

  bool has_daylight_saving = false;

  Transition daylight_start{}, daylight_end{};

public:
  /**
   * Parse a POSIX TZ string.
   *
   * @return std::nullopt if the string is malformed or if it omits the
   * daylight saving time rules (which POSIX declares
   * implementation-defined)
   */
  [[gnu::pure]]
  static std::optional<PosixTimeZone> Parse(std::string_view s) noexcept;

  constexpr bool HasDaylightSaving() const noexcept {
    return has_daylight_saving;
  }

  constexpr std::chrono::seconds GetStandardOffset() const noexcept {
    return standard_offset;
  }

  constexpr std::chrono::seconds GetDaylightOffset() const noexcept {
    return daylight_offset;
  }

  /**
   * Determine the UTC offset (seconds east of UTC) which is in effect
   * at the given point in time.
   */
  [[gnu::pure]]
  std::chrono::seconds GetOffset(std::chrono::system_clock::time_point t) const noexcept;
};
