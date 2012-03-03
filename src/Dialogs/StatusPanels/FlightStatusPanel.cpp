/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "FlightStatusPanel.hpp"
#include "Util/Macros.hpp"
#include "Util/StaticString.hpp"
#include "Interface.hpp"
#include "Form/Util.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"
#include "Engine/Waypoint/Waypoint.hpp"

void
FlightStatusPanel::Refresh()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  StaticString<32> buffer;

  if (basic.location_available) {
    FormatGeoPoint(basic.location, buffer.buffer(), buffer.MAX_SIZE);
    SetFormValue(form, _T("prpLocation"), buffer);
  } else
    SetFormValue(form, _T("prpLocation"), _T(""));

  if (basic.gps_altitude_available) {
    FormatUserAltitude(basic.gps_altitude,
                              buffer.buffer(), buffer.MAX_SIZE);
    SetFormValue(form, _T("prpAltitude"), buffer);
  } else
    SetFormValue(form, _T("prpAltitude"), _T(""));

  FormatUserAltitude(calculated.max_height_gain,
                            buffer.buffer(), buffer.MAX_SIZE);
  SetFormValue(form, _T("prpMaxHeightGain"), buffer);

  if (nearest_waypoint) {
    GeoVector vec(basic.location,
                  nearest_waypoint->location);

    SetFormValue(form, _T("prpNear"), nearest_waypoint->name.c_str());

    FormatBearing(buffer.buffer(), buffer.MAX_SIZE, vec.bearing, _T(""));
    SetFormValue(form, _T("prpBearing"), buffer);

    FormatUserDistanceSmart(vec.distance, buffer.buffer(), buffer.MAX_SIZE);
    SetFormValue(form, _T("prpDistance"), buffer);
  } else {
    SetFormValue(form, _T("prpNear"), _T("-"));
    SetFormValue(form, _T("prpBearing"), _T("-"));
    SetFormValue(form, _T("prpDistance"), _T("-"));
  }
}

void
FlightStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, _T("IDR_XML_STATUS_FLIGHT"));
}
