// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "UnifiedTraffic.hpp"
#include "FLARM/List.hpp"
#include "FLARM/Traffic.hpp"
#include "Tracking/SkyLines/Data.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/Stamp.hpp"
#include "time/FloatDuration.hxx"

#include <map>
#include <chrono>

/**
 * Aggregates traffic from multiple sources (FLARM, OGN, SkyLines, etc.)
 * with deduplication and priority handling.
 */
class TrafficAggregator {
  /**
   * Unified traffic storage, keyed by FlarmId
   */
  std::map<FlarmId, UnifiedTraffic> traffic_map;

  /**
   * FLARM range threshold (meters).
   * When traffic is within this range, FLARM data takes priority.
   */
  static constexpr double FLARM_PRIORITY_RANGE = 5000.0;  // 5km

  /**
   * Maximum age for traffic data before expiration (seconds)
   */
  static constexpr FloatDuration MAX_TRAFFIC_AGE{60.0};

public:
  TrafficAggregator() = default;

  /**
   * Clear all traffic
   */
  void Clear() noexcept {
    traffic_map.clear();
  }

  /**
   * Feed FLARM traffic into the aggregator
   */
  void FeedFLARM(const FlarmTraffic &flarm, TimeStamp now) noexcept;

  /**
   * Feed OGN/SkyLines traffic into the aggregator
   */
  void FeedOGN(UnifiedTraffic::Source source, uint32_t source_id,
               FlarmId flarm_id, const GeoPoint &location,
               int altitude, unsigned track, unsigned speed,
               int climb_rate, TimeStamp timestamp,
               double turn_rate = 0,
               FlarmTraffic::AircraftType aircraft_type = FlarmTraffic::AircraftType::UNKNOWN) noexcept;

  /**
   * Expire old traffic entries
   */
  void Expire(TimeStamp now) noexcept;

  /**
   * Get unified traffic list (for compatibility with TrafficList)
   */
  TrafficList GetUnifiedTrafficList(const GeoPoint &own_location,
                                     TimeStamp now) const noexcept;

  /**
   * Get all unified traffic
   */
  const std::map<FlarmId, UnifiedTraffic> &GetTraffic() const noexcept {
    return traffic_map;
  }

  /**
   * Check if aggregator is empty
   */
  bool IsEmpty() const noexcept {
    return traffic_map.empty();
  }

private:
  /**
   * Determine if new traffic should replace existing traffic
   */
  [[gnu::pure]]
  bool ShouldReplace(const UnifiedTraffic &existing,
                     const UnifiedTraffic &new_traffic,
                     const GeoPoint &own_location) const noexcept;

  /**
   * Calculate distance from own aircraft to traffic
   */
  [[gnu::pure]]
  double CalculateDistance(const UnifiedTraffic &traffic,
                           const GeoPoint &own_location) const noexcept;
};

