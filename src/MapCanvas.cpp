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

#include "MapCanvas.hpp"
#include "Screen/Canvas.hpp"
#include "WindowProjection.hpp"
#include "Asset.hpp"

void
MapCanvas::line(GeoPoint a, GeoPoint b)
{
  if (!clip.clip_line(a, b))
    return;

  RasterPoint pts[2];
  pts[0] = projection.GeoToScreen(a);
  pts[1] = projection.GeoToScreen(b);

  canvas.line(pts[0], pts[1]);
}

void
MapCanvas::circle(const GeoPoint &center, fixed radius)
{
  RasterPoint screen_center = projection.GeoToScreen(center);
  unsigned screen_radius = projection.GeoToScreenDistance(radius);
  canvas.circle(screen_center.x, screen_center.y, screen_radius);
}

void
MapCanvas::project(const Projection &projection,
                   const SearchPointVector &points, RasterPoint *screen)
{
  for (SearchPointVector::const_iterator it = points.begin();
       it != points.end(); ++it)
    *screen++ = projection.GeoToScreen(it->get_location());
}

static void
update_bounds(PixelRect &bounds, const RasterPoint &pt)
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
MapCanvas::visible(const Canvas &canvas,
                   const RasterPoint *screen, unsigned num)
{
  PixelRect bounds;
  bounds.left = 0x7fff;
  bounds.top = 0x7fff;
  bounds.right = -1;
  bounds.bottom = -1;

  for (unsigned i = 0; i < num; ++i)
    update_bounds(bounds, screen[i]);

  return bounds.left < (int)canvas.get_width() && bounds.right >= 0 &&
    bounds.top < (int)canvas.get_height() && bounds.bottom >= 0;
}

bool
MapCanvas::prepare_polygon(const SearchPointVector &points)
{
  unsigned num_points = points.size();
  if (num_points < 3)
    return false;

  /* copy all SearchPointVector elements to geo_points */
  geo_points.grow_discard(num_points * 3);
  for (unsigned i = 0; i < num_points; ++i)
    geo_points[i] = points[i].get_location();

  /* clip them */
  num_raster_points = clip.clip_polygon(geo_points.begin(),
                                 geo_points.begin(), num_points);
  if (num_raster_points < 3)
    /* it's completely outside the screen */
    return false;

  /* project all GeoPoints to screen coordinates */
  raster_points.grow_discard(num_raster_points);
  for (unsigned i = 0; i < num_raster_points; ++i)
    raster_points[i] = projection.GeoToScreen(geo_points[i]);

  return visible(raster_points.begin(), num_raster_points);
}

void
MapCanvas::draw_prepared()
{
  /* draw it all */
  canvas.polygon(raster_points.begin(), num_raster_points);
}
