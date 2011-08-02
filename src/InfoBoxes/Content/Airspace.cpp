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
#include "Units/UnitsFormatter.hpp"

class HorizontalAirspaceCondition : public AirspacePredicate {
  const AirspaceWarningConfig &config;
  GeoPoint location;
  AltitudeState altitude;
  bool altitude_available;

public:
  HorizontalAirspaceCondition(const AirspaceWarningConfig &_config,
                              const MoreData &basic,
                              const DerivedInfo &calculated)
    :config(_config), location(basic.location),
     altitude_available(basic.baro_altitude_available ||
                        basic.gps_altitude_available)
  {
    if (altitude_available) {
      altitude.altitude = basic.NavAltitude;
      altitude.altitude_agl = calculated.altitude_agl;
    }
  }

  virtual bool operator()(const AbstractAirspace &airspace) const {
    return config.class_enabled(airspace.get_type()) &&
      /* skip airspaces that we already entered */
      !airspace.inside(location) &&
      /* check altitude; hard-coded margin of 50m (for now) */
      (!altitude_available ||
       (airspace.get_base().is_below(altitude, fixed(50)) &&
        airspace.get_top().is_above(altitude, fixed(50))));
  }
};

void
InfoBoxContentNearestAirspaceHorizontal::Update(InfoBoxWindow &infobox)
{
  const MoreData &basic = CommonInterface::Basic();
  if (!basic.location_available) {
    /* can't check for airspaces without a GPS fix */
    infobox.SetInvalid();
    return;
  }

  /* find the nearest airspace */
  HorizontalAirspaceCondition condition(CommonInterface::SettingsComputer().airspace.warnings,
                                        basic, CommonInterface::Calculated());
  const Airspace *found = airspace_database.find_nearest(basic.location, condition);
  if (found == NULL) {
    infobox.SetInvalid();
    return;
  }

  /* calculate distance to the nearest point */
  const AbstractAirspace &as = *found->get_airspace();
  const GeoPoint closest = as.closest_point(basic.location);
  SetValueFromDistance(infobox, basic.location.distance(closest));
  infobox.SetComment(as.get_name_text(true).c_str());
}

class VerticalAirspaceVisitor : public AirspaceVisitor {
  const AirspaceWarningConfig &config;
  GeoPoint location;
  AltitudeState altitude;

  const AbstractAirspace *nearest;
  fixed nearest_delta;

public:
  VerticalAirspaceVisitor(const AirspaceWarningConfig &_config,
                          const MoreData &basic,
                          const DerivedInfo &calculated)
    :config(_config), location(basic.location),
     nearest(NULL), nearest_delta(100000) {
    assert(basic.baro_altitude_available || basic.gps_altitude_available);
    altitude.altitude = basic.NavAltitude;
    altitude.altitude_agl = calculated.altitude_agl;
  }

protected:
  void Check(const AbstractAirspace &airspace) {
    if (!config.class_enabled(airspace.get_type()))
      return;

    /* check delta below */
    fixed base = airspace.get_base().get_altitude(altitude);
    fixed base_delta = base - altitude.altitude;
    if (!negative(base_delta) && base_delta < nearest_delta) {
      nearest = &airspace;
      nearest_delta = base_delta;
    }

    /* check delta above */
    fixed top = airspace.get_top().get_altitude(altitude);
    fixed top_delta = altitude.altitude - top;
    if (!negative(top_delta) && top_delta < nearest_delta) {
      nearest = &airspace;
      nearest_delta = top_delta;
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

void
InfoBoxContentNearestAirspaceVertical::Update(InfoBoxWindow &infobox)
{
  const MoreData &basic = CommonInterface::Basic();

  if (!basic.location_available ||
      (!basic.baro_altitude_available && !basic.gps_altitude_available)) {
    /* can't check for airspaces without a GPS fix and altitude
       value */
    infobox.SetInvalid();
    return;
  }

  /* find the nearest airspace */
  VerticalAirspaceVisitor visitor(CommonInterface::SettingsComputer().airspace.warnings,
                                  basic, CommonInterface::Calculated());
  airspace_database.visit_inside(basic.location, visitor);
  if (visitor.GetNearest() == NULL) {
    infobox.SetInvalid();
    return;
  }

  TCHAR buffer[32];
  Units::FormatUserAltitude(visitor.GetNearestDelta(), buffer,
                            sizeof(buffer) / sizeof(buffer[0]), false);
  infobox.SetValue(buffer);
  infobox.SetValueUnit(Units::Current.AltitudeUnit);
  infobox.SetComment(visitor.GetNearest()->get_name_text(true).c_str());
}
