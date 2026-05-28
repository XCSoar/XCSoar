// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "util/StaticArray.hxx"
#include "Geo/GeoPoint.hpp"

#include <optional>
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

  const AirspaceWarningConfig *warning_config = nullptr;

  Serial serial;

  /* Current aircraft altitude, if known. When set, IsClearedAtCurrentAltitude()
     additionally checks that the aircraft is within the cleared airspace's
     vertical band. */
  std::optional<AltitudeState> altitude_state;

public:
  AirspaceWarningCopy() noexcept {
    ids_cleared.reserve(16);
  }

  auto GetSerial() const noexcept {
    return serial;
  }

  void SetAltitudeState(const AltitudeState &state) noexcept {
    altitude_state = state;
  }

  void Visit(const AirspaceWarning& as) noexcept {
    if (!as.IsCoveredByClearance() && !as.IsCleared()) {
      if (as.IsInside()) {
        ids_inside.checked_append(&as.GetAirspace());
      } else if (as.IsWarning()) {
        ids_warning.checked_append(&as.GetAirspace());
        if (as.IsAckExpired())
          locations.checked_append(as.GetSolution().location);
      }

      if (!as.IsAckExpired())
        ids_acked.checked_append(&as.GetAirspace());
    } else if (!as.IsCleared() && as.HasExplicitAck()) {
      // Covered by clearance but explicitly acked: suppress fill
      ids_acked.checked_append(&as.GetAirspace());
    }

    if (as.IsCleared())
      ids_cleared.push_back(&as.GetAirspace());
  }

  void Visit(const AirspaceWarningManager &awm) noexcept {
    serial = awm.GetSerial();
    warning_config = &awm.GetConfig();

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

  /**
   * Returns true if the airspace is cleared AND the current aircraft
   * altitude is within its vertical band. If no altitude state is set,
   * falls back to the plain IsCleared() check. Renderers use this so
   * that a clearance only affects drawing when it covers the current
   * flight level.
   */
  bool IsClearedAtCurrentAltitude(const AbstractAirspace &as) const noexcept {
    if (!IsCleared(as))
      return false;
    if (!altitude_state.has_value())
      return true;
    return as.Inside(*altitude_state);
  }

  bool IsWarningCapable(const AbstractAirspace &as) const noexcept {
    return warning_config != nullptr &&
      (warning_config->IsClassEnabled(as.GetClassOrType()) ||
       warning_config->IsClassEnabled(as.GetTypeOrClass()));
  }

private:
  bool Find(const AbstractAirspace& as,
            const StaticArray<const AbstractAirspace *,64> &list) const noexcept {
    return list.contains(&as);
  }
};
