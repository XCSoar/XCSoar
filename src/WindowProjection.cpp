/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "WindowProjection.hpp"

#include "Math/Angle.hpp"

bool
WindowProjection::GeoVisible(const GeoPoint &loc) const
{
  return screenbounds_latlon.inside(loc);
}

bool
WindowProjection::GeoToScreenIfVisible(const GeoPoint &loc, POINT &sc) const
{
  if (GeoVisible(loc)) {
    sc = GeoToScreen(loc);
    return ScreenVisible(sc);
  }

  return false;
}

bool
WindowProjection::ScreenVisible(const POINT &P) const
{
  if ((P.x >= MapRect.left) &&
      (P.x <= MapRect.right) &&
      (P.y >= MapRect.top) &&
      (P.y <= MapRect.bottom))
    return true;

  return false;
}

fixed
WindowProjection::GetScreenDistanceMeters() const
{
  return DistancePixelsToMeters(max_dimension(GetMapRect()));
}

void
WindowProjection::UpdateScreenBounds()
{
  screenbounds_latlon = CalculateScreenBounds(fixed_zero);
}

GeoBounds
WindowProjection::CalculateScreenBounds(const fixed scale) const
{
  // compute lat lon extents of visible screen
  if (scale >= fixed_one)
    return screenbounds_latlon.scale(scale);

  GeoBounds sb(ScreenToGeo(MapRect.left, MapRect.top));
  sb.merge(ScreenToGeo(MapRect.right, MapRect.top));
  sb.merge(ScreenToGeo(MapRect.right, MapRect.bottom));
  sb.merge(ScreenToGeo(MapRect.left, MapRect.bottom));

  return sb;
}

long
WindowProjection::max_dimension(const RECT &rc)
{
  return std::max(rc.right - rc.left, rc.bottom - rc.top);
}
