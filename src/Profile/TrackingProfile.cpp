/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifdef HAVE_TRACKING

namespace Profile {
  static void Load(SkyLinesTracking::Settings &settings) {
    Get(ProfileSkyLinesTrackingEnabled, settings.enabled);

    const TCHAR *key = Get(ProfileSkyLinesTrackingKey);
    if (key != NULL) {
#ifdef _UNICODE
      settings.key = wcstoull(key, NULL, 16);
#else
      settings.key = strtoull(key, NULL, 16);
#endif
    }
  }
}

void
Profile::Load(LiveTrack24Settings &settings)
{
  Get(ProfileLiveTrack24Enabled, settings.enabled);
  settings.server = Get(ProfileLiveTrack24Server, _T("www.livetrack24.com"));
  settings.username = Get(ProfileLiveTrack24Username, _T(""));
  settings.password = Get(ProfileLiveTrack24Password, _T(""));
}

void
Profile::Load(TrackingSettings &settings)
{
  Get(ProfileTrackingInterval, settings.interval);
  GetEnum(ProfileTrackingVehicleType, settings.vehicleType);
  Load(settings.skylines);
  Load(settings.livetrack24);
}

#endif /* HAVE_TRACKING */
