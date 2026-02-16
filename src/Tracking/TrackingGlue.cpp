// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrackingGlue.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "NMEA/MoreData.hpp"
#include "LogFile.hpp"

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
                        const GeoPoint &location, int altitude)
{
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
