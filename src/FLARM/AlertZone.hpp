// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Id.hpp"
#include "Traffic.hpp"
#include "Geo/GeoPoint.hpp"
#include "Language/Language.hpp"
#include "NMEA/Validity.hpp"
#include "util/TrivialArray.hxx"

#include <type_traits>
#include <cstdint>

/**
 * A FLARM Alert Zone (vertical cylinder) received via PFLAO.
 * @see FTD-012 Data Port ICD, section 8.13
 */
struct FlarmAlertZone {
  FlarmTraffic::AlarmType alarm_level;

  /** True if we are inside the zone's horizontal and vertical limits */
  bool inside;

  /** Center of the cylinder in WGS84 */
  GeoPoint center;

  /** Radius of the cylinder in meters (0-2000) */
  unsigned radius;

  /** Bottom of the cylinder in meters above WGS84 ellipsoid */
  int bottom;

  /** Top of the cylinder in meters above WGS84 ellipsoid */
  int top;

  /**
   * End of activity in seconds since 1970-01-01 00:00 UTC.
   * 0 = no set end time.
   */
  uint32_t activity_limit;

  FlarmId id;

  FlarmTraffic::IdType id_type;

  /** Zone type (hex 0x10-0xFF) */
  uint8_t zone_type;

  /** When was this zone last received? */
  Validity valid;

  static constexpr const char *GetZoneTypeString(uint8_t type) noexcept {
    switch (type) {
    case 0x41:
      return N_("Skydiver drop zone");
    case 0x42:
      return N_("Aerodrome traffic zone");
    case 0x43:
      return N_("Military firing area");
    case 0x44:
      return N_("Kite flying zone");
    case 0x45:
      return N_("Winch launching area");
    case 0x46:
      return N_("RC flying area");
    case 0x47:
      return N_("UAS flying area");
    case 0x48:
      return N_("Aerobatic box");
    case 0x7E:
      return N_("Danger area");
    case 0x7F:
      return N_("Prohibited area");
    default:
      return N_("Alert zone");
    }
  }
};

/**
 * Container for FLARM Alert Zones received via PFLAO sentences.
 */
struct FlarmAlertZoneList {
  static constexpr size_t MAX_COUNT = 15;

  Validity modified;

  TrivialArray<FlarmAlertZone, MAX_COUNT> list;

  constexpr void Clear() noexcept {
    modified.Clear();
    list.clear();
  }

  constexpr void Complement(const FlarmAlertZoneList &add) noexcept {
    if (add.modified.Modified(modified))
      modified = add.modified;

    if (list.empty() && !add.list.empty()) {
      list = add.list;
      return;
    }

    for (const auto &zone : add.list) {
      if (FindZone(zone.id) == nullptr) {
        FlarmAlertZone *slot = AllocateZone();
        if (slot == nullptr)
          return;
        *slot = zone;
      }
    }
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    modified.Expire(clock, std::chrono::minutes(5));

    for (unsigned i = list.size(); i-- > 0;)
      list[i].valid.Expire(clock, std::chrono::minutes(5));

    for (unsigned i = list.size(); i-- > 0;)
      if (!list[i].valid.IsValid())
        list.quick_remove(i);
  }

  constexpr FlarmAlertZone *FindZone(FlarmId id) noexcept {
    for (auto &zone : list)
      if (zone.id == id)
        return &zone;
    return nullptr;
  }

  constexpr const FlarmAlertZone *FindZone(FlarmId id) const noexcept {
    for (const auto &zone : list)
      if (zone.id == id)
        return &zone;
    return nullptr;
  }

  constexpr FlarmAlertZone *AllocateZone() noexcept {
    return list.full() ? nullptr : &list.append();
  }
};

static_assert(std::is_trivial<FlarmAlertZone>::value,
              "type is not trivial");
static_assert(std::is_trivial<FlarmAlertZoneList>::value,
              "type is not trivial");
