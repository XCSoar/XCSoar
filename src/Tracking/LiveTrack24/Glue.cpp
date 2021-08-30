/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Glue.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Units/System.hpp"
#include "Operation/Operation.hpp"
#include "LogFile.hpp"
#include "co/Task.hxx"
#include "net/http/Global.hxx"
#include "util/Macros.hpp"

namespace LiveTrack24 {

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
  :client(curl),
   inject_task(curl.GetEventLoop())
{
  settings.SetDefaults();
  client.SetServer(settings.server);
}

void
Glue::SetSettings(const Settings &_settings)
{
  if (_settings.server != settings.server ||
      _settings.username != settings.username ||
      _settings.password != settings.password) {
    /* wait for the current job to finish */
    inject_task.Cancel();

    /* now it's safe to access these variables */
    settings = _settings;
    state.ResetSession();
    client.SetServer(_settings.server);
  } else {
    /* no fundamental setting changes */
    settings = _settings;
  }
}

void
Glue::OnTimer(const MoreData &basic, const DerivedInfo &calculated)
{
  if (!settings.enabled)
    /* disabled by configuration */
    /* note that we are allowed to read "settings" without locking the
       mutex, because the background thread never writes to this
       attribute */
    return;

  if (!basic.time_available || !basic.gps.real || !basic.location_available)
    /* can't track without a valid GPS fix */
    return;

  if (!clock.CheckUpdate(std::chrono::seconds(settings.interval)))
    /* later */
    return;

  if (inject_task)
    /* still running, skip this submission */
    return;

  date_time = basic.date_time_utc;
  if (!date_time.IsDatePlausible())
    /* use "today" if the GPS didn't provide a date */
    (BrokenDate &)date_time = BrokenDate::TodayUTC();

  location = basic.location;
  /* XXX use nav_altitude? */
  altitude = basic.NavAltitudeAvailable() && basic.nav_altitude > 0
    ? (unsigned)basic.nav_altitude
    : 0u;
  ground_speed = basic.ground_speed_available
    ? (unsigned)Units::ToUserUnit(basic.ground_speed, Unit::KILOMETER_PER_HOUR)
    : 0u;
  track = basic.track_available
    ? basic.track
    : Angle::Zero();

  last_flying = flying;
  flying = calculated.flight.flying;

  inject_task.Start(Tick(settings), BIND_THIS_METHOD(OnCompletion));
}

Co::InvokeTask
Glue::Tick(Settings settings)
{
  assert(settings.enabled);

  if (!flying) {
    if (last_flying && state.HasSession()) {
      /* landing: end tracking session */
      co_await client.EndTracking(state.session_id, state.packet_id);
      state.ResetSession();
      last_timestamp = {};
    }

    /* don't track if not flying */
    co_return;
  }

  const auto current_timestamp = date_time.ToTimePoint();

  if (state.HasSession() &&
      current_timestamp + std::chrono::minutes(1) < last_timestamp) {
    /* time warp: create a new session */
    const auto old_state = state;
    state.ResetSession();
    co_await client.EndTracking(old_state.session_id, old_state.packet_id);
  }

  last_timestamp = current_timestamp;

  if (!state.HasSession()) {
    UserID user_id = 0;
    if (!settings.username.empty() && !settings.password.empty())
      user_id = co_await client.GetUserID(settings.username, settings.password);

    if (user_id == 0) {
      settings.username.clear();
      settings.password.clear();
      state.session_id = GenerateSessionID();
    } else {
      state.session_id = GenerateSessionID(user_id);
    }

    try {
      co_await client.StartTracking(state.session_id, settings.username,
                                    settings.password, settings.interval,
                                    MapVehicleTypeToLivetrack24(settings.vehicleType),
                                    settings.vehicle_name);
    } catch (...) {
      state.ResetSession();
      throw;
    }

    state.packet_id = 2;
  }

  co_await client.SendPosition(state.session_id, state.packet_id++,
                               location, altitude, ground_speed, track,
                               current_timestamp);
}

void
Glue::OnCompletion(std::exception_ptr error) noexcept
{
  if (error)
    LogError(error, "LiveTrack24 error");
}

} // namespace Livetrack24
