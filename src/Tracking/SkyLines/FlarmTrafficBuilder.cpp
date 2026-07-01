// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlarmTrafficBuilder.hpp"
#include "TrafficDisplay.hpp"
#include "TrafficExtensions.hpp"
#include "Geo/GeoVector.hpp"
#include "Math/Angle.hpp"
#include "util/StringCompare.hxx"

FlarmId
SkyLinesTracking::FlarmTrafficBuilder::ResolveId(uint32_t pilot_id,
                                                 FlarmId flarm_id) noexcept
{
  if (flarm_id.IsDefined())
    return flarm_id;

  return FlarmId::FromValue(pilot_id & 0xffffffu);
}

FlarmTraffic::SourceType
SkyLinesTracking::FlarmTrafficBuilder::SourceForOnline(
  uint32_t pilot_id, TrafficSource source) noexcept
{
  if (source == TrafficSource::SKYLINES)
    return FlarmTraffic::SourceType::SKYLINES;

  if ((pilot_id & OGN_PILOT_ID_MASK) != 0)
    return FlarmTraffic::SourceType::OGN;

  return FlarmTraffic::SourceType::CLOUD;
}

bool
SkyLinesTracking::FlarmTrafficBuilder::FillRelative(FlarmTraffic &traffic,
                                                    const NMEAInfo &basic) noexcept
{
  if (!basic.location_available || !traffic.location.IsValid())
    return false;

  const GeoVector vec = basic.location.DistanceBearing(traffic.location);
  const auto sc = vec.bearing.SinCos();
  traffic.relative_north = vec.distance * sc.second;
  traffic.relative_east = vec.distance * sc.first;

  /* DrawFLARMTraffic skips targets with relative_east==0 */
  if (traffic.relative_east == 0 && traffic.relative_north != 0)
    traffic.relative_east = 1e-3;

  traffic.location_available = true;

  if (traffic.altitude_available) {
    if (basic.gps_altitude_available)
      traffic.relative_altitude =
        traffic.altitude - RoughAltitude(basic.gps_altitude);
    else
      traffic.relative_altitude = traffic.altitude;
  }

  return true;
}

FlarmTraffic
SkyLinesTracking::FlarmTrafficBuilder::Build(uint32_t pilot_id,
                                             const GeoPoint &location,
                                             int altitude,
                                             bool altitude_available,
                                             TrafficSource source,
                                             unsigned track_deg,
                                             bool track_valid,
                                             FlarmId flarm_id,
                                             unsigned aircraft_type,
                                             const char *server_name) noexcept
{
  FlarmTraffic traffic{};

  traffic.id = ResolveId(pilot_id, flarm_id);
  traffic.id_type = flarm_id.IsDefined()
    ? FlarmTraffic::IdType::FLARM
    : FlarmTraffic::IdType::UNKNOWN;
  traffic.source = SourceForOnline(pilot_id, source);

  traffic.location = location;
  traffic.location_available = location.IsValid();
  traffic.altitude = RoughAltitude(altitude);
  traffic.altitude_available = altitude_available;

  if (track_valid && track_deg <= 359) {
    traffic.track = RoughAngle(Angle::Degrees(track_deg));
    traffic.track_received = true;
  }

  if (aircraft_type <= 15 && aircraft_type != 14)
    traffic.type = FlarmTraffic::AircraftType(aircraft_type);
  else
    traffic.type = FlarmTraffic::AircraftType::UNKNOWN;

  StaticString<64> title;
  FormatTrafficTitle(title, pilot_id, flarm_id, server_name);
  if (!title.empty())
    traffic.name.SetUTF8(title.c_str());

  traffic.alarm_level = FlarmTraffic::AlarmType::NONE;
  traffic.stealth = false;
  traffic.no_track = false;

  return traffic;
}
