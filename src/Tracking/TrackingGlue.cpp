/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "TrackingGlue.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Units/System.hpp"
#include "Operation/Operation.hpp"
#include "LogFile.hpp"
#include "Util/Macros.hpp"

static LiveTrack24::VehicleType
MapVehicleTypeToLivetrack24(TrackingSettings::VehicleType vt)
{
  static constexpr LiveTrack24::VehicleType vehicleTypeMap[] = {
    LiveTrack24::VehicleType::GLIDER,
    LiveTrack24::VehicleType::PARAGLIDER,
    LiveTrack24::VehicleType::POWERED_AIRCRAFT,
    LiveTrack24::VehicleType::HOT_AIR_BALLOON,
    LiveTrack24::VehicleType::FLEX_WING_FAI1,
    LiveTrack24::VehicleType::RIGID_WING_FAI5,
  };

  unsigned vti = (unsigned) vt;
  if (vti >= ARRAY_SIZE(vehicleTypeMap))
    vti = 0;

  return vehicleTypeMap[vti];
}

TrackingGlue::TrackingGlue(boost::asio::io_service &io_service)
  :StandbyThread("Tracking"),
   skylines(io_service, this)
{
  settings.SetDefaults();
  LiveTrack24::SetServer(settings.livetrack24.server);
}

void
TrackingGlue::StopAsync()
{
  ScopeLock protect(mutex);
  StandbyThread::StopAsync();
}

void
TrackingGlue::WaitStopped()
{
  ScopeLock protect(mutex);
  StandbyThread::WaitStopped();
}

void
TrackingGlue::SetSettings(const TrackingSettings &_settings)
{
  skylines.SetSettings(_settings.skylines);

  if (_settings.livetrack24.server != settings.livetrack24.server ||
      _settings.livetrack24.username != settings.livetrack24.username ||
      _settings.livetrack24.password != settings.livetrack24.password) {
    /* wait for the current job to finish */
    LockWaitDone();

    /* now it's safe to access these variables without a lock */
    settings = _settings;
    state.ResetSession();
    LiveTrack24::SetServer(_settings.livetrack24.server);
  } else {
    /* no fundamental setting changes; the write needs to be protected
       by the mutex, because another job may be running already */
    ScopeLock protect(mutex);
    settings = _settings;
  }
}

void
TrackingGlue::OnTimer(const MoreData &basic, const DerivedInfo &calculated)
{
  try {
    skylines.Tick(basic, calculated);
  } catch (const std::runtime_error &e) {
    LogError("SkyLines error", e);
  }

  if (!settings.livetrack24.enabled)
    /* disabled by configuration */
    /* note that we are allowed to read "settings" without locking the
       mutex, because the background thread never writes to this
       attribute */
    return;

  if (!basic.time_available || !basic.gps.real || !basic.location_available)
    /* can't track without a valid GPS fix */
    return;

  if (!clock.CheckUpdate(settings.interval * 1000))
    /* later */
    return;

  ScopeLock protect(mutex);
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
TrackingGlue::Tick()
{
  if (!settings.livetrack24.enabled)
    /* settings have been cleared meanwhile, bail out */
    return;

  unsigned tracking_interval = settings.interval;
  LiveTrack24Settings copy = this->settings.livetrack24;

  const ScopeUnlock unlock(mutex);

  QuietOperationEnvironment env;

  try {
    if (!flying) {
      if (last_flying && state.HasSession()) {
        /* landing: end tracking session */
        LiveTrack24::EndTracking(state.session_id, state.packet_id, env);
        state.ResetSession();
        last_timestamp = 0;
      }

      /* don't track if not flying */
      return;
    }

    const int64_t current_timestamp = date_time.ToUnixTimeUTC();

    if (state.HasSession() && current_timestamp + 60 < last_timestamp) {
      /* time warp: create a new session */
      LiveTrack24::EndTracking(state.session_id, state.packet_id, env);
      state.ResetSession();
    }

    last_timestamp = current_timestamp;

    if (!state.HasSession()) {
      LiveTrack24::UserID user_id = 0;
      if (!copy.username.empty() && !copy.password.empty())
        user_id = LiveTrack24::GetUserID(copy.username, copy.password, env);

      if (user_id == 0) {
        copy.username.clear();
        copy.password.clear();
        state.session_id = LiveTrack24::GenerateSessionID();
      } else {
        state.session_id = LiveTrack24::GenerateSessionID(user_id);
      }

      if (!LiveTrack24::StartTracking(state.session_id, copy.username,
                                      copy.password, tracking_interval,
                                      MapVehicleTypeToLivetrack24(settings.vehicleType),
                                      settings.vehicle_name,
                                      env)) {
        state.ResetSession();
        return;
      }

      state.packet_id = 2;
    }

    LiveTrack24::SendPosition(state.session_id, state.packet_id++,
                              location, altitude, ground_speed, track,
                              current_timestamp,
                              env);
  } catch (const std::exception &exception) {
    LogError("LiveTrack24 error", exception);
  }
}

void
TrackingGlue::OnTraffic(uint32_t pilot_id, unsigned time_of_day_ms,
                        const GeoPoint &location, int altitude)
{
  bool user_known;

  {
    const ScopeLock protect(skylines_data.mutex);
    const SkyLinesTracking::Data::Traffic traffic(time_of_day_ms,
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
TrackingGlue::OnUserName(uint32_t user_id, const TCHAR *name)
{
  const ScopeLock protect(skylines_data.mutex);
  skylines_data.user_names[user_id] = name;
}

void
TrackingGlue::OnWave(unsigned time_of_day_ms,
                     const GeoPoint &a, const GeoPoint &b)
{
  const ScopeLock protect(skylines_data.mutex);

  /* garbage collection - hard-coded upper limit */
  auto n = skylines_data.waves.size();
  while (n-- >= 64)
    skylines_data.waves.pop_front();

  // TODO: replace existing item?
  skylines_data.waves.emplace_back(time_of_day_ms, a, b);
}

void
TrackingGlue::OnThermal(unsigned time_of_day_ms,
                        const AGeoPoint &bottom, const AGeoPoint &top,
                        double lift)
{
  const ScopeLock protect(skylines_data.mutex);

  /* garbage collection - hard-coded upper limit */
  auto n = skylines_data.thermals.size();
  while (n-- >= 64)
    skylines_data.thermals.pop_front();

  // TODO: replace existing item?
  skylines_data.thermals.emplace_back(bottom, top, lift);
}

void
TrackingGlue::OnSkyLinesError(const std::exception &e)
{
  LogError("SkyLines error", e);
}
