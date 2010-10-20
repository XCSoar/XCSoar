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

#include "Projection.hpp"

#include "Math/Earth.hpp"
#include "Math/Angle.hpp"
#include "Screen/Layout.hpp"
#include "Units.hpp"

Projection::Projection() :
  PanLocation(Angle::native(fixed_zero), Angle::native(fixed_zero)),
  DisplayAngle(Angle::native(fixed_zero)),
  m_scale_meters_to_screen(fixed_zero)
{
}

/**
 * Converts screen coordinates to a GeoPoint
 * @param x x-Coordinate on the screen
 * @param y y-Coordinate on the screen
 */
GeoPoint
Projection::Screen2LonLat(int x, int y) const
{
  const FastIntegerRotation::Pair p =
    DisplayAngle.Rotate(x - Orig_Screen.x, y - Orig_Screen.y);
  const GeoPoint pg(Angle::radians(fixed(p.first)*InvDrawScale),
                    Angle::radians(fixed(p.second)*InvDrawScale));

  GeoPoint g;
  g.Latitude = PanLocation.Latitude - pg.Latitude;
  g.Longitude = PanLocation.Longitude
              + pg.Longitude * g.Latitude.invfastcosine();
  return g;
}

/**
 * Converts a GeoPoint to screen coordinates
 * @param g GeoPoint to convert
 */
POINT
Projection::LonLat2Screen(const GeoPoint &g) const
{
  const GeoPoint d = PanLocation-g;
  const FastIntegerRotation::Pair p =
    DisplayAngle.Rotate((int)(d.Longitude.value_radians()
                              * g.Latitude.fastcosine() * DrawScale),
                        (int)(d.Latitude.value_radians() * DrawScale));

  POINT sc;
  sc.x = Orig_Screen.x - p.first;
  sc.y = Orig_Screen.y + p.second;
  return sc;
}

/**
 * Converts a LatLon-based polygon to screen coordinates
 *
 * This one is optimised for long polygons.
 * @param ptin Input polygon
 * @param ptout Output polygon
 * @param n Number of points in the polygon
 * @param skip Number of corners to skip after a successful conversion
 */
void
Projection::LonLat2Screen(const GeoPoint *ptin, POINT *ptout,
                          unsigned n) const
{
  const GeoPoint *p = ptin;
  const GeoPoint *ptend = ptin + n;

  while (p < ptend) {
    *ptout++ = LonLat2Screen(*p);
    p++;
  }
}

bool
Projection::LonLatVisible(const GeoPoint &loc) const
{
  return screenbounds_latlon.inside(loc);
}

bool
Projection::LonLat2ScreenIfVisible(const GeoPoint &loc,
                                   POINT &sc) const
{
  if (LonLatVisible(loc)) {
    sc = LonLat2Screen(loc);
    return PointVisible(sc);
  }

  return false;
}

bool
Projection::PointVisible(const POINT &P) const
{
  if ((P.x >= MapRect.left) &&
      (P.x <= MapRect.right) &&
      (P.y >= MapRect.top) &&
      (P.y <= MapRect.bottom))
    return true;

  return false;
}

void 
Projection::SetScaleMetersToScreen(const fixed scale_meters_to_screen)
{
  m_scale_meters_to_screen = scale_meters_to_screen;
  DrawScale = fixed_earth_r * m_scale_meters_to_screen;
  InvDrawScale = fixed_one / DrawScale;
}

void
Projection::UpdateScreenBounds() 
{
  screenbounds_latlon = CalculateScreenBounds(fixed_zero);
}

BoundsRectangle
Projection::CalculateScreenBounds(const fixed scale) const
{
  // compute lat lon extents of visible screen
  BoundsRectangle sb;

  if (scale >= fixed_one) {
    POINT screen_center = LonLat2Screen(PanLocation);

    sb.west = sb.east = PanLocation.Longitude;
    sb.south = sb.north = PanLocation.Latitude;

    int dx, dy;
    unsigned int maxsc = 0;
    dx = screen_center.x - MapRect.right;
    dy = screen_center.y - MapRect.top;
    maxsc = max(maxsc, isqrt4(dx * dx + dy * dy));
    dx = screen_center.x - MapRect.left;
    dy = screen_center.y - MapRect.top;
    maxsc = max(maxsc, isqrt4(dx * dx + dy * dy));
    dx = screen_center.x - MapRect.left;
    dy = screen_center.y - MapRect.bottom;
    maxsc = max(maxsc, isqrt4(dx * dx + dy * dy));
    dx = screen_center.x - MapRect.right;
    dy = screen_center.y - MapRect.bottom;
    maxsc = max(maxsc, isqrt4(dx * dx + dy * dy));

    for (int i = 0; i < 10; i++) {
      const Angle ang = Angle::degrees(i * fixed_360 / 10);
      POINT p;
      p.x = screen_center.x + iround(ang.fastcosine() * maxsc * scale);
      p.y = screen_center.y + iround(ang.fastsine() * maxsc * scale);
      GeoPoint g = Screen2LonLat(p.x, p.y);
      sb.west = min(g.Longitude, sb.west);
      sb.south = min(g.Latitude, sb.south);
      sb.east = max(g.Longitude, sb.east);
      sb.north = max(g.Latitude, sb.north);
    }
  } else {
    GeoPoint g = Screen2LonLat(MapRect.left, MapRect.top);
    sb.west = g.Longitude;
    sb.east = g.Longitude;
    sb.south = g.Latitude;
    sb.north = g.Latitude;

    g = Screen2LonLat(MapRect.right, MapRect.top);
    sb.west = min(sb.west, g.Longitude);
    sb.east = max(sb.east, g.Longitude);
    sb.south = min(sb.south, g.Latitude);
    sb.north = max(sb.north, g.Latitude);

    g = Screen2LonLat(MapRect.right, MapRect.bottom);
    sb.west = min(sb.west, g.Longitude);
    sb.east = max(sb.east, g.Longitude);
    sb.south = min(sb.south, g.Latitude);
    sb.north = max(sb.north, g.Latitude);

    g = Screen2LonLat(MapRect.left, MapRect.bottom);
    sb.west = min(sb.west, g.Longitude);
    sb.east = max(sb.east, g.Longitude);
    sb.south = min(sb.south, g.Latitude);
    sb.north = max(sb.north, g.Latitude);
  }

  return sb;
}

long
Projection::max_dimension(const RECT &rc)
{
  return std::max(rc.right - rc.left, rc.bottom - rc.top);
}

fixed
Projection::GetMapScaleUser() const
{
  return fixed(GetMapResolutionFactor()) / m_scale_meters_to_screen;
}

int
Projection::GetMapResolutionFactor()
{
  return IBLSCALE(30);
}
