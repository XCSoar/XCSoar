// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Builder.hpp"
#include "MapItem.hpp"
#include "List.hpp"
#include "util/StaticArray.hxx"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "NMEA/Aircraft.hpp"

class AirspaceWarningList
{
  StaticArray<const AbstractAirspace *,64> list;

public:
  void Add(const AirspaceWarning& as) {
    if (as.IsWarning())
      list.checked_append(&as.GetAirspace());
  }

  void Fill(const AirspaceWarningManager &awm) {
    for (const AirspaceWarning &as : awm)
      Add(as);
  }

  void Fill(const ProtectedAirspaceWarningManager &awm) {
    const ProtectedAirspaceWarningManager::Lease lease(awm);
    Fill(lease);
  }

  bool Contains(const AbstractAirspace& as) const {
    return list.contains(&as);
  }
};

class AirspaceAtPointPredicate
{
  const AirspaceVisibility visible_predicate;
  const AirspaceWarningList &warnings;
  const GeoPoint location;

public:
  AirspaceAtPointPredicate(const AirspaceComputerSettings &_computer_settings,
                           const AirspaceRendererSettings &_renderer_settings,
                           const AircraftState& _state,
                           const AirspaceWarningList &_warnings,
                           const GeoPoint _location)
    :visible_predicate(_computer_settings, _renderer_settings, _state),
     warnings(_warnings),
     location(_location) {}

  bool operator()(const AbstractAirspace& airspace) const {
    // Airspace should be visible or have a warning/inside status
    // and airspace needs to be at specified location

    return (visible_predicate(airspace) || warnings.Contains(airspace)) &&
      airspace.Inside(location);
  }
};

void
MapItemListBuilder::AddVisibleAirspace(
    const Airspaces &airspaces,
    const ProtectedAirspaceWarningManager *warning_manager,
    const AirspaceComputerSettings &computer_settings,
    const AirspaceRendererSettings &renderer_settings,
    const MoreData &basic, const DerivedInfo &calculated)
{
  AirspaceWarningList warnings;
  if (warning_manager != nullptr)
    warnings.Fill(*warning_manager);

  const AircraftState aircraft = ToAircraftState(basic, calculated);
  AirspaceAtPointPredicate predicate(computer_settings, renderer_settings,
                                     aircraft,
                                     warnings, location);

  for (const auto &i : airspaces.QueryWithinRange(location, 100)) {
    if (list.full())
      break;

    auto airspace = i.GetAirspacePtr();
    if (predicate(*airspace))
      list.append(new AirspaceMapItem(std::move(airspace)));
  }
}
