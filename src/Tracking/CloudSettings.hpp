// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/SkyLines/Features.hpp"
#include "util/TriState.hpp"
#include "util/StaticString.hxx"

#ifdef HAVE_SKYLINES_TRACKING

#include <cstdint>

struct CloudSettings {
  static constexpr const char *DEFAULT_HOST = "cloud.xcsoar.org";
  static constexpr unsigned DEFAULT_PORT = 5597;

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

  void SetDefaults() {
    enabled = TriState::UNKNOWN;
    show_traffic = true;
    show_thermals = true;
    roaming = true;
    key = 0;
    host = DEFAULT_HOST;
    port = DEFAULT_PORT;
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
};

#endif
