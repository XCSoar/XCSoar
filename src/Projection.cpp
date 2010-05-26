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

Projection::Projection():
  DisplayAngle (),
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
  const GEOPOINT pg(Angle::native(fixed(p.first)*InvDrawScale),
                    Angle::native(fixed(p.second)*InvDrawScale));

  g.Latitude = PanLocation.Latitude - pg.Latitude;
  g.Longitude = PanLocation.Longitude + pg.Longitude*g.Latitude.invfastcosine();
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
    DisplayAngle.Rotate((int)(d.Longitude.value_native() * g.Latitude.fastcosine()
                              * DrawScale),
                        (int)(d.Latitude.value_native() * DrawScale));

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
  static Angle lastangle(Angle::native(fixed_minus_one));
  static int cost=1024, sint=0;

  if (GetDisplayAngle() != lastangle) {
    lastangle = GetDisplayAngle();
    cost = lastangle.ifastcosine();
    sint = lastangle.ifastsine();
  }
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const fixed mDrawScale = DrawScale;
  const GEOPOINT mPan = PanLocation;
  const GEOPOINT *p = ptin;
  const GEOPOINT *ptend = ptin + n;

  while (p<ptend) {
    int Y = Real2Int((mPan.Latitude - p->Latitude).value_native() * mDrawScale);
    int X = Real2Int((mPan.Longitude - p->Longitude).value_native() *
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
  static Angle lastangle(Angle::native(fixed_minus_one));
  static int cost=1024, sint=0;

  if (GetDisplayAngle() != lastangle) {
    lastangle = GetDisplayAngle();
    cost = lastangle.ifastcosine();
    sint = lastangle.ifastsine();
  }
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const fixed mDrawScale = DrawScale;
  const GEOPOINT mPan = PanLocation;
  pointObj const * p = ptin;
  const pointObj* ptend = ptin+n;

  while (p<ptend) {
    const GEOPOINT g = point2GeoPoint(*p);

    const int Y = Real2Int((mPan.Latitude-g.Latitude).value_native()*mDrawScale);
    const int X = Real2Int((mPan.Longitude-g.Longitude).value_native()
                           *g.Latitude.fastcosine()*mDrawScale);

    ptout->x = (xxs-X*cost + Y*sint)/1024;
    ptout->y = (Y*cost + X*sint + yys)/1024;
    ptout++;
    p+= skip;
  }
}


bool
Projection::LonLatVisible(const GEOPOINT &loc) const
{
  if (loc.Longitude.value_native() > fixed(screenbounds_latlon.minx) &&
      loc.Longitude.value_native() < fixed(screenbounds_latlon.maxx) &&
      loc.Latitude.value_native() > fixed(screenbounds_latlon.miny) &&
      loc.Latitude.value_native() < fixed(screenbounds_latlon.maxy))
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
  static const fixed fixed_r 
    (Angle::native(fixed(fixed_earth_r)).value_radians());

  m_scale_meters_to_screen = scale_meters_to_screen;
  DrawScale = fixed_r*m_scale_meters_to_screen;
  InvDrawScale = fixed_one / DrawScale;
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

  if (scale >= fixed_one) {
    POINT screen_center;
    LonLat2Screen(PanLocation, screen_center);

    sb.minx = sb.maxx = PanLocation.Longitude.value_native();
    sb.miny = sb.maxy = PanLocation.Latitude.value_native();

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
      const Angle ang = Angle::degrees(i*fixed_360/10);
      POINT p;
      GEOPOINT g;
      p.x = screen_center.x + iround(ang.fastcosine()*maxsc*scale);
      p.y = screen_center.y + iround(ang.fastsine()*maxsc*scale);
      Screen2LonLat(p.x, p.y, g);
      sb.minx = min((double)g.Longitude.value_native(), sb.minx);
      sb.miny = min((double)g.Latitude.value_native(), sb.miny);
      sb.maxx = max((double)g.Longitude.value_native(), sb.maxx);
      sb.maxy = max((double)g.Latitude.value_native(), sb.maxy);
    }

  } else {

    fixed xmin, xmax, ymin, ymax;
    int x, y;
    GEOPOINT g;

    x = MapRect.left;
    y = MapRect.top;
    Screen2LonLat(x, y, g);
    xmin = g.Longitude.value_native(); xmax = g.Longitude.value_native();
    ymin = g.Latitude.value_native(); ymax = g.Latitude.value_native();

    x = MapRect.right;
    y = MapRect.top;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, g.Longitude.value_native());
    xmax = max(xmax, g.Longitude.value_native());
    ymin = min(ymin, g.Latitude.value_native());
    ymax = max(ymax, g.Latitude.value_native());

    x = MapRect.right;
    y = MapRect.bottom;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, g.Longitude.value_native());
    xmax = max(xmax, g.Longitude.value_native());
    ymin = min(ymin, g.Latitude.value_native());
    ymax = max(ymax, g.Latitude.value_native());

    x = MapRect.left;
    y = MapRect.bottom;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, g.Longitude.value_native());
    xmax = max(xmax, g.Longitude.value_native());
    ymin = min(ymin, g.Latitude.value_native());
    ymax = max(ymax, g.Latitude.value_native());

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

fixed 
Projection::GetMapScaleUser() const 
{
  fixed map_scale = fixed(GetMapResolutionFactor())/m_scale_meters_to_screen;
  return map_scale; // Units::ToUserUnit(map_scale, Units::DistanceUnit);
}

int
Projection::GetMapResolutionFactor(void) const
{
  return IBLSCALE(30);
}

