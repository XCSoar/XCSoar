// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "AppKey.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Units/System.hpp"
#include "Operation/Operation.hpp"
#include "LogFile.hpp"
#include "Version.hpp"
#include "co/Task.hxx"
#include "lib/curl/Global.hxx"
#include "net/State.hpp"
#include "util/Macros.hpp"

#include <cmath>

namespace LiveTrack24 {

/**
 * Sent instead of a device serial number; the protocol only needs a
 * stable string that is not "0".
 */
static constexpr const char *DEVICE_ID = "xcsoar";

static VehicleType
MapVehicleTypeToLivetrack24(Settings::VehicleType vt)
{
  static constexpr VehicleType vehicleTypeMap[] = {
    VehicleType::GLIDER,
    VehicleType::PARAGLIDER,
    VehicleType::POWERED_AIRCRAFT,
    VehicleType::HOT_AIR_BALLOON,
    VehicleType::FLEX_WING_FAI1,
    VehicleType::RIGID_WING_FAI5,
  };

  unsigned vti = (unsigned) vt;
  if (vti >= ARRAY_SIZE(vehicleTypeMap))
    vti = 0;

  return vehicleTypeMap[vti];
}

Glue::Glue(CurlGlobal &curl) noexcept
  :client(curl, AppInfo{GetAppKey(), XCSoar_Version, DEVICE_ID}),
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
Glue::SetSettings(const Settings &_settings)
{
  if (_settings.username != settings.username ||
      _settings.password != settings.password) {
    /* wait for the current job to finish */
    task.Cancel();

    /* now it's safe to access these variables */
    settings = _settings;
    session = {};
  } else {
    /* no fundamental setting changes */
    settings = _settings;
  }
}

void
Glue::OnTimer(const MoreData &basic, const DerivedInfo &calculated)
{
  if (task.IsShuttingDown())
    return;

  if (!settings.enabled)
    /* disabled by configuration */
    /* note that we are allowed to read "settings" without locking the
       mutex, because the background thread never writes to this
       attribute */
    return;

  if (!HasAppKey())
    /* this build has no LiveTrack24 key */
    return;

  if (settings.username.empty() || settings.password.empty())
    /* the server only takes tracks of a logged-in pilot */
    return;

  if (GetNetState() == NetState::DISCONNECTED)
    /* no link; do not start HTTP to LiveTrack24 (see SkyLines Glue) */
    return;

  if (!basic.time_available || !basic.gps.real || !basic.location_available)
    /* can't track without a valid GPS fix */
    return;

  if (!clock.CheckUpdate(std::chrono::seconds(settings.interval)))
    /* later */
    return;

  if (task.IsRunning())
    /* still running, skip this submission */
    return;

  date_time = basic.date_time_utc;
  if (!date_time.IsDatePlausible())
    /* use "today" if the GPS didn't provide a date */
    (BrokenDate &)date_time = BrokenDate::TodayUTC();

  location = basic.location;
  altitude = basic.NavAltitudeAvailable()
    ? (int)std::lround(basic.nav_altitude)
    : 0;
  ground_speed = basic.ground_speed_available
    ? (unsigned)std::lround(Units::ToUserUnit(basic.ground_speed,
                                              Unit::KILOMETER_PER_HOUR))
    : 0u;
  course = basic.track_available
    ? (unsigned)std::lround(basic.track.AsBearing().Degrees()) % 360
    : 0u;

  last_flying = flying;
  flying = calculated.flight.flying;

  task.Start(Tick(settings), BIND_THIS_METHOD(OnCompletion));
}

Co::InvokeTask
Glue::Tick(Settings settings)
{
  assert(settings.enabled);

  if (!flying) {
    if (last_flying && session.IsDefined()) {
      /* landing: end tracking session */
      const Session old_session = std::exchange(session, Session{});
      last_timestamp = {};
      co_await client.EndTracking(old_session, packet_id);
    }

    /* don't track if not flying */
    co_return;
  }

  const auto current_timestamp = date_time.ToTimePoint();

  if (session.IsDefined() &&
      current_timestamp + std::chrono::minutes(1) < last_timestamp) {
    /* time warp: create a new session */
    const Session old_session = std::exchange(session, Session{});
    co_await client.EndTracking(old_session, packet_id);
  }

  last_timestamp = current_timestamp;

  if (!session.IsDefined()) {
    Session new_session;
    new_session.user = co_await client.GetUserID(settings.username,
                                                 settings.password,
                                                 GetAppSecret());
    new_session.id = GenerateSessionID();
    new_session.token = MakeToken(settings.password.c_str(),
                                  new_session.id, GetAppSecret());
    session = std::move(new_session);
    packet_id = 1;
  }

  const Fix fix{current_timestamp, location, altitude, ground_speed, course};

  if (packet_id == 1) {
    /* only the first packet describes the track */
    const TrackInfo info{
      MapVehicleTypeToLivetrack24(settings.vehicleType),
      settings.vehicle_name.c_str(),
      "XCSoar",
    };

    try {
      co_await client.SendPosition(session, packet_id, fix, &info);
    } catch (...) {
      /* start over, so the next attempt sends the track info again */
      session = {};
      throw;
    }
  } else
    co_await client.SendPosition(session, packet_id, fix, nullptr);

  ++packet_id;
}

void
Glue::OnCompletion(std::exception_ptr error) noexcept
{
  if (error)
    LogError(error, "LiveTrack24 error");
}

} // namespace LiveTrack24
