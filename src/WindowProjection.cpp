/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

void
WindowProjection::UpdateScreenBounds()
{
  screenbounds_latlon = CalculateScreenBounds(fixed_zero);
}

BoundsRectangle
WindowProjection::CalculateScreenBounds(const fixed scale) const
{
  // compute lat lon extents of visible screen
  BoundsRectangle sb;

  if (scale >= fixed_one) {
    return screenbounds_latlon.scale(scale);
  } else {
    GeoPoint g = ScreenToGeo(MapRect.left, MapRect.top);
    sb.west = g.Longitude;
    sb.east = g.Longitude;
    sb.south = g.Latitude;
    sb.north = g.Latitude;

    g = ScreenToGeo(MapRect.right, MapRect.top);
    sb.west = min(sb.west, g.Longitude);
    sb.east = max(sb.east, g.Longitude);
    sb.south = min(sb.south, g.Latitude);
    sb.north = max(sb.north, g.Latitude);

    g = ScreenToGeo(MapRect.right, MapRect.bottom);
    sb.west = min(sb.west, g.Longitude);
    sb.east = max(sb.east, g.Longitude);
    sb.south = min(sb.south, g.Latitude);
    sb.north = max(sb.north, g.Latitude);

    g = ScreenToGeo(MapRect.left, MapRect.bottom);
    sb.west = min(sb.west, g.Longitude);
    sb.east = max(sb.east, g.Longitude);
    sb.south = min(sb.south, g.Latitude);
    sb.north = max(sb.north, g.Latitude);
  }

  return sb;
}

long
WindowProjection::max_dimension(const RECT &rc)
{
  return std::max(rc.right - rc.left, rc.bottom - rc.top);
}
