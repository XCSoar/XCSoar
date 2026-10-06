// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <vector>

/**
 * The radio range statistics a PowerFLARM keeps across flights until
 * they are reset (Continuous Analyzer of Radio Performance, "CARP"),
 * as "$PFLAN,A,RANGE" sends them.
 *
 * @see FTD-012 Data Port ICD 7.22, chapter 8.18
 */
struct FlarmRange {
  using Values = std::vector<std::optional<unsigned>>;

  /**
   * The per-sector values of one radio channel.  Sector 0 starts at
   * the longitudinal axis (0 degrees relative bearing), the others
   * follow clockwise.  Their number is whatever the device sends
   * (20 sectors of 18 degrees today, which the ICD says may change).
   * An empty value means the device could not compute it for that
   * sector; that is not zero.
   */
  struct Channel {
    /** mean range [m] (RFTOP) */
    Values mean;

    /**
     * number of data points behind #mean (RFCNT); a sector needs
     * enough of them for its mean to be significant
     */
    Values count;

    /** standard deviation [m] (RFDEV) */
    Values deviation;
  };

  /** radio channels A and B */
  std::array<Channel, 2> channels;

  /** data points for all horizontal sectors and both channels (STATS) */
  std::optional<unsigned> points;

  /** the first and last RF packet used (TIMESPAN) */
  std::optional<std::chrono::system_clock::time_point> first, last;
};
