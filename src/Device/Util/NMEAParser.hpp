// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Checksum.hpp"
#include "NMEA/InputLine.hpp"

#include <utility>

/**
 * Helper for Device driver ParseNMEA() implementations.
 *
 * Performs checksum verification and constructs a #NMEAInputLine, then
 * invokes the callback to parse the sentence.
 */
template<typename F>
[[nodiscard]] static inline bool
ParseNMEAWithChecksum(const char *line, F &&f) noexcept(noexcept(f(
                                     std::declval<NMEAInputLine &>())))
{
  if (!VerifyNMEAChecksum(line))
    return false;

  NMEAInputLine input_line(line);
  return f(input_line);
}

