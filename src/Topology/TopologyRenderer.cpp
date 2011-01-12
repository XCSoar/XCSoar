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
  :file(_file), pen(file.get_pen_width(), file.get_color()),
   brush(file.get_color())
{
  if (file.get_icon() == IDB_TOWN)
    icon.load_big(IDB_TOWN, IDB_TOWN_HD);
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
TopologyFileRenderer::Paint(Canvas &canvas,
                            const WindowProjection &projection) const
{
  fixed map_scale = projection.GetMapScale();
  if (!file.is_visible(map_scale))
    return;

  // TODO code: only draw inside screen!
  // this will save time with rendering pixmaps especially
  // we already do an outer visibility test, but may need a test
  // in screen coords

  shape_renderer.configure(&pen, &brush);

  // get drawing info

  int iskip = file.GetSkipSteps(map_scale);

  const rectObj screenRect =
    ConvertRect(projection.GetScreenBounds());

  for (unsigned i = 0; i < file.size(); ++i) {
    const XShape *cshape = file[i];
    if (!cshape || !cshape->is_visible(file.get_label_field()))
      continue;

    if (!projection.GetScreenBounds().overlaps(cshape->get_bounds()))
      continue;

    const unsigned short *lines = cshape->get_lines();
    const unsigned short *end_lines = lines + cshape->get_number_of_lines();
    const GeoPoint *points = cshape->get_points();

    switch (cshape->get_type()) {
    case MS_SHAPE_POINT:
      if (!icon.defined())
        break;

      for (; lines < end_lines; ++lines) {
        const GeoPoint *end = points + *lines;
        for (; points < end; ++points) {
          RasterPoint sc;
          if (projection.GeoToScreenIfVisible(*points, sc))
            icon.draw(canvas, sc.x, sc.y);
        }
      }
      break;

    case MS_SHAPE_LINE:
      for (; lines < end_lines; ++lines) {
        unsigned msize = *lines;
        shape_renderer.begin_shape(msize);

        const GeoPoint *end = points + msize;
        for (; points < end; ++points)
          shape_renderer.add_point_if_distant(projection.GeoToScreen(*points));

        shape_renderer.finish_polyline(canvas);
      }
      break;

    case MS_SHAPE_POLYGON:
      for (; lines < end_lines; ++lines) {
        unsigned msize = *lines;
        shape_renderer.begin_shape(msize / iskip);

        const GeoPoint *end = points + msize;
        for (; points < end; points += iskip)
          shape_renderer.add_point_if_distant(projection.GeoToScreen(*points));
        points = end;

        shape_renderer.finish_polygon(canvas);
      }
      break;
    }
  }

  shape_renderer.commit();
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

    if (!projection.GetScreenBounds().overlaps(cshape->get_bounds()))
      continue;

    const unsigned short *lines = cshape->get_lines();
    const unsigned short *end_lines = lines + cshape->get_number_of_lines();
    const GeoPoint *points = cshape->get_points();

    for (; lines < end_lines; ++lines) {
      int minx = canvas.get_width();
      int miny = canvas.get_height();

      const GeoPoint *end = points + *lines;
      for (; points < end; points += iskip) {
        RasterPoint pt = projection.GeoToScreen(*points);

        if (pt.x <= minx) {
          minx = pt.x;
          miny = pt.y;
        }
      }

      points = end;

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
TopologyRenderer::Draw(Canvas &canvas,
                       const WindowProjection &projection) const
{
  for (unsigned i = 0; i < store.size(); ++i)
    files[i]->Paint(canvas, projection);
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
