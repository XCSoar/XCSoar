// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Id.hpp"
#include "Geo/GeoPoint.hpp"
#include "NMEA/Validity.hpp"
#include "Rough/RoughAltitude.hpp"
#include "Rough/RoughDistance.hpp"

#include <cstdint>
#include <memory>
#include <tchar.h>

class AbstractAirspace;

struct FlarmAlertZone {
  enum class ZoneType: uint8_t {
    SKYDIVER_DROP_ZONE = 0x41,
    AERODROME_TRAFFIC_ZONE = 0x42,
    MILITARY_FIRING_AREA = 0x43,
    KITE_FLYING_ZONE = 0x44,
    WINCH_LAUNCHING_AREA = 0x45,
    RC_FLYING_AREA = 0x46,
    UAS_FLYING_AREA = 0x47,
    AEROBATIC_BOX = 0x48,
    GENERIC_DANGER_AREA = 0x7E,
    GENERIC_PROHIBITED_AREA = 0x7F,
    OTHER = 0x10,  // Any other value 10-FF
  };

  enum class AlarmType: uint8_t {
    NONE = 0,
    LOW = 1,
    IMPORTANT = 2,
    URGENT = 3,
  };

  /** Center location of the alert zone */
  GeoPoint center;

  /** Radius of the cylinder in meters */
  RoughDistance radius;

  /** Bottom altitude in meters above WGS84 ellipsoid */
  RoughAltitude bottom;

  /** Top altitude in meters above WGS84 ellipsoid */
  RoughAltitude top;

  /** End of activity in seconds since epoch (0 = no end time) */
  uint32_t activity_limit;

  /** FLARM ID of the alert zone station */
  FlarmId id;

  /** Type of the alert zone */
  ZoneType zone_type;

  /** Alarm level */
  AlarmType alarm_level;

  /** Is the aircraft currently inside the zone? */
  bool inside;

  /** Is this zone valid, or has it expired already? */
  Validity valid;

  bool IsDefined() const noexcept {
    return valid;
  }

  bool HasAlarm() const noexcept {
    return alarm_level != AlarmType::NONE;
  }

  void Clear() noexcept {
    valid.Clear();
    center = GeoPoint::Invalid();
    radius = 0;
    bottom = 0;
    top = 0;
    activity_limit = 0;
    id.Clear();
    zone_type = ZoneType::OTHER;
    alarm_level = AlarmType::NONE;
    inside = false;
  }

  bool Refresh(TimeStamp clock) noexcept {
    valid.Expire(clock, std::chrono::seconds(2));
    return valid;
  }

  static const TCHAR *GetZoneTypeString(ZoneType type) noexcept;

  /**
   * Convert this alert zone to an AirspaceCircle object.
   * @return AirspacePtr to the created airspace, or nullptr if invalid
   */
  std::shared_ptr<AbstractAirspace> ToAirspace() const noexcept;
};

