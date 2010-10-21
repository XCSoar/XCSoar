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

#include "MapCanvas.hpp"
#include "Screen/Canvas.hpp"
#include "WindowProjection.hpp"
#include "Asset.hpp"

void
MapCanvas::line(const GeoPoint &a, const GeoPoint &b)
{
  POINT pts[2];
  pts[0] = projection.GeoToScreen(a);
  pts[1] = projection.GeoToScreen(b);

  if (need_clipping())
    canvas.autoclip_polyline(pts, 2);
  else
    canvas.line(pts[0], pts[1]);
}

void
MapCanvas::circle(const GeoPoint &center, fixed radius)
{
  POINT screen_center = projection.GeoToScreen(center);
  unsigned screen_radius = projection.DistanceMetersToScreen(radius);
  canvas.circle(screen_center.x, screen_center.y, screen_radius);
}

void
MapCanvas::project(const SearchPointVector &points, POINT *screen) const
{
  for (SearchPointVector::const_iterator it = points.begin();
       it != points.end(); ++it)
    *screen++ = projection.GeoToScreen(it->get_location());
}

static void
update_bounds(RECT &bounds, const POINT &pt)
{
  if (pt.x < bounds.left)
    bounds.left = pt.x;
  if (pt.x >= bounds.right)
    bounds.right = pt.x + 1;
  if (pt.y < bounds.top)
    bounds.top = pt.y;
  if (pt.y >= bounds.bottom)
    bounds.bottom = pt.y + 1;
}

bool
MapCanvas::visible(const POINT *screen, unsigned num)
{
  RECT bounds;
  bounds.left = 0x7fff;
  bounds.top = 0x7fff;
  bounds.right = -1;
  bounds.bottom = -1;

  for (unsigned i = 0; i < num; ++i)
    update_bounds(bounds, screen[i]);

  return bounds.left < (int)canvas.get_width() && bounds.right >= 0 &&
    bounds.top < (int)canvas.get_height() && bounds.bottom >= 0;
}

void
MapCanvas::draw(const SearchPointVector &points)
{
  const unsigned num_points = points.size();
  if (num_points < 3)
    return;

  POINT pts[num_points];
  project(points, pts);

  if (visible(pts, num_points))
    canvas.autoclip_polygon(pts, num_points);
}
