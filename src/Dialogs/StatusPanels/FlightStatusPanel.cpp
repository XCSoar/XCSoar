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

#include "FlightStatusPanel.hpp"
#include "Util/Macros.hpp"
#include "Util/StaticString.hpp"
#include "Interface.hpp"
#include "Form/Edit.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Engine/Waypoint/Waypoint.hpp"

void
FlightStatusPanel::Refresh()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  WndProperty *wp;
  StaticString<32> buffer;

  Units::LongitudeToString(basic.location.longitude,
                           buffer.buffer(), buffer.MAX_SIZE);
  wp = (WndProperty*)form.FindByName(_T("prpLongitude"));
  assert(wp != NULL);
  wp->SetText(buffer);

  Units::LatitudeToString(basic.location.latitude,
                          buffer.buffer(), buffer.MAX_SIZE);
  wp = (WndProperty*)form.FindByName(_T("prpLatitude"));
  assert(wp != NULL);
  wp->SetText(buffer);

  wp = (WndProperty*)form.FindByName(_T("prpAltitude"));
  assert(wp != NULL);
  if (basic.gps_altitude_available) {
    Units::FormatUserAltitude(basic.gps_altitude,
                              buffer.buffer(), buffer.MAX_SIZE);
    wp->SetText(buffer);
  }

  wp = (WndProperty*)form.FindByName(_T("prpMaxHeightGain"));
  assert(wp != NULL);
  Units::FormatUserAltitude(calculated.max_height_gain,
                            buffer.buffer(), buffer.MAX_SIZE);
  wp->SetText(buffer);

  if (nearest_waypoint) {
    GeoVector vec(basic.location,
                  nearest_waypoint->location);

    wp = (WndProperty*)form.FindByName(_T("prpNear"));
    assert(wp != NULL);
    wp->SetText(nearest_waypoint->name.c_str());

    wp = (WndProperty*)form.FindByName(_T("prpBearing"));
    assert(wp != NULL);
    buffer.UnsafeFormat(_T("%d")_T(DEG), (int)vec.bearing.Degrees());
    wp->SetText(buffer);

    wp = (WndProperty*)form.FindByName(_T("prpDistance"));
    assert(wp != NULL);
    Units::FormatUserDistance(vec.distance, buffer.buffer(), buffer.MAX_SIZE);
    wp->SetText(buffer);
  } else {
    wp = (WndProperty*)form.FindByName(_T("prpNear"));
    assert(wp != NULL);
    wp->SetText(_T("-"));

    wp = (WndProperty*)form.FindByName(_T("prpBearing"));
    assert(wp != NULL);
    wp->SetText(_T("-"));

    wp = (WndProperty*)form.FindByName(_T("prpDistance"));
    assert(wp != NULL);
    wp->SetText(_T("-"));
  }
}

void
FlightStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, _T("IDR_XML_STATUS_FLIGHT"));
}
