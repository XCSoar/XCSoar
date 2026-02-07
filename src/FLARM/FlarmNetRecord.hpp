// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"
#include "RadioFrequency.hpp"
#include "Id.hpp"
static constexpr std::size_t
LatinBufferSize(std::size_t size) noexcept
{
/* ..., but when we convert Latin-1 to UTF-8, we need a little bit
   more buffer */
  return size * 3 / 2 + 1;
}

/**
 * FlarmNet.org file entry
 */
struct FlarmNetRecord {
  /**< FLARM id */
  FlarmId id;

  /**< Name 15 bytes */
  StaticString<LatinBufferSize(22)> pilot;

  /**< Airfield 4 bytes */
  StaticString<LatinBufferSize(22)> airfield;

  /**< Aircraft type 1 byte */
  StaticString<LatinBufferSize(22)> plane_type;

  /**< Registration 7 bytes */
  StaticString<LatinBufferSize(8)> registration;

  /**< Callsign 3 bytes */
  StaticString<LatinBufferSize(4)> callsign;

  /**< Radio frequency value (parsed) */
  RadioFrequency frequency = RadioFrequency::Null();

  /** 
   * Format a char value; returns nullptr if empty.
   * @param buffer Present for interface compatibility with other Format
   *        overloads, but unused in this specialization
   * @return Formatted string pointer; must not be ignored 
   */
  [[nodiscard]] const char *Format([[maybe_unused]] StaticString<256> &buffer,
                                     const char *value) const noexcept;
};
