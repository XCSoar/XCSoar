// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/SkyLines/Handler.hpp"
#include "FLARM/Traffic.hpp"
#include "FLARM/Id.hpp"
#include "Geo/GeoPoint.hpp"
#include "NMEA/Info.hpp"

namespace SkyLinesTracking {

/**
 * Build and merge online traffic into #FlarmTraffic records.
 */
class FlarmTrafficBuilder {
public:
  [[gnu::const]]
  static FlarmId ResolveId(uint32_t pilot_id,
                           FlarmId flarm_id) noexcept;

  [[gnu::const]]
  static FlarmTraffic::SourceType
  SourceForOnline(uint32_t pilot_id, TrafficSource source) noexcept;

  /**
   * True when online traffic #traffic_id matches the own-ship FLARM
   * radio id (drop self from OGN / cloud feeds).
   */
  [[gnu::const]]
  static bool IsOwnShipId(FlarmId own_radio_id,
                          FlarmId traffic_id) noexcept;

  /**
   * Populate relative_north/east/altitude from own-ship GPS.
   *
   * @return false if relative position cannot be computed
   */
  static bool FillRelative(FlarmTraffic &traffic,
                           const NMEAInfo &basic) noexcept;

  static FlarmTraffic Build(uint32_t pilot_id,
                            const GeoPoint &location, int altitude,
                            bool altitude_available,
                            TrafficSource source,
                            unsigned track_deg, bool track_valid,
                            FlarmId flarm_id, unsigned aircraft_type,
                            const char *server_name) noexcept;
};

} // namespace SkyLinesTracking
