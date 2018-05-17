/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Builder.hpp"
#include "MapItem.hpp"
#include "List.hpp"
#include "Util/StaticArray.hxx"
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
    if (as.GetWarningState() > AirspaceWarning::WARNING_CLEAR)
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

class AirspaceAtPointPredicate: public AirspacePredicate
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

    const AbstractAirspace &airspace = i.GetAirspace();
    if (predicate(airspace))
      list.append(new AirspaceMapItem(airspace));
  }
}
