// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

class FlarmId;

static constexpr std::size_t
LatinBufferSize(std::size_t size) noexcept
{
#ifdef _UNICODE
/* with wide characters, the exact size of the FLARMNet database field
   (plus one for the terminator) is just right, ... */
  return size;
#else
/* ..., but when we convert Latin-1 to UTF-8, we need a little bit
   more buffer */
  return size * 3 / 2 + 1;
#endif
}

/**
 * FlarmNet.org file entry
 */
struct FlarmNetRecord {
  /**< FLARM id 6 bytes */
  StaticString<LatinBufferSize(7)> id;

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

  /**< Radio frequency 6 bytes */
  StaticString<LatinBufferSize(8)> frequency;

  [[gnu::pure]]
  FlarmId GetId() const noexcept;
};
