// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "util/StaticArray.hxx"
#include "Geo/GeoPoint.hpp"

class AirspaceWarningCopy
{
private:
  StaticArray<const AbstractAirspace *,64> ids_inside, ids_warning, ids_acked;
  StaticArray<GeoPoint,32> locations;

  Serial serial;

public:
  auto GetSerial() const noexcept {
    return serial;
  }

  void Visit(const AirspaceWarning& as) noexcept {
    if (as.IsInside()) {
      ids_inside.checked_append(&as.GetAirspace());
    } else if (as.IsWarning()) {
      ids_warning.checked_append(&as.GetAirspace());
      locations.checked_append(as.GetSolution().location);
    }

    if (!as.IsAckExpired())
      ids_acked.checked_append(&as.GetAirspace());
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

private:
  bool Find(const AbstractAirspace& as,
            const StaticArray<const AbstractAirspace *,64> &list) const noexcept {
    return list.contains(&as);
  }
};
