// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/SkyLines/Features.hpp"
#include "util/TriState.hpp"
#include "util/StaticArray.hxx"
#include "util/StaticString.hxx"
#include "util/IterableSplitString.hxx"
#include "util/StringStrip.hxx"
#include "FLARM/Id.hpp"

#ifdef HAVE_SKYLINES_TRACKING

#include <cstdint>
#include <cstring>
#include <cstddef>

struct CloudSettings {
  static constexpr const char *DEFAULT_HOST = "cloud.xcsoar.org";
  static constexpr unsigned DEFAULT_PORT = 5597;

  /** Maximum manual own-ship FLARM ids (comma-separated profile value). */
  static constexpr unsigned MAX_OWN_FLARM_IDS = 8;

  /**
   * Buffer for formatted list: 8 hex digits per id, commas, NUL.
   * FlarmId::Format() can emit up to 8 hex digits for a uint32_t.
   */
  static constexpr std::size_t OWN_FLARM_IDS_TEXT_SIZE =
    MAX_OWN_FLARM_IDS * 8 +
    (MAX_OWN_FLARM_IDS > 0 ? MAX_OWN_FLARM_IDS - 1 : 0) + 1;

  using OwnFlarmIdList = StaticArray<FlarmId, MAX_OWN_FLARM_IDS>;

  /**
   * Is submitting data to the (experimental) XCSoar Cloud enabled?
   * TriState::UNKNOWN means the user has not yet been asked about
   * it.
   */
  TriState enabled;

  /**
   * Receive traffic from the XCSoar Cloud server (including OGN)?
   */
  bool show_traffic;

  bool show_thermals;

  /**
   * Enable cloud communication while on a "roamed" connection?
   */
  bool roaming;

  uint64_t key;

  /** Hostname of the XCSoar Cloud server (UDP SkyLines protocol). */
  StaticString<64> host;

  /** UDP port of the XCSoar Cloud server. */
  unsigned port = DEFAULT_PORT;

  /**
   * Manual own-ship FLARM ids used to filter self from OGN/cloud
   * traffic when the device radio id is not available (e.g. FLARM
   * behind an LX passthrough), or to hide additional own aircraft.
   * Comma-separated hex ids in the profile.
   */
  OwnFlarmIdList own_flarm_ids;

  void SetDefaults() {
    enabled = TriState::UNKNOWN;
    show_traffic = true;
    show_thermals = true;
    roaming = true;
    key = 0;
    host = DEFAULT_HOST;
    port = DEFAULT_PORT;
    own_flarm_ids.clear();
  }

  [[gnu::pure]]
  const char *HostCStr() const noexcept {
    return host.empty() ? DEFAULT_HOST : host.c_str();
  }

  [[gnu::pure]]
  unsigned EffectivePort() const noexcept {
    if (port > 0 && port <= 65535u)
      return port;

    return DEFAULT_PORT;
  }

  /**
   * Parse a comma-separated list of hex FLARM / ICAO ids.  Invalid
   * tokens and duplicates are skipped.  Accepts a single id (legacy
   * profile values).
   */
  [[gnu::pure]]
  static OwnFlarmIdList ParseOwnFlarmIds(const char *text) noexcept {
    OwnFlarmIdList ids;
    if (text == nullptr || *text == '\0')
      return ids;

    for (const auto token : IterableSplitString(text, ',')) {
      if (ids.full())
        break;

      const auto trimmed = Strip(token);
      if (trimmed.empty())
        continue;

      char buf[16];
      if (trimmed.size() >= sizeof(buf))
        continue;

      memcpy(buf, trimmed.data(), trimmed.size());
      buf[trimmed.size()] = '\0';

      char *endptr;
      const FlarmId id = FlarmId::Parse(buf, &endptr);
      if (!id.IsDefined() || endptr == buf || *endptr != '\0')
        continue;

      if (!ids.contains(id))
        ids.append(id);
    }

    return ids;
  }

  /** Format @ids as comma-separated hex (empty string when none). */
  static void FormatOwnFlarmIds(const OwnFlarmIdList &ids,
                                char *buffer, std::size_t size) noexcept {
    if (buffer == nullptr || size == 0)
      return;

    buffer[0] = '\0';
    if (ids.empty() || size < 2)
      return;

    std::size_t used = 0;
    for (unsigned i = 0; i < ids.size(); ++i) {
      char hex[16];
      ids[i].Format(hex);
      const std::size_t hex_len = strlen(hex);
      const std::size_t need = (i > 0 ? 1 : 0) + hex_len;
      if (used + need + 1 > size)
        break;

      if (i > 0)
        buffer[used++] = ',';
      memcpy(buffer + used, hex, hex_len);
      used += hex_len;
      buffer[used] = '\0';
    }
  }
};

#endif
