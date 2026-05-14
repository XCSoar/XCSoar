// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/SquareEnvelopeBounds.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "ui/dim/Rect.hpp"

#include <algorithm>

namespace MapPreviewFit {

inline constexpr double kBoundsMargin = 1.14;
inline constexpr double kMinGeoExtentM = 500.;

[[gnu::pure]]
inline double
UniformScaleForGeoBoundsInRect(const GeoBounds &tb,
                               unsigned pixel_width, unsigned pixel_height,
                               double min_geo_extent_m) noexcept
{
  const double gw = std::max(tb.GetGeoWidth(), min_geo_extent_m);
  const double gh = std::max(tb.GetGeoHeight(), min_geo_extent_m);
  const unsigned sw = std::max(pixel_width, 4u);
  const unsigned sh = std::max(pixel_height, 4u);
  return std::min(double(sw) / gw, double(sh) / gh);
}

[[gnu::pure]]
inline double
UniformScaleForGeoBoundsInRect(const GeoBounds &tb, const PixelRect &rc,
                               double min_geo_extent_m) noexcept
{
  return UniformScaleForGeoBoundsInRect(tb, rc.GetWidth(), rc.GetHeight(),
                                       min_geo_extent_m);
}

/**
 * Geographic bounds covering the turnpoint centre and full observation zone
 * geometry (same as #OrderedTaskPoint::ScanBounds).
 */
[[gnu::pure]]
inline GeoBounds
TaskPointFullBounds(const OrderedTaskPoint &tp) noexcept
{
  GeoBounds bounds;
  bounds.SetInvalid();
  tp.ScanBounds(bounds);
  return bounds;
}

inline void
GeoBoundsToProjection(MapWindowProjection &projection,
                      GeoBounds tb,
                      const PixelRect &rc) noexcept
{
  tb = tb.Scale(kBoundsMargin);

  projection.SetGeoLocation(tb.GetCenter());
  projection.SetScale(UniformScaleForGeoBoundsInRect(tb, rc, kMinGeoExtentM));
}

/**
 * Like #GeoBoundsToProjection, but map #center (e.g. turn waypoint) to the
 * screen origin instead of the bbox centre — keeps the waypoint centred while
 * still fitting the bounds (via corner extents at unit scale).
 */
inline void
GeoBoundsToProjectionCentered(MapWindowProjection &projection,
                              GeoBounds tb,
                              const GeoPoint &center,
                              const PixelRect &rc) noexcept
{
  if (!center.IsValid()) {
    GeoBoundsToProjection(projection, tb, rc);
    return;
  }

  tb = tb.Scale(kBoundsMargin);

  const unsigned sw = std::max(rc.GetWidth(), 4u);
  const unsigned sh = std::max(rc.GetHeight(), 4u);

  const double scale_cap =
    UniformScaleForGeoBoundsInRect(tb, sw, sh, kMinGeoExtentM);

  projection.SetGeoLocation(center);
  projection.SetScale(1.);

  double max_abs_dx = 1.;
  double max_abs_dy = 1.;
  const GeoPoint corners[4] = {
    tb.GetNorthWest(),
    tb.GetNorthEast(),
    tb.GetSouthWest(),
    tb.GetSouthEast(),
  };

  const PixelPoint origin = projection.GetScreenOrigin();

  for (const GeoPoint &p : corners) {
    const PixelPoint sc = projection.GeoToScreen(p);
    max_abs_dx =
      std::max(max_abs_dx, double(std::abs(sc.x - origin.x)));
    max_abs_dy =
      std::max(max_abs_dy, double(std::abs(sc.y - origin.y)));
  }

  const double scale_fit =
    std::min(double(sw / 2) / max_abs_dx, double(sh / 2) / max_abs_dy);

  projection.SetScale(std::min(scale_fit, scale_cap));
}

/**
 * Frame a small neighbourhood around a point (waypoint / map tap).
 */
inline void
PointNeighbourhood(MapWindowProjection &projection,
                   const GeoPoint &center,
                   double radius_m,
                   const PixelRect &rc) noexcept
{
  GeoBoundsToProjection(projection, SquareEnvelopeGeoBounds(center, radius_m),
                        rc);
}

} // namespace MapPreviewFit
