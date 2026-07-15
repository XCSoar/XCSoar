// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrackingGlue.hpp"
#include "Tracking/SkyLines/FlarmTrafficBuilder.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Info.hpp"
#include "LogFile.hpp"

#include <chrono>

using namespace std::chrono;

static constexpr int MAX_ONLINE_TRAFFIC_ALTITUDE_SEPARATION = 5000;

[[gnu::pure]]
static int
OwnAltitudeMeters(const NMEAInfo &basic) noexcept
{
  if (basic.baro_altitude_available)
    return int(basic.baro_altitude);

  if (basic.gps_altitude_available)
    return int(basic.gps_altitude);

  return -1;
}

[[gnu::pure]]
static bool
WithinOnlineAltitudeBand(int own_alt, int target_alt,
                         bool target_altitude_valid) noexcept
{
  if (own_alt < 0)
    return !target_altitude_valid;

  if (!target_altitude_valid)
    return false;

  const int separation = target_alt - own_alt;
  return separation <= MAX_ONLINE_TRAFFIC_ALTITUDE_SEPARATION &&
    separation >= -MAX_ONLINE_TRAFFIC_ALTITUDE_SEPARATION;
}

static constexpr auto ONLINE_BUFFER_STALE = minutes(5);

/**
 * Configured own FLARM ids plus the device radio id when known.
 */
static StaticArray<FlarmId, CloudSettings::MAX_OWN_FLARM_IDS + 1>
MakeEffectiveOwnFlarmIds(const CloudSettings::OwnFlarmIdList &configured,
                         FlarmId radio_id) noexcept
{
  StaticArray<FlarmId, CloudSettings::MAX_OWN_FLARM_IDS + 1> ids;
  for (const FlarmId id : configured)
    ids.append(id);
  if (radio_id.IsDefined() && !ids.contains(radio_id))
    ids.append(radio_id);
  return ids;
}

TrackingGlue::TrackingGlue(EventLoop &event_loop,
                           CurlGlobal &curl) noexcept
  :skylines(event_loop, this),
   livetrack24(curl)
{
  online_traffic.Clear();
}

void
TrackingGlue::SetSettings(const TrackingSettings &_settings)
{
  cloud_enabled = _settings.cloud.enabled;
  cloud_show_traffic = _settings.cloud.show_traffic;

  {
    const std::lock_guard lock{online_mutex};
    configured_own_flarm_ids = _settings.cloud.own_flarm_ids;
    own_flarm_ids = MakeEffectiveOwnFlarmIds(configured_own_flarm_ids,
                                             device_radio_id);

    if (cloud_enabled != TriState::TRUE || !cloud_show_traffic) {
      online_traffic.Clear();
      online_pilot_ids.clear();
      online_last_received.clear();
    }
  }

  skylines.SetSettings(_settings.skylines, _settings.cloud);
  livetrack24.SetSettings(_settings.livetrack24);
}

void
TrackingGlue::BeginShutdown() noexcept
{
  if (shutting_down)
    return;

  shutting_down = true;
  skylines.BeginShutdown();
  livetrack24.BeginShutdown();
}

void
TrackingGlue::OnTimer(const MoreData &basic, const DerivedInfo &calculated)
{
  if (shutting_down)
    return;

  {
    const std::lock_guard lock{online_mutex};
    own_altitude = OwnAltitudeMeters(basic);
    device_radio_id = basic.flarm.hardware.radio_id;
    own_flarm_ids = MakeEffectiveOwnFlarmIds(configured_own_flarm_ids,
                                             device_radio_id);
  }

  try {
    skylines.Tick(basic, calculated);
  } catch (...) {
    LogError(std::current_exception(), "SkyLines error");
  }

  livetrack24.OnTimer(basic, calculated);
}

void
TrackingGlue::MergeOnlineTraffic(FlarmData &flarm,
                                 const NMEAInfo &basic) noexcept
{
  flarm.traffic.ClampListSize();

  const std::lock_guard lock{online_mutex};

  own_altitude = OwnAltitudeMeters(basic);
  device_radio_id = flarm.hardware.radio_id;
  own_flarm_ids = MakeEffectiveOwnFlarmIds(configured_own_flarm_ids,
                                           device_radio_id);

  online_traffic.ClampListSize();

  const auto now = steady_clock::now();

  for (unsigned i = 0; i < online_traffic.list.size(); ) {
    const FlarmTraffic &t = online_traffic.list[i];
    const auto last_i = online_last_received.find(t.id);
    if (last_i == online_last_received.end() ||
        now - last_i->second > ONLINE_BUFFER_STALE ||
        !WithinOnlineAltitudeBand(own_altitude, int(t.altitude),
                                  t.altitude_available) ||
        SkyLinesTracking::FlarmTrafficBuilder::IsOwnShipId(own_flarm_ids,
                                                           t.id)) {
      online_last_received.erase(t.id);
      online_pilot_ids.erase(t.id);
      online_traffic.list.quick_remove(i);
    } else
      ++i;
  }

  for (const auto &online : online_traffic.list) {
    FlarmTraffic *existing = flarm.traffic.FindTraffic(online.id);
    if (existing != nullptr &&
        !FlarmTraffic::IsInjectedSource(existing->source) &&
        existing->valid)
      continue;

    FlarmTraffic built = online;
    if (!SkyLinesTracking::FlarmTrafficBuilder::FillRelative(built, basic))
      continue;

    if (existing != nullptr) {
      existing->UpdateOnline(built);
      if (basic.time_available)
        existing->valid.Update(basic.time);
    } else {
      FlarmTraffic *slot = flarm.traffic.AllocateTraffic();
      if (slot == nullptr)
        continue;

      *slot = built;
      if (basic.time_available) {
        slot->valid.Update(basic.time);
        flarm.traffic.new_traffic.Update(basic.time);
      }
    }

    if (basic.time_available)
      flarm.traffic.modified.Update(basic.time);
  }
}

uint32_t
TrackingGlue::GetOnlinePilotId(FlarmId id) const noexcept
{
  const std::lock_guard lock{online_mutex};
  const auto i = online_pilot_ids.find(id);
  return i != online_pilot_ids.end() ? i->second : 0;
}

void
TrackingGlue::OnTraffic(uint32_t pilot_id,
                        [[maybe_unused]] unsigned time_of_day_ms,
                        const GeoPoint &location, int altitude,
                        bool altitude_valid,
                        SkyLinesTracking::TrafficSource source,
                        unsigned track_deg, bool track_valid,
                        FlarmId flarm_id, unsigned aircraft_type)
{
  if (source == SkyLinesTracking::TrafficSource::CLOUD) {
    if (cloud_enabled != TriState::TRUE || !cloud_show_traffic)
      return;
  }

  int own_alt;
  StaticArray<FlarmId, CloudSettings::MAX_OWN_FLARM_IDS + 1> own_ids;
  {
    const std::lock_guard lock{online_mutex};
    own_alt = own_altitude;
    own_ids = own_flarm_ids;
  }

  if (!WithinOnlineAltitudeBand(own_alt, altitude, altitude_valid))
    return;

  StaticString<64> server_name_buffer;
  CopyOnlineUserName(pilot_id, server_name_buffer);
  const char *server_name = server_name_buffer.empty()
    ? nullptr
    : server_name_buffer.c_str();

  FlarmTraffic built = SkyLinesTracking::FlarmTrafficBuilder::Build(
    pilot_id, location, altitude, altitude_valid, source,
    track_deg, track_valid, flarm_id, aircraft_type, server_name);

  if (!built.location_available)
    return;

  if (SkyLinesTracking::FlarmTrafficBuilder::IsOwnShipId(own_ids, built.id))
    return;

  bool user_known;

  {
    const std::lock_guard lock{online_mutex};

    online_traffic.ClampListSize();

    FlarmTraffic *slot = online_traffic.FindTraffic(built.id);
    if (slot == nullptr) {
      slot = online_traffic.AllocateTraffic();
      if (slot == nullptr)
        return;

      slot->Clear();
      slot->id = built.id;
    }

    slot->UpdateOnline(built);

    online_pilot_ids[built.id] = pilot_id;
    online_last_received[built.id] = steady_clock::now();
  }

  {
    const std::lock_guard lock{skylines_data.mutex};
    user_known = skylines_data.IsUserKnown(pilot_id);
  }

  if (!user_known)
    skylines.RequestUserName(pilot_id);
}

void
TrackingGlue::OnUserName(uint32_t user_id, const char *name)
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
