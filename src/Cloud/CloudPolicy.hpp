// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <chrono>

/**
 * Tunables for xcsoar-cloud-server and related tools.
 *
 * Live-response ages apply to UDP traffic/thermal replies. Export ages apply
 * to KML generation (longer window so maps stay useful offline).
 */
struct CloudPolicy {
  static constexpr double traffic_query_range_m = 50000;
  static constexpr double thermal_query_range_m = 50000;

  static constexpr std::chrono::steady_clock::duration live_max_traffic_age =
    std::chrono::minutes(15);
  static constexpr std::chrono::steady_clock::duration live_max_thermal_age =
    std::chrono::minutes(30);

  static constexpr std::chrono::steady_clock::duration subscription_ttl =
    std::chrono::minutes(5);

  /** Clients older than this relative to Expire() \a before are removed. */
  static constexpr std::chrono::steady_clock::duration client_expire_cutoff =
    std::chrono::minutes(10);

  static constexpr unsigned max_traffic_per_response = 64;
  static constexpr unsigned max_thermal_per_response = 256;

  static constexpr std::chrono::steady_clock::duration save_interval =
    std::chrono::minutes(1);
  static constexpr std::chrono::steady_clock::duration expire_timer_interval =
    std::chrono::minutes(5);

  static constexpr std::chrono::steady_clock::duration export_max_traffic_age =
    std::chrono::hours(12);
  static constexpr std::chrono::steady_clock::duration export_max_thermal_age =
    std::chrono::hours(12);
};

inline constexpr CloudPolicy cloud_policy{};
