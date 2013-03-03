/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Util/Macros.hpp"

#ifdef HAVE_LIVETRACK24

static LiveTrack24::VehicleType
MapVehicleTypeToLifetrack24(TrackingSettings::VehicleType vt)
{
  static constexpr LiveTrack24::VehicleType vehicleTypeMap[] = {
    LiveTrack24::VehicleType::GLIDER,
    LiveTrack24::VehicleType::PARAGLIDER,
    LiveTrack24::VehicleType::POWERED_AIRCRAFT,
    LiveTrack24::VehicleType::HOT_AIR_BALLOON
  };

  unsigned vti = (unsigned) vt;
  if (vti >= ARRAY_SIZE(vehicleTypeMap))
    vti = 0;

  return vehicleTypeMap[vti];
}

#endif

TrackingGlue::TrackingGlue()
#ifdef HAVE_LIVETRACK24
  :last_timestamp(0),
   flying(false)
#endif
{
  settings.SetDefaults();
#ifdef HAVE_LIVETRACK24
  LiveTrack24::SetServer(settings.livetrack24.server);
#endif

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  skylines.SetHandler(this);
#endif
}

#ifdef HAVE_LIVETRACK24

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

#endif

void
TrackingGlue::SetSettings(const TrackingSettings &_settings)
{
#ifdef HAVE_SKYLINES_TRACKING
  skylines.SetSettings(_settings.skylines);
#endif

#ifdef HAVE_LIVETRACK24
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
#endif
}

void
TrackingGlue::OnTimer(const MoreData &basic, const DerivedInfo &calculated)
{
#ifdef HAVE_SKYLINES_TRACKING
  skylines.Tick(basic);
#endif

#ifdef HAVE_LIVETRACK24
  if (!settings.livetrack24.enabled)
    /* disabled by configuration */
    /* note that we are allowed to read "settings" without locking the
       mutex, because the background thread never writes to this
       attribute */
    return;

  if (!basic.time_available || !basic.location_available)
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
  if (!basic.date_available)
    /* use "today" if the GPS didn't provide a date */
    (BrokenDate &)date_time = BrokenDate::TodayUTC();

  location = basic.location;
  /* XXX use nav_altitude? */
  altitude = basic.NavAltitudeAvailable() && positive(basic.nav_altitude)
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
#endif
}

#ifdef HAVE_LIVETRACK24

void
TrackingGlue::Tick()
{
  if (!settings.livetrack24.enabled)
    /* settings have been cleared meanwhile, bail out */
    return;

  unsigned tracking_interval = settings.interval;
  LiveTrack24Settings copy = this->settings.livetrack24;

  mutex.Unlock();

  if (!flying) {
    if (last_flying && state.HasSession()) {
      /* landing: end tracking session */
      LiveTrack24::EndTracking(state.session_id, state.packet_id);
      state.ResetSession();
      last_timestamp = 0;
    }

    /* don't track if not flying */
    mutex.Lock();
    return;
  }

  const int64_t current_timestamp = date_time.ToUnixTimeUTC();

  if (state.HasSession() && current_timestamp + 60 < last_timestamp) {
    /* time warp: create a new session */
    LiveTrack24::EndTracking(state.session_id, state.packet_id);
    state.ResetSession();
  }

  last_timestamp = current_timestamp;

  if (!state.HasSession()) {
    LiveTrack24::UserID user_id = 0;
    if (!copy.username.empty() && !copy.password.empty())
      user_id = LiveTrack24::GetUserID(copy.username, copy.password);

    if (user_id == 0) {
      copy.username.clear();
      copy.password.clear();
      state.session_id = LiveTrack24::GenerateSessionID();
    } else {
      state.session_id = LiveTrack24::GenerateSessionID(user_id);
    }

    if (!LiveTrack24::StartTracking(state.session_id, copy.username,
                                    copy.password, tracking_interval,
                                    MapVehicleTypeToLifetrack24(settings.vehicleType))) {
      state.ResetSession();
      mutex.Lock();
      return;
    }

    state.packet_id = 2;
  }

  LiveTrack24::SendPosition(state.session_id, state.packet_id++,
                            location, altitude, ground_speed, track,
                            current_timestamp);

  mutex.Lock();
}

#endif

#ifdef HAVE_SKYLINES_TRACKING_HANDLER

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

#endif
