// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Validity.hpp"
#include "util/StaticString.hxx"

#include <type_traits>

/**
 * Operations progress information from the PFLAQ sentence.
 * Sent by FLARM during firmware updates, obstacle database updates,
 * IGC file downloads, and other long-running operations.
 *
 * @see FTD-012 Data Port ICD, PFLAQ sentence
 */
struct FlarmProgress {
  Validity available;

  /** Operation identifier: "FW", "OBST", "IGC", "DUMP", "RESTORE", "SCAN" */
  StaticString<11> operation;

  /** Optional filename (PowerFLARM only, omitted on Classic FLARM) */
  StaticString<64> info;

  /** Progress percentage, range 0..100 */
  unsigned progress;

  constexpr void Clear() noexcept {
    available.Clear();
  }

  constexpr void Complement(const FlarmProgress &add) noexcept {
    if (available.Complement(add.available)) {
      operation = add.operation;
      info = add.info;
      progress = add.progress;
    }
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    available.Expire(clock, std::chrono::seconds{10});
  }
};

static_assert(std::is_trivial<FlarmProgress>::value, "type is not trivial");
