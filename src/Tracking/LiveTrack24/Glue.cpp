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
  :StandbyThread("Tracking"),
   client(curl)
{
  settings.SetDefaults();
  client.SetServer(settings.server);
}

void
Glue::StopAsync()
{
  std::lock_guard<Mutex> lock(mutex);
  StandbyThread::StopAsync();
}

void
Glue::WaitStopped()
{
  std::lock_guard<Mutex> lock(mutex);
  StandbyThread::WaitStopped();
}

void
Glue::SetSettings(const Settings &_settings)
{
  if (_settings.server != settings.server ||
      _settings.username != settings.username ||
      _settings.password != settings.password) {
    /* wait for the current job to finish */
    LockWaitDone();

    /* now it's safe to access these variables without a lock */
    settings = _settings;
    state.ResetSession();
    client.SetServer(_settings.server);
  } else {
    /* no fundamental setting changes; the write needs to be protected
       by the mutex, because another job may be running already */
    std::lock_guard<Mutex> lock(mutex);
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

  std::lock_guard<Mutex> lock(mutex);
  if (IsBusy())
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

  Trigger();
}

void
Glue::Tick() noexcept
{
  if (!settings.enabled)
    /* settings have been cleared meanwhile, bail out */
    return;

  unsigned tracking_interval = settings.interval;
  auto copy = settings;

  const ScopeUnlock unlock(mutex);

  QuietOperationEnvironment env;

  try {
    if (!flying) {
      if (last_flying && state.HasSession()) {
        /* landing: end tracking session */
        client.EndTracking(state.session_id, state.packet_id, env);
        state.ResetSession();
        last_timestamp = {};
      }

      /* don't track if not flying */
      return;
    }

    const auto current_timestamp = date_time.ToTimePoint();

    if (state.HasSession() &&
        current_timestamp + std::chrono::minutes(1) < last_timestamp) {
      /* time warp: create a new session */
      client.EndTracking(state.session_id, state.packet_id, env);
      state.ResetSession();
    }

    last_timestamp = current_timestamp;

    if (!state.HasSession()) {
      UserID user_id = 0;
      if (!copy.username.empty() && !copy.password.empty())
        user_id = client.GetUserID(copy.username, copy.password,
                                               env);

      if (user_id == 0) {
        copy.username.clear();
        copy.password.clear();
        state.session_id = GenerateSessionID();
      } else {
        state.session_id = GenerateSessionID(user_id);
      }

      if (!client.StartTracking(state.session_id, copy.username,
                                            copy.password, tracking_interval,
                                            MapVehicleTypeToLivetrack24(settings.vehicleType),
                                            settings.vehicle_name,
                                            env)) {
        state.ResetSession();
        return;
      }

      state.packet_id = 2;
    }

    client.SendPosition(state.session_id, state.packet_id++,
                                    location, altitude, ground_speed, track,
                                    current_timestamp,
                                    env);
  } catch (...) {
    LogError(std::current_exception(), "LiveTrack24 error");
  }
}

} // namespace Livetrack24
