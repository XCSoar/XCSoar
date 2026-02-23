// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/Ptr.hpp"
#include "FLARM/Id.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/DeltaTime.hpp"

#include <map>

class Airspaces;
struct ComputerSettings;
struct FlarmData;
struct MoreData;
struct DerivedInfo;
struct AirspaceWarningsInfo;

/**
 * Manage airspace warnings.
 */
class WarningComputer {
  DeltaTime delta_time;

  Airspaces &airspaces;

  AirspaceWarningManager manager;
  ProtectedAirspaceWarningManager protected_manager;

  bool initialised;

  /**
   * Cached AirspaceCircle objects for FLARM alert zones, keyed by
   * FlarmId.  Reusing the same shared_ptr across ticks preserves
   * warning acknowledgement state.  Since AirspaceCircle is immutable,
   * it must be replaced when the zone geometry/metadata changes.
   */
  struct FlarmZoneCache {
    AirspacePtr airspace;
    GeoPoint center;
    double radius;
    unsigned bottom, top;
    uint8_t zone_type;
  };

  std::map<FlarmId, FlarmZoneCache> flarm_zone_airspaces;

public:
  WarningComputer(const AirspaceWarningConfig &_config,
                  Airspaces &_airspaces);

  ProtectedAirspaceWarningManager &GetManager() {
    return protected_manager;
  }

  const ProtectedAirspaceWarningManager &GetManager() const {
    return protected_manager;
  }

  void Reset() {
    delta_time.Reset();
    initialised = false;
  }

  void Update(const ComputerSettings &settings_computer,
              const MoreData &basic,
              const DerivedInfo &calculated,
              AirspaceWarningsInfo &result);

private:
  void UpdateFlarmZones(const FlarmData &flarm,
                        AirspaceWarningManager &mgr);
};
