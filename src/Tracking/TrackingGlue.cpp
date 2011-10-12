/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "NMEA/Info.hpp"
#include "Units/Units.hpp"

static const unsigned tracking_interval_s = 60;

struct NMEAInfo;

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
  ScopeLock protect(mutex);

  if (_settings.livetrack24.username != settings.username ||
      _settings.livetrack24.password != settings.password)
    user_id = 0;

  settings = _settings.livetrack24;
}

void
TrackingGlue::OnTimer(const NMEAInfo &basic)
{
  if (!settings.IsDefined())
    /* disabled by configuration */
    /* note that we are allowed to read "settings" without locking the
       mutex, because the background thread never writes to this
       attribute */
    return;

  if (!basic.time_available || !basic.location_available)
    /* can't track without a valid GPS fix */
    return;

  if (!clock.check_update(tracking_interval_s * 1000))
    /* later */
    return;

  ScopeLock protect(mutex);
  if (IsBusy())
    /* still running, skip this submission */
    return;

  date_time = basic.date_time_utc;
  if (!basic.date_available)
    /* use "today" if the GPS didn't provide a date */
    (BrokenDate &)date_time = (BrokenDate)BrokenDateTime::NowUTC();

  location = basic.location;
  /* XXX use nav_altitude? */
  altitude = basic.gps_altitude_available && positive(basic.gps_altitude)
    ? (unsigned)basic.gps_altitude
    : 0u;
  ground_speed = basic.ground_speed_available
    ? (unsigned)Units::ToUserUnit(basic.ground_speed, unKiloMeterPerHour)
    : 0u;
  track = basic.track_available
    ? basic.track
    : Angle::Zero();

  Trigger();
}

void
TrackingGlue::Tick()
{
  if (!settings.IsDefined())
    /* settings have been cleared meanwhile, bail out */
    return;

  const LiveTrack24Settings copy = this->settings;

  mutex.Unlock();

  if (user_id == 0) {
    user_id = LiveTrack24::GetUserID(copy.username, copy.password);
    if (user_id == 0) {
      mutex.Lock();
      return;
    }

    session_id = LiveTrack24::GenerateSessionID(user_id);

    if (!StartTracking(session_id, copy.username, copy.password,
                       tracking_interval_s,
                       LiveTrack24::VT_GLIDER, _T("XCSoar"))) {
      user_id = 0;
      mutex.Lock();
      return;
    }

    packet_id = 2;
  }

  LiveTrack24::SendPosition(session_id, packet_id++,
                            location, altitude, ground_speed, track,
                            date_time.ToUnixTimeUTC());

  mutex.Lock();
}
