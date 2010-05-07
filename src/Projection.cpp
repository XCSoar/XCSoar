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
#include "Math/Angle.hpp"

Projection::Projection():
  DisplayAngle (fixed_zero),
  m_scale_meters_to_screen (fixed_zero)
{
  PanLocation.Latitude = Angle();
  PanLocation.Longitude = Angle();
}

/**
 * Converts screen coordinates to a GEOPOINT
 * @param x x-Coordinate on the screen
 * @param y y-Coordinate on the screen
 * @param g Output GEOPOINT
 */
void 
Projection::Screen2LonLat(const int &x,
                          const int &y,
                          GEOPOINT &g) const
{
  const FastIntegerRotation::Pair p =
    DisplayAngle.Rotate(x - Orig_Screen.x, y - Orig_Screen.y);
  g.Latitude = PanLocation.Latitude - Angle(p.second * InvDrawScale);
  g.Longitude = PanLocation.Longitude + Angle(p.first * invfastcosine(g.Latitude.value())
                                              * InvDrawScale);
}

/**
 * Converts a GEOPOINT to screen coordinates
 * @param g GEOPOINT to convert
 * @param sc Output screen coordinate
 */
void
Projection::LonLat2Screen(const GEOPOINT &g,
                          POINT &sc) const
{
  const GEOPOINT d = PanLocation-g;
  const FastIntegerRotation::Pair p =
    DisplayAngle.Rotate((int)(d.Longitude.value() * g.Latitude.fastcosine()
                              * DrawScale),
                        (int)(d.Latitude.value() * DrawScale));

  sc.x = Orig_Screen.x - p.first;
  sc.y = Orig_Screen.y + p.second;
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
Projection::LonLat2Screen(const GEOPOINT *ptin, POINT *ptout,
                          unsigned n, unsigned skip) const
{
  static Angle lastangle(fixed(-1));
  static int cost=1024, sint=0;

  if (GetDisplayAngle() != lastangle) {
    lastangle = GetDisplayAngle();
    cost = ifastcosine(lastangle.value());
    sint = ifastsine(lastangle.value());
  }
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const fixed mDrawScale = DrawScale;
  const GEOPOINT mPan = PanLocation;
  const GEOPOINT *p = ptin;
  const GEOPOINT *ptend = ptin + n;

  while (p<ptend) {
    int Y = Real2Int((mPan.Latitude - p->Latitude).value() * mDrawScale);
    int X = Real2Int((mPan.Longitude - p->Longitude).value() *
                     p->Latitude.fastcosine() * mDrawScale);
    ptout->x = (xxs-X*cost + Y*sint)/1024;
    ptout->y = (Y*cost + X*sint + yys)/1024;
    ptout++;
    p+= skip;
  }
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
Projection::LonLat2Screen(const pointObj* const ptin,
                          POINT *ptout,
                          const int n,
                          const int skip) const
{
  static Angle lastangle(fixed(-1));
  static int cost=1024, sint=0;

  if (GetDisplayAngle() != lastangle) {
    lastangle = GetDisplayAngle();
    cost = ifastcosine(lastangle.value());
    sint = ifastsine(lastangle.value());
  }
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const fixed mDrawScale = DrawScale;
  const GEOPOINT mPan = PanLocation;
  pointObj const * p = ptin;
  const pointObj* ptend = ptin+n;

  while (p<ptend) {
    int Y = Real2Int((mPan.Latitude-fixed(p->y)).value()*mDrawScale);
    int X = Real2Int((mPan.Longitude-fixed(p->x)).value()*fastcosine(fixed(p->y))*mDrawScale);
    ptout->x = (xxs-X*cost + Y*sint)/1024;
    ptout->y = (Y*cost + X*sint + yys)/1024;
    ptout++;
    p+= skip;
  }
}

bool
Projection::LonLatVisible(const GEOPOINT &loc) const
{
  if ((loc.Longitude.value()> screenbounds_latlon.minx) &&
      (loc.Longitude.value()< screenbounds_latlon.maxx) &&
      (loc.Latitude.value()> screenbounds_latlon.miny) &&
      (loc.Latitude.value()< screenbounds_latlon.maxy))
    return true;
  else
    return false;
}

bool
Projection::LonLat2ScreenIfVisible(const GEOPOINT &loc,
                                   POINT *sc) const
{
  if (LonLatVisible(loc)) {
    LonLat2Screen(loc, *sc);
    return PointVisible(*sc);
  } else {
    return false;
  }
}


bool
Projection::PointVisible(const POINT &P) const
{
  if(( P.x >= MapRect.left )
     &&
     ( P.x <= MapRect.right )
     &&
     ( P.y >= MapRect.top  )
     &&
     ( P.y <= MapRect.bottom  )
     )
    return true;
  else
    return false;
}


void 
Projection::SetScaleMetersToScreen(const fixed scale_meters_to_screen)
{
  m_scale_meters_to_screen = scale_meters_to_screen;
  DrawScale = 111194*m_scale_meters_to_screen;
  InvDrawScale = 1.0/DrawScale;
}

void
Projection::UpdateScreenBounds() 
{
  screenbounds_latlon = CalculateScreenBounds(fixed_zero);
}


rectObj
Projection::CalculateScreenBounds(const fixed scale) const
{
  // compute lat lon extents of visible screen
  rectObj sb;

  if (scale>= 1.0) {
    POINT screen_center;
    LonLat2Screen(PanLocation, screen_center);

    sb.minx = sb.maxx = PanLocation.Longitude.value();
    sb.miny = sb.maxy = PanLocation.Latitude.value();

    int dx, dy;
    unsigned int maxsc=0;
    dx = screen_center.x-MapRect.right;
    dy = screen_center.y-MapRect.top;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.left;
    dy = screen_center.y-MapRect.top;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.left;
    dy = screen_center.y-MapRect.bottom;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.right;
    dy = screen_center.y-MapRect.bottom;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));

    for (int i=0; i<10; i++) {
      const fixed ang(i*360.0/10);
      POINT p;
      GEOPOINT g;
      p.x = screen_center.x + iround(fastcosine(ang)*maxsc*scale);
      p.y = screen_center.y + iround(fastsine(ang)*maxsc*scale);
      Screen2LonLat(p.x, p.y, g);
      sb.minx = min((double)g.Longitude.value(), sb.minx);
      sb.miny = min((double)g.Latitude.value(), sb.miny);
      sb.maxx = max((double)g.Longitude.value(), sb.maxx);
      sb.maxy = max((double)g.Latitude.value(), sb.maxy);
    }

  } else {

    double xmin, xmax, ymin, ymax;
    int x, y;
    GEOPOINT g;

    x = MapRect.left;
    y = MapRect.top;
    Screen2LonLat(x, y, g);
    xmin = g.Longitude.value(); xmax = g.Longitude.value();
    ymin = g.Latitude.value(); ymax = g.Latitude.value();

    x = MapRect.right;
    y = MapRect.top;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, (double)g.Longitude.value());
    xmax = max(xmax, (double)g.Longitude.value());
    ymin = min(ymin, (double)g.Latitude.value());
    ymax = max(ymax, (double)g.Latitude.value());

    x = MapRect.right;
    y = MapRect.bottom;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, (double)g.Longitude.value());
    xmax = max(xmax, (double)g.Longitude.value());
    ymin = min(ymin, (double)g.Latitude.value());
    ymax = max(ymax, (double)g.Latitude.value());

    x = MapRect.left;
    y = MapRect.bottom;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, (double)g.Longitude.value());
    xmax = max(xmax, (double)g.Longitude.value());
    ymin = min(ymin, (double)g.Latitude.value());
    ymax = max(ymax, (double)g.Latitude.value());

    sb.minx = xmin;
    sb.maxx = xmax;
    sb.miny = ymin;
    sb.maxy = ymax;

  }

  return sb;
}


/*
  
  set PanLocation to latlon center 
  set MapRect
  Orig_Screen.x = (rc.left + rc.right)/2;
  Orig_Screen.y = (rc.bottom + rc.top)/2;
  UpdateScreenBounds();
*/


long
Projection::max_dimension(const RECT &rc) const
{
  return std::max(rc.right-rc.left, rc.bottom-rc.top);
}

