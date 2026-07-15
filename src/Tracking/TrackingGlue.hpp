// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/Features.hpp"

#ifdef HAVE_TRACKING

#include "Tracking/SkyLines/Handler.hpp"
#include "FLARM/Id.hpp"
#include "FLARM/List.hpp"
#include "FLARM/Data.hpp"
#include "Tracking/SkyLines/Glue.hpp"
#include "Tracking/SkyLines/Data.hpp"
#include "Tracking/LiveTrack24/Glue.hpp"
#include "util/StaticString.hxx"
#include "util/TriState.hpp"
#include "thread/Mutex.hxx"

#include <chrono>
#include <map>

struct TrackingSettings;
struct MoreData;
struct DerivedInfo;
struct NMEAInfo;
class CurlGlobal;

class TrackingGlue final
  : private SkyLinesTracking::Handler
{
  SkyLinesTracking::Glue skylines;

  SkyLinesTracking::Data skylines_data;

  LiveTrack24::Glue livetrack24;

  mutable Mutex online_mutex;

  TrafficList online_traffic;

  /** SkyLines pilot_id for online #FlarmId (display labels). */
  std::map<FlarmId, uint32_t> online_pilot_ids;

  /** Last network update per online target (online buffer GC). */
  std::map<FlarmId, std::chrono::steady_clock::time_point> online_last_received;

  bool shutting_down = false;

  TriState cloud_enabled = TriState::UNKNOWN;
  bool cloud_show_traffic = true;

  /** Own-ship altitude [m MSL] for online-traffic filtering; -1 if unknown. */
  int own_altitude = -1;

  /** Own-ship FLARM radio id when known (filters OGN self). */
  FlarmId own_flarm_id = FlarmId::Undefined();

public:
  TrackingGlue(EventLoop &event_loop, CurlGlobal &curl) noexcept;

  void SetSettings(const TrackingSettings &_settings);

  void BeginShutdown() noexcept;

  void OnTimer(const MoreData &basic, const DerivedInfo &calculated);

  void MergeOnlineTraffic(FlarmData &flarm,
                          const NMEAInfo &basic) noexcept;

  const SkyLinesTracking::Data &GetSkyLinesData() const {
    return skylines_data;
  }

  /**
   * SkyLines pilot_id for an online traffic #FlarmId, or 0 if unknown.
   */
  [[gnu::pure]]
  uint32_t GetOnlinePilotId(FlarmId id) const noexcept;

  /**
   * Copy the server display name for a SkyLines pilot_id into @dest.
   * Clears @dest when unknown.
   */
  template<std::size_t N>
  void CopyOnlineUserName(uint32_t pilot_id,
                          StaticString<N> &dest) const noexcept
  {
    dest.clear();

    const std::lock_guard lock{skylines_data.mutex};
    const auto i = skylines_data.user_names.find(pilot_id);
    if (i == skylines_data.user_names.end() || i->second.empty())
      return;

    dest = i->second;
  }

private:
  /* virtual methods from SkyLinesTracking::Handler */
  void OnTraffic(uint32_t pilot_id, unsigned time_of_day_ms,
                 const GeoPoint &location, int altitude,
                 bool altitude_valid,
                 SkyLinesTracking::TrafficSource source,
                 unsigned track_deg, bool track_valid,
                 FlarmId flarm_id, unsigned aircraft_type) override;
  void OnUserName(uint32_t user_id, const char *name) override;
  void OnWave(unsigned time_of_day_ms,
              const GeoPoint &a, const GeoPoint &b) override;
  void OnThermal(unsigned time_of_day_ms,
                 const AGeoPoint &bottom, const AGeoPoint &top,
                 double lift) override;
  void OnSkyLinesError(std::exception_ptr e) override;
};

#endif /* HAVE_TRACKING */
