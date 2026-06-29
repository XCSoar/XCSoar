// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrackingGlue.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "NMEA/MoreData.hpp"
#include "LogFile.hpp"

#include <cstdlib>
#include <cstring>

/* TEMPORARY: remove before upstream merge */
static bool
CloudDebugEnabled() noexcept
{
  const char *value = std::getenv("XCS_CLOUD_DEBUG");
  return value != nullptr && std::strcmp(value, "1") == 0;
}

TrackingGlue::TrackingGlue(EventLoop &event_loop,
                           CurlGlobal &curl) noexcept
  :skylines(event_loop, this),
   livetrack24(curl)
{
}

void
TrackingGlue::SetSettings(const TrackingSettings &_settings)
{
  cloud_enabled = _settings.cloud.enabled;
  cloud_show_traffic = _settings.cloud.show_traffic;

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

  try {
    skylines.Tick(basic, calculated);
  } catch (...) {
    LogError(std::current_exception(), "SkyLines error");
  }

  livetrack24.OnTimer(basic, calculated);
}

void
TrackingGlue::OnTraffic(uint32_t pilot_id, unsigned time_of_day_ms,
                        const GeoPoint &location, int altitude,
                        SkyLinesTracking::TrafficSource source)
{
  if (source == SkyLinesTracking::TrafficSource::CLOUD) {
    if (cloud_enabled != TriState::TRUE || !cloud_show_traffic)
      return;

    if (CloudDebugEnabled())
      LogFmt("Cloud: TRAFFIC_RESPONSE pilot_id={:#x} alt={}m lat={} lon={}",
             pilot_id, altitude,
             location.latitude.Degrees(),
             location.longitude.Degrees());
  }

  bool user_known;

  {
    const std::lock_guard lock{skylines_data.mutex};
    const SkyLinesTracking::Data::Traffic traffic(SkyLinesTracking::Data::Time{time_of_day_ms},
                                                  location, altitude);
    skylines_data.traffic[pilot_id] = traffic;

    user_known = skylines_data.IsUserKnown(pilot_id);
  }

  if (!user_known)
    /* we don't know this user's name yet - try to find it out by
       asking the server */
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
