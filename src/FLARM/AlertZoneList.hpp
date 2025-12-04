// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AlertZone.hpp"
#include "NMEA/Validity.hpp"
#include "util/TrivialArray.hxx"

#include <type_traits>

class Airspaces;

/**
 * This class keeps track of the alert zones received from a FLARM.
 */
struct AlertZoneList {
  static constexpr size_t MAX_COUNT = 10;

  /**
   * Time stamp of the latest modification to this object.
   */
  Validity modified;

  /** Flarm alert zone information */
  TrivialArray<FlarmAlertZone, MAX_COUNT> list;

  constexpr void Clear() noexcept {
    modified.Clear();
    list.clear();
  }

  constexpr bool IsEmpty() const noexcept {
    return list.empty();
  }

  /**
   * Looks up an item in the alert zone list.
   *
   * @param id FLARM id
   * @return the FlarmAlertZone pointer, nullptr if not found
   */
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

  /**
   * Allocates a new slot in the list.
   *
   * @return Pointer to the new slot, or nullptr if the list is full
   */
  constexpr FlarmAlertZone *AllocateZone() noexcept {
    return list.full()
      ? nullptr
      : &list.append();
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    modified.Expire(clock, std::chrono::minutes(5));

    for (unsigned i = list.size(); i-- > 0;)
      if (!list[i].Refresh(clock))
        list.quick_remove(i);
  }

  constexpr unsigned GetActiveZoneCount() const noexcept {
    return list.size();
  }

  /**
   * Update an Airspaces container with the current alert zones.
   * Converts all valid zones to AirspaceCircle objects and adds them.
   * @param airspaces The Airspaces container to update
   */
  void UpdateAirspaces(Airspaces &airspaces) const noexcept;
};

static_assert(std::is_trivial<AlertZoneList>::value, "type is not trivial");

