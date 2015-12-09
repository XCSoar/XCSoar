/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Engine/Airspace/Minimum.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

gcc_pure
__attribute__((always_inline))
static inline NearestAirspace
CalculateNearestAirspaceHorizontal(const GeoPoint &location,
                                   const FlatProjection &projection,
                                   const AbstractAirspace &airspace)
{
  const auto closest = airspace.ClosestPoint(location, projection);
  assert(closest.IsValid());

  return NearestAirspace(airspace, closest.DistanceS(location));
}

struct CompareNearestAirspace {
  gcc_pure
  bool operator()(const NearestAirspace &a, const NearestAirspace &b) const {
    return !b.IsDefined() || a.distance < b.distance;
  }
};

gcc_pure
static NearestAirspace
FindHorizontal(const GeoPoint &location,
               const Airspaces &airspace_database,
               const AirspacePredicate &predicate)
{
  const auto &projection = airspace_database.GetProjection();
  return FindMinimum(airspace_database, location, fixed(30000), predicate,
                     [&location, &projection](const AbstractAirspace &airspace){
                       return CalculateNearestAirspaceHorizontal(location, projection, airspace);
                     },
                     CompareNearestAirspace());
}

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
  const auto outside_and_active =
    MakeAndPredicate(ActiveAirspacePredicate(&airspace_warnings),
                     OutsideAirspacePredicate(AGeoPoint(basic.location,
                                                        RoughAltitude(0))));

  //if altitude is available, filter airspaces in same height as airplane
  if (basic.NavAltitudeAvailable()) {
    /* check altitude; hard-coded margin of 50m (for now) */
    const auto outside_and_active_and_height =
      MakeAndPredicate(outside_and_active,
                       AirspacePredicateHeightRange(RoughAltitude(basic.nav_altitude - 50),
                                                    RoughAltitude(basic.nav_altitude + 50)));
    const auto predicate = WrapAirspacePredicate(outside_and_active_and_height);
    return ::FindHorizontal(basic.location, airspace_database, predicate);
  } else {
    /* only filter outside and active */
    const auto predicate = WrapAirspacePredicate(outside_and_active);
    return ::FindHorizontal(basic.location, airspace_database, predicate);
  }
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
    auto base = airspace.GetBase().GetAltitude(altitude);
    auto base_delta = base - altitude.altitude;
    if (!negative(base_delta) && base_delta < fabs(nearest_delta)) {
      nearest = &airspace;
      nearest_delta = base_delta;
    }

    /* check delta above */
    auto top = airspace.GetTop().GetAltitude(altitude);
    auto top_delta = altitude.altitude - top;
    if (!negative(top_delta) && top_delta < fabs(nearest_delta)) {
      nearest = &airspace;
      nearest_delta = -top_delta;
    }
  }

  void Visit(const AbstractAirspace &as) override {
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
