// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrackingProfile.hpp"
#include "Map.hpp"
#include "Keys.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "util/NumberParser.hxx"

#ifdef HAVE_TRACKING

namespace Profile {
static void Load(const ProfileMap &map, CloudSettings &settings) {
  bool bvalue;
  settings.enabled = map.Get(ProfileKeys::CloudEnabled, bvalue)
    ? (bvalue ? TriState::TRUE : TriState::FALSE)
    : TriState::UNKNOWN;

  map.Get(ProfileKeys::CloudShowTraffic, settings.show_traffic);
  map.Get(ProfileKeys::CloudShowThermals, settings.show_thermals);
  map.Get(ProfileKeys::CloudRoaming, settings.roaming);

  if (!map.Get(ProfileKeys::CloudHost, settings.host) ||
      settings.host.empty())
    settings.host = CloudSettings::DEFAULT_HOST;

  unsigned port = 0;
  if (!map.Get(ProfileKeys::CloudPort, port) ||
      port == 0 || port > 65535u)
    settings.port = CloudSettings::DEFAULT_PORT;
  else
    settings.port = port;

  const char *key = map.Get(ProfileKeys::CloudKey);
  if (key != nullptr)
    ParseIntegerTo(key, settings.key, 16);

  settings.own_flarm_ids =
    CloudSettings::ParseOwnFlarmIds(map.Get(ProfileKeys::CloudOwnFlarmId));
}

static void Load(const ProfileMap &map,
                 SkyLinesTracking::Settings &settings) {
  map.Get(ProfileKeys::SkyLinesTrackingEnabled, settings.enabled);
  map.Get(ProfileKeys::SkyLinesRoaming, settings.roaming);
  map.Get(ProfileKeys::SkyLinesTrackingInterval, settings.interval);
  map.Get(ProfileKeys::SkyLinesTrafficEnabled, settings.traffic_enabled);
  map.Get(ProfileKeys::SkyLinesNearTrafficEnabled, settings.near_traffic_enabled);

  const char *key = map.Get(ProfileKeys::SkyLinesTrackingKey);
  if (key != nullptr)
    ParseIntegerTo(key, settings.key, 16);
}

static void Load(const ProfileMap &map,
                 LiveTrack24::Settings &settings) {
  map.Get(ProfileKeys::LiveTrack24Enabled, settings.enabled);

  if (!map.Get(ProfileKeys::LiveTrack24Server, settings.server))
    settings.server = "www.livetrack24.com";
  else if (StringIsEqual(settings.server, "livexc.dhv1.de")) {
    // DHV tracking server moved to new host (#3208)
    settings.server = "livexc.dhv.de";
  }

  map.Get(ProfileKeys::LiveTrack24Username, settings.username);
  map.Get(ProfileKeys::LiveTrack24Password, settings.password);

  map.Get(ProfileKeys::LiveTrack24TrackingInterval, settings.interval);
  map.GetEnum(ProfileKeys::LiveTrack24TrackingVehicleType, settings.vehicleType);
  map.Get(ProfileKeys::LiveTrack24TrackingVehicleName, settings.vehicle_name);
}
}

void
Profile::Load(const ProfileMap &map, TrackingSettings &settings)
{
  Load(map, settings.skylines);
  Load(map, settings.cloud);
  Load(map, settings.livetrack24);
}

#endif /* HAVE_TRACKING */
