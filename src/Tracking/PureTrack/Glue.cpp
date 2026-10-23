// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"

#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "LogFile.hpp"
#include "co/Task.hxx"
#include "lib/curl/Global.hxx"
#include "net/State.hpp"
#include "util/Macros.hpp"

namespace PureTrack {

Glue::Glue(CurlGlobal &curl) noexcept
  :client(curl),
   task(curl.GetEventLoop())
{
  settings.SetDefaults();
}

void
Glue::BeginShutdown() noexcept
{
  task.BeginShutdown();
}

void
Glue::OnTimer(const MoreData &basic,
              [[maybe_unused]] const DerivedInfo &calculated)
{
  if (task.IsShuttingDown())
    return;

  if (!settings.enabled || settings.app_key.empty() ||
      settings.device_id.empty())
    return;

  if (GetNetState() == NetState::DISCONNECTED)
    return;

  if (!basic.time_available || !basic.gps.real ||
      !basic.location_available)
    return;

  /* A running insert must not consume the next interval. */
  if (task.IsRunning())
    return;

  if (settings.interval == 0 ||
      !clock.CheckUpdate(std::chrono::seconds(settings.interval)))
    return;

  Sample sample;
  sample.timestamp = basic.date_time_utc.IsDatePlausible()
    ? basic.date_time_utc.ToTimePoint()
    : std::chrono::system_clock::now();
  sample.location = basic.location;
  sample.altitude = basic.NavAltitudeAvailable()
    ? basic.nav_altitude
    : 0;
  sample.speed = basic.ground_speed_available
    ? basic.ground_speed
    : 0;
  sample.course = basic.track_available
    ? basic.track.AsBearing().Degrees()
    : 0;
  sample.vertical_speed = basic.brutto_vario_available
    ? basic.brutto_vario
    : 0;

  task.Start(Tick(settings, sample), BIND_THIS_METHOD(OnCompletion));
}

Co::InvokeTask
Glue::Tick(Settings _settings, Sample sample)
{
  co_await client.Insert(_settings, sample);
}

void
Glue::OnCompletion(std::exception_ptr error) noexcept
{
  if (error)
    LogError(error, "PureTrack error");
}

} // namespace PureTrack
