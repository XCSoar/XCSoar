/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

void
Projection::SetScale(const double _scale) noexcept
{
  scale = _scale;

  // Calculate earth radius in pixels
  draw_scale = FAISphere::REARTH * scale;
  // Save inverted value for faster calculations
  inv_draw_scale = 1. / draw_scale;
}
