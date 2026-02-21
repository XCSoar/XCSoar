// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/Traffic.hpp"

static constexpr const char *acTypes[] = {
  "Unknown",
  "Glider",
  "TowPlane",
  "Helicopter",
  "Parachute",
  "DropPlane",
  "HangGlider",
  "ParaGlider",
  "PoweredAircraft",
  "JetAircraft",
  "FlyingSaucer",
  "Balloon",
  "Airship",
  "UAV",
  "Unknown",
  "StaticObject" 
};

const char *
FlarmTraffic::GetTypeString(AircraftType type) noexcept
{
  std::size_t index = static_cast<std::size_t>(type);
  if (index < std::size(acTypes))
    return acTypes[index];

  return NULL;
}

const char *
FlarmTraffic::GetSourceString(SourceType source) noexcept
{
  switch (source) {
  case SourceType::FLARM:
    return "FLARM";
  case SourceType::ADSB:
    return "ADS-B";
  case SourceType::ADSR:
    return "ADS-R";
  case SourceType::TISB:
    return "TIS-B";
  case SourceType::MODES:
    return "Mode-S";
  }

  return "Unknown";
}

void
FlarmTraffic::Update(const FlarmTraffic &other) noexcept
{
  alarm_level = other.alarm_level;
  relative_north = other.relative_north;
  relative_east = other.relative_east;
  relative_altitude = other.relative_altitude;
  track = other.track;
  track_received = other.track_received;
  turn_rate = other.turn_rate;
  turn_rate_received = other.turn_rate_received;
  speed = other.speed;
  speed_received = other.speed_received;
  climb_rate = other.climb_rate;
  climb_rate_received = other.climb_rate_received;
  stealth = other.stealth;
  type = other.type;
  source = other.source;
  id_type = other.id_type;
  rssi = other.rssi;
  rssi_available = other.rssi_available;
  no_track = other.no_track;
}
