// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_set>

class Path;

/**
 * The radio addresses whose owners asked the OGN device database
 * (ddb.glidernet.org) not to track or not to identify their aircraft.
 * The XCSoar Cloud does not forward their fixes to OGN.
 */
class OGNDeviceDatabase {
  /**
   * OGN ids: address type in bits 24-25, address in bits 0-23.
   */
  std::unordered_set<uint32_t> blocked;

  bool loaded = false;

public:
  /**
   * Has a database been loaded?  Without one, nothing may be
   * forwarded.
   */
  bool IsLoaded() const noexcept {
    return loaded;
  }

  std::size_t GetBlockedCount() const noexcept {
    return blocked.size();
  }

  /**
   * Parse the CSV download of the OGN device database, replacing
   * what was loaded before.
   *
   * @return false if this is not the database (the header is missing)
   * or a line is malformed; the old database stays
   */
  bool Parse(std::string_view csv) noexcept;

  /**
   * Load the CSV download from a file.
   *
   * @return false on error; the old database stays
   */
  bool LoadFile(Path path) noexcept;

  /**
   * May a fix of this aircraft be forwarded?
   *
   * @param id the OGN id (see #SkyLinesTracking::FixOGNExtension::id)
   */
  [[gnu::pure]]
  bool IsAllowed(uint32_t id) const noexcept {
    return loaded && !blocked.contains(id & 0x03ffffff);
  }
};
