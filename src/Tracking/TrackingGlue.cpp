// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrackingGlue.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "NMEA/MoreData.hpp"
#include "LogFile.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "Geo/CoordinateFormat.hpp"
#include "util/ConvertString.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "Traffic/Aggregator.hpp"
#include "FLARM/Id.hpp"
#include "time/Cast.hxx"

TrackingGlue::TrackingGlue(EventLoop &event_loop,
                           CurlGlobal &curl) noexcept
  :skylines(event_loop, this),
   livetrack24(curl)
{
}

void
TrackingGlue::SetSettings(const TrackingSettings &_settings)
{
  skylines.SetSettings(_settings.skylines);
  livetrack24.SetSettings(_settings.livetrack24);
}

void
TrackingGlue::OnTimer(const MoreData &basic, const DerivedInfo &calculated)
{
  try {
    skylines.Tick(basic, calculated);
  } catch (...) {
    LogError(std::current_exception(), "SkyLines error");
  }

  livetrack24.OnTimer(basic, calculated);
}

void
TrackingGlue::OnTraffic(uint32_t pilot_id, unsigned time_of_day_ms,
                        const GeoPoint &location, int altitude, uint32_t flarm_id,
                        unsigned track,
                        double turn_rate,
                        FlarmTraffic::AircraftType aircraft_type)
{
  // Log received traffic (from either SkyLines or xcsoar-cloud)
  if (location.IsValid()) {
    const auto formatted = FormatGeoPoint(location,
                                         CoordinateFormat::DDMMSS);
    const WideToUTF8Converter location_str(formatted.c_str());
    if (flarm_id != 0) {
      LogFormat("Traffic received: id=%u flarm_id=%u at %s altitude %dm track=%u° turn_rate=%.1f type=%u",
                pilot_id, flarm_id, location_str.c_str(), altitude, track, turn_rate, (unsigned)aircraft_type);
    } else {
      LogFormat("Traffic received: id=%u at %s altitude %dm track=%u° turn_rate=%.1f type=%u",
                pilot_id, location_str.c_str(), altitude, track, turn_rate, (unsigned)aircraft_type);
    }
  } else {
    LogFormat("Traffic received: id=%u altitude %dm (no location)",
              pilot_id, altitude);
  }

  bool user_known;

  {
    const std::lock_guard lock{skylines_data.mutex};
    const SkyLinesTracking::Data::Traffic traffic(SkyLinesTracking::Data::Time{time_of_day_ms},
                                                  location, altitude, flarm_id);
    skylines_data.traffic[pilot_id] = traffic;

    user_known = skylines_data.IsUserKnown(pilot_id);
  }

  // Feed into traffic aggregator
  if (net_components != nullptr && net_components->traffic_aggregator != nullptr &&
      location.IsValid()) {
    // Determine source: if flarm_id is set, it's from xcsoar-cloud (OGN)
    // Otherwise it's from regular SkyLines
    UnifiedTraffic::Source source = flarm_id != 0
      ? UnifiedTraffic::Source::OGN_CLOUD
      : UnifiedTraffic::Source::SKYLINES;

    FlarmId flarm_id_obj = flarm_id != 0
      ? FlarmId::FromValue(flarm_id)
      : FlarmId::Undefined();

    // If no FlarmId, we can't deduplicate - skip aggregator
    // (will still be stored in SkyLines data for backward compatibility)
    if (flarm_id_obj.IsDefined()) {
      // Convert time_of_day_ms to TimeStamp
      // time_of_day_ms is milliseconds since midnight UTC
      // TimeStamp is FloatDuration (seconds) since midnight
      // For traffic aggregation, we use the time_of_day directly
      // The actual absolute time doesn't matter for deduplication/expiration
      const TimeStamp timestamp = TimeStamp{FloatDuration{time_of_day_ms / 1000.0}};

      // Extract track and speed from SkyLines data if available
      // Track is now passed as parameter
      unsigned speed = 0;
      int climb_rate = 0;

      net_components->traffic_aggregator->FeedOGN(
        source, pilot_id, flarm_id_obj, location, altitude,
        track, speed, climb_rate, timestamp,
        turn_rate, aircraft_type);
    }
  }

  if (!user_known)
    /* we don't know this user's name yet - try to find it out by
       asking the server */
    skylines.RequestUserName(pilot_id);
}

void
TrackingGlue::OnUserName(uint32_t user_id, const TCHAR *name)
{
  const std::lock_guard lock{skylines_data.mutex};
  skylines_data.user_names[user_id] = name;
}

void
TrackingGlue::OnWave(unsigned time_of_day_ms,
                     const GeoPoint &a, const GeoPoint &b)
{
  const std::lock_guard lock{skylines_data.mutex};

  /* garbage collection - hard-coded upper limit */
  auto n = skylines_data.waves.size();
  while (n-- >= 64)
    skylines_data.waves.pop_front();

  // TODO: replace existing item?
  skylines_data.waves.emplace_back(SkyLinesTracking::Data::Time{time_of_day_ms},
                                   a, b);
}

void
TrackingGlue::OnThermal([[maybe_unused]] unsigned time_of_day_ms,
                        const AGeoPoint &bottom, const AGeoPoint &top,
                        double lift)
{
  const std::lock_guard lock{skylines_data.mutex};

  /* garbage collection - hard-coded upper limit */
  auto n = skylines_data.thermals.size();
  while (n-- >= 64)
    skylines_data.thermals.pop_front();

  // TODO: replace existing item?
  skylines_data.thermals.emplace_back(bottom, top, lift);
}

void
TrackingGlue::OnSkyLinesError(std::exception_ptr e)
{
  LogError(e, "SkyLines error");
}
