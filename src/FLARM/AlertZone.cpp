// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AlertZone.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Engine/Airspace/Ptr.hpp"
#include "TransponderCode.hpp"
#include "Geo/AltitudeReference.hpp"
#include "util/Macros.hpp"
#include "util/tstring.hpp"
#include <tchar.h>
#include <memory>

static constexpr const TCHAR *zoneTypes[] = {
    _T("Other"),
    _T("Skydiver Drop Zone"),
    _T("Aerodrome Traffic Zone"),
    _T("Military Firing Area"),
    _T("Kite Flying Zone"),
    _T("Winch Launching Area"),
    _T("RC Flying Area"),
    _T("UAS Flying Area"),
    _T("Aerobatic Box"),
    _T("Generic Danger Area"),
    _T("Generic Prohibited Area"),
};

static_assert(ARRAY_SIZE(zoneTypes) == 11,
              "Wrong array size");

const TCHAR *
FlarmAlertZone::GetZoneTypeString(ZoneType type) noexcept
{
  switch (type) {
  case ZoneType::SKYDIVER_DROP_ZONE:
    return zoneTypes[1];
  case ZoneType::AERODROME_TRAFFIC_ZONE:
    return zoneTypes[2];
  case ZoneType::MILITARY_FIRING_AREA:
    return zoneTypes[3];
  case ZoneType::KITE_FLYING_ZONE:
    return zoneTypes[4];
  case ZoneType::WINCH_LAUNCHING_AREA:
    return zoneTypes[5];
  case ZoneType::RC_FLYING_AREA:
    return zoneTypes[6];
  case ZoneType::UAS_FLYING_AREA:
    return zoneTypes[7];
  case ZoneType::AEROBATIC_BOX:
    return zoneTypes[8];
  case ZoneType::GENERIC_DANGER_AREA:
    return zoneTypes[9];
  case ZoneType::GENERIC_PROHIBITED_AREA:
    return zoneTypes[10];
  case ZoneType::OTHER:
  default:
    return zoneTypes[0];
  }
}

static AirspaceClass
MapZoneTypeToAirspaceClass(FlarmAlertZone::ZoneType zone_type) noexcept
{
  switch (zone_type) {
  case FlarmAlertZone::ZoneType::MILITARY_FIRING_AREA:
  case FlarmAlertZone::ZoneType::GENERIC_DANGER_AREA:
    return AirspaceClass::DANGER;

  case FlarmAlertZone::ZoneType::GENERIC_PROHIBITED_AREA:
    return AirspaceClass::PROHIBITED;

  case FlarmAlertZone::ZoneType::SKYDIVER_DROP_ZONE:
  case FlarmAlertZone::ZoneType::AERODROME_TRAFFIC_ZONE:
  case FlarmAlertZone::ZoneType::KITE_FLYING_ZONE:
  case FlarmAlertZone::ZoneType::WINCH_LAUNCHING_AREA:
  case FlarmAlertZone::ZoneType::RC_FLYING_AREA:
  case FlarmAlertZone::ZoneType::UAS_FLYING_AREA:
  case FlarmAlertZone::ZoneType::AEROBATIC_BOX:
  case FlarmAlertZone::ZoneType::OTHER:
  default:
    return AirspaceClass::RESTRICTED;
  }
}

std::shared_ptr<AbstractAirspace>
FlarmAlertZone::ToAirspace() const noexcept
{
  if (!IsDefined() || !center.IsValid() || radius <= 0)
    return nullptr;

  auto airspace = std::make_shared<AirspaceCircle>(center, (double)radius);

  /* Alert zones use WGS84 ellipsoid altitudes, convert to MSL */
  AirspaceAltitude base;
  base.reference = AltitudeReference::MSL;
  base.altitude = (double)bottom;
  base.flight_level = 0;
  base.altitude_above_terrain = 0;

  AirspaceAltitude top_alt;
  top_alt.reference = AltitudeReference::MSL;
  top_alt.altitude = (double)top;
  top_alt.flight_level = 0;
  top_alt.altitude_above_terrain = 0;

  /* Map zone type to airspace class */
  AirspaceClass asclass = MapZoneTypeToAirspaceClass(zone_type);

  /* Set name from zone ID */
  char id_buffer[10];
  tstring name = id.Format(id_buffer);

  /* Set properties */
  airspace->SetProperties(std::move(name), tstring{},
                          TransponderCode::Null(),
                          asclass, asclass, base, top_alt);

  return airspace;
}
