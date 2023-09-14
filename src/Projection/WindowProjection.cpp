// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindowProjection.hpp"
#include "Geo/Quadrilateral.hpp"

std::optional<PixelPoint>
WindowProjection::GeoToScreenIfVisible(const GeoPoint &loc) const noexcept
{
  if (!GeoVisible(loc))
    return {};

  auto p = GeoToScreen(loc);
  if (!ScreenVisible(p))
    return {};

  return p;
}

void
WindowProjection::SetScaleFromRadius(double radius) noexcept
{
  SetScale(double(GetMinScreenDistance()) / (radius * 2));
}

double
WindowProjection::GetMapScale() const noexcept
{
  return DistancePixelsToMeters(GetMapResolutionFactor());
}

double
WindowProjection::GetScreenDistanceMeters() const noexcept
{
  return DistancePixelsToMeters(GetScreenDistance());
}

GeoPoint
WindowProjection::GetGeoScreenCenter() const noexcept
{
  return ScreenToGeo(GetScreenCenter());
}

GeoQuadrilateral
WindowProjection::GetGeoQuadrilateral() const noexcept
{
  const auto r = GetScreenRect();
  return {
    ScreenToGeo(r.GetTopLeft()),
    ScreenToGeo(r.GetTopRight()),
    ScreenToGeo(r.GetBottomLeft()),
    ScreenToGeo(r.GetBottomRight()),
  };
}

void
WindowProjection::UpdateScreenBounds() noexcept
{
  assert(screen_size_initialised);

  if (!IsValid())
    return;

  screen_bounds = GetGeoQuadrilateral().GetBounds();
}
