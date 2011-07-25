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

#include "Projection.hpp"
#include "Math/Earth.hpp"
#include "Math/Angle.hpp"
#include "Screen/Layout.hpp"

Projection::Projection() :
  GeoLocation(Angle::zero(), Angle::zero()),
  ScreenRotation(Angle::zero())
{
  SetScale(fixed_one);
  ScreenOrigin.x = 0;
  ScreenOrigin.y = 0;
}

fixed
Projection::GetMapScale() const
{
  return fixed(GetMapResolutionFactor()) / scale;
}

fixed
Projection::GetScale() const
{
  return scale;
}

GeoPoint
Projection::ScreenToGeo(int x, int y) const
{
  const FastIntegerRotation::Pair p =
    ScreenRotation.Rotate(x - ScreenOrigin.x, y - ScreenOrigin.y);

  GeoPoint g(PixelsToAngle(p.first), PixelsToAngle(p.second));

  g.Latitude = GeoLocation.Latitude - g.Latitude;
  g.Longitude = GeoLocation.Longitude + g.Longitude * g.Latitude.invfastcosine();

  return g;
}

RasterPoint
Projection::GeoToScreen(const GeoPoint &g) const
{
  const GeoPoint d = GeoLocation-g;

  const FastIntegerRotation::Pair p =
    ScreenRotation.Rotate((int)fast_mult(g.Latitude.fastcosine(),
                                         AngleToPixels(d.Longitude), 16),
                          (int)AngleToPixels(d.Latitude));

  RasterPoint sc;
  sc.x = ScreenOrigin.x - p.first;
  sc.y = ScreenOrigin.y + p.second;
  return sc;
}

void 
Projection::SetScale(const fixed _scale)
{
  scale = _scale;

  // Calculate earth radius in pixels
  DrawScale = fixed_earth_r * scale;
  // Save inverted value for faster calculations
  InvDrawScale = fixed_one / DrawScale;
}

int
Projection::GetMapResolutionFactor()
{
  return Layout::Scale(30);
}
