// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "util/StaticArray.hxx"
#include "Geo/GeoPoint.hpp"

#include <vector>

class AirspaceWarningCopy
{
private:
  StaticArray<const AbstractAirspace *,64> ids_inside, ids_warning, ids_acked;
  StaticArray<GeoPoint,32> locations;

  /* List of cleared airspaces.
     These are user-set with no upper limit, and not reduced by warning-manager 
     proximity heuristics, so use a vector with a small reservation. */
  std::vector<const AbstractAirspace *> ids_cleared;

  Serial serial;

public:
  AirspaceWarningCopy() noexcept {
    ids_cleared.reserve(16);
  }

  auto GetSerial() const noexcept {
    return serial;
  }

  void Visit(const AirspaceWarning& as) noexcept {
    if (as.IsInside()) {
      ids_inside.checked_append(&as.GetAirspace());
    } else if (as.IsWarning()) {
      ids_warning.checked_append(&as.GetAirspace());
      if (as.IsAckExpired())
        locations.checked_append(as.GetSolution().location);
    }

    if (!as.IsAckExpired())
      ids_acked.checked_append(&as.GetAirspace());

    if (as.IsCleared())
      ids_cleared.push_back(&as.GetAirspace());
  }

  void Visit(const AirspaceWarningManager &awm) noexcept {
    serial = awm.GetSerial();

    for (const auto &i : awm)
      Visit(i);
  }

  void Visit(const ProtectedAirspaceWarningManager &awm) noexcept {
    const ProtectedAirspaceWarningManager::Lease lease(awm);
    Visit(lease);
  }

  const StaticArray<GeoPoint,32> &GetLocations() const noexcept {
    return locations;
  }

  bool HasWarning(const AbstractAirspace &as) const noexcept {
    return as.IsActive() && Find(as, ids_warning);
  }

  bool IsAcked(const AbstractAirspace &as) const noexcept {
    return (!as.IsActive()) || Find(as, ids_acked);
  }

  bool IsInside(const AbstractAirspace &as) const noexcept {
    return as.IsActive() && Find(as, ids_inside);
  }

  bool IsCleared(const AbstractAirspace &as) const noexcept {
    for (const auto *p : ids_cleared)
      if (p == &as)
        return true;
    return false;
  }

private:
  bool Find(const AbstractAirspace& as,
            const StaticArray<const AbstractAirspace *,64> &list) const noexcept {
    return list.contains(&as);
  }
};
