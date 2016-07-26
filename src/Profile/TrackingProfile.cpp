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

#include "TrackingProfile.hpp"
#include "Map.hpp"
#include "ProfileKeys.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "Util/NumberParser.hpp"

#ifdef HAVE_TRACKING

namespace Profile {
static void Load(const ProfileMap &map,
                 SkyLinesTracking::CloudSettings &settings) {
  bool bvalue;
  settings.enabled = map.Get(ProfileKeys::CloudEnabled, bvalue)
    ? (bvalue ? TriState::TRUE : TriState::FALSE)
    : TriState::UNKNOWN;

  map.Get(ProfileKeys::CloudShowThermals, settings.show_thermals);

  const char *key = map.Get(ProfileKeys::CloudKey);
  if (key != nullptr)
    settings.key = ParseUint64(key, nullptr, 16);
}

static void Load(const ProfileMap &map,
                 SkyLinesTracking::Settings &settings) {
  map.Get(ProfileKeys::SkyLinesTrackingEnabled, settings.enabled);
  map.Get(ProfileKeys::SkyLinesRoaming, settings.roaming);
  map.Get(ProfileKeys::SkyLinesTrackingInterval, settings.interval);
  map.Get(ProfileKeys::SkyLinesTrafficEnabled, settings.traffic_enabled);

  const char *key = map.Get(ProfileKeys::SkyLinesTrackingKey);
  if (key != NULL)
    settings.key = ParseUint64(key, NULL, 16);

  Load(map, settings.cloud);
}

static void Load(const ProfileMap &map,
                 LiveTrack24::Settings &settings) {
  map.Get(ProfileKeys::LiveTrack24Enabled, settings.enabled);

  if (!map.Get(ProfileKeys::LiveTrack24Server, settings.server))
    settings.server = _T("www.livetrack24.com");
  else if (StringIsEqual(settings.server, _T("livexc.dhv1.de"))) {
    // DHV tracking server moved to new host (#3208)
    settings.server = _T("livexc.dhv.de");
  }

  map.Get(ProfileKeys::LiveTrack24Username, settings.username);
  map.Get(ProfileKeys::LiveTrack24Password, settings.password);
}
}

void
Profile::Load(const ProfileMap &map, TrackingSettings &settings)
{
  map.Get(ProfileKeys::TrackingInterval, settings.interval);
  map.GetEnum(ProfileKeys::TrackingVehicleType, settings.vehicleType);
  map.Get(ProfileKeys::TrackingVehicleName, settings.vehicle_name);
  Load(map, settings.skylines);
  Load(map, settings.livetrack24);
}

#endif /* HAVE_TRACKING */
