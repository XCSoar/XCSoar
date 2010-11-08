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

#include "Topology/TopologyRenderer.hpp"
#include "Topology/TopologyFile.hpp"
#include "Topology/XShape.hpp"
#include "WindowProjection.hpp"
#include "SettingsMap.hpp"
#include "resource.h"
#include "Screen/Canvas.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/LabelBlock.hpp"
#include "shapelib/map.h"

#include <algorithm>

TopologyFileRenderer::TopologyFileRenderer(const TopologyFile &_file)
  :file(_file), pen(1, file.get_color()), brush(file.get_color())
{
  if (file.get_icon() != 0) {
    if (file.get_icon() == IDB_TOWN)
      icon.load_big(IDB_TOWN, IDB_TOWN_HD);
    else
      icon.load(file.get_icon());
  }
}

gcc_pure
static GeoPoint
point2GeoPoint(const pointObj& p)
{
  return GeoPoint(Angle::native(fixed(p.x)), Angle::native(fixed(p.y)));
}

gcc_const
static rectObj
ConvertRect(const GeoBounds br)
{
  rectObj dest;
  dest.minx = br.west.value_degrees();
  dest.maxx = br.east.value_degrees();
  dest.miny = br.south.value_degrees();
  dest.maxy = br.north.value_degrees();
  return dest;
}

void
TopologyFileRenderer::Paint(Canvas &canvas, BitmapCanvas &bitmap_canvas,
                            const WindowProjection &projection) const
{
  fixed map_scale = projection.GetMapScale();
  if (!file.is_visible(map_scale))
    return;

  // TODO code: only draw inside screen!
  // this will save time with rendering pixmaps especially
  // we already do an outer visibility test, but may need a test
  // in screen coords

  enum { NONE, OUTLINE, SOLID } mode = NONE;

  // get drawing info

  int iskip = file.GetSkipSteps(map_scale);

  const rectObj screenRect =
    ConvertRect(projection.GetScreenBounds());

  for (unsigned i = 0; i < file.size(); ++i) {
    const XShape *cshape = file[i];
    if (!cshape || !cshape->is_visible(file.get_label_field()))
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
      if (mode != OUTLINE) {
        canvas.select(pen);
        mode = OUTLINE;
      }

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
      if (mode != SOLID) {
        canvas.null_pen();
        canvas.select(brush);
        mode = SOLID;
      }

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
TopologyFileRenderer::PaintLabels(Canvas &canvas,
                                  const WindowProjection &projection,
                                  LabelBlock &label_block,
                                  const SETTINGS_MAP &settings_map) const
{
  if (settings_map.DeclutterLabels >= 2)
    return;

  fixed map_scale = projection.GetMapScale();
  if (!file.is_visible(map_scale))
    return;

  // TODO code: only draw inside screen!
  // this will save time with rendering pixmaps especially
  // we already do an outer visibility test, but may need a test
  // in screen coords

  canvas.select(Fonts::MapLabel);
  canvas.set_text_color(Color(0x20, 0x20, 0x20));
  canvas.background_transparent();

  // get drawing info

  int iskip = file.GetSkipSteps(map_scale);

  rectObj screenRect =
    ConvertRect(projection.GetScreenBounds());

  for (unsigned i = 0; i < file.size(); ++i) {
    const XShape *cshape = file[i];
    if (!cshape || !cshape->is_visible(file.get_label_field()))
      continue;


    const TCHAR *label = cshape->get_label();
    if (label == NULL)
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

      minx += 2;
      miny += 2;

      SIZE tsize = canvas.text_size(label);
      RECT brect;
      brect.left = minx;
      brect.right = brect.left + tsize.cx;
      brect.top = miny;
      brect.bottom = brect.top + tsize.cy;

      if (!label_block.check(brect))
        continue;

      canvas.text(minx, miny, label);
    }
  }
}

TopologyRenderer::TopologyRenderer(const TopologyStore &_store)
  :store(_store)
{
  for (unsigned i = 0; i < store.size(); ++i)
    files[i] = new TopologyFileRenderer(store[i]);
}

TopologyRenderer::~TopologyRenderer()
{
  for (unsigned i = 0; i < store.size(); ++i)
    delete files[i];
}

void
TopologyRenderer::Draw(Canvas &canvas, BitmapCanvas &bitmap_canvas,
                       const WindowProjection &projection) const
{
  for (unsigned i = 0; i < store.size(); ++i)
    files[i]->Paint(canvas, bitmap_canvas, projection);
}

void
TopologyRenderer::DrawLabels(Canvas &canvas,
                             const WindowProjection &projection,
                             LabelBlock &label_block,
                             const SETTINGS_MAP &settings_map) const
{
  for (unsigned i = 0; i < store.size(); ++i)
    files[i]->PaintLabels(canvas, projection, label_block, settings_map);
}
