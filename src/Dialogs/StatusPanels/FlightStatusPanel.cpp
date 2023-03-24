// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightStatusPanel.hpp"
#include "Interface.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Language/Language.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

enum Controls {
  Location,
  Altitude,
  MaxHeightGain,
  Near,
  Bearing,
  Distance,
#ifdef ANDROID
  ShareButton,
#endif
};

void
FlightStatusPanel::Refresh() noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (basic.location_available)
    SetText(Location, FormatGeoPoint(basic.location));
  else
    ClearText(Location);
#ifdef ANDROID
  SetRowEnabled(ShareButton, basic.location_available);
#endif

  if (basic.gps_altitude_available)
    SetText(Altitude, FormatUserAltitude(basic.gps_altitude));
  else
    ClearText(Altitude);

  SetText(MaxHeightGain, FormatUserAltitude(calculated.max_height_gain));

  if (nearest_waypoint) {
    GeoVector vec(basic.location,
                  nearest_waypoint->location);

    SetText(Near, nearest_waypoint->name.c_str());

    SetText(Bearing, FormatBearing(vec.bearing).c_str());

    SetText(Distance, FormatUserDistanceSmart(vec.distance));
  } else {
    SetText(Near, _T("-"));
    SetText(Bearing, _T("-"));
    SetText(Distance, _T("-"));
  }
}

void
FlightStatusPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddReadOnly(_("Location"));
  AddReadOnly(_("Altitude"));
  AddReadOnly(_("Max. height gain"));
  AddReadOnly(_("Near"));
  AddReadOnly(_("Bearing"));
  AddReadOnly(_("Distance"));

#ifdef ANDROID
  AddButton(_("Share"), [this](){
    native_view->ShareText(Java::GetEnv(), GetText(Location));
  });

  SetRowEnabled(ShareButton, false);
#endif
}
