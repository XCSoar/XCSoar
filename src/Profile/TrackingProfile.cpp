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

#include "TrackingProfile.hpp"
#include "Profile.hpp"
#include "ProfileKeys.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "Util/NumberParser.hpp"

#ifdef HAVE_TRACKING

#ifdef HAVE_SKYLINES_TRACKING
namespace Profile {
  static void Load(SkyLinesTracking::Settings &settings) {
    Get(ProfileKeys::SkyLinesTrackingEnabled, settings.enabled);
    Get(ProfileKeys::SkyLinesTrackingInterval, settings.interval);
    Get(ProfileKeys::SkyLinesTrafficEnabled, settings.traffic_enabled);

    const char *key = Get(ProfileKeys::SkyLinesTrackingKey);
    if (key != NULL)
      settings.key = ParseUint64(key, NULL, 16);
  }
}
#endif

#ifdef HAVE_LIVETRACK24

void
Profile::Load(LiveTrack24Settings &settings)
{
  Get(ProfileKeys::LiveTrack24Enabled, settings.enabled);

  if (!Get(ProfileKeys::LiveTrack24Server, settings.server))
    settings.server = _T("www.livetrack24.com");

  Get(ProfileKeys::LiveTrack24Username, settings.username);
  Get(ProfileKeys::LiveTrack24Password, settings.password);
}

#endif

void
Profile::Load(TrackingSettings &settings)
{
#ifdef HAVE_LIVETRACK24
  Get(ProfileKeys::TrackingInterval, settings.interval);
  GetEnum(ProfileKeys::TrackingVehicleType, settings.vehicleType);
#endif
#ifdef HAVE_SKYLINES_TRACKING
  Load(settings.skylines);
#endif
#ifdef HAVE_LIVETRACK24
  Load(settings.livetrack24);
#endif
}

#endif /* HAVE_TRACKING */
