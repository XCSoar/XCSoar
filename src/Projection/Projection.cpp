// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Projection.hpp"
#include "Geo/FAISphere.hpp"
#include "Math/Angle.hpp"

#include <algorithm>

Projection::Projection() noexcept
{
  SetScale(1);
}

GeoPoint
Projection::ScreenToGeo(PixelPoint src) const noexcept
{
  assert(IsValid());

  const auto p =
    screen_rotation.Rotate(src - screen_origin);

  GeoPoint g(PixelsToAngle(p.x), PixelsToAngle(p.y));

  g.latitude = geo_location.latitude - g.latitude;

  /* paranoid sanity check to avoid integer overflow near the poles;
     our projection isn't doing well at all there; this check avoids
     assertion failures when the user pans all the way up/down */
  const Angle latitude(std::min(Angle::Degrees(80),
                                std::max(Angle::Degrees(-80), g.latitude)));

  g.longitude = geo_location.longitude + g.longitude * latitude.invfastcosine();

  return g;
}

PixelPoint
Projection::GeoToScreen(const GeoPoint &g) const noexcept
{
  assert(IsValid());

  const GeoPoint d = geo_location-g;

  const auto p =
    screen_rotation.Rotate(PixelPoint(int(g.latitude.fastcosine() *
                                          AngleToPixels(d.longitude)),
                                      (int)AngleToPixels(d.latitude)));

  PixelPoint sc;
  sc.x = screen_origin.x - p.x;
  sc.y = screen_origin.y + p.y;
  return sc;
}

FloatPoint2D
Projection::GeoToScreenF(const GeoPoint &g,
                         const FastRotation &rotation) const noexcept
{
  assert(IsValid());

  const GeoPoint d = geo_location - g;
  const auto p = rotation.Rotate(DoublePoint2D{
    g.latitude.fastcosine() * AngleToPixels(d.longitude),
    AngleToPixels(d.latitude),
  });

  return {
    float(screen_origin.x - p.x),
    float(screen_origin.y + p.y),
  };
}

FloatPoint2D
Projection::GeoToScreenF(const GeoPoint &g) const noexcept
{
  return GeoToScreenF(g, FastRotation{screen_angle});
}

void
Projection::SetScale(const double _scale) noexcept
{
  scale = _scale;

  // Calculate earth radius in pixels
  draw_scale = FAISphere::REARTH * scale;
  // Save inverted value for faster calculations
  inv_draw_scale = 1. / draw_scale;
}
