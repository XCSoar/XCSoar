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

#include "Topology/TopologyFile.hpp"
#include "Topology/XShape.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Util.hpp"
#include "WindowProjection.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/LabelBlock.hpp"
#include "SettingsMap.hpp"
#include "Navigation/GeoPoint.hpp"
#include "resource.h"
#include "shapelib/map.h"

#include <stdlib.h>
#include <tchar.h>
#include <ctype.h> // needed for Wine

gcc_pure
static GeoPoint
point2GeoPoint(const pointObj& p)
{
  return GeoPoint(Angle::native(fixed(p.x)), Angle::native(fixed(p.y)));
}

void
TopologyFile::loadIcon(const int res_id)
{
  if (res_id == IDB_TOWN)
    icon.load_big(IDB_TOWN, IDB_TOWN_HD);
  else
    icon.load(res_id);
}

TopologyFile::TopologyFile(const char *filename, const Color thecolor,
                   int _label_field)
  :label_field(_label_field),
  scaleThreshold(1000000),
  hPen(1, thecolor),
  hbBrush(thecolor),
  shapefileopen(false)
{
  if (msSHPOpenFile(&shpfile, "rb", filename) == -1)
    return;

  shpCache.resize_discard(shpfile.numshapes);

  shapefileopen = true;

  cache_bounds.west = cache_bounds.east =
    cache_bounds.south = cache_bounds.north = Angle::native(fixed_zero);

  for (int i = 0; i < shpfile.numshapes; i++)
    shpCache[i] = NULL;
}

TopologyFile::~TopologyFile()
{
  if (!shapefileopen)
    return;

  ClearCache();
  msSHPCloseFile(&shpfile);
}

void
TopologyFile::ClearCache()
{
  for (unsigned i = 0; i < shpCache.size(); i++) {
    delete shpCache[i];
    shpCache[i] = NULL;
  }
}

rectObj
TopologyFile::ConvertRect(const GeoBounds &br)
{
  rectObj dest;
  dest.minx = br.west.value_degrees();
  dest.maxx = br.east.value_degrees();
  dest.miny = br.south.value_degrees();
  dest.maxy = br.north.value_degrees();
  return dest;
}

void
TopologyFile::updateCache(const WindowProjection &map_projection)
{
  if (!shapefileopen)
    return;

  if (map_projection.GetMapScale() > scaleThreshold)
    /* not visible, don't update cache now */
    return;

  const GeoBounds screenRect =
    map_projection.GetScreenBounds();
  if (cache_bounds.inside(screenRect))
    /* the cache is still fresh */
    return;

  cache_bounds = map_projection.GetScreenBounds().scale(fixed_two);

  rectObj deg_bounds = ConvertRect(cache_bounds);

  // Test which shapes are inside the given bounds and save the
  // status to shpfile.status
  msSHPWhichShapes(&shpfile, deg_bounds, 0);

  // If not a single shape is inside the bounds
  if (!shpfile.status) {
    // ... clear the whole buffer
    ClearCache();
    return;
  }

  // Iterate through the shapefile entries
  for (int i = 0; i < shpfile.numshapes; i++) {
    if (!msGetBit(shpfile.status, i)) {
      // If the shape is outside the bounds
      // delete the shape from the cache
      delete shpCache[i];
      shpCache[i] = NULL;
    } else if (shpCache[i] == NULL) {
      // If the shape is inside the bounds and if the
      // shape isn't cached yet -> cache the shape
      shpCache[i] = new XShape(&shpfile, i, label_field);
    }
  }
}

unsigned
TopologyFile::GetSkipSteps(fixed map_scale) const
{
  if (map_scale * 4 > scaleThreshold * 3)
    return 4;
  if (map_scale * 2 > scaleThreshold)
    return 3;
  if (map_scale * 4 > scaleThreshold)
    return 2;

  return 1;
}

void
TopologyFile::Paint(Canvas &canvas, BitmapCanvas &bitmap_canvas,
                const WindowProjection &projection) const
{
  if (!shapefileopen)
    return;

  fixed map_scale = projection.GetMapScale();
  if (map_scale > scaleThreshold)
    return;

  // TODO code: only draw inside screen!
  // this will save time with rendering pixmaps especially
  // we already do an outer visibility test, but may need a test
  // in screen coords

  canvas.select(hPen);
  canvas.select(hbBrush);

  // get drawing info

  int iskip = GetSkipSteps(map_scale);

  const rectObj screenRect =
    ConvertRect(projection.GetScreenBounds());

  for (int ixshp = 0; ixshp < shpfile.numshapes; ixshp++) {
    const XShape *cshape = shpCache[ixshp];
    if (!cshape || !cshape->is_visible(label_field))
      continue;

    const shapeObj &shape = cshape->shape;

    if (!msRectOverlap(&shape.bounds, &screenRect))
      continue;

    switch (shape.type) {
    case MS_SHAPE_POINT:
      for (int tt = 0; tt < shape.numlines; ++tt) {
        const lineObj &line = shape.line[tt];

        for (int jj = 0; jj < line.numpoints; ++jj) {
          POINT sc;
          const GeoPoint l = point2GeoPoint(line.point[jj]);

          if (projection.GeoToScreenIfVisible(l, sc))
            icon.draw(canvas, bitmap_canvas, sc.x, sc.y);
        }
      }
      break;

    case MS_SHAPE_LINE:
      for (int tt = 0; tt < shape.numlines; ++tt) {
        const lineObj &line = shape.line[tt];
        unsigned msize = line.numpoints;
        POINT pt[msize];

        for (unsigned i = 0; i < msize; ++i) {
          GeoPoint g = point2GeoPoint(line.point[i]);
          pt[i] = projection.GeoToScreen(g);
        }

        canvas.polyline(pt, msize);
      }
      break;

    case MS_SHAPE_POLYGON:
      for (int tt = 0; tt < shape.numlines; ++tt) {
        const lineObj &line = shape.line[tt];
        unsigned msize = line.numpoints / iskip;
        POINT pt[msize];

        const pointObj *in = line.point;
        for (unsigned i = 0; i < msize; ++i) {
          GeoPoint g = point2GeoPoint(*in);
          in += iskip;
          pt[i] = projection.GeoToScreen(g);
        }

        canvas.polygon(pt, msize);
      }
      break;
    }
  }
}

void
TopologyFile::PaintLabels(Canvas &canvas,
                      const WindowProjection &projection, LabelBlock &label_block,
                      const SETTINGS_MAP &settings_map) const
{
  if (!shapefileopen || settings_map.DeclutterLabels >= 2)
    return;

  fixed map_scale = projection.GetMapScale();
  if (map_scale > scaleThreshold)
    return;

  // TODO code: only draw inside screen!
  // this will save time with rendering pixmaps especially
  // we already do an outer visibility test, but may need a test
  // in screen coords

  canvas.select(Fonts::MapLabel);

  // get drawing info

  int iskip = GetSkipSteps(map_scale);

  rectObj screenRect =
    ConvertRect(projection.GetScreenBounds());

  for (int ixshp = 0; ixshp < shpfile.numshapes; ixshp++) {
    const XShape *cshape = shpCache[ixshp];
    if (!cshape || !cshape->is_visible(label_field))
      continue;

    const shapeObj &shape = cshape->shape;

    if (!msRectOverlap(&shape.bounds, &screenRect))
      continue;

    for (int tt = 0; tt < shape.numlines; ++tt) {
      const lineObj &line = shape.line[tt];

      int minx = canvas.get_width();
      int miny = canvas.get_height();
      const pointObj *in = line.point;
      for (unsigned i = 0; i < (unsigned)line.numpoints; i += iskip) {
        GeoPoint g = point2GeoPoint(line.point[i]);
        in += iskip;
        POINT pt = projection.GeoToScreen(g);

        if (pt.x <= minx) {
          minx = pt.x;
          miny = pt.y;
        }
      }

      cshape->DrawLabel(canvas, label_block, minx, miny);
    }
  }
}
