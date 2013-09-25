/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Topography/TopographyFileRenderer.hpp"
#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "Look/GlobalFonts.hpp"
#include "Renderer/LabelBlock.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Features.hpp"
#include "Screen/Layout.hpp"
#include "Math/Matrix2D.hpp"
#include "shapelib/mapserver.h"
#include "Util/AllocatedArray.hpp"
#include "Util/tstring.hpp"
#include "Geo/GeoClip.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

#include <algorithm>
#include <set>

TopographyFileRenderer::TopographyFileRenderer(const TopographyFile &_file)
  :file(_file), pen(file.GetPenWidth(), file.GetColor()),
   brush(file.GetColor())
{
  ResourceId icon_ID = file.GetIcon();
  if (icon_ID.IsDefined())
    icon.LoadResource(icon_ID, file.GetBigIcon());
}

void
TopographyFileRenderer::UpdateVisibleShapes(const WindowProjection &projection)
{
  if (file.GetSerial() == visible_serial &&
      visible_bounds.IsInside(projection.GetScreenBounds()) &&
      projection.GetScreenBounds().Scale(fixed(2)).IsInside(visible_bounds))
    /* cache is clean */
    return;

  visible_serial = file.GetSerial();
  visible_bounds = projection.GetScreenBounds().Scale(fixed(1.2));
  visible_shapes.clear();
  visible_labels.clear();

  for (auto it = file.begin(), end = file.end(); it != end; ++it) {
    const XShape &shape = *it;

    if (!visible_bounds.Overlaps(shape.get_bounds()))
      continue;

    if (shape.get_type() != MS_SHAPE_NULL)
      visible_shapes.push_back(&shape);

    if (shape.get_label() != NULL)
      visible_labels.push_back(&shape);
  }
}

#ifdef ENABLE_OPENGL

void
TopographyFileRenderer::PaintPoint(Canvas &canvas,
                                   const WindowProjection &projection,
                                   const XShape &shape,
                                   float *opengl_matrix) const
{
  if (!icon.IsDefined())
    return;

  // TODO: for now i assume there is only one point for point-XShapes

  RasterPoint sc;
  if (!projection.GeoToScreenIfVisible(shape.get_center(), sc))
    return;

#ifndef HAVE_GLES
  glPushMatrix();
  glLoadMatrixf(opengl_matrix);
#endif
  icon.Draw(canvas, sc.x, sc.y);
#ifndef HAVE_GLES
  glPopMatrix();
#endif
}

#else

void
TopographyFileRenderer::PaintPoint(Canvas &canvas,
                                   const WindowProjection &projection,
                                   const unsigned short *lines,
                                   const unsigned short *end_lines,
                                   const GeoPoint *points) const
{
  if (!icon.IsDefined())
    return;

  for (; lines < end_lines; ++lines) {
    const GeoPoint *end = points + *lines;
    for (; points < end; ++points) {
      RasterPoint sc;
      if (projection.GeoToScreenIfVisible(*points, sc))
        icon.Draw(canvas, sc.x, sc.y);
    }
  }
}

#endif

void
TopographyFileRenderer::Paint(Canvas &canvas,
                              const WindowProjection &projection)
{
  if (file.IsEmpty())
    return;

  fixed map_scale = projection.GetMapScale();
  if (!file.IsVisible(map_scale))
    return;

  UpdateVisibleShapes(projection);

  if (visible_shapes.empty())
    return;

  // TODO code: only draw inside screen!
  // this will save time with rendering pixmaps especially
  // we already do an outer visibility test, but may need a test
  // in screen coords

#ifdef ENABLE_OPENGL
  pen.Bind();
  brush.Set();
#else
  shape_renderer.Configure(&pen, &brush);
#endif

  // get drawing info

#ifdef ENABLE_OPENGL
  const unsigned level = file.GetThinningLevel(map_scale);
  const unsigned min_distance = file.GetMinimumPointDistance(level)
    / Layout::Scale(1);

#ifndef HAVE_GLES
  float opengl_matrix[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, opengl_matrix);
#endif

  glPushMatrix();
  fixed angle = projection.GetScreenAngle().Degrees();
  fixed scale = projection.GetScale();
  const RasterPoint &screen_origin = projection.GetScreenOrigin();
#ifdef HAVE_GLES
#ifdef FIXED_MATH
  GLfixed fixed_angle = angle.as_glfixed();
  GLfixed fixed_scale = scale.as_glfixed_scale();
#else
  GLfixed fixed_angle = angle * (1<<16);
  GLfixed fixed_scale = scale * (1LL<<32);
#endif
  glTranslatex((int)screen_origin.x << 16, (int)screen_origin.y << 16, 0);
  glRotatex(fixed_angle, 0, 0, -(1<<16));
  glScalex(fixed_scale, fixed_scale, 1<<16);
#else
  glTranslatef(screen_origin.x, screen_origin.y, 0.);
  glRotatef((GLfloat)angle, 0., 0., -1.);
  glScalef((GLfloat)scale, (GLfloat)scale, 1.);
#endif
#else // !ENABLE_OPENGL
  const GeoClip clip(projection.GetScreenBounds().Scale(fixed(1.1)));
  AllocatedArray<GeoPoint> geo_points;

  int iskip = file.GetSkipSteps(map_scale);
#endif

  for (auto it = visible_shapes.begin(), end = visible_shapes.end();
       it != end; ++it) {
    const XShape &shape = **it;

    if (!projection.GetScreenBounds().Overlaps(shape.get_bounds()))
      continue;

#ifdef ENABLE_OPENGL
    const ShapePoint *points = shape.get_points();

    const ShapePoint translation =
      shape.shape_translation(projection.GetGeoLocation());
    glPushMatrix();
#ifdef HAVE_GLES
    glTranslatex(translation.x, translation.y, 0);
#else
    glTranslatef(translation.x, translation.y, 0.);
#endif
#else // !ENABLE_OPENGL
    const unsigned short *lines = shape.get_lines();
    const unsigned short *end_lines = lines + shape.get_number_of_lines();
    const GeoPoint *points = shape.get_points();
#endif

    switch (shape.get_type()) {
    case MS_SHAPE_NULL:
      break;

    case MS_SHAPE_POINT:
#ifdef ENABLE_OPENGL
#ifdef HAVE_GLES
      PaintPoint(canvas, projection, shape, NULL);
#else
      PaintPoint(canvas, projection, shape, opengl_matrix);
#endif
#else // !ENABLE_OPENGL
      PaintPoint(canvas, projection, lines, end_lines, points);
#endif
      break;

    case MS_SHAPE_LINE:
      {
#ifdef ENABLE_OPENGL
#ifdef HAVE_GLES
        glVertexPointer(2, GL_FIXED, 0, &points[0].x);
#else
        glVertexPointer(2, GL_INT, 0, &points[0].x);
#endif

        const GLushort *indices, *count;
        if (level == 0 ||
            (indices = shape.get_indices(level, min_distance, count)) == NULL) {
          count = shape.get_lines();
          const GLushort *end_count = count + shape.get_number_of_lines();
          for (int offset = 0; count < end_count; offset += *count++)
            glDrawArrays(GL_LINE_STRIP, offset, *count);
        } else {
          const GLushort *end_count = count + shape.get_number_of_lines();
          for (; count < end_count; indices += *count++)
            glDrawElements(GL_LINE_STRIP, *count, GL_UNSIGNED_SHORT, indices);
        }
#else // !ENABLE_OPENGL
      for (; lines < end_lines; ++lines) {
        unsigned msize = *lines;
        shape_renderer.Begin(msize);

        const GeoPoint *end = points + msize - 1;
        for (; points < end; ++points)
          shape_renderer.AddPointIfDistant(projection.GeoToScreen(*points));

        // make sure we always draw the last point
        shape_renderer.AddPoint(projection.GeoToScreen(*points));

        shape_renderer.FinishPolyline(canvas);
      }
#endif
      }
      break;

    case MS_SHAPE_POLYGON:
#ifdef ENABLE_OPENGL
      {
        const GLushort *index_count;
        const GLushort *triangles = shape.get_indices(level, min_distance,
                                                        index_count);

#ifdef HAVE_GLES
        glVertexPointer(2, GL_FIXED, 0, &points[0].x);
#else
        glVertexPointer(2, GL_INT, 0, &points[0].x);
#endif
        if (!brush.GetColor().IsOpaque()) {
          const GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          glDrawElements(GL_TRIANGLE_STRIP, *index_count, GL_UNSIGNED_SHORT,
                         triangles);
        } else
          glDrawElements(GL_TRIANGLE_STRIP, *index_count, GL_UNSIGNED_SHORT,
                         triangles);
      }
#else // !ENABLE_OPENGL
      for (; lines < end_lines; ++lines) {
        unsigned msize = *lines / iskip;

        /* copy all polygon points into the geo_points array and clip
           them, to avoid integer overflows (as RasterPoint may store
           only 16 bit integers on some platforms) */

        geo_points.GrowDiscard(msize * 3);

        for (unsigned i = 0; i < msize; ++i)
          geo_points[i] = points[i * iskip];

        msize = clip.ClipPolygon(geo_points.begin(),
                                 geo_points.begin(), msize);
        if (msize < 3)
          continue;

        shape_renderer.Begin(msize);

        for (unsigned i = 0; i < msize; ++i) {
          GeoPoint g = geo_points[i];
          shape_renderer.AddPointIfDistant(projection.GeoToScreen(g));
        }

        shape_renderer.FinishPolygon(canvas);
      }
#endif
      break;
    }
#ifdef ENABLE_OPENGL
    glPopMatrix();
#endif
  }
#ifdef ENABLE_OPENGL
  glPopMatrix();
  pen.Unbind();
#else
  shape_renderer.Commit();
#endif
}

void
TopographyFileRenderer::PaintLabels(Canvas &canvas,
                                    const WindowProjection &projection,
                                    LabelBlock &label_block)
{
  if (file.IsEmpty())
    return;

  fixed map_scale = projection.GetMapScale();
  if (!file.IsVisible(map_scale) || !file.IsLabelVisible(map_scale))
    return;

  UpdateVisibleShapes(projection);

  if (visible_labels.empty())
    return;

  // TODO code: only draw inside screen!
  // this will save time with rendering pixmaps especially
  // we already do an outer visibility test, but may need a test
  // in screen coords

  canvas.Select(file.IsLabelImportant(map_scale) ?
                Fonts::map_label_important : Fonts::map_label);
  canvas.SetTextColor(file.IsLabelImportant(map_scale) ?
                COLOR_BLACK : COLOR_DARK_GRAY);
  canvas.SetBackgroundTransparent();

  // get drawing info

  int iskip = file.GetSkipSteps(map_scale);

#ifdef ENABLE_OPENGL
  Matrix2D m1;
  m1.Translate(projection.GetScreenOrigin());
  m1.Rotate(projection.GetScreenAngle());
  m1.Scale(projection.GetScale());
#endif

  std::set<tstring> drawn_labels;

  // Iterate over all shapes in the file
  for (auto it = visible_labels.begin(), end = visible_labels.end();
       it != end; ++it) {
    const XShape &shape = **it;

    if (!projection.GetScreenBounds().Overlaps(shape.get_bounds()))
      continue;

    // Skip shapes without a label
    const TCHAR *label = shape.get_label();
    if (label == NULL)
      continue;

    const unsigned short *lines = shape.get_lines();
    const unsigned short *end_lines = lines + shape.get_number_of_lines();
#ifdef ENABLE_OPENGL
    const ShapePoint *points = shape.get_points();

    Matrix2D m2(m1);
    m2.Translatex(shape.shape_translation(projection.GetGeoLocation()));
#else
    const GeoPoint *points = shape.get_points();
#endif

    for (; lines < end_lines; ++lines) {
      int minx = canvas.GetWidth();
      int miny = canvas.GetHeight();

#ifdef ENABLE_OPENGL
      const ShapePoint *end = points + *lines;
#else
      const GeoPoint *end = points + *lines;
#endif
      for (; points < end; points += iskip) {
#ifdef ENABLE_OPENGL
        RasterPoint pt = m2.Apply(*points);
#else
        RasterPoint pt = projection.GeoToScreen(*points);
#endif

        if (pt.x <= minx) {
          minx = pt.x;
          miny = pt.y;
        }
      }

      points = end;

      minx += 2;
      miny += 2;

      PixelSize tsize = canvas.CalcTextSize(label);
      PixelRect brect;
      brect.left = minx;
      brect.right = brect.left + tsize.cx;
      brect.top = miny;
      brect.bottom = brect.top + tsize.cy;

      if (!label_block.check(brect))
        continue;

      if (!drawn_labels.insert(label).second)
        continue;

      canvas.DrawText(minx, miny, label);
    }
  }
}
