/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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
#include "shapelib/mapserver.h"
#include "Util/AllocatedArray.hpp"
#include "Util/tstring.hpp"
#include "Geo/GeoClip.hpp"
#include "Geo/Constants.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/VertexPointer.hpp"
#include "Screen/OpenGL/Buffer.hpp"
#include "Screen/OpenGL/Dynamic.hpp"
#endif

#ifdef USE_GLSL
#include "Screen/OpenGL/Program.hpp"
#include "Screen/OpenGL/Shaders.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif

#include <algorithm>
#include <numeric>
#include <set>

TopographyFileRenderer::TopographyFileRenderer(const TopographyFile &_file)
  :file(_file), pen(file.GetPenWidth(), file.GetColor()),
#ifdef ENABLE_OPENGL
   array_buffer(nullptr)
#else
   brush(file.GetColor())
#endif
{
  ResourceId icon_ID = file.GetIcon();
  if (icon_ID.IsDefined())
    icon.LoadResource(icon_ID, file.GetBigIcon());

#ifdef ENABLE_OPENGL
  AddSurfaceListener(*this);
#endif
}

TopographyFileRenderer::~TopographyFileRenderer()
{
#ifdef ENABLE_OPENGL
  RemoveSurfaceListener(*this);

  delete array_buffer;
#endif
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

    if (shape.get_label() != nullptr)
      visible_labels.push_back(&shape);
  }
}

#ifdef ENABLE_OPENGL

inline bool
TopographyFileRenderer::UpdateArrayBuffer()
{
  if (!OpenGL::vertex_buffer_object)
    return false;

  if (array_buffer == nullptr)
    array_buffer = new GLArrayBuffer();
  else if (file.GetSerial() == array_buffer_serial)
    return true;

  array_buffer_serial = file.GetSerial();

  unsigned n = 0;
  for (auto &shape : file) {
    shape.SetOffset(n);
    n = std::accumulate(shape.get_lines(),
                        shape.get_lines() + shape.get_number_of_lines(),
                        n);
  }

  ShapePoint *p = (ShapePoint *)
    array_buffer->BeginWrite(n * sizeof(*p));
  assert (p != nullptr);

  for (const auto &shape : file) {
    const auto *lines = shape.get_lines();
    const unsigned n_lines = shape.get_number_of_lines();
    const ShapePoint *src = shape.get_points();
    for (unsigned i = 0; i < n_lines; ++i) {
      const unsigned n_points = lines[i];
      p = std::copy_n(src, n_points, p);
      src += n_points;
    }
  }

  array_buffer->CommitWrite(n * sizeof(*p), p - n);

  return true;
}

inline void
TopographyFileRenderer::PaintPoint(Canvas &canvas,
                                   const WindowProjection &projection,
                                   const XShape &shape,
                                   const float *opengl_matrix) const
{
  if (!icon.IsDefined())
    return;

  // TODO: for now i assume there is only one point for point-XShapes

  RasterPoint sc;
  if (!projection.GeoToScreenIfVisible(file.ToGeoPoint(shape.get_points()[0]),
                                       sc))
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

inline void
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

#ifdef USE_GLSL
  OpenGL::solid_shader->Use();
#endif

#ifdef ENABLE_OPENGL
  const bool use_vbo = UpdateArrayBuffer();

  if (use_vbo)
    array_buffer->Bind();

  pen.Bind();

  if (!pen.GetColor().IsOpaque()) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
#else
  shape_renderer.Configure(&pen, &brush);
#endif

  // get drawing info

#ifdef ENABLE_OPENGL
  const unsigned level = file.GetThinningLevel(map_scale);
  const ShapeScalar min_distance =
    ShapeScalar(file.GetMinimumPointDistance(level))
    / (Layout::Scale(1) * REARTH);

#ifdef HAVE_GLES
  const float *const opengl_matrix = nullptr;
#else
  float opengl_matrix[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, opengl_matrix);
#endif

  fixed angle = projection.GetScreenAngle().Degrees();
  fixed scale = projection.GetScale();
  const RasterPoint &screen_origin = projection.GetScreenOrigin();
  const GeoPoint &screen_location = projection.GetGeoLocation();
  const GeoPoint projection_delta = file.GetCenter() - screen_location;

  const fixed scale_r = scale * REARTH;
  const fixed scale_x = scale_r * screen_location.latitude.fastcosine();
  const fixed scale_y = -scale_r;

#ifdef USE_GLSL
  const glm::vec3 scale_vec(GLfloat(scale_x), GLfloat(scale_y), 1);

  glm::mat4 matrix = glm::scale(glm::rotate(glm::translate(glm::mat4(),
                                                           glm::vec3(screen_origin.x,
                                                                     screen_origin.y,
                                                                     0)),
                                            GLfloat(angle),
                                            glm::vec3(0, 0, -1)),
                                scale_vec);
  matrix = glm::translate(matrix,
                          glm::vec3(GLfloat(projection_delta.longitude.Native()),
                                    GLfloat(projection_delta.latitude.Native()),
                                    0.));
  glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(matrix));
#else
  glPushMatrix();
#ifdef HAVE_GLES
#ifdef FIXED_MATH
  GLfixed fixed_angle = angle.as_glfixed();
#else
  GLfixed fixed_angle = angle * (1<<16);
#endif
  glTranslatex((int)screen_origin.x << 16, (int)screen_origin.y << 16, 0);
  glRotatex(fixed_angle, 0, 0, -(1<<16));
#else
  glTranslatef(screen_origin.x, screen_origin.y, 0.);
  glRotatef((GLfloat)angle, 0., 0., -1.);
#endif

  glScalef(GLfloat(scale_x), GLfloat(scale_y), 1.);
  glTranslatef(GLfloat(projection_delta.longitude.Native()),
               GLfloat(projection_delta.latitude.Native()),
               0.);
#endif /* !USE_GLSL */
#else // !ENABLE_OPENGL
  const GeoClip clip(projection.GetScreenBounds().Scale(fixed(1.1)));
  AllocatedArray<GeoPoint> geo_points;

  int iskip = file.GetSkipSteps(map_scale);
#endif

#ifdef ENABLE_OPENGL
  ScopeVertexPointer vp;

#ifdef GL_EXT_multi_draw_arrays
  std::vector<GLsizei> polygon_counts;
  std::vector<GLshort> polygon_indices;
#endif
#endif

  for (auto it = visible_shapes.begin(), end = visible_shapes.end();
       it != end; ++it) {
    const XShape &shape = **it;

#ifdef ENABLE_OPENGL
    const ShapePoint *points = use_vbo
      /* pointer relative to the VBO */
      ? (const ShapePoint *)(shape.GetOffset() * sizeof(*points))
      /* regular pointer */
      : shape.get_points();
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
#ifdef USE_GLSL
      /* disable the ScopeVertexPointer instance because PaintPoint()
         uses that attribute */
      glDisableVertexAttribArray(OpenGL::Attribute::POSITION);
#endif

      PaintPoint(canvas, projection, shape, opengl_matrix);

#ifdef USE_GLSL
      /* reenable the ScopeVertexPointer instance because PaintPoint()
         left it disabled */
      glEnableVertexAttribArray(OpenGL::Attribute::POSITION);
#endif
#else // !ENABLE_OPENGL
      PaintPoint(canvas, projection, lines, end_lines, points);
#endif
      break;

    case MS_SHAPE_LINE:
      {
#ifdef ENABLE_OPENGL
        vp.Update(GL_FLOAT, points);

        const GLushort *indices, *count;
        if (level == 0 ||
            (indices = shape.get_indices(level, min_distance, count)) == nullptr) {
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
        const unsigned n = *index_count;

#ifdef GL_EXT_multi_draw_arrays
        const unsigned offset = shape.GetOffset();
        if (GLExt::HaveMultiDrawElements() && offset + n < 0x10000) {
          /* postpone, draw many polygons with a single
             glMultiDrawElements() call */
          polygon_counts.push_back(n);
          const size_t size = polygon_indices.size();
          polygon_indices.resize(size + n, offset);
          for (unsigned i = 0; i < n; ++i)
            polygon_indices[size + i] += triangles[i];
          break;
        }
#endif

        vp.Update(GL_FLOAT, points);
        glDrawElements(GL_TRIANGLE_STRIP, n, GL_UNSIGNED_SHORT,
                       triangles);
      }
#else // !ENABLE_OPENGL
      for (const GeoPoint *src = &points[0]; lines < end_lines;
           src += *lines, ++lines) {
        unsigned msize = *lines / iskip;

        /* copy all polygon points into the geo_points array and clip
           them, to avoid integer overflows (as RasterPoint may store
           only 16 bit integers on some platforms) */

        geo_points.GrowDiscard(msize * 3);
        for (unsigned i = 0; i < msize; ++i)
          geo_points[i] = src[i * iskip];

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
  }
#ifdef ENABLE_OPENGL

#ifdef GL_EXT_multi_draw_arrays
  if (!polygon_indices.empty()) {
    assert(GLExt::HaveMultiDrawElements());

    std::vector<const GLshort *> polygon_pointers;
    unsigned i = 0;
    for (auto count : polygon_counts) {
      polygon_pointers.push_back(polygon_indices.data() + i);
      i += count;
    }

    vp.Update(GL_FLOAT, nullptr);

    GLExt::MultiDrawElements(GL_TRIANGLE_STRIP, polygon_counts.data(),
                             GL_UNSIGNED_SHORT,
                             (const GLvoid **)polygon_pointers.data(),
                             polygon_counts.size());
  }
#endif

#ifdef USE_GLSL
  glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(glm::mat4()));
#else
  glPopMatrix();
#endif
  if (!pen.GetColor().IsOpaque())
    glDisable(GL_BLEND);

  pen.Unbind();

  if (use_vbo)
    array_buffer->Unbind();
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

  canvas.Select(file.IsLabelImportant(map_scale) ?
                Fonts::map_label_important : Fonts::map_label);
  canvas.SetTextColor(file.IsLabelImportant(map_scale) ?
                COLOR_BLACK : COLOR_DARK_GRAY);
  canvas.SetBackgroundTransparent();

  // get drawing info

  int iskip = file.GetSkipSteps(map_scale);

  std::set<tstring> drawn_labels;

  // Iterate over all shapes in the file
  for (auto it = visible_labels.begin(), end = visible_labels.end();
       it != end; ++it) {
    const XShape &shape = **it;

    // Skip shapes without a label
    const TCHAR *label = shape.get_label();
    assert(label != nullptr);

    const unsigned short *lines = shape.get_lines();
    const unsigned short *end_lines = lines + shape.get_number_of_lines();
#ifdef ENABLE_OPENGL
    const ShapePoint *points = shape.get_points();
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
        RasterPoint pt = projection.GeoToScreen(file.ToGeoPoint(*points));
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

#ifdef ENABLE_OPENGL

void
TopographyFileRenderer::SurfaceCreated()
{
}

void
TopographyFileRenderer::SurfaceDestroyed()
{
  delete array_buffer;
  array_buffer = nullptr;
}

#endif
