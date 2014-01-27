/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

#include "InfoBoxes/Content/Airspace.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceVisitor.hpp"
#include "Computer/GlideComputer.hpp"
#include "Airspace/NearestAirspace.hpp"

void
UpdateInfoBoxNearestAirspaceHorizontal(InfoBoxData &data)
{
  NearestAirspace nearest = NearestAirspace::FindHorizontal(glide_computer->GetAirspaceWarnings(), CommonInterface::Basic());
  if (!nearest.IsDefined()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromDistance(nearest.distance);
  data.SetComment(nearest.airspace->GetName());
}

class VerticalAirspaceVisitor : public AirspaceVisitor {
  AltitudeState altitude;

  const AbstractAirspace *nearest;
  fixed nearest_delta;
  const ActiveAirspacePredicate active_predicate;

public:
  VerticalAirspaceVisitor(const MoreData &basic,
                          const DerivedInfo &calculated)
    :nearest(NULL),
     nearest_delta(100000),
     active_predicate(&glide_computer->GetAirspaceWarnings())
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
static NearestAirspace
FindNearestVerticalAirspace()
{
  const MoreData &basic = CommonInterface::Basic();
  if (!basic.location_available ||
      (!basic.baro_altitude_available && !basic.gps_altitude_available))
    /* can't check for airspaces without a GPS fix and altitude
       value */
    return NearestAirspace();

  /* find the nearest airspace */
  VerticalAirspaceVisitor visitor(basic, CommonInterface::Calculated());
  airspace_database.VisitInside(basic.location, visitor);
  if (visitor.GetNearest() == NULL)
    return NearestAirspace();

  return NearestAirspace(*visitor.GetNearest(), visitor.GetNearestDelta());
}

void
UpdateInfoBoxNearestAirspaceVertical(InfoBoxData &data)
{
  NearestAirspace nearest = FindNearestVerticalAirspace();
  if (!nearest.IsDefined()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromArrival(nearest.distance);
  data.SetComment(nearest.airspace->GetName());
}
