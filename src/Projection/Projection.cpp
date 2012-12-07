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

#include "Projection.hpp"
#include "Geo/Constants.hpp"
#include "Math/Angle.hpp"

Projection::Projection() :
  geo_location(Angle::Zero(), Angle::Zero()),
  screen_rotation(Angle::Zero())
{
  SetScale(fixed(1));
  screen_origin.x = 0;
  screen_origin.y = 0;
}

GeoPoint
Projection::ScreenToGeo(int x, int y) const
{
  const FastIntegerRotation::Pair p =
    screen_rotation.Rotate(x - screen_origin.x, y - screen_origin.y);

  GeoPoint g(PixelsToAngle(p.first), PixelsToAngle(p.second));

  g.latitude = geo_location.latitude - g.latitude;

  /* paranoid sanity check to avoid integer overflow near the poles;
     our projection isn't doing well at all there; this check avoids
     assertion failures when the user pans all the way up/down */
  const Angle latitude(std::min(Angle::Degrees(80),
                                std::max(Angle::Degrees(-80), g.latitude)));

  g.longitude = geo_location.longitude + g.longitude * latitude.invfastcosine();

  return g;
}

RasterPoint
Projection::GeoToScreen(const GeoPoint &g) const
{
  const GeoPoint d = geo_location-g;

  const FastIntegerRotation::Pair p =
    screen_rotation.Rotate((int)fast_mult(g.latitude.fastcosine(),
                                         AngleToPixels(d.longitude), 16),
                          (int)AngleToPixels(d.latitude));

  RasterPoint sc;
  sc.x = screen_origin.x - p.first;
  sc.y = screen_origin.y + p.second;
  return sc;
}

void 
Projection::SetScale(const fixed _scale)
{
  scale = _scale;

  // Calculate earth radius in pixels
  draw_scale = fixed_earth_r * scale;
  // Save inverted value for faster calculations
  inv_draw_scale = fixed(1) / draw_scale;
}
