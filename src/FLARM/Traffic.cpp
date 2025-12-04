// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/Traffic.hpp"
#include "Language/Language.hpp"

static constexpr const TCHAR *acTypes[] = {
  _T("Unknown"),
  _T("Glider"),
  _T("TowPlane"),
  _T("Helicopter"),
  _T("Parachute"),
  _T("DropPlane"),
  _T("HangGlider"),
  _T("ParaGlider"),
  _T("PoweredAircraft"),
  _T("JetAircraft"),
  _T("FlyingSaucer"),
  _T("Balloon"),
  _T("Airship"),
  _T("UAV"),
  _T("Unknown"),
  _T("StaticObject")
};

const TCHAR *
FlarmTraffic::GetTypeString(AircraftType type) noexcept
{
  std::size_t index = static_cast<std::size_t>(type);
  if (index < std::size(acTypes))
    return acTypes[index];

  return NULL;
}

const TCHAR *
FlarmTraffic::GetSourceString(Source source) noexcept
{
  switch (source) {
  case Source::FLARM:
    return _("FLARM");
  case Source::ADS_B:
    return _T("ADS-B");
  case Source::ADS_R:
    return _T("ADS-R");
  case Source::TIS_B:
    return _T("TIS-B");
  case Source::MODE_S:
    return _T("Mode-S");
  default:
    return _("Unknown");
  }
}

const TCHAR *
FlarmTraffic::GetTrafficSourceTitle(const FlarmTraffic *traffic) noexcept
{
  if (traffic == nullptr || traffic->source == Source::UNKNOWN)
    return _("FLARM");

  return GetSourceString(traffic->source);
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
}
