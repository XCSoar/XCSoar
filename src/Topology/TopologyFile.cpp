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

#include "Topology/TopologyFile.hpp"
#include "Topology/XShape.hpp"
#include <ctype.h> // needed for Wine
#include "Screen/Util.hpp"
#include "Projection.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/LabelBlock.hpp"
#include "SettingsUser.hpp"
#include "Navigation/GeoPoint.hpp"

#include <stdlib.h>
#include <tchar.h>

void
TopologyFile::loadIcon(const int res_id)
{
  icon.load(res_id);
}

TopologyFile::TopologyFile(const char *filename, const Color thecolor,
                   int _label_field)
  :label_field(_label_field),
  scaleThreshold(1000.0),
  triggerUpdateCache(false),
  in_scale(false),
  hPen(1, thecolor),
  hbBrush(thecolor),
  shapefileopen(false)
{
  if (msSHPOpenFile(&shpfile, "rb", filename) == -1)
    return;

  shpCache = (XShape**)malloc(sizeof(XShape*) * shpfile.numshapes);
  if (!shpCache)
    return;

  shapefileopen = true;

  for (int i = 0; i < shpfile.numshapes; i++)
    shpCache[i] = NULL;
}

TopologyFile::~TopologyFile()
{
  if (!shapefileopen)
    return;

  if (shpCache) {
    ClearCache();
    free(shpCache);
    shpCache = NULL;
  }

  msSHPCloseFile(&shpfile);
}

bool
TopologyFile::CheckScale(const double map_scale) const
{
  return (map_scale <= scaleThreshold);
}

void
TopologyFile::TriggerIfScaleNowVisible(const Projection &map_projection)
{
  triggerUpdateCache |=
      (CheckScale(map_projection.GetMapScaleUser()) != in_scale);
}

void
TopologyFile::ClearCache()
{
  for (int i = 0; i < shpfile.numshapes; i++) {
    delete shpCache[i];
    shpCache[i] = NULL;
  }
}

rectObj
TopologyFile::ConvertRect(const rectObj &src)
{
  rectObj dest = src;

#ifdef RADIANS
  dest.minx *= RAD_TO_DEG;
  dest.miny *= RAD_TO_DEG;
  dest.maxx *= RAD_TO_DEG;
  dest.maxy *= RAD_TO_DEG;
#endif

  return dest;
}

void
TopologyFile::updateCache(Projection &map_projection, 
                      const rectObj &thebounds,
                      bool purgeonly)
{
  if (!triggerUpdateCache || !shapefileopen)
    return;

  in_scale = CheckScale(map_projection.GetMapScaleUser());

  if (!in_scale) {
    // not visible, so flush the cache
    // otherwise we waste time on looking up which shapes are in bounds
    ClearCache();
    triggerUpdateCache = false;
    return;
  }

  if (purgeonly)
    return;

  triggerUpdateCache = false;

  rectObj thebounds_deg = ConvertRect(thebounds);

  msSHPWhichShapes(&shpfile, thebounds_deg, 0);
  if (!shpfile.status) {
    // this happens if entire shape is out of range
    // so clear buffer.
    ClearCache();
    return;
  }

  for (int i = 0; i < shpfile.numshapes; i++) {
    if (msGetBit(shpfile.status, i)) {
      if (shpCache[i] == NULL) {
        // shape is now in range, and wasn't before
        shpCache[i] = new XShape(&shpfile, i, label_field);
      }
    } else {
      delete shpCache[i];
      shpCache[i] = NULL;
    }
  }
}

unsigned
TopologyFile::GetSkipSteps(double map_scale) const
{
  if (map_scale > 0.75 * scaleThreshold)
    return 4;
  if (map_scale > 0.5 * scaleThreshold)
    return 3;
  if (map_scale > 0.25 * scaleThreshold)
    return 2;

  return 1;
}

void
TopologyFile::Paint(Canvas &canvas, BitmapCanvas &bitmap_canvas,
                const Projection &projection) const
{
  if (!shapefileopen)
    return;

  double map_scale = projection.GetMapScaleUser();
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

  rectObj screenRect = projection.CalculateScreenBounds(fixed_zero);

#ifdef RADIANS
  screenRect.minx *= RAD_TO_DEG;
  screenRect.miny *= RAD_TO_DEG;
  screenRect.maxx *= RAD_TO_DEG;
  screenRect.maxy *= RAD_TO_DEG;
#endif

  static POINT pt[MAXCLIPPOLYGON];

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
          const GEOPOINT l = projection.point2GeoPoint(line.point[jj]);

          if (projection.LonLat2ScreenIfVisible(l, &sc))
            icon.draw(canvas, bitmap_canvas, sc.x, sc.y);
        }
      }
      break;

    case MS_SHAPE_LINE:
      for (int tt = 0; tt < shape.numlines; ++tt) {
        const lineObj &line = shape.line[tt];
        int msize = min(line.numpoints, (int)MAXCLIPPOLYGON);

        projection.LonLat2Screen(line.point, pt, msize, 1);

        canvas.polyline(pt, msize);
      }
      break;

    case MS_SHAPE_POLYGON:
      for (int tt = 0; tt < shape.numlines; ++tt) {
        const lineObj &line = shape.line[tt];
        int msize = min(line.numpoints / iskip, (int)MAXCLIPPOLYGON);

        projection.LonLat2Screen(line.point, pt, msize * iskip, iskip);

        canvas.polygon(pt, msize);
      }
      break;
    }
  }
}

void
TopologyFile::PaintLabels(Canvas &canvas,
                      const Projection &projection, LabelBlock &label_block,
                      const SETTINGS_MAP &settings_map) const
{
  if (!shapefileopen || settings_map.DeclutterLabels >= 2)
    return;

  double map_scale = projection.GetMapScaleUser();
  if (map_scale > scaleThreshold)
    return;

  // TODO code: only draw inside screen!
  // this will save time with rendering pixmaps especially
  // we already do an outer visibility test, but may need a test
  // in screen coords

  canvas.select(Fonts::MapLabel);

  // get drawing info

  int iskip = GetSkipSteps(map_scale);

  rectObj screenRect = projection.CalculateScreenBounds(fixed_zero);

#ifdef RADIANS
  screenRect.minx *= RAD_TO_DEG;
  screenRect.miny *= RAD_TO_DEG;
  screenRect.maxx *= RAD_TO_DEG;
  screenRect.maxy *= RAD_TO_DEG;
#endif

  static POINT pt[MAXCLIPPOLYGON];

  for (int ixshp = 0; ixshp < shpfile.numshapes; ixshp++) {
    const XShape *cshape = shpCache[ixshp];
    if (!cshape || !cshape->is_visible(label_field))
      continue;

    const shapeObj &shape = cshape->shape;

    if (!msRectOverlap(&shape.bounds, &screenRect))
      continue;

    for (int tt = 0; tt < shape.numlines; ++tt) {
      const lineObj &line = shape.line[tt];
      int msize = min(line.numpoints / iskip, (int)MAXCLIPPOLYGON);

      projection.LonLat2Screen(line.point, pt, msize * iskip, iskip);

      int minx = canvas.get_width();
      int miny = canvas.get_height();
      for (int jj = 0; jj < msize; ++jj) {
        if (pt[jj].x <= minx) {
          minx = pt[jj].x;
          miny = pt[jj].y;
        }
      }

      cshape->DrawLabel(canvas, label_block, minx, miny);
    }
  }
}
