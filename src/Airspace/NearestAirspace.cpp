/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "NearestAirspace.hpp"
#include "ProtectedAirspaceWarningManager.hpp"
#include "Airspace/ActivePredicate.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceVisitor.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicateHeightRange.hpp"
#include "Engine/Airspace/Predicate/OutsideAirspacePredicate.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

gcc_pure
NearestAirspace
NearestAirspace::FindHorizontal(const MoreData &basic,
                                const ProtectedAirspaceWarningManager &airspace_warnings,
                                const Airspaces &airspace_database)
{
  if (!basic.location_available)
    /* can't check for airspaces without a GPS fix */
    return NearestAirspace();

  /* find the nearest airspace */
  //consider only active airspaces
  const WrapAirspacePredicate<ActiveAirspacePredicate> active_predicate(&airspace_warnings);
  const WrapAirspacePredicate<OutsideAirspacePredicate> outside_predicate(AGeoPoint(basic.location, RoughAltitude(0)));
  const AndAirspacePredicate outside_and_active_predicate(active_predicate, outside_predicate);
  const Airspace *airspace;

  //if altitude is available, filter airspaces in same height as airplane
  if (basic.NavAltitudeAvailable()) {
    /* check altitude; hard-coded margin of 50m (for now) */
    WrapAirspacePredicate<AirspacePredicateHeightRange> height_range_predicate(RoughAltitude(basic.nav_altitude-fixed(50)),
                                                                               RoughAltitude(basic.nav_altitude+fixed(50)));
     AndAirspacePredicate and_predicate(outside_and_active_predicate, height_range_predicate);
     airspace = airspace_database.FindNearest(basic.location, and_predicate);
  } else //only filter outside and active
    airspace = airspace_database.FindNearest(basic.location, outside_and_active_predicate);

  if (airspace == nullptr)
    return NearestAirspace();

  const AbstractAirspace &as = airspace->GetAirspace();

  /* calculate distance to the nearest point */
  const GeoPoint closest = as.ClosestPoint(basic.location,
                                           airspace_database.GetProjection());
  return NearestAirspace(as, basic.location.Distance(closest));
}

/**
 * This AirspaceVisitor extracts the vertical nearest airspace
 */
class VerticalAirspaceVisitor : public AirspaceVisitor {
  AltitudeState altitude;

  const AbstractAirspace *nearest;
  fixed nearest_delta;
  const ActiveAirspacePredicate active_predicate;

public:
  VerticalAirspaceVisitor(const MoreData &basic,
                          const DerivedInfo &calculated,
                          const ProtectedAirspaceWarningManager &airspace_warnings)
    :nearest(nullptr),
     nearest_delta(100000),
     active_predicate(&airspace_warnings)
  {
    assert(basic.baro_altitude_available || basic.gps_altitude_available);
    altitude.altitude = basic.nav_altitude;
    altitude.altitude_agl = calculated.altitude_agl;
  }

protected:
  void Check(const AbstractAirspace &airspace) {
    if (!active_predicate(airspace))
      return;

    /* check delta below */
    fixed base = airspace.GetBase().GetAltitude(altitude);
    fixed base_delta = base - altitude.altitude;
    if (!negative(base_delta) && base_delta < fabs(nearest_delta)) {
      nearest = &airspace;
      nearest_delta = base_delta;
    }

    /* check delta above */
    fixed top = airspace.GetTop().GetAltitude(altitude);
    fixed top_delta = altitude.altitude - top;
    if (!negative(top_delta) && top_delta < fabs(nearest_delta)) {
      nearest = &airspace;
      nearest_delta = -top_delta;
    }
  }

  virtual void Visit(const AbstractAirspace &as) override {
    Check(as);
  }

public:
  const AbstractAirspace *GetNearest() const {
    return nearest;
  }

  fixed GetNearestDelta() const {
    return nearest_delta;
  }
};

gcc_pure
NearestAirspace
NearestAirspace::FindVertical(const MoreData &basic,
                      const DerivedInfo &calculated,
                      const ProtectedAirspaceWarningManager &airspace_warnings,
                      const Airspaces &airspace_database)
{
  if (!basic.location_available ||
      (!basic.baro_altitude_available && !basic.gps_altitude_available))
    /* can't check for airspaces without a GPS fix and altitude
       value */
    return NearestAirspace();

  /* find the nearest airspace */
  VerticalAirspaceVisitor visitor(basic, calculated, airspace_warnings);
  airspace_database.VisitInside(basic.location, visitor);
  if (visitor.GetNearest() == nullptr)
    return NearestAirspace();

  return NearestAirspace(*visitor.GetNearest(), visitor.GetNearestDelta());
}
