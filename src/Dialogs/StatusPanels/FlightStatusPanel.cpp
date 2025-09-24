// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightStatusPanel.hpp"
#include "Interface.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Language/Language.hpp"
#include "system/OpenLink.hpp"

#include <fmt/format.h>

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#include "Apple/Share.hpp"
#endif
#endif

enum Controls {
  Location,
  Altitude,
  MaxHeightGain,
  Near,
  Bearing,
  Distance,
  ShareLocationButton,
};

static void
ShareLocation(const GeoPoint &location) noexcept
{
  const auto uri = fmt::format("geo:{:.6f},{:.6f}",
                               location.latitude.Degrees(),
                               location.longitude.Degrees());
#ifdef ANDROID
  if (native_view != nullptr)
    native_view->ShareText(Java::GetEnv(), uri.c_str());
#elif defined(__APPLE__) && TARGET_OS_IPHONE
  ShareTextIOS(uri.c_str());
#else
  OpenLink(uri.c_str());
#endif
}

void
FlightStatusPanel::Refresh() noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (basic.location_available)
    SetText(Location, FormatGeoPoint(basic.location));
  else
    ClearText(Location);
  SetRowEnabled(ShareLocationButton, basic.location_available);

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
    SetText(Near, "-");
    SetText(Bearing, "-");
    SetText(Distance, "-");
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

  AddButton(_("Share location"), [](){
    const auto &basic = CommonInterface::Basic();
    if (!basic.location_available)
      return;

    ShareLocation(basic.location);
  });

  SetRowEnabled(ShareLocationButton, false);
}
