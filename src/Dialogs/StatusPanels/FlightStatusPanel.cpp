// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightStatusPanel.hpp"
#include "Interface.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Language/Language.hpp"

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
#if defined(ANDROID) || (defined(__APPLE__) && TARGET_OS_IPHONE)
  ShareLocationButton,
#endif
};

#if defined(ANDROID) || (defined(__APPLE__) && TARGET_OS_IPHONE)
static NarrowString<64>
BuildGeoURI(const GeoPoint &location) noexcept
{
  NarrowString<64> uri;

  if (!location.Check())
    return uri;

  char location_string[32];
  FormatGeoPoint(location, location_string, sizeof(location_string),
               CoordinateFormat::PLAIN_DECIMAL, ',');

  char buffer[64];
  int n = snprintf(buffer, sizeof(buffer), "geo:%s", location_string);

  if (n < 0 || (size_t)n >= uri.capacity())
    return uri;

  uri.assign(buffer);
  return uri;
}

#endif

void
FlightStatusPanel::Refresh() noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (basic.location_available)
    SetText(Location, FormatGeoPoint(basic.location));
  else
    ClearText(Location);
#if defined(ANDROID) || (defined(__APPLE__) && TARGET_OS_IPHONE)
  SetRowEnabled(ShareLocationButton, basic.location_available);
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

#if defined(ANDROID) || (defined(__APPLE__) && TARGET_OS_IPHONE)
  AddButton(_("Share location"), [](){
    const auto &basic = CommonInterface::Basic();
    if (!basic.location_available)
      return;
    const auto uri = BuildGeoURI(basic.location);
    if (uri.empty())
      return;
#ifdef ANDROID
    native_view->ShareText(Java::GetEnv(), uri.c_str());
#elif defined(__APPLE__) && TARGET_OS_IPHONE
    ShareTextIOS(uri.c_str());
#endif
  });

  SetRowEnabled(ShareLocationButton, false);
#endif
}
