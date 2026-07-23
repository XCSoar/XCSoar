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
  const AirspaceRendererSettings &renderer_settings;
  const AirspaceWarningList &warnings;
  const GeoPoint location;

public:
  AirspaceAtPointPredicate(const AirspaceRendererSettings &_renderer_settings,
                           const AirspaceWarningList &_warnings,
                           const GeoPoint _location)
    :renderer_settings(_renderer_settings),
     warnings(_warnings),
     location(_location) {}

  bool operator()(const AbstractAirspace& airspace) const {
    // Airspace should be visible or have a warning/inside status
    // and airspace needs to be at specified location

    return (IsAirspaceTypeOrClassVisible(airspace, renderer_settings) || warnings.Contains(airspace)) &&
      airspace.Inside(location);
  }
};

void
MapItemListBuilder::AddVisibleAirspace(
    const Airspaces &airspaces,
    const ProtectedAirspaceWarningManager *warning_manager,
    const AirspaceRendererSettings &renderer_settings)
{
  AirspaceWarningList warnings;
  if (warning_manager != nullptr)
    warnings.Fill(*warning_manager);

  AirspaceAtPointPredicate predicate(renderer_settings,
                                     warnings, location);

  for (const auto &i : airspaces.QueryWithinRange(location, 100)) {
    if (list.full())
      break;

    auto airspace = i.GetAirspacePtr();
    if (predicate(*airspace))
      list.append(new AirspaceMapItem(std::move(airspace)));
  }
}
