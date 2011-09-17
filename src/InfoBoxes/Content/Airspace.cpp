/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspacePredicate.hpp"
#include "Engine/Airspace/AirspaceVisitor.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspacePolygon.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Util/Macros.hpp"

struct NearestAirspace {
  const AbstractAirspace *airspace;

  /**
   * The horizontal or vertical distance [m], depending on which
   * function filled this object.
   */
  fixed distance;

  NearestAirspace():airspace(NULL) {}
  NearestAirspace(const AbstractAirspace &_airspace, fixed _distance)
    :airspace(&_airspace), distance(_distance) {}

  bool IsDefined() const {
    return airspace != NULL;
  }
};

gcc_pure
static bool
IsAcked(const AbstractAirspace &airspace)
{
  return airspace_warnings != NULL && airspace_warnings->get_ack_day(airspace);
}

gcc_pure
static bool
CheckAirspace(const AbstractAirspace &airspace)
{
  const AirspaceWarningConfig &config =
    CommonInterface::SettingsComputer().airspace.warnings;

  return config.class_enabled(airspace.GetType()) && !IsAcked(airspace);
}

class HorizontalAirspaceCondition : public AirspacePredicate {
  GeoPoint location;
  AltitudeState altitude;
  bool altitude_available;

public:
  HorizontalAirspaceCondition(const MoreData &basic,
                              const DerivedInfo &calculated)
    :location(basic.location),
     altitude_available(basic.baro_altitude_available ||
                        basic.gps_altitude_available)
  {
    if (altitude_available) {
      altitude.altitude = basic.NavAltitude;
      altitude.altitude_agl = calculated.altitude_agl;
    }
  }

  virtual bool condition(const AbstractAirspace &airspace) const {
    return CheckAirspace(airspace) &&
      /* skip airspaces that we already entered */
      !airspace.Inside(location) &&
      /* check altitude; hard-coded margin of 50m (for now) */
      (!altitude_available ||
       (airspace.GetBase().IsBelow(altitude, fixed(50)) &&
        airspace.GetTop().IsAbove(altitude, fixed(50))));
  }
};

gcc_pure
static NearestAirspace
FindNearestHorizontalAirspace()
{
  const MoreData &basic = CommonInterface::Basic();
  if (!basic.location_available)
    /* can't check for airspaces without a GPS fix */
    return NearestAirspace();

  /* find the nearest airspace */
  HorizontalAirspaceCondition condition(basic, CommonInterface::Calculated());
  const Airspace *airspace = airspace_database.find_nearest(basic.location, condition);
  if (airspace == NULL)
    return NearestAirspace();

  const AbstractAirspace &as = *airspace->get_airspace();

  /* calculate distance to the nearest point */
  const GeoPoint closest = as.ClosestPoint(basic.location);
  return NearestAirspace(as, basic.location.distance(closest));
}

void
InfoBoxContentNearestAirspaceHorizontal::Update(InfoBoxWindow &infobox)
{
  NearestAirspace nearest = FindNearestHorizontalAirspace();
  if (!nearest.IsDefined()) {
    infobox.SetInvalid();
    return;
  }

  SetValueFromDistance(infobox, nearest.distance);
  infobox.SetComment(nearest.airspace->GetName());
}

class VerticalAirspaceVisitor : public AirspaceVisitor {
  GeoPoint location;
  AltitudeState altitude;

  const AbstractAirspace *nearest;
  fixed nearest_delta;

public:
  VerticalAirspaceVisitor(const MoreData &basic,
                          const DerivedInfo &calculated)
    :location(basic.location),
     nearest(NULL), nearest_delta(100000) {
    assert(basic.baro_altitude_available || basic.gps_altitude_available);
    altitude.altitude = basic.NavAltitude;
    altitude.altitude_agl = calculated.altitude_agl;
  }

protected:
  void Check(const AbstractAirspace &airspace) {
    if (!CheckAirspace(airspace))
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

  virtual void Visit(const AirspaceCircle &as) {
    Check(as);
  }

  virtual void Visit(const AirspacePolygon &as) {
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
  airspace_database.visit_inside(basic.location, visitor);
  if (visitor.GetNearest() == NULL)
    return NearestAirspace();

  return NearestAirspace(*visitor.GetNearest(), visitor.GetNearestDelta());
}

void
InfoBoxContentNearestAirspaceVertical::Update(InfoBoxWindow &infobox)
{
  NearestAirspace nearest = FindNearestVerticalAirspace();
  if (!nearest.IsDefined()) {
    infobox.SetInvalid();
    return;
  }

  TCHAR buffer[32];
  Units::FormatUserArrival(nearest.distance, buffer,
                           ARRAY_SIZE(buffer), false);
  infobox.SetValue(buffer);
  infobox.SetValueUnit(Units::Current.AltitudeUnit);
  infobox.SetComment(nearest.airspace->GetName());
}
