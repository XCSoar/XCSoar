// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/Traffic.hpp"

#include "util/Macros.hpp"

#include <cstddef>

namespace {

struct SourceStringEntry {
  FlarmTraffic::SourceType type;
  const char *label;
};

constexpr SourceStringEntry source_strings[] = {
  { FlarmTraffic::SourceType::FLARM, "FLARM" },
  { FlarmTraffic::SourceType::ADSB, "ADS-B" },
  { FlarmTraffic::SourceType::ADSR, "ADS-R" },
  { FlarmTraffic::SourceType::TISB, "TIS-B" },
  { FlarmTraffic::SourceType::MODES, "Mode-S" },
  { FlarmTraffic::SourceType::OGN, "OGN" },
  { FlarmTraffic::SourceType::SKYLINES, "SkyLines" },
  { FlarmTraffic::SourceType::CLOUD, "Cloud" },
};

static_assert(ARRAY_SIZE(source_strings) == 8,
              "source_strings must list every SourceType label");

} // namespace

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
  for (const auto &entry : source_strings) {
    if (entry.type == source)
      return entry.label;
  }

  return "Unknown";
}

bool
FlarmTraffic::IsInjectedSource(SourceType source) noexcept
{
  return source == SourceType::OGN ||
    source == SourceType::SKYLINES ||
    source == SourceType::CLOUD;
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

void
FlarmTraffic::UpdateOnline(const FlarmTraffic &built) noexcept
{
  location = built.location;
  location_available = built.location_available;
  altitude = built.altitude;
  altitude_available = built.altitude_available;
  name = built.name;
  source = built.source;
  type = built.type;
  id_type = built.id_type;
  relative_north = built.relative_north;
  relative_east = built.relative_east;
  relative_altitude = built.relative_altitude;
  if (built.track_received) {
    track = built.track;
    track_received = true;
  }
}
